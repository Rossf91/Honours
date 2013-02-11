//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// TranslationEmit is responsible for generating, formatting, and outputting
// JIT code.
//
// =====================================================================

#include <sstream>
#include <string>
#include <iomanip>

#include "api/types.h"

#include "define.h"

#include "Globals.h"

// The following includes vital type definitions needed by the JIT compiler
//
#include "sys/cpu/state.h"
#include "sys/mem/types.h"

#include "translate/TranslationRuntimeApi.h"
#include "translate/TranslationEmit.h"

// The following includes provide code generation methods functional and cycle
// accurate simulation
//
#include "sys/mem/Memory.h"
#include "uarch/memory/MemoryModel.h"
#include "uarch/processor/ProcessorPipelineInterface.h"

#include "util/CodeBuffer.h"

// -----------------------------------------------------------------------------
// TUNIT_VAR_GLOBAL - Types and JIT API function declarations
//
#define TUNIT_VAR_GLOBAL QUOTEME(\
T_TYPES_BASE                     \
T_TYPE_PAGE_CACHE_ENTRY          \
T_TYPE_LATENCY_CACHE_TAG         \
T_TYPE_LATENCY_CACHE_VAL         \
T_TYPE_CPU_CONTEXT               \
E_ENUM_PIPELINE                  \
E_ENUM_MEM_TYPE_TAGS             \
E_ENUM_PENDING_ACTIONS           \
STRUCT_REG_STATS                 \
STRUCT_CPU_STATE                 \
JIT_API_TYPES                    \
JIT_API_RUNTIME_CALLBACKS      \n\
\n)

// -----------------------------------------------------------------------------
// Translation function label
//
void
TranslationEmit::block_label(arcsim::util::CodeBuffer& buf, uint32 addr)
{
  buf.append("L_0x%08x", addr); // assemble block label
}

// -----------------------------------------------------------------------------
// Translation function signature
//
void
TranslationEmit::block_signature(arcsim::util::CodeBuffer& buf, uint32 addr)
{  
  arcsim::util::CodeBuffer name(512*arcsim::B);
  
  block_label(name, addr); // generate function name
  buf.append("\nvoid\n%s(cpuState* const s) {\n", name.get_buffer()); // assemble signature
}


// -----------------------------------------------------------------------------
// Emit everything that needs to be available in a TranslationModule so sub-sequent
// code generation works.
//
bool
TranslationEmit::emit(arcsim::util::CodeBuffer&     buf,
                      const SimOptions&             sim_opts,
                      const IsaOptions&             isa_opts,
                      CoreArch&                     core_arch,
                      arcsim::sys::mem::Memory&     mem,
                      MemoryModel*                  mem_model,
                      ProcessorPipelineInterface*   pipeline,
                      arcsim::sys::cpu::CounterManager&   cnt_ctx) // NOLINT(runtime/references)
{
  bool success = true;

  // Emit datastructures and variables
  buf.append(TUNIT_VAR_GLOBAL);
  
  // Emit function memory access functions
  if (success)
    success = mem.jit_emit_memory_access_functions(buf, core_arch);
  
  // Emit memory model access functions
  if (success
      && (sim_opts.memory_sim || sim_opts.cycle_sim)
      && mem_model) {
    success = mem_model->jit_emit_memory_model_access_functions(buf, core_arch);
  }
  
  // In cycle accurate mode we need to emit pipeline update/accessor functions
  if (success
      && sim_opts.cycle_sim
      && pipeline) {
    pipeline->jit_emit_translation_unit_begin(buf,cnt_ctx,sim_opts,isa_opts);
  }

  return success;
}
