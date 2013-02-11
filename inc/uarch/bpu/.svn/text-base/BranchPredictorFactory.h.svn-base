//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Branch predictor factory responsible for instantiating the right
// branch predictor implementation.
//
// =====================================================================

#ifndef INC_UARCH_PROCESSOR_BRANCHPREDICTORFACTORY_H_
#define INC_UARCH_PROCESSOR_BRANCHPREDICTORFACTORY_H_

#include "sim_types.h"

// 'Static' factory class creating BranchPredictor model instances
//
class BranchPredictorFactory {
private:
  // private constructor to prevent anyone from creating instances
  //
  BranchPredictorFactory() { /* EMPTY */ };

  DISALLOW_COPY_AND_ASSIGN(BranchPredictorFactory);

public:
  // Factory method creating instances of branch predictor models
  //
  static BranchPredictorInterface* create_branch_predictor(BpuArch& _bpu_arch);

};



#endif  // INC_UARCH_PROCESSOR_BRANCHPREDICTORFACTORY_H_
