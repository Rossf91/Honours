//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// A dynamic branch predictor which always predicts correctly
//
//
// =====================================================================

#ifndef INC_UARCH_BPU_BRANCHPREDICTOR_ORACLE_H_
#define INC_UARCH_BPU_BRANCHPREDICTOR_ORACLE_H_

// =====================================================================
// HEADERS
// =====================================================================

#include "sim_types.h"

#include "arch/Configuration.h"

#include "uarch/bpu/BranchPredictorInterface.h"


// =====================================================================
// CLASSES
// =====================================================================

// Oracle Branch Predictor
//
class BranchPredictorOracle : public BranchPredictorInterface
{
private:	
  // Branch predictor performace counters
  //
  uint64 num_hits;
  uint64 num_misses;
	
public:
  
	BranchPredictorOracle(BpuArch& _bpu_arch);
	~BranchPredictorOracle();

	PredictionOutcome commit_branch (uint32 pc,
                                   uint32 next_seq_pc,
                                   uint32 next_pc,
                                   uint32 ret_addr,
                                   bool isReturn,
                                   bool isCall);

  void print_stats ();
};

#endif // INC_UARCH_BPU_BRANCHPREDICTOR_ORACLE_H_
