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

#ifndef INC_UARCH_BPU_BRANCHPREDICTOR_TWOLEVEL_H_
#define INC_UARCH_BPU_BRANCHPREDICTOR_TWOLEVEL_H_

#include "sim_types.h"

#include "arch/Configuration.h"

#include "uarch/bpu/BranchPredictorInterface.h"

// Two level branch predictor
//
class BranchPredictorTwoLevel: public BranchPredictorInterface
{
private:
  // Branch predictor configuration
  //
  char  bp_type;
  int   btac_sets;
  int   btac_ways;
  int   bht_entries;
  int   ras_entries;
  
  // Two level predictor kind
  //
  enum Kind { GShare, GSelect };
  Kind kind;
  
  
  // Branch predictor state
  //
  BTACentry**     btac;     // array of pointers to btac ways
  unsigned char*  bht;      // array of BHT entries
  uint32          ghr;      // global history register
  uint32*         ras;      // return address stack
  int             ras_ptr;  // index of head of return stack

  // Predictor convenience variables
  //
  uint32 btac_index_mask;
  uint32 bht_index_mask;
  
  // Branch predictor performace counters
  //
  uint64 num_hits;
  uint64 num_misses;
  
  uint32 bht_reads;
  uint32 btac_reads;
  uint32 btac_writes;

  uint32     get_bht_ix (uint32 pc) const;
  
  Prediction predict_next_pc (uint32 pc, uint32 next_seq_pc, uint32 &pred_pc);
  
public:

  BranchPredictorTwoLevel (BpuArch& _bpu_arch);
  ~BranchPredictorTwoLevel ();

  PredictionOutcome commit_branch (uint32 pc,
                                   uint32 next_seq_pc,
                                   uint32 next_pc,
                                   uint32 ret_addr,
                                   bool isReturn,
                                   bool isCall);

  virtual void print_stats ();
	
};


#endif  // INC_UARCH_BPU_BRANCHPREDICTOR_TWOLEVEL_H_

