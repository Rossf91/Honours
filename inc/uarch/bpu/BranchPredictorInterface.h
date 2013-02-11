//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// BranchPredictor iface.
//
// =====================================================================

#ifndef INC_UARCH_BPU_BRANCHPREDICTORINTERFACE_H_
#define INC_UARCH_BPU_BRANCHPREDICTORINTERFACE_H_

#include "sim_types.h"
#include "api/types.h"

struct BTACentry {
	uint32 tag;
	uint32 target;
	bool   callReturn;
};


class BranchPredictorInterface
{
protected:
  // Constructor MUST be protected and empty!
  //
  BranchPredictorInterface()
  { /* EMPTY */ }
  
public:
  
  enum PredictionOutcome
  {
		CORRECT_PRED_TAKEN,
    CORRECT_PRED_NOT_TAKEN,
		CORRECT_PRED_NONE,
		INCORRECT_PRED_TAKEN,
		INCORRECT_PRED_NOT_TAKEN,
		INCORRECT_PRED_NONE
  };
  
  enum Prediction
  {
    PredTaken,
    PredNotTaken,
    NoPrediction
  };

  // Destructor MUST be declared AND virtual so all implementations of
  // this interface can be destroyed correctly.
  //
  virtual ~BranchPredictorInterface()
  { /* EMPTY */ }
  
  // Interface methods ---------------------------------------------------
  //

  // Called when branch commits
  //
  virtual PredictionOutcome commit_branch (uint32 pc,
                                   uint32 next_seq_pc,
                                   uint32 next_pc,
                                   uint32 return_addr,
                                   bool   is_return,
                                   bool   is_call) = 0;
  
  virtual void print_stats ()  = 0;

};          


#endif  // INC_UARCH_BPU_BRANCHPREDICTORINTERFACE_H_
