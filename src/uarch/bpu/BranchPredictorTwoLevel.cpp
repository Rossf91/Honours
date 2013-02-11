//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  This file defines the classes used to represent the state of the
//  branch predictors within a simulated processor.
//
// =====================================================================

#include "uarch/bpu/BranchPredictorTwoLevel.h"

#include <iomanip>

#include "util/Log.h"

static const char strengthen_prediction[4] = { 0, 0, 3, 3 };
static const char weaken_prediction[4] = { 1, 2, 1, 2 };
static const char* predString[3] = { "PT", "PN", "NP" };
static const char* predOutcomeString[6] = {"CPT", "CPNT", "CPN", "IPT", "IPNT", "IPN"};


BranchPredictorTwoLevel::BranchPredictorTwoLevel(BpuArch& _bpu_arch)
: btac_sets(_bpu_arch.sets),
  btac_ways(_bpu_arch.ways),
  bht_entries(_bpu_arch.bht_entries),
  ras_entries(_bpu_arch.ras_entries),
  kind(GShare),
  ras_ptr(0),
  num_hits(0),
  num_misses(0)
{
  btac_index_mask = btac_sets   - 1;
  bht_index_mask  = bht_entries - 1;
  
  btac = new BTACentry*[btac_ways];

  for (uint32 w = 0; w < btac_ways; ++w) {
    btac[w] = new BTACentry[btac_sets];
    for (uint32 s = 0; s < btac_sets; ++s) {
      btac[w][s].tag = 1;
      btac[w][s].target = 0;
      btac[w][s].callReturn = false;
    }
  }
  
  bht = new unsigned char[bht_entries];
  for (uint32 w = 0; w < bht_entries; ++w) {
    bht[w] = 2;
  }
  
  ras = new uint32[ras_entries];
  for (uint32 w = 0; w < ras_entries; ++w) {
    ras[w] = 0;
  }
  
  // Determine kind of two level predictor
  //
  switch (_bpu_arch.bp_type) {
    case 'A': { kind = GShare;  break; }
    case 'E': { kind = GSelect; break; }
  }
}

BranchPredictorTwoLevel::~BranchPredictorTwoLevel()
{
  for (int w = 0; w < btac_ways; ++w) {
    delete [] btac[w];
  }
  delete [] btac;
  delete [] bht;
  delete [] ras;
}

uint32
BranchPredictorTwoLevel::get_bht_ix(uint32 pc) const
{
  uint32 bht_ix = 0;
  // Hit in the BTAC, now see what the BHT predicts as outcome
  //
  switch (kind) {
    case GSelect: {
      uint64 lower_pc = (pc>>1) & (bht_index_mask/2);
      uint64 lower_ghr = ghr & (bht_index_mask/2-1);
      
      // Concatenates the lower GHR with the lower pc, pc bits at MSB end of result
      //
      bht_ix = ((lower_pc <<(bht_index_mask/2-1)) | lower_ghr) & bht_index_mask; 
      break;
    }
    case GShare: {
      bht_ix = (ghr ^ (pc >> 1)) & bht_index_mask;
      break;
    }
  }
  return bht_ix;
}


BranchPredictorTwoLevel::Prediction
BranchPredictorTwoLevel::predict_next_pc(uint32 pc, uint32 next_seq_pc, uint32 &pred_pc) 
{
  Prediction pred     = NoPrediction;
  int        btac_ix  = (pc >> 1) & btac_index_mask;
  
  // next_seq_pc is the default prediction
  //
  pred_pc = next_seq_pc;
  ++btac_reads;
  
  for (int w = 0; w < btac_ways; w++)
  {
    if (btac[w][btac_ix].tag == pc) {
      uint32 bht_ix = get_bht_ix(pc);
      
      ++bht_reads;
      if (bht[bht_ix] > 1) {
        // BHT predicts 'taken'
        pred_pc = btac[w][btac_ix].callReturn ? ras[ras_ptr] : btac[w][btac_ix].target;
        pred = PredTaken;
      } else {
	      // BHT predicts 'not-taken'
	      pred = PredNotTaken;
      }
      LOG(LOG_DEBUG3) << std::hex << std::setw(8) << std::setfill('0')
                      << btac_ix << " BTAC hit at way " << w;
    }
  } // end for
  
  if (pred == NoPrediction) {
	  LOG(LOG_DEBUG3) << std::hex << std::setw(8) << std::setfill('0')
                    << btac_ix << " BTAC miss";
  }
  
  return pred;
}

BranchPredictorTwoLevel::PredictionOutcome
BranchPredictorTwoLevel::commit_branch(uint32 pc,
                                       uint32 next_seq_pc,
                                       uint32 next_pc,
                                       uint32 ret_addr,
                                       bool isReturn,
                                       bool isCall)
{
  // Called on completion of each branch, call or return instruction
  //
  bool hit = false;

  // Get the predicted outcome.
  uint32 pred_pc;
  Prediction pred = predict_next_pc(pc, next_seq_pc, pred_pc);

  LOG(LOG_DEBUG3) << std::hex << std::setw(8) << std::setfill('0') << pc << ", "
                  << std::hex << std::setw(8) << std::setfill('0') << next_seq_pc << ", "
                  << std::hex << std::setw(8) << std::setfill('0') << next_pc << ", "
                  << predString[pred];

  uint32 bht_ix = get_bht_ix(pc);

  // Check whether this prediction is correct.
  //
  hit = (next_pc == pred_pc);

  if (hit) {
    // Correct prediction
    //
    ++num_hits;

    LOG(LOG_DEBUG3) << "-C";

    // Strengthen current bht predictor
    //
    bht[bht_ix] = strengthen_prediction[bht[bht_ix]];

    // Update global history register with the branch direction.
    //
    ghr = (ghr << 1) | ((pred == PredTaken) ? 1 : 0);
  } else {
    // Incorrect prediction
    //
    ++num_misses;
    LOG(LOG_DEBUG3) << "-I ("
                    << std::hex << std::setw(8) << std::setfill('0')
                    << next_pc << ", "
                    << std::hex << std::setw(8) << std::setfill('0')
                    << pred_pc << ")";

    if (pred == NoPrediction) {
      // No prediction found for this branch in the BTAC
      // set it up from scratch
      //
      LOG(LOG_DEBUG3) << " BTAC miss";

      uint32 way = (uint32)((num_hits ^ num_misses) & 1);
      uint32 btac_ix = (pc >> 1) & btac_index_mask;
      btac[way][btac_ix].tag = pc;
      btac[way][btac_ix].callReturn = isReturn;
      btac[way][btac_ix].target = next_pc;
    }

    // Weaken the bht predictor
    bht[bht_ix] = weaken_prediction[bht[bht_ix]];

    ghr = (ghr << 1) | ((pred == PredTaken) ? 0 : 1);
  }

  // Pop return stack if this is a return instruction
  //
  if (isReturn) {
    LOG(LOG_DEBUG3) << "-RET";
    ras_ptr = (ras_ptr > 0) ? ras_ptr - 1 : ras_entries - 1;
  }

  // Push return address onto stack if this is a call instruction
  //
  if (isCall) {

    LOG(LOG_DEBUG3) << "-CALL";

    ras_ptr++;
    if (ras_ptr == ras_entries) {
      ras_ptr = 0;
    }
    ras[ras_ptr] = ret_addr;
  }

  PredictionOutcome predOutcome;
  if (hit) {
	  if (pred == PredTaken){
		  predOutcome = CORRECT_PRED_TAKEN;
	  } else if (pred == PredNotTaken) {
		  predOutcome = CORRECT_PRED_NOT_TAKEN;
	  } else {
		  predOutcome = CORRECT_PRED_NONE;
	  }
  } else {
	  if (pred == PredTaken){
		  predOutcome = INCORRECT_PRED_TAKEN;
	  } else if (pred == PredNotTaken) {
		  predOutcome = INCORRECT_PRED_NOT_TAKEN;
	  } else {
		  predOutcome = INCORRECT_PRED_NONE;
	  }
  }

  LOG(LOG_DEBUG3) << hit << ", "
                  << std::hex << std::setw(8) << std::setfill('0')
                  << bht_ix << ", "
                  << predOutcomeString[predOutcome] << "\n";

  return predOutcome;
}

void
BranchPredictorTwoLevel::print_stats()
{
  // FIXME: Display name
  //
  LOG(LOG_INFO) <<	"Branch Predictor Statistics\n"
                << "-------------------------------------\n"
                << " hits   "     << num_hits << "\n"
                << " misses "     << num_misses << "\n"
                << " hit ratio  " << (num_hits ? (100.0*(double)num_hits/(double)(num_hits+num_misses)) : 0 ) << "\n";
}
