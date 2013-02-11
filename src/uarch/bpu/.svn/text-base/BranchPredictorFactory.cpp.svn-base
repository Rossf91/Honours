//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Factory class used for branch-predictor instantiation.
//
// =====================================================================

// =====================================================================
// HEADERS
// =====================================================================


#include "define.h"

#include "Assertion.h"

#include "uarch/bpu/BranchPredictorInterface.h"
#include "uarch/bpu/BranchPredictorOracle.h"
#include "uarch/bpu/BranchPredictorTwoLevel.h"
#include "uarch/bpu/BranchPredictorFactory.h"


BranchPredictorInterface*
BranchPredictorFactory::create_branch_predictor(BpuArch& _bpu_arch)
{
	BranchPredictorInterface* bp = NULL;

#ifdef ENABLE_BPRED
  // If a branch predictor is configured then return a valid instance
  //
  if (_bpu_arch.is_configured) {
    switch (_bpu_arch.bp_type) {
      case 'A': { bp = new BranchPredictorTwoLevel(_bpu_arch); break; }
      case 'E': { bp = new BranchPredictorTwoLevel(_bpu_arch); break; }
      case 'O': { bp = new BranchPredictorOracle  (_bpu_arch); break; }
      default:  { UNIMPLEMENTED(); /* FIXME */ }
    }
  }
#endif // ENABLE_BPRED
  
  return bp;
}
