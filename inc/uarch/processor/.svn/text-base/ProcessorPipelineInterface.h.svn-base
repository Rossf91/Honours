//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Processor micro-architecture model is implemented in classes derived
// from ProcessorPipeline.
//
// =====================================================================

#ifndef INC_UARCH_PROCESSOR_PROCESSORPIPELINEINTERFACE_H_
#define INC_UARCH_PROCESSOR_PROCESSORPIPELINEINTERFACE_H_

#include "api/types.h"

// --------------------------------------------------------------------------
// MACROS
//

// Pipeline invariant macro
//
#define CHECK_PIPELINE_INVARIANT(_CPU, _STAGE_CUR, _STAGE_NEXT)                \
  if (_CPU.state.pl[_STAGE_CUR] < _CPU.state.pl[_STAGE_NEXT]) {                \
    _CPU.state.pl[_STAGE_CUR] = _CPU.state.pl[_STAGE_NEXT];                    \
}


// --------------------------------------------------------------------------
// FORWARD DECLARATIONS
//

namespace arcsim {
  namespace isa {
    namespace arc {
      class Dcode;
  } }
  namespace util {
    class CodeBuffer;
  }
  
  namespace sys {
    namespace cpu {
      class Processor;
      class CounterManager;  
} } }

class IsaOptions;
class SimOptions;

// Interface class for processor pipeline
//
class ProcessorPipelineInterface
{
protected:
  // Constructor MUST be protected and empty!
  //
  ProcessorPipelineInterface ()
  { /* EMPTY */ }

public:
  
  /**
   * Destructor of interface must be declared AND virtual so all implementations
   * of this interface can be destroyed correctly.
   */
  virtual ~ProcessorPipelineInterface ()
  { /* EMPTY */ }
    
  // Abstract method.
  // This method is called by the decoder to allow instruction timing
  // behaviour to be pre-computed and cached with the decoded instruction
  // for later use by the performance model.
  //
  virtual bool precompute_pipeline_model(arcsim::isa::arc::Dcode& inst,
                                         const IsaOptions&        isa_opts) = 0;
  
  // Abstract method.
  // This method is called in interpretive mode to update the microarchitectural
  // performance model.
  //
  virtual bool update_pipeline(arcsim::sys::cpu::Processor& cpu) = 0;
  
  
  // ---------------------------------------------------------------------------
    
  // Abstract methods called by the JIT at the beginning and end of a translation unit
  //
  virtual void jit_emit_translation_unit_begin(arcsim::util::CodeBuffer&         buf,
                                               arcsim::sys::cpu::CounterManager& cnt_ctx,
                                               const SimOptions&                 opts,
                                               const IsaOptions&                 isa_opts) = 0;
  virtual void jit_emit_translation_unit_end  (arcsim::util::CodeBuffer&         buf,
                                               arcsim::sys::cpu::CounterManager& cnt_ctx,
                                               const SimOptions&                 opts,
                                               const IsaOptions&                 isa_opts) = 0;
  
  // Abstract methods called by the JIT at the beginning, and at the end, of a block
  //
  virtual void jit_emit_block_begin(arcsim::util::CodeBuffer&          buf,
                                    arcsim::sys::cpu::CounterManager&  cnt_ctx,
                                    const SimOptions&                  opts,
                                    const IsaOptions&                  isa_opts) = 0;
  virtual void jit_emit_block_end  (arcsim::util::CodeBuffer&          buf,
                                    arcsim::sys::cpu::CounterManager&  cnt_ctx,
                                    const SimOptions&                  opts,
                                    const IsaOptions&                  isa_opts) = 0;
  
  
  // Abstract methods called by the JIT at the beginning, and at the end, of each instruction
  //
  virtual void jit_emit_instr_begin(arcsim::util::CodeBuffer&          buf,
                                    const arcsim::isa::arc::Dcode&     inst,
                                    uint32                             pc,
                                    arcsim::sys::cpu::CounterManager&  cnt_ctx,
                                    const SimOptions&                  opts) = 0;
  virtual void jit_emit_instr_end  (arcsim::util::CodeBuffer&          buf,
                                    const arcsim::isa::arc::Dcode&     inst,
                                    uint32                             pc,
                                    arcsim::sys::cpu::CounterManager&  cnt_ctx,
                                    const SimOptions&                  opts) = 0;
  
  
  // Abstract methods called by the JIT in case of taken or not-taken branch
  //
  virtual void jit_emit_instr_branch_taken     (arcsim::util::CodeBuffer&      buf,
                                                const arcsim::isa::arc::Dcode& inst,
                                                uint32                         pc) = 0;
  virtual void jit_emit_instr_branch_not_taken (arcsim::util::CodeBuffer&      buf,
                                                const arcsim::isa::arc::Dcode& inst,
                                                uint32                         pc) = 0;
  
  // Abstract method called by the JIT to update the pipeline timing model,
  // after the sub-methods for instruction begin, instruciton fetch and memory access
  // have been called.
  //
  virtual void jit_emit_instr_pipeline_update (arcsim::util::CodeBuffer&      buf,
                                               const arcsim::isa::arc::Dcode& inst,
                                               const char *src1,
                                               const char *src2,
                                               const char *dsc1,
                                               const char *dsc2
                                               ) = 0;

};

// -----------------------------------------------------------------------------


#endif // INC_UARCH_PROCESSOR_PROCESSORPIPELINEINTERFACE_H_

