//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// EC7 Processor micro-architecture model is implemented in classes derived
// from Pipeline.
//
// Each instruction has two or three pointers to source operand definition
// times, and two pointers to destintation operand definition times.
// Any pointer for which there is not a corresponding input operand will
// point to 'state.t0', and any pointer for which there is not a corresponding
// output operand will point to 'state.ignore'. In this way, there is no need
// to guard the reading and writing of definition times for each operand.
//
// EnCore EC7 has 7 PIPELINE_STAGES (see Pipeline enum defined in state.h):
//      FET | ALN | DEC | REG | EX | MEM | WB
//
//
// Read our SAMOS'X paper for further documentation:
// @http://groups.inf.ed.ac.uk/pasta/pub_SAMOS_X_2010.html
//
// =====================================================================


// =====================================================================
// HEADERS
// =====================================================================

#ifdef CYCLE_ACC_SIM
// Only in cycle accurate mode do we compile performance models
//

#include "Assertion.h"

#include "uarch/processor/ProcessorPipelineInterface.h"
#include "uarch/processor/ProcessorPipelineEncore7.h"

#include "isa/arc/Opcode.h"

#include "uarch/bpu/BranchPredictorInterface.h"
#include "sys/cpu/CounterManager.h"
#include "sys/cpu/processor.h"

#include "util/CodeBuffer.h"
#include "util/Counter.h"
#include "util/MultiHistogram.h"

// =====================================================================
// TYPES: pipeline update types
// =====================================================================
typedef enum {
  ENUM_PL_UPDATE,
  ENUM_PL_UPDATE_NO_DEP,
  ENUM_PL_UPDATE_NO_OPD_DEP,
  ENUM_PL_UPDATE_NO_MEM,
  ENUM_PL_UPDATE_NO_DEP_NO_MEM,
  ENUM_PL_UPDATE_NO_OPD_DEP_NO_MEM,
  ENUM_PL_UPDATE_VARIANT_COUNT,
} EnumPipelineUpdateVariant;


// =====================================================================
// FUNCTION PROTOTYPES
// =====================================================================

static void generate_pipeline_update_function(arcsim::util::CodeBuffer&      buf,
                                              const IsaOptions&              isa_opts,
                                              arcsim::sys::cpu::CounterManager& cnt_ctx,
                                              EnumPipelineUpdateVariant      variant);


// =====================================================================
// METHODS
// =====================================================================

// Pre-computation of model parameters for a specific instruction,
// performed at decode time.
//
bool
ProcessorPipelineEncore7::precompute_pipeline_model(arcsim::isa::arc::Dcode& inst, const IsaOptions& isa_opts)
{
  const bool success = true;
  
  // Assign the default model parameters for this instruction, and these
  // will apply unless a specifically decoded condition says otherwise.
  //
  inst.br_cycles = 0;     // additional delay applied to next fetch
  inst.extra_cycles  = 0;     // flat-rate stall applied per pipeline
  inst.pipe_flush  = false; // indicator that inst will flush the pipe

  using namespace arcsim::isa::arc;
  switch (inst.code)
  {
      // The following instruction all flush the pipeline on commit.
      // the model must take this into account.
      //       
    case OpCode::TRAP0:
    case OpCode::RTIE:
    case OpCode::SYNC:
    case OpCode::FLAG:
    case OpCode::SWI:
    case OpCode::BREAK:
    case OpCode::EXCEPTION:
      {
        inst.pipe_flush = true;
        break;
      }
  }
  return success;
}

// Implementation of 7-Stage EnCore pipeline
//
bool
ProcessorPipelineEncore7::update_pipeline(arcsim::sys::cpu::Processor& cpu)
{
  const bool                         success = true;
  arcsim::isa::arc::Dcode const * const inst = cpu.current_interpreted_inst();
  
  // ---------------------------------------------------------------------------
  //  START OF PIPELINE
  // ---------------------------------------------------------------------------
  
  // =====================================================================
  //  FET Stage - FET_ST
  // =====================================================================
  
  cpu.state.pl[FET_ST] += inst->fet_cycles;
  
  CHECK_PIPELINE_INVARIANT(cpu, FET_ST, ALN_ST);
  
  // =====================================================================
  //  ALN Stage - ALN_ST
  // =====================================================================

  cpu.state.pl[ALN_ST] = cpu.state.pl[FET_ST] + 1;
  
  CHECK_PIPELINE_INVARIANT(cpu, ALN_ST, DEC_ST);
  
  // =====================================================================
  //  DEC Stage - DEC_ST
  // =====================================================================
  cpu.state.pl[DEC_ST] = cpu.state.pl[ALN_ST] + 1;
  
  CHECK_PIPELINE_INVARIANT(cpu, DEC_ST, REG_ST);

  // =====================================================================
  //  REG Stage - REG_ST
  // =====================================================================
  cpu.state.pl[REG_ST] = cpu.state.pl[DEC_ST] + 1;

  cpu.state.pl[REG_ST] = MAX3(cpu.state.pl[REG_ST],
                              *inst->src1_avail,
                              *inst->src2_avail);
  
  CHECK_PIPELINE_INVARIANT(cpu, REG_ST, EX_ST);
  
  // =====================================================================
  //  EX Stage - EX_ST
  // =====================================================================
  
  cpu.state.pl[EX_ST]   = cpu.state.pl[REG_ST] + inst->exe_cycles;
  
  *inst->dst1_avail = cpu.state.pl[EX_ST];  // register availability execute of result
  
  CHECK_PIPELINE_INVARIANT(cpu, EX_ST, MEM_ST);
  
  // =====================================================================
  //  MEM/COMMIT Stage - MEM_ST
  // =====================================================================
  
  cpu.state.pl[MEM_ST]  = cpu.state.pl[EX_ST] + inst->mem_cycles;
  
  *inst->dst2_avail = cpu.state.pl[MEM_ST]; // register availability load of result
  
#ifdef ENABLE_BPRED /* defined(ENABLE_BPRED) -> run configurable branch predictor */
  using namespace arcsim::isa::arc;
  if (cpu.bpu) {
    switch (inst->code) {//check for branch instruction
      case OpCode::BCC: //0
      case OpCode::BR: //1
      case OpCode::BRCC: //2
      case OpCode::JCC_SRC1: //3
      case OpCode::JCC_SRC2:  //4
      case OpCode::BBIT0:  //91
      case OpCode::BBIT1: //92
      {
        BranchPredictorInterface::PredictionOutcome
            pred_out = cpu.bpu->commit_branch(cpu.state.pc,                          // current pc
                                              cpu.state.pc + inst->size,         // next sequential pc
                                              cpu.state.next_pc,                     // next pc
                                              cpu.state.pc + inst->link_offset,  // return address
                                              inst->info.isReturn || cpu.state.delayed_return,
                                              inst->link || cpu.state.delayed_call);
        
        bool hit = (   (pred_out == BranchPredictorInterface::CORRECT_PRED_TAKEN)
                    || (pred_out == BranchPredictorInterface::CORRECT_PRED_NOT_TAKEN)
                    || (pred_out == BranchPredictorInterface::CORRECT_PRED_NONE)
                   );
        
        if (hit){
          cpu.state.pl[FET_ST] += cpu.core_arch.bpu.miss_penalty;
        }
        
        if (inst->dslot) {
          cpu.state.delayed_return = inst->info.isReturn;
          cpu.state.delayed_call   = inst->link;
        } else {
          cpu.state.delayed_return = false;
          cpu.state.delayed_call   = false;
        }
      }
    }
  }
  
#else /* !defined(ENABLE_BPRED) -> static branch prediction */
  using namespace arcsim::isa::arc;
  switch (inst->code) {
      // Branch penalties for Bcc, BLcc, Jcc, JLcc that are evaluated in DEC_ST
      //
    case OpCode::BR:        case OpCode::BCC:
    case OpCode::JCC_SRC1:  case OpCode::JCC_SRC2:
    {
      if (!inst->dslot) { // PENALTY for no delay slot (.d)
        cpu.state.pl[FET_ST] = cpu.state.pl[DEC_ST];
      }
      break;
    }
      
      // Branch penalties for BRcc and BBITn are actually evaluated in EX_ST.
      // If ENABLE_BPRED is turned OFF we run static branch prediction code
      // as it is implemented in EC5 Castle and Calton-II.
      //
    case OpCode::BRCC:
    case OpCode::BBIT0: case OpCode::BBIT1:
    {
      bool bwd_and_not_taken = ((inst->jmp_target < cpu.state.pc) && (!inst->taken_branch));
      bool fwd_and_taken     = ((inst->jmp_target > cpu.state.pc) && (inst->taken_branch));
      if (bwd_and_not_taken || fwd_and_taken) { // wrong prediction - add penalty
        if (!inst->dslot) { // no delay slot (.d) specified
          cpu.state.pl[FET_ST] = cpu.state.pl[MEM_ST] ;
        } else {
          cpu.state.pl[FET_ST] = cpu.state.pl[MEM_ST] - 1 ;
        }
      }
      break;
    }
  } /* END switch (inst->code) */
#endif /* ENABLE_BPRED */
  
  CHECK_PIPELINE_INVARIANT(cpu, MEM_ST, WB_ST);
  
  // =====================================================================
  //  WB Stage - WB_ST
  // =====================================================================
  
  cpu.state.pl[WB_ST] = cpu.state.pl[MEM_ST] + 1;
  
  // Detect pipeline flushes and set the next fetch cycle to be the
  // current Commit-stage cycle when the flush happens.
  // N.B. the requirement to flush the pipeline can be determined statically
  // i.e. at decode time.
  //
  if (inst->pipe_flush) {
    cpu.state.pl[FET_ST] = cpu.state.pl[WB_ST] + 1;
  }

  // Update per Opcode latency distribution
  //
  if (cpu.sim_opts.is_opcode_latency_distrib_recording_enabled) {
    cpu.cnt_ctx.opcode_latency_multihist.inc(inst->code,
                                             (uint32)(cpu.state.pl[WB_ST] - cpu.cnt_ctx.cycle_count.get_value()));
  }
  
  // Update per PC cycle count
  //
  if (cpu.sim_opts.is_inst_cycle_recording_enabled) {
    cpu.cnt_ctx.inst_cycles_hist.inc(cpu.state.pc,
                                     (uint32)(cpu.state.pl[WB_ST] - cpu.cnt_ctx.cycle_count.get_value()));
  }
  
  // finally update cycle count
  //
  cpu.cnt_ctx.cycle_count.set_value(cpu.state.pl[WB_ST]);
    
  // Check whether cycle count is beyond the timeout value
  //
  if (cpu.state.pl[WB_ST] > cpu.state.timer_expiry) {
    cpu.timer_sync();
  }

  // =====================================================================
  //  END OF PIPELINE
  // =====================================================================
  
  return success;
}

// ----------------- Methods called by JIT during code emission ----------------


// NOLINT(runtime/references)
void
ProcessorPipelineEncore7::jit_emit_translation_unit_begin(arcsim::util::CodeBuffer&      buf,
                                                          arcsim::sys::cpu::CounterManager& cnt_ctx,
                                                          const SimOptions&              opts,
                                                          const IsaOptions&              isa_opts) // NOLINT(runtime/references)
{
  generate_pipeline_update_function(buf, isa_opts, cnt_ctx, ENUM_PL_UPDATE);
  generate_pipeline_update_function(buf, isa_opts, cnt_ctx, ENUM_PL_UPDATE_NO_DEP);
  generate_pipeline_update_function(buf, isa_opts, cnt_ctx, ENUM_PL_UPDATE_NO_OPD_DEP);
  generate_pipeline_update_function(buf, isa_opts, cnt_ctx, ENUM_PL_UPDATE_NO_MEM);
  generate_pipeline_update_function(buf, isa_opts, cnt_ctx, ENUM_PL_UPDATE_NO_DEP_NO_MEM);
  generate_pipeline_update_function(buf, isa_opts, cnt_ctx, ENUM_PL_UPDATE_NO_OPD_DEP_NO_MEM);
}

void
ProcessorPipelineEncore7::jit_emit_translation_unit_end(arcsim::util::CodeBuffer&      buf,
                                                        arcsim::sys::cpu::CounterManager& cnt_ctx,
                                                        const SimOptions&              opts,
                                                        const IsaOptions&              isa_opts) // NOLINT(runtime/references)
{
  /* EMPTY */
}

void
ProcessorPipelineEncore7::jit_emit_block_begin(arcsim::util::CodeBuffer&      buf,
                                               arcsim::sys::cpu::CounterManager& cnt_ctx,
                                               const SimOptions&              opts,
                                               const IsaOptions&              isa_opts)
{
  // Emit local variables needed for cycle accurate simulation and create
  // pipeline snapshot.    
  //
  buf.append("\tuint32 fc, mc;\n\tuint64 prev_wb_st;\n");  
}

void
ProcessorPipelineEncore7::jit_emit_block_end  (arcsim::util::CodeBuffer&      buf,
                                               arcsim::sys::cpu::CounterManager& cnt_ctx,
                                               const SimOptions&              opts,
                                               const IsaOptions&              isa_opts)
{
  buf.append("*((uint64 * const)(%#p))=s->pl[WB_ST];\n",
             (void*)cnt_ctx.cycle_count.get_ptr());
}


void
ProcessorPipelineEncore7::jit_emit_instr_begin(arcsim::util::CodeBuffer&      buf,
                                               const arcsim::isa::arc::Dcode& inst,
                                               uint32                         pc,
                                               arcsim::sys::cpu::CounterManager& cnt_ctx,
                                               const SimOptions&              opts)
{
  // Take a snapshot of the commit time of the previous instruction
  // (we use this to compute the time taken by the current instruction)
  //
  if (opts.is_inst_cycle_recording_enabled || opts.is_opcode_latency_distrib_recording_enabled) {
    buf.append("\tprev_wb_st = s->pl[WB_ST];\n");
  }
}

void
ProcessorPipelineEncore7::jit_emit_instr_end  (arcsim::util::CodeBuffer&      buf,
                                               const arcsim::isa::arc::Dcode& inst,
                                               uint32                         pc,
                                               arcsim::sys::cpu::CounterManager& cnt_ctx,
                                               const SimOptions&              opts)
{
  if (opts.is_opcode_latency_distrib_recording_enabled) {
    buf.append("\tcpuHistogramInc((void*)(%#p),(uint32)(s->pl[WB_ST] - prev_wb_st));\n",
              (void*)cnt_ctx.opcode_latency_multihist.get_hist_ptr_at_index(inst.code));
  }
  if (opts.is_inst_cycle_recording_enabled) {
    buf.append("\t(*(uint32*)(%#p)) += (uint32)(s->pl[WB_ST] - prev_wb_st);\n",
               (void*)cnt_ctx.inst_cycles_hist.get_value_ptr_at_index(pc));
  }
}

void
ProcessorPipelineEncore7::jit_emit_instr_branch_taken(arcsim::util::CodeBuffer&      buf,
                                                      const arcsim::isa::arc::Dcode& inst,
                                                      uint32 pc)
{
  using namespace arcsim::isa::arc;
  
  switch (inst.code) {
    case OpCode::BR:       case OpCode::BCC:
    case OpCode::JCC_SRC1: case OpCode::JCC_SRC2:
    {
      if (!inst.dslot) { buf.append("\ts->pl[FET_ST] = s->pl[DEC_ST];\n"); }
      break;
    }
      // Conditional branches may cause speculative fetches
      //
    case OpCode::BRCC:  case OpCode::BBIT0:
    case OpCode::BBIT1:
    {
      // Static branch prediction is the default with EC5
      //
      if (inst.jmp_target > pc)
      {
        if (!inst.dslot) { buf.append("\ts->pl[FET_ST] = s->pl[MEM_ST];\n");     }
        else             { buf.append("\ts->pl[FET_ST] = s->pl[MEM_ST] - 1;\n"); }
      }
      break;
    }
    default: { break; }
  }
}

void
ProcessorPipelineEncore7::jit_emit_instr_branch_not_taken(arcsim::util::CodeBuffer&      buf,
                                                          const arcsim::isa::arc::Dcode& inst,
                                                          uint32 pc)
{
  using namespace arcsim::isa::arc;
  
  switch (inst.code) {
    case OpCode::BR:       case OpCode::BCC:
    case OpCode::JCC_SRC1: case OpCode::JCC_SRC2:
    {
      if (!inst.dslot) { buf.append("\ts->pl[FET_ST] = s->pl[DEC_ST];\n"); }
      break;
    }
      // Conditional branches may cause speculative fetches
      //
    case OpCode::BRCC:  case OpCode::BBIT0:
    case OpCode::BBIT1:
    {
      // Static branch prediction is the default with EC5
      //
      if (inst.jmp_target < pc)
      {
        if (!inst.dslot) { buf.append("\ts->pl[FET_ST] = s->pl[MEM_ST];\n");     }
        else             { buf.append("\ts->pl[FET_ST] = s->pl[MEM_ST] - 1;\n"); }
      }
      break;
    }
    default: { break; }
  }
}
  
void
ProcessorPipelineEncore7::jit_emit_instr_pipeline_update (arcsim::util::CodeBuffer&      buf,
                                                          const arcsim::isa::arc::Dcode& inst,
                                                          const char *src1,
                                                          const char *src2,
                                                          const char *dst1,
                                                          const char *dst2)
{
  // Determine the variant of pl_update that needs to be called, and
  // then call it.
  //
  bool dst  = inst.info.rf_wenb0 || inst.info.rf_wenb1;
  bool dep  = inst.info.rf_renb0 || inst.info.rf_renb1;
  bool mem  = inst.is_memory_kind_inst();
  
  // call name
  buf.append("\tpl_update");
  if (!dep && !dst) { // no destination and operand dependencies
    buf.append("_no_dep");
  } else if(!dep)   { // no operand dependencies
    buf.append("_no_opd_dep");
  }
  if (!mem)         // no memory instruction
    buf.append("_no_mem");
  
  // call arguments
  buf.append("(s");
  
  if (!dep && !dst) {       // no destination and operand dependencies
    /* EMPTY */
  } else if (!dep && dst) { // deal with destination dependencies
    buf.append(", &(%s), &(%s)", dst1, dst2);
  } else {                // full dependencies
    buf.append(", %s, %s, &(%s), &(%s)", src1, src2, dst1, dst2);
  }
  
  // Instruction fetch and execution cycles are always part of the parameters
  //
  buf.append(", fc, %d", inst.exe_cycles);
  
  // For memory instructions we call a special function and pass a memory
  // latency parameter
  //
  if (mem)
    buf.append(", mc");
  
  buf.append(");\n");  
}

// ---------------------- C-Code emission helper functions ---------------------

static void
generate_pipeline_update_function(arcsim::util::CodeBuffer&         buf,
                                  const IsaOptions&                 isa_opts,
                                  arcsim::sys::cpu::CounterManager& cnt_ctx,
                                  EnumPipelineUpdateVariant         variant) // NOLINT(runtime/references)
{
  buf.append("\nstatic inline void "); // return type definition is always the same
  
  switch (variant) {
    case ENUM_PL_UPDATE: {
      buf.append("pl_update(cpuState * const s, uint64 src1, uint64 src2, uint64 *dst1, uint64 *dst2, uint32 fc, uint32 ec, uint32 mc) {\n")  
         .append("s->pl[FET_ST] += fc; if (s->pl[FET_ST] < s->pl[ALN_ST]) s->pl[FET_ST] = s->pl[ALN_ST];\n")
         .append("s->pl[ALN_ST] = s->pl[FET_ST] + 1; if (s->pl[ALN_ST] < s->pl[DEC_ST]) s->pl[ALN_ST] = s->pl[DEC_ST];\n")
         .append("s->pl[DEC_ST] = s->pl[ALN_ST] + 1; if (s->pl[DEC_ST] < s->pl[REG_ST]) s->pl[DEC_ST] = s->pl[REG_ST];\n")
         .append("s->pl[REG_ST] = s->pl[DEC_ST] + 1;\n")
         .append("s->pl[REG_ST] = (s->pl[REG_ST] > src1) ? ( (s->pl[REG_ST] > src2) ? s->pl[REG_ST] : src2 ) : ( (src1 > src2) ? src1 : src2 );\n")
         .append("if (s->pl[REG_ST] < s->pl[EX_ST]) s->pl[REG_ST] = s->pl[EX_ST];\n")
         .append("s->pl[EX_ST]  = *dst1 = s->pl[REG_ST] + ec; if (s->pl[EX_ST]  < s->pl[MEM_ST]) s->pl[EX_ST]  = s->pl[MEM_ST];\n")
         .append("s->pl[MEM_ST] = *dst2 = s->pl[EX_ST]  + mc; if (s->pl[MEM_ST] < s->pl[WB_ST])  s->pl[MEM_ST] = s->pl[WB_ST];\n");
      break;
    }
    case ENUM_PL_UPDATE_NO_DEP: {
      buf.append("pl_update_no_dep (cpuState * const s, uint32 fc, uint32 ec, uint32 mc) {\n")
         .append("s->pl[FET_ST] += fc; if (s->pl[FET_ST] < s->pl[ALN_ST]) s->pl[FET_ST] = s->pl[ALN_ST];\n")
         .append("s->pl[ALN_ST] = s->pl[FET_ST] + 1; if (s->pl[ALN_ST] < s->pl[DEC_ST]) s->pl[ALN_ST] = s->pl[DEC_ST];\n")
         .append("s->pl[DEC_ST] = s->pl[ALN_ST] + 1; if (s->pl[DEC_ST] < s->pl[REG_ST]) s->pl[DEC_ST] = s->pl[REG_ST];\n")
         .append("s->pl[REG_ST] = s->pl[DEC_ST] + 1;  if (s->pl[REG_ST] < s->pl[EX_ST])  s->pl[REG_ST] = s->pl[EX_ST];\n")
         .append("s->pl[EX_ST]  = s->pl[REG_ST] + ec; if (s->pl[EX_ST]  < s->pl[MEM_ST]) s->pl[EX_ST]  = s->pl[MEM_ST];\n")
         .append("s->pl[MEM_ST] = s->pl[EX_ST]  + mc; if (s->pl[MEM_ST] < s->pl[WB_ST])  s->pl[MEM_ST] = s->pl[WB_ST];\n");
      break;
    }
    case ENUM_PL_UPDATE_NO_OPD_DEP: {
      buf.append("pl_update_no_opd_dep (cpuState * const s, uint64 *dst1, uint64 *dst2, uint32 fc, uint32 ec, uint32 mc) {\n")
         .append("s->pl[FET_ST] += fc; if (s->pl[FET_ST] < s->pl[ALN_ST]) s->pl[FET_ST] = s->pl[ALN_ST];\n")
         .append("s->pl[ALN_ST] = s->pl[FET_ST] + 1; if (s->pl[ALN_ST] < s->pl[DEC_ST]) s->pl[ALN_ST] = s->pl[DEC_ST];\n")
         .append("s->pl[DEC_ST] = s->pl[ALN_ST] + 1; if (s->pl[DEC_ST] < s->pl[REG_ST]) s->pl[DEC_ST] = s->pl[REG_ST];\n")
         .append("s->pl[REG_ST] = s->pl[DEC_ST] + 1;  if (s->pl[REG_ST] < s->pl[EX_ST])  s->pl[REG_ST] = s->pl[EX_ST];\n")
         .append("s->pl[EX_ST]  = *dst1 = s->pl[REG_ST] + ec; if (s->pl[EX_ST]  < s->pl[MEM_ST]) s->pl[EX_ST]  = s->pl[MEM_ST];\n")
         .append("s->pl[MEM_ST] = *dst2 = s->pl[EX_ST]  + mc; if (s->pl[MEM_ST] < s->pl[WB_ST])  s->pl[MEM_ST] = s->pl[WB_ST];\n");
      break;
    }
    case ENUM_PL_UPDATE_NO_MEM: {
      buf.append("pl_update_no_mem (cpuState * const s, uint64 src1, uint64 src2,uint64 *dst1, uint64 *dst2, uint32 fc, uint32 ec) {\n")
         .append("s->pl[FET_ST] += fc; if (s->pl[FET_ST] < s->pl[ALN_ST]) s->pl[FET_ST] = s->pl[ALN_ST];\n")
         .append("s->pl[ALN_ST] = s->pl[FET_ST] + 1; if (s->pl[ALN_ST] < s->pl[DEC_ST]) s->pl[ALN_ST] = s->pl[DEC_ST];\n")
         .append("s->pl[DEC_ST] = s->pl[ALN_ST] + 1; if (s->pl[DEC_ST] < s->pl[REG_ST]) s->pl[DEC_ST] = s->pl[REG_ST];\n")
         .append("s->pl[REG_ST] = s->pl[DEC_ST] + 1;\n")
         .append("s->pl[REG_ST] = (s->pl[REG_ST] > src1) ? ( (s->pl[REG_ST] > src2) ? s->pl[REG_ST] : src2 ) : ( (src1 > src2) ? src1 : src2 );\n")
         .append("if (s->pl[REG_ST] < s->pl[EX_ST])  s->pl[REG_ST] = s->pl[EX_ST];\n")
         .append("s->pl[EX_ST]  = *dst1 = s->pl[REG_ST] + ec; if (s->pl[EX_ST]  < s->pl[MEM_ST]) s->pl[EX_ST]  = s->pl[MEM_ST];\n")
         .append("s->pl[MEM_ST] = *dst2 = s->pl[EX_ST] + 1; if (s->pl[MEM_ST] < s->pl[WB_ST]) s->pl[MEM_ST] = s->pl[WB_ST];\n");
      break;
    }
    case ENUM_PL_UPDATE_NO_DEP_NO_MEM: {
      buf.append("pl_update_no_dep_no_mem (cpuState * const s, uint32 fc, uint32 ec) {\n")
         .append("s->pl[FET_ST] += fc; if (s->pl[FET_ST] < s->pl[ALN_ST]) s->pl[FET_ST] = s->pl[ALN_ST];\n")
         .append("s->pl[ALN_ST] = s->pl[FET_ST] + 1; if (s->pl[ALN_ST] < s->pl[DEC_ST]) s->pl[ALN_ST] = s->pl[DEC_ST];\n")
         .append("s->pl[DEC_ST] = s->pl[ALN_ST] + 1; if (s->pl[DEC_ST] < s->pl[REG_ST]) s->pl[DEC_ST] = s->pl[REG_ST];\n")
         .append("s->pl[REG_ST] = s->pl[DEC_ST] + 1;  if (s->pl[REG_ST] < s->pl[EX_ST])  s->pl[REG_ST] = s->pl[EX_ST];\n")
         .append("s->pl[EX_ST]  = s->pl[REG_ST] + ec; if (s->pl[EX_ST]  < s->pl[MEM_ST]) s->pl[EX_ST] = s->pl[MEM_ST];\n")
         .append("s->pl[MEM_ST] = s->pl[EX_ST]  + 1; if (s->pl[MEM_ST] < s->pl[WB_ST]) s->pl[MEM_ST] = s->pl[WB_ST];\n");
      break;
    }
    case ENUM_PL_UPDATE_NO_OPD_DEP_NO_MEM: {
      buf.append("pl_update_no_opd_dep_no_mem (cpuState * const s, uint64 *dst1, uint64 *dst2, uint32 fc, uint32 ec) {\n")
         .append("s->pl[FET_ST] += fc; if (s->pl[FET_ST] < s->pl[ALN_ST]) s->pl[FET_ST] = s->pl[ALN_ST];\n")
         .append("s->pl[ALN_ST] = s->pl[FET_ST] + 1; if (s->pl[ALN_ST] < s->pl[DEC_ST]) s->pl[ALN_ST] = s->pl[DEC_ST];\n")
         .append("s->pl[DEC_ST] = s->pl[ALN_ST] + 1; if (s->pl[DEC_ST] < s->pl[REG_ST]) s->pl[DEC_ST] = s->pl[REG_ST];\n")
         .append("s->pl[REG_ST] = s->pl[DEC_ST] + 1;  if (s->pl[REG_ST] < s->pl[EX_ST])  s->pl[REG_ST] = s->pl[EX_ST];\n")
         .append("s->pl[EX_ST]  = *dst1 = s->pl[REG_ST] + ec; if (s->pl[EX_ST]  < s->pl[MEM_ST]) s->pl[EX_ST] = s->pl[MEM_ST];\n")
         .append("s->pl[MEM_ST] = *dst2 = s->pl[EX_ST]  + 1; if (s->pl[MEM_ST] < s->pl[WB_ST])  s->pl[MEM_ST] = s->pl[WB_ST];\n");
      break;
    }
    default: { UNIMPLEMENTED(); }
  }
  
  // Last statement is always the same
  buf.append("s->pl[WB_ST]  = s->pl[MEM_ST] + 1;"); 
  
  // Check whether cycle count is beyond the timeout value
  //
  if (isa_opts.has_timer0 || isa_opts.has_timer1) {
    buf.append("\nif (s->pl[WB_ST] > s->timer_expiry) {")
       // commit current cycle count before calling cpuTimerSync method
       .append("\t*((uint64 * const)(%#p))=s->pl[WB_ST];", (void*)cnt_ctx.cycle_count.get_ptr())
       // call timer sync method
       .append("\tcpuTimerSync(s->cpu_ctx); }");
  }
  
  buf.append("\n}\n");                 // close function scope
}

#endif // CYCLE_ACC_SIM

