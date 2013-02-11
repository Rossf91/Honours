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

#include "uarch/bpu/BranchPredictorOracle.h"

#include <iomanip>

#include "util/Log.h"


BranchPredictorOracle::BranchPredictorOracle (BpuArch& _bpu_arch)
: num_hits(0),
  num_misses(0)
{ /* EMPTY */ }

BranchPredictorOracle::~BranchPredictorOracle ()
{ /* EMPTY */ }

BranchPredictorOracle::PredictionOutcome
BranchPredictorOracle::commit_branch (uint32 pc,
                                      uint32 next_seq_pc,
                                      uint32 next_pc,
                                      uint32 ret_addr,
                                      bool isReturn,
                                      bool isCall)
{
  // Called on completion of each branch, call or return instruction.
  //
  ++num_hits;

  // Get the predicted outcome.
  //
  if (next_pc == next_seq_pc) { return CORRECT_PRED_NOT_TAKEN; }
  else                        { return CORRECT_PRED_TAKEN;     }
}

void
BranchPredictorOracle::print_stats ()
{
  LOG(LOG_INFO) << "Branch Predictor Statistics (Oracle)\n"
                << "-------------------------------------\n"
                << " BPU hits   "     << num_hits << "\n"
                << " BPU misses "     << num_misses << "\n"
                << " BPU hit ratio  " << (num_hits ? (100.0*(double)num_hits/(double)(num_hits+num_misses)) : 0 ) << "\n";
}
