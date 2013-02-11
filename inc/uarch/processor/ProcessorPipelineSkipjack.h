//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================

#ifndef INC_UARCH_PROCESSOR_PROCESSORPIPELINESKIPJACK_H_
#define INC_UARCH_PROCESSOR_PROCESSORPIPELINESKIPJACK_H_

// =====================================================================
// HEADERS
// =====================================================================

#include "uarch/processor/ProcessorPipelineInterface.h"

// =====================================================================
// CLASSES
// =====================================================================

// Skipjack pipeline model
//
class ProcessorPipelineSkipjack : public ProcessorPipelineInterface
{
public:
  
  ProcessorPipelineSkipjack()  { /* EMPTY */ }
  ~ProcessorPipelineSkipjack() { /* EMPTY */ }
  
  virtual bool precompute_pipeline_model(arcsim::isa::arc::Dcode& inst, const IsaOptions& isa_opts);

  virtual bool update_pipeline(arcsim::sys::cpu::Processor& _cpu);
  
  virtual void jit_emit_translation_unit_begin(arcsim::util::CodeBuffer&         buf,
                                               arcsim::sys::cpu::CounterManager& cnt_ctx,
                                               const SimOptions&                 opts,
                                               const IsaOptions&                 isa_opts);
  virtual void jit_emit_translation_unit_end  (arcsim::util::CodeBuffer&         buf,
                                               arcsim::sys::cpu::CounterManager& cnt_ctx,
                                               const SimOptions&              opts,
                                               const IsaOptions&              isa_opts);
  
  virtual void jit_emit_block_begin(arcsim::util::CodeBuffer&         buf,
                                    arcsim::sys::cpu::CounterManager& cnt_ctx,
                                    const SimOptions&                 opts,
                                    const IsaOptions&                 isa_opts);
  virtual void jit_emit_block_end  (arcsim::util::CodeBuffer&         buf,
                                    arcsim::sys::cpu::CounterManager& cnt_ctx,
                                    const SimOptions&                 opts,
                                    const IsaOptions&                 isa_opts);

  virtual void jit_emit_instr_begin(arcsim::util::CodeBuffer&         buf,
                                    const arcsim::isa::arc::Dcode&    inst,
                                    uint32                            pc,
                                    arcsim::sys::cpu::CounterManager& cnt_ctx,
                                    const SimOptions&                 opts);
  virtual void jit_emit_instr_end  (arcsim::util::CodeBuffer&         buf,
                                    const arcsim::isa::arc::Dcode&    inst,
                                    uint32                            pc,
                                    arcsim::sys::cpu::CounterManager& cnt_ctx,
                                    const SimOptions&                 opts);
  
  virtual void jit_emit_instr_branch_taken(arcsim::util::CodeBuffer&      buf,
                                           const arcsim::isa::arc::Dcode& inst,
                                           uint32                         pc);
  virtual void jit_emit_instr_branch_not_taken(arcsim::util::CodeBuffer&      buf,
                                               const arcsim::isa::arc::Dcode& inst,
                                               uint32                         pc);
  
  virtual void jit_emit_instr_pipeline_update (arcsim::util::CodeBuffer&      buf,
                                               const arcsim::isa::arc::Dcode& inst,
                                               const char *src1,
                                               const char *src2,
                                               const char *dsc1,
                                               const char *dsc2);
};

#endif /* INC_UARCH_PROCESSOR_PROCESSORPIPELINESKIPJACK_H_ */
