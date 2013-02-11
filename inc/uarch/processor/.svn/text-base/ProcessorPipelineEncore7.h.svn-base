//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================

#ifndef _processor_pipline_encore_7_h_
#define _processor_pipline_encore_7_h_

// =====================================================================
// HEADERS
// =====================================================================

#include "uarch/processor/ProcessorPipelineInterface.h"

// =====================================================================
// CLASSES
// =====================================================================

// 7-stage EnCore Castle pipeline
//
class ProcessorPipelineEncore7 : public ProcessorPipelineInterface {
public:
  
  ProcessorPipelineEncore7() { /* EMPTY */ }
  ~ProcessorPipelineEncore7() { /* EMPTY */ }
  
  virtual bool precompute_pipeline_model(arcsim::isa::arc::Dcode& inst, const IsaOptions& isa_opts);

  virtual bool update_pipeline(arcsim::sys::cpu::Processor& _cpu);
  
  virtual void jit_emit_translation_unit_begin(arcsim::util::CodeBuffer&         buf,
                                               arcsim::sys::cpu::CounterManager& cnt_ctx,
                                               const SimOptions&                 opts,
                                               const IsaOptions&                 isa_opts);
  virtual void jit_emit_translation_unit_end(arcsim::util::CodeBuffer&         buf,
                                             arcsim::sys::cpu::CounterManager& cnt_ctx,
                                             const SimOptions&                 opts,
                                             const IsaOptions&                 isa_opts);
  
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

  virtual void jit_emit_instr_branch_taken    (arcsim::util::CodeBuffer&      buf,
                                               const arcsim::isa::arc::Dcode& inst,
                                               uint32                         pc);
  virtual void jit_emit_instr_branch_not_taken(arcsim::util::CodeBuffer&      buf,
                                               const arcsim::isa::arc::Dcode& inst,
                                               uint32                         pc);
  
  virtual void jit_emit_instr_pipeline_update (arcsim::util::CodeBuffer&         buf,
                                               const arcsim::isa::arc::Dcode&   inst,
                                               const char *src1,
                                               const char *src2,
                                               const char *dst1,
                                               const char *dst2);
};

#endif /* _processor_pipline_encore_7_h_ */
