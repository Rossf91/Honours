//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Processor micro-architecture model is implemented in classes derived
// from Pipeline.
//
// =====================================================================

// =====================================================================
// HEADERS
// =====================================================================

#include "define.h"

#include "Assertion.h"

#include "uarch/processor/ProcessorPipelineInterface.h"
#include "uarch/processor/ProcessorPipelineFactory.h"
#include "uarch/processor/ProcessorPipelineEncore5.h"
#include "uarch/processor/ProcessorPipelineEncore7.h"
#include "uarch/processor/ProcessorPipelineSkipjack.h"


// =====================================================================
// METHODS
// =====================================================================

ProcessorPipelineInterface* 
ProcessorPipelineFactory::create_pipeline(ProcessorPipelineVariant p) {
  ProcessorPipelineInterface* pl = 0;
  
#ifdef CYCLE_ACC_SIM
  // Only if cycle accurate mode is enabled do we actually instantiate proper
  // pipeline performance models 
  //
  switch (p) {
    case E_PL_EC5:        { pl = new ProcessorPipelineEncore5();     break; }
    case E_PL_EC7:        { pl = new ProcessorPipelineEncore7();     break; }
    case E_PL_SKIPJACK:   { pl = new ProcessorPipelineSkipjack();    break; }
    default:              { UNIMPLEMENTED(); }
  }
#endif // CYCLE_ACC_SIM
  
  return pl;
}
