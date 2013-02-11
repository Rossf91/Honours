//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Processor pipeline factory responsible for instantiating the right
// pipeline implementation.
//
// =====================================================================

#ifndef INC_UARCH_PROCESSOR_PROCESSORPIPELINEFACTORY_H_
#define INC_UARCH_PROCESSOR_PROCESSORPIPELINEFACTORY_H_

#include "sim_types.h"

// 'Static' factory class creating ProcessorPipeline model instances
//
class ProcessorPipelineFactory {
private:
  // private constructor to prevent anyone from creating instances
  //
  ProcessorPipelineFactory() { /* EMPTY */ };
  
  
  ProcessorPipelineFactory(const ProcessorPipelineFactory&);  // DO NOT COPY
  void operator=(const ProcessorPipelineFactory&);            // DO NOT ASSIGN
  
public:
  // Factory method creating instances of processor pipeline models
  //
  static ProcessorPipelineInterface* create_pipeline(ProcessorPipelineVariant p);
  
};



#endif  // INC_UARCH_PROCESSOR_PROCESSORPIPELINEFACTORY_H_
