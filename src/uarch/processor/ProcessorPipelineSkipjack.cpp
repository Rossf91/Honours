//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description: @FIXME correct documentation and implementation. Currently
// this class is just a copy of EC5.
//
// Skipjack processor micro-architecture model is implemented in classes derived
// from Pipeline.
//
// Each instruction has two or three pointers to source operand definition
// times, and two pointers to destintation operand definition times.
// Any pointer for which there is not a corresponding input operand will
// point to 'state.t0', and any pointer for which there is not a corresponding
// output operand will point to 'state.ignore'. In this way, there is no need
// to guard the reading and writing of definition times for each operand.
//
// Skipjack has 3 real pipeline stages, but is modeled in terms of 
// 4 stages. The first stage effectively computes the next fetch address,
// making it easier to model the impact of unused branch delay slots.
//
//   FET : next fetch address stage
//   DEC : instruction fetch
//   EX  : instruction execution
//   WB  : memory access and write-back
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

#include <cmath>

#include "uarch/processor/ProcessorPipelineInterface.h"
#include "uarch/processor/ProcessorPipelineSkipjack.h"

#include "arch/Configuration.h"
#include "isa/arc/Opcode.h"
#include "sys/cpu/CounterManager.h"
#include "sys/cpu/processor.h"
#include "ise/eia/EiaInstructionInterface.h"

#include "util/CodeBuffer.h"
#include "util/Log.h"
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
  ENUM_PL_UPDATE_NO_MEM_COND,
  ENUM_PL_UPDATE_NO_DEP_NO_MEM_COND,
  ENUM_PL_UPDATE_NO_OPD_DEP_NO_MEM_COND,
  ENUM_PL_UPDATE_NO_MEM_FLAG,
  ENUM_PL_UPDATE_NO_DEP_NO_MEM_FLAG,
  ENUM_PL_UPDATE_NO_OPD_DEP_NO_MEM_FLAG,
  ENUM_PL_UPDATE_VARIANT_COUNT,
} EnumPipelineUpdateVariant;

// =====================================================================
// FUNCTION PROTOTYPES
// =====================================================================

static void generate_pipeline_update_function(arcsim::util::CodeBuffer&         buf,
                                              const IsaOptions&                 isa_opts,
                                              arcsim::sys::cpu::CounterManager& cnt_ctx,
                                              bool cc,  // is conditional
                                              bool dep, // has dependencies
                                              bool mem, // has memory op
                                              bool dst, // writes destination
                                              bool flag // updates flags
                                              );

// =====================================================================
// METHODS
// =====================================================================

// Pre-computation of model parameters for a specific instruction,
// performed at decode time.
//
bool
ProcessorPipelineSkipjack::precompute_pipeline_model(arcsim::isa::arc::Dcode& inst, const IsaOptions& isa_opts)
{
  const bool success = true;
  
  // Assign the default model parameters for this instruction, and these
  // will apply unless a specifically decoded condition says otherwise.
  //
  inst.br_cycles = 0;     // additional delay applied to next fetch
  inst.extra_cycles  = 0;     // issue cycles (from enter to leave EX)
  inst.pipe_flush  = false; // indicator that inst will flush the pipe
  inst.exe_cycles    = 1;     // default execution latency

  // Execution latencies for ARCv2EM multiplication operations
  // are set here. These represent the number of cycles from the 
  // time at which a producing instruction exits the Execute stage, 
  // to the time at which the using instruction may exit the Execute stage.
  //
  using namespace arcsim::isa::arc;
  switch (inst.code)
  {         
    case OpCode::MPYW:
    case OpCode::MPY:
    case OpCode::MPYU:
    {
      // Set inst.exe_cycles depending on the multiplier configuration
      // and the number of bits of result required by the operator type.
      //
      inst.exe_cycles = isa_opts.mpy_lat_option;
      break;
    }
    case OpCode::MPYH:
    case OpCode::MPYHU:
    {
      inst.exe_cycles = isa_opts.mpy_lat_option;
      
      // The 3-cycle and 4-cycle multiplier options take an extra cycle
      // when computing MPYH operations.
      //
      if ((isa_opts.mpy_lat_option == 3) || (isa_opts.mpy_lat_option == 4))
        inst.exe_cycles += 1;
      break;
    }
    
    case OpCode::DIV:
    case OpCode::DIVU:
    case OpCode::REM:
    case OpCode::REMU:
    {
      if(isa_opts.div_rem_option == 2)
      {
        // When div_rem_option is set to 2, the latency can be approximated as X/2, where X is 
        // ceiling(log2(abs(dividend)+1))+2.
        //
        uint32 dividend = *(inst.src1);
        
        // cast to uint32 is done as uint32 = floor(value), therefore we can get ceiling by 
        // adding 1
        //
        uint32 latency_x_2 = (uint32) (((log2(abs(dividend)+1))+2) + 1); 
        inst.exe_cycles = latency_x_2 / 2;
      }
      else
        inst.exe_cycles = 33;
      break;
    }
    
      // Branch penalties for Bcc, BLcc, Jcc and JLcc depend only on
      // whether there is a delay slot for this instruction.
      // 
    case OpCode::BR:        case OpCode::BCC:
    case OpCode::JCC_SRC1:  case OpCode::JCC_SRC2:
    {
      inst.br_cycles = (inst.dslot ? 0 : 1);
      break;
    }

      // Branch penalties for BRcc and BBITn depend on whether the 
      // turbo mode is enabled. When enabled, this adds one cycle
      // to the overall branch penalty, but operating frequencies
      // are higher.
      //
    case OpCode::BRCC:
    case OpCode::BBIT0: case OpCode::BBIT1:
    {
      inst.br_cycles = (inst.dslot ? 0 : 1);
      
      if (isa_opts.turbo_boost) { inst.extra_cycles = 1; }
      break;
    }
    
      // enter_s and leave_s instructions are multi-cycle, with
      // the number of cycles dependent on the number of registers
      // saved or restored in each case.
      //
    case OpCode::ENTER:
    {
      inst.extra_cycles += inst.shimm            // gpr stores
                       + (inst.link  ? 1 : 0)  // blink store
                       + (inst.dslot ? 2 : 0); // fp store, sp mov
      
      if (inst.extra_cycles != 0) ++inst.extra_cycles;  // sp sub
      break;
    }
    case OpCode::LEAVE:
    {
      inst.extra_cycles += inst.shimm            // gpr loads
                       + (inst.link  ? 1 : 0)  // blink load
                       + (inst.dslot ? 2 : 0); // fp save, sp mov
                  
      // If there some registers to restore, then any return
      // jump will use a delay-slot instruction with lower
      // branch cost.
      //
      if (inst.info.isReturn)
        inst.br_cycles = (inst.extra_cycles != 0) ? 1 : 2;

      break;      
    }
    
      // OpCode::BI, OpCode::BIH and OpCode::JLI_S are all branch instructions
      // that have no delay slot. Hence, they always have a branch cost
      // of 2 cycles.
      //
    case OpCode::BI:
    case OpCode::BIH:
    case OpCode::JLI_S:
      {
        inst.br_cycles = 2;
        break;
      }
    
      // The following operations trigger a pipeline flush.
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
      
      // EIA operations obtain their instruction timings from the
      // extension library that defines each instruction.
      //
    case OpCode::EIA_ZOP:
    case OpCode::EIA_ZOP_F:
    case OpCode::EIA_SOP:
    case OpCode::EIA_SOP_F:
    case OpCode::EIA_DOP:
    case OpCode::EIA_DOP_F:
      {
        // Blocking instructions stall the pipeline from the Execute
        // stage, and for the duration of the instruction's latency.
        // Non-blocking instructions are pipelined, and therefore 
        // affect only the number of execution cycles that must 
        // elapse between producer and consumer.
        //
        if (inst.eia_inst->is_blocking()) {
          inst.extra_cycles = inst.eia_inst->get_cycles();
        } else {
          inst.exe_cycles   = inst.eia_inst->get_cycles();        
        }
        break;
      }

  } /* END switch (inst.code) */
  
  // If there are two destination values to be written to the
  // register file and there is only one write port, then add
  // 1 cycle to inst.extra_cycles to approximate the penalty.
  // in the writeback stage.
  //
  if (!isa_opts.rf_4port && inst.info.rf_wenb1 && inst.info.rf_wenb0) {
    inst.extra_cycles += 1;
  }

  return success; /* always succeeds */
}

// Interpretive model update for the ARCv2EM (EC3) EnCore pipeline.
//
bool
ProcessorPipelineSkipjack::update_pipeline(arcsim::sys::cpu::Processor& cpu)
{
  const bool                         success = true;
  arcsim::isa::arc::Dcode const * const inst = cpu.current_interpreted_inst();
  
  // ---------------------------------------------------------------------------
  //  START OF PIPELINE
  // ---------------------------------------------------------------------------

  CHECK_PIPELINE_INVARIANT(cpu, FET_ST, DEC_ST);
    
  // =====================================================================
  //  DEC Stage - Fetch the instruction
  // =====================================================================
  // JIT: s1 //
  cpu.state.pl[DEC_ST] = cpu.state.pl[FET_ST] + inst->fet_cycles;
  
  // fprintf (stdout, "[ %3d:", inst->fet_cycles);

  CHECK_PIPELINE_INVARIANT(cpu, DEC_ST, EX_ST);
  
  // =====================================================================
  //  EX Stage - EX_ST
  // =====================================================================

  cpu.state.pl[EX_ST] = cpu.state.pl[DEC_ST] + 1;
  
  // If inst uses flags, then synchronize on flag availability time
  //
  if (inst->q_field)
    cpu.state.pl[EX_ST] = MAX2(cpu.state.pl[EX_ST],cpu.state.flag_avail);
  
  // JIT: s2 only if source operands exist
  //
  cpu.state.pl[EX_ST]  = MAX3(cpu.state.pl[EX_ST],
                              *inst->src1_avail,
                              *inst->src2_avail);
  
  // Add any extra stall cycles that have been pre-computed for this
  // instruction. This would include any multi-cycle stalls that are
  // data dependent but known at decode time.
  //
  // JIT: s3 
  //
  cpu.state.pl[DEC_ST] += inst->extra_cycles;
  cpu.state.pl[EX_ST]  += inst->extra_cycles;
  
  CHECK_PIPELINE_INVARIANT(cpu, EX_ST, WB_ST);
  
  // =====================================================================
  //  WB Stage - WB_ST
  // =====================================================================
  
  // JIT: s4 - if this is a load, then mem_cycles will be the latency
  // of reading memory. If this is not a load, then mem_cycles will
  // be 1.
  //
  cpu.state.pl[WB_ST]  = cpu.state.pl[EX_ST] + inst->mem_cycles;

  // Result register availability times can be made more accurate by 
  // taking account of the serialization of writes to the register file
  // in configurations with only one write port.
  //
  // JIT: s5
  //
  *inst->dst1_avail = cpu.state.pl[EX_ST] + inst->exe_cycles;
  *inst->dst2_avail = cpu.state.pl[WB_ST] + 1; // load value is delayed
  
  // If inst defines flags, then set flag next availability time
  //
  if (inst->flag_enable)
    cpu.state.flag_avail = cpu.state.pl[WB_ST];
  
  // Branch penalty applies iff the branch is taken
  //
  // JIT: separate JIT function, called only in 'taken' arm of a branch
  //
  if (inst->taken_branch && !inst->dslot) {
    cpu.state.pl[FET_ST] = cpu.state.pl[EX_ST];
  }

  // Detect pipeline flushes and set the next fetch cycle to be the
  // current Commit-stage cycle when the flush happens.
  // N.B. the requirement to flush the pipeline can be determined statically
  // i.e. at decode time.
  //
  // JIT: inserted for pipe-flushing instructions only
  //
  if (inst->pipe_flush) {
    cpu.state.pl[FET_ST] = cpu.state.pl[WB_ST];
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
  
  // fprintf (stdout, " (%d) %8x %4d :", inst->fetches, cpu.state.ibuff_addr << 1, inst->fet_cycles);
  // fprintf (stdout, " %8d |",   cpu.state.pl[FET_ST]);
  // fprintf (stdout, " %8d |",   cpu.state.pl[DEC_ST]);
  // fprintf (stdout, " %8d |",   cpu.state.pl[EX_ST]);
  // fprintf (stdout, " %8d ]     ",  cpu.state.pl[WB_ST]);
  
  return success;
}

// ----------------- Methods called by JIT during code emission ----------------


// NOLINT(runtime/references)
void
ProcessorPipelineSkipjack::jit_emit_translation_unit_begin(arcsim::util::CodeBuffer&         buf,
                                                           arcsim::sys::cpu::CounterManager& cnt_ctx,
                                                           const SimOptions&                 opts,
                                                           const IsaOptions&                 isa_opts)
{
  bool p[2] = { false, true };
  int  icc, idep, imem, idst, iflag;
  
  // Generate all 32 different variants of pipeline update function
  //
  for (icc = 0; icc < 2; icc++) {
    for (idep = 0; idep < 2; idep++) {
      for (imem = 0; imem < 2; imem++) {
        for (idst = 0; idst < 2; idst++) {
          for (iflag = 0; iflag < 2; iflag++) {
            generate_pipeline_update_function(buf, isa_opts, cnt_ctx,
                                              p[icc], p[idep], p[imem], p[idst], p[iflag]);
          }
        }
      }
    }
  }
}

void
ProcessorPipelineSkipjack::jit_emit_translation_unit_end(arcsim::util::CodeBuffer&         buf,
                                                         arcsim::sys::cpu::CounterManager& cnt_ctx,
                                                         const SimOptions&                 opts,
                                                         const IsaOptions&                 isa_opts)
{
  /* EMPTY */
}


void
ProcessorPipelineSkipjack::jit_emit_block_begin(arcsim::util::CodeBuffer&         buf,
                                                arcsim::sys::cpu::CounterManager& cnt_ctx,
                                                const SimOptions&                 opts,
                                                const IsaOptions&                 isa_opts)
{
  // Emit local variables needed for cycle accurate simulation and create
  // pipeline snapshot.    
  //
  buf.append("\tuint32 fc, mc;\n\tuint64 prev_wb_st;\n");
}

void
ProcessorPipelineSkipjack::jit_emit_block_end  (arcsim::util::CodeBuffer&         buf,
                                                arcsim::sys::cpu::CounterManager& cnt_ctx,
                                                const SimOptions&                 opts,
                                                const IsaOptions&                 isa_opts)
{
  // Update cycle count at the end of a block
  //
  buf.append("*((uint64 * const)(%#p))=s->pl[WB_ST];\n", (void*)cnt_ctx.cycle_count.get_ptr());
}

void
ProcessorPipelineSkipjack::jit_emit_instr_begin(arcsim::util::CodeBuffer&         buf,
                                                const arcsim::isa::arc::Dcode&    inst,
                                                uint32                            pc,
                                                arcsim::sys::cpu::CounterManager& cnt_ctx,
                                                const SimOptions&                 opts)
{
  // Take a snapshot of the commit time of the previous instruction
  // (we use this to compute the time taken by the current instruction)
  //
  if (opts.is_inst_cycle_recording_enabled || opts.is_opcode_latency_distrib_recording_enabled) {
    buf.append("\tprev_wb_st = s->pl[WB_ST];\n");
  }
}

void
ProcessorPipelineSkipjack::jit_emit_instr_end  (arcsim::util::CodeBuffer&         buf,
                                                const arcsim::isa::arc::Dcode&    inst,
                                                uint32                            pc,
                                                arcsim::sys::cpu::CounterManager& cnt_ctx,
                                                const SimOptions&                 opts)
{
  // If this instruction generates a pipeline flush then emit code to
  // model the timing impact of a pipeline flush.
  //
  if (inst.pipe_flush) {
    buf.append("\ts->pl[FET_ST] = s->pl[WB_ST];\n");
  }
  
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
ProcessorPipelineSkipjack::jit_emit_instr_branch_taken(arcsim::util::CodeBuffer&      buf,
                                                       const arcsim::isa::arc::Dcode& inst,
                                                       uint32                         pc)
{
  if (!inst.dslot)
    buf.append("\ts->pl[FET_ST] = s->pl[EX_ST];\n");
}

void
ProcessorPipelineSkipjack::jit_emit_instr_branch_not_taken(arcsim::util::CodeBuffer&      buf,
                                                           const arcsim::isa::arc::Dcode& inst,
                                                           uint32                         pc)
{
  /* No penalty if branch is not taken */
}
 
 
void
ProcessorPipelineSkipjack::jit_emit_instr_pipeline_update(arcsim::util::CodeBuffer&       buf,
                                                          const arcsim::isa::arc::Dcode& inst,
                                                          const char *src1,
                                                          const char *src2,
                                                          const char *dst1,
                                                          const char *dst2
                                                          )
{ // Determine the variant of pl_update that needs to be called, and then call it.
  //
  bool dst  = inst.info.rf_wenb0 || inst.info.rf_wenb1;
  bool dep  = inst.info.rf_renb0 || inst.info.rf_renb1;
  bool mem  = inst.is_memory_kind_inst();
  bool cc   = inst.q_field != 0;
  bool flag = inst.flag_enable;
  
  // function name
  buf.append("\tpl_update");
  if (cc)
    buf.append("_cc");
  if (dep)
    buf.append("_dep");
  if (mem)
    buf.append("_mem");
  if (dst)
    buf.append("_dst");
  if (flag)
    buf.append("_flag");

  // arguments
  buf.append("(s, fc, %d, %d", inst.exe_cycles ,inst.extra_cycles);
  if (dep)
    buf.append(", %s, %s", src1, src2);
  if (mem)
    buf.append(", mc");
  if (dst)
    buf.append(", &(%s), &(%s)", dst1, dst2);
  if (flag || cc)
    buf.append(", &(s->flag_avail)");  
  buf.append(");\n");
}


// ---------------------- C-Code emission helper functions ---------------------

static void
generate_pipeline_update_function(arcsim::util::CodeBuffer&         buf,
                                  const IsaOptions&                 isa_opts,
                                  arcsim::sys::cpu::CounterManager& cnt_ctx,
                                  bool   cc,    // is conditional
                                  bool   dep,   // has dependencies
                                  bool   mem,   // has memory op
                                  bool   dst,   // writes destination
                                  bool   flag   // updates flags
                                  ) // NOLINT(runtime/references)
{  
  // Function prototype
  buf.append("\nstatic inline void pl_update");
  if (cc)
    buf.append("_cc");
  if (dep)
    buf.append("_dep");
  if (mem)
    buf.append("_mem");
  if (dst)
    buf.append("_dst");
  if (flag)
    buf.append("_flag");
  // Function arguments
  buf.append("(cpuState * const s, uint32 fc, uint32 ec, uint32 bc");
  if (dep)
    buf.append(", uint64 src1, uint64 src2");
  if (mem)
    buf.append(", uint32 mc");
  if (dst)
    buf.append(", uint64 *dst1, uint64 *dst2");
  if (flag || cc)
    buf.append(", uint64 *flag_avail");
  buf.append(") {\n");
  
  // Enforce stage 1 pipeline invariant: pl[FET_ST] >= pl[DEC_ST];
  //
  buf.append("if (s->pl[FET_ST] < s->pl[DEC_ST]) s->pl[FET_ST] = s->pl[DEC_ST];\n"); 
  
  // Add fetch cycles to time at which instruction leaves DEC_ST
  //
  buf.append("s->pl[DEC_ST] = s->pl[FET_ST] + fc;\n");

  // Enforce stage 2 pipeline invariant: pl[DEC_ST] >= pl[EX_ST]
  //
  buf.append("if (s->pl[DEC_ST] < s->pl[EX_ST]) s->pl[DEC_ST] = s->pl[EX_ST];\n");
  
  // Instruction cannot leave EX_ST earlier than 1 cycle after it leaves DEC_ST
  //
  buf.append("s->pl[EX_ST] = s->pl[DEC_ST] + 1;\n");

  // If inst uses flags, then earliest time to leave EX_ST is determined
  // by the flag availability time.
  //
  if (cc) {
    buf.append("if (s->pl[EX_ST] < *flag_avail) s->pl[EX_ST] = *flag_avail;\n");
  }

  // If inst has register-based dependencies, then earliest time to
  // leave EX_ST is determined by the lastest resolving dependency.
  //
  if (dep) {
    buf.append("s->pl[EX_ST] = (s->pl[EX_ST] > src1) ? ( (s->pl[EX_ST] > src2) ? s->pl[EX_ST] : src2 ) : ( (src1 > src2) ? src1 : src2 );\n");      
  }
  
  // Generate stall code for turbo mode or blocking instructions if necessary.
  // This involves adding a constant additional latency to the DEC_ST and EX_ST.
  //
  buf.append("s->pl[DEC_ST] += bc; s->pl[EX_ST] += bc;\n");
  
  // Enforce stage 3 pipeline invariant: pl[EX_ST] >= pl[WB_ST]
  //
  buf.append("if (s->pl[EX_ST] < s->pl[WB_ST]) s->pl[EX_ST] = s->pl[WB_ST];\n");
  
  // If there is a memory reference in this instruction, then add the
  // latency cycles to departure time of EX_ST and assign to earliest
  // departure time of WB_ST. 
  // If there is no memory reference, then each instruction will
  // take 1 cycle to get through the WB_ST.
  //
  if (mem) {
    buf.append("s->pl[WB_ST] = s->pl[EX_ST] + mc;\n");
  } else {
    buf.append("s->pl[WB_ST] = s->pl[EX_ST] + 1;\n");  
  }

  // Assign the availability times of destination registers, if any
  // destinations are defined.
  //
  if (dst) {
    buf.append("*dst1 = s->pl[EX_ST] + ec;");
    if (mem)
      buf.append("*dst2 = s->pl[WB_ST] + 1;\n");
    else
      buf.append("\n");
  }
  
  // If the instruction defines flags, then set the flags next availability time
  //
  if (flag) {
    buf.append("*flag_avail = s->pl[WB_ST];\n");
  }
  
  // Check whether cycle count is beyond the timeout value
  //
  if (isa_opts.has_timer0 || isa_opts.has_timer1) {
    buf.append("if (s->pl[WB_ST] > s->timer_expiry) {")
         // commit current cycle count before calling cpuTimerSync method
       .append("\t*((uint64 * const)(%#p))=s->pl[WB_ST];",
               (void*)cnt_ctx.cycle_count.get_ptr())
         // call timer sync method
       .append("\tcpuTimerSync(s->cpu_ctx); }\n");
  }

  buf.append("}\n");                                // close function scope
}

#endif // CYCLE_ACC_SIM



