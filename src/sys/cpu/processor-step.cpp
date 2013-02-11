//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2005 The University of Edinburgh
//                        All Rights Reserved
//
//
// =====================================================================
//
// Description:
//
// This file implements the 'step' and 'step_fast' methods defined in the
// Processor class. NOTE that this file is included into processor.cpp
//
// =====================================================================

// -----------------------------------------------------------------------------
// MACRO DEFINITIONS
//

#include "sys/cpu/enter-leave.h"

// NOTE: THE FOLLOWING MACRO DEFINITIONS MUST BE GUARDED
//
#ifndef SRC_INTERNAL_SYSTEM_CPU_PROCESSOR_STEP_CPP
#define SRC_INTERNAL_SYSTEM_CPU_PROCESSOR_STEP_CPP

#define WORD_MASK(addr) 0xffffffff
#define HALF_MASK(addr) 0xffff << ((((addr) >> 1) & 0x1) * 16)
#define BYTE_MASK(addr) 0xff   << (((addr) & 0x3) * 8)

// This macro performs a conditional instruction execution check by short 
// circuiting the common (true) case of q_field == 0 before actually evaluating
// the conditional test using Z, N, C and V bits
//
#define IF_CC(inst,state) if (   (commit = (inst->q_field == 0)   ) \
                              || (commit = eval_cc(inst->q_field) ))

// This macro performs a check to see if the state.D bit is set, and to raise an
// Illegal Instruction Sequence exception if it is. This check is performed before
// interpretation of any instruction that is disallowed in the delay slot of a
// taken branch or jump instruction.
//
#define START_ILLEGAL_IN_DSLOT                                                  \
  if (state.D || state.ES) {                                                    \
    enter_exception(ECR(sys_arch.isa_opts.EV_InstructionError, IllegalSequence, 0),\
                    state.pc,                                                   \
                    state.pc);                                                  \
    commit = false;                                                             \
  } else {

#define END_ILLEGAL_IN_DSLOT }

// Recorded addresses accessed by memory operations in memory model address queue
//
#define MEMORY_ACCESS(_addr) \
  do { if (sim_opts.memory_sim) mem_model->addr_queue.push(_addr); } while(0)


#ifdef STEP
  #define IF_STEP_TRACE_INSTR(...) TRACE_INST(__VA_ARGS__);
#else
  #define IF_STEP_TRACE_INSTR(...)
#endif


#endif // SRC_INTERNAL_SYSTEM_CPU_PROCESSOR_STEP_CPP

// -----------------------------------------------------------------------------
// Processor::step() is used to single step through a program
// with the capability to trace execution of each instruction
// In the checking mode (check == true), this function is used to
// evaluate the effect of executing an instruction for the purposes
// of comparing its result with that of translated code.
//
// Arguments:
//
//  UpdatePacket *delta  - ptr to structure containing state changes
//  bool          check  - true if verifying translated code
//
//
// Processor::step_single_fast () is used to interpret a program as fast as
// possible, and hence without the capability to trace execution of
// each instruction.
//
bool
#ifdef STEP
Processor::step_single (UpdatePacket *delta, bool check)
#endif
#ifdef STEP_FAST
Processor::step_single_fast ()
#endif
{
  uint32  efa                  = 0;
  uint32  ecause               = 0;
  
  bool    next_D               = false;
  bool    next_E               = false;
  bool    commit               = true;
  bool    loop_back            = false;

  bool    return_before_commit = false;

#ifdef STEP
  bool    trace_loop_count     = false; // true for ZOL loop-back instructions
#endif
    
  // Reset flag indicating whether an exception should be raised
  state.raise_exception = 0;
  
  // Pre-decode next instruction
  if ((ecause = decode_instruction_using_cache (state.pc, inst, efa, prev_had_dslot))) {
    enter_exception (ecause, efa, state.pc);

    IF_STEP_TRACE_INSTR(this, trace_exception())
    
    return !state.H;
  }
  
  // ---------------------------------------------------------------------------
  // HandleBeginInstructionExecutionIPT check
  //
  if (ipt_mgr.is_enabled()) {
    if (ipt_mgr.is_enabled(arcsim::ipt::IPTManager::kIPTBeginInstruction))
      ipt_mgr.notify_begin_instruction_execution_ipt_handlers(state.pc, inst->size);
  }
  
  // ---------------------------------------------------------------------------
  // In case Actionpoints might be enabled, copy the pre-decoded breakpoint
  // Actionpoint set that triggers on this instruction. These may be combined
  // with other Actionpoints to trigger a compound condition later on.
  //
  aps.init_aps_matches(inst->aps_inst_matches);
  
  // ---------------------------------------------------------------------------
  // EIA extensions
  //
  if (eia_mgr.any_eia_extensions_defined) {
    // Check if any XPU permissions are required by this instruction
    // when operating in User mode. If so, then check current XPU
    // register against all extension permissions required by this
    // instruction.
    //
    if (inst->xpu_required) {
      if (state.U && ((inst->xpu_required & state.auxs[AUX_XPU]) != inst->xpu_required)) {
        uint32 missing_permission = inst->xpu_required & ~state.auxs[AUX_XPU];
        uint32 extension = 0;
        while (missing_permission >>= 1) ++extension;
        enter_exception (ECR(sys_arch.isa_opts.EV_PrivilegeV, DisabledExtension, extension), efa, state.pc);
        IF_STEP_TRACE_INSTR(this, trace_exception())
        return !state.H;
      }
    }
  }

#ifdef STEP
  // Skip former parts of instruction trace, including disassembly, for cosim
  if (!sim_opts.cosim) {
    IF_STEP_TRACE_INSTR(this, trace_instruction(state.pc, inst->info.ir, inst->limm))
  }
#endif

  // At this point we do not know if branch is taken so we initilise to 'false'
  //
  inst->taken_branch = false;

  // Logic to determine if the end of a basic block has been reached
  // `prev_had_dslot' is true iff the previously-stepped instruction
  // had a delay-slot. That being the case, this instruction is the
  // last one of the current basic block, so we set end_of_block.
  // We also clear prev_had_dslot so that it can be set for this
  // branch or jump instruction if required.
  //
  end_of_block   = prev_had_dslot;
  prev_had_dslot = false;

  // Compute the next PC value (which may be overridden by non-delayed
  // branches executed in this cycle).
  //
  if (state.D || state.ES) { // this is a delay-slot or execution-slot instruction
    state.next_pc  = state.auxs[AUX_BTA];
    
    // If this instruction has LIMM data, and it is in a delay-slot,
    // then it must raise an Illegal Instruction Sequence exception.
    //
    if (inst->has_limm && state.D) {
      enter_exception(ECR(sys_arch.isa_opts.EV_InstructionError, IllegalSequence, 0), state.pc, state.pc);
      IF_STEP_TRACE_INSTR(this, trace_exception());
      return !state.H;
    }
    
    // Check if we have reached the end of a zero-overhead loop.
    // If end of loop body is detected, and looping is not disabled,
    // then LP_COUNT must be decremented. Note, this happens
    // even if this last loop-body instruction is in the
    // delay-slot of a taken branch, which will override the
    // branching part of the loop-back mechanism.
    //
    // if and ei_s instruction targets this instruction we don't want to change the loop counter
    //
    if ( ((state.pc + inst->size) == state.auxs[AUX_LP_END])
          && !state.L  && !state.ES) {

      LOG(LOG_DEBUG4) << "Decrementing LP_COUNT";
      state.next_lpc = state.gprs[LP_COUNT] - 1;
#ifdef STEP
      trace_loop_count = true;
#endif
    } else {
      state.next_lpc = state.gprs[LP_COUNT];
    }
  } else {
    // Compute next sequential PC value, as this will be used if this instruction
    // does not branch; and it is also used to detect the case where we have reached
    // the end of a zero-overhead loop.
    //
    state.next_pc = state.pc + inst->size;

    // Check for zero-overhead loop-back, setting end_of_block
    // if end of loop body is detected and looping is not disabled
    // by:
    //    * the STATUS32[L] bit being set
    //    * the STATUS32[D] bit being set
    //
    if ((state.next_pc == state.auxs[AUX_LP_END])
          && !state.D
          && !state.ES
          && !state.L ) {

      if (state.gprs[LP_COUNT] != 1) {
        state.next_pc = state.auxs[AUX_LP_START];
        loop_back     = true;
        // If the current instruction is a branch or jump with a delay slot, 
        // then we must raise an Illegal Instruction Sequence exception.
        //
        if (inst->has_dslot_inst()) {
          enter_exception(ECR(sys_arch.isa_opts.EV_InstructionError, IllegalSequence, 0), state.pc, state.pc);
          IF_STEP_TRACE_INSTR(this, trace_exception());
          return !state.H;
        }
      }

      end_of_block = true;
      
      LOG(LOG_DEBUG4) << "Decrementing LP_COUNT";
      state.next_lpc = state.gprs[LP_COUNT] - 1;
#ifdef STEP
      trace_loop_count = true;
#endif
    } else {
      state.next_lpc = state.gprs[LP_COUNT];
    }
  }

  // ---------------------------------------------------------------------------
  //
  // This is the main interpreter switch for each opcode
  //
  //
  using namespace arcsim::isa::arc;
  switch (inst->code)
  {
    case OpCode::BCC:
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block   = !inst->dslot;
          prev_had_dslot = inst->dslot;
        
          if ((commit = eval_cc (inst->q_field) ))
          {
            *(inst->dst2) = inst->jmp_target;
            inst->taken_branch = true;
            next_D = inst->dslot;
            if (inst->link) {
              *(inst->dst1) = state.pc + inst->link_offset;
              // Profiling counters
              //
              if (sim_opts.is_call_freq_recording_enabled)  {
                cnt_ctx.call_freq_hist.inc(*(inst->dst2));
              }
              if (sim_opts.is_call_graph_recording_enabled) {
                cnt_ctx.call_graph_multihist.inc(state.pc,*(inst->dst2));
              }
            }
            // Profiling counters
            //
            if (sim_opts.is_dkilled_recording_enabled
                && !inst->dslot) {
              cnt_ctx.dkilled_freq_hist.inc(state.pc + inst->size);
            }
          }
        END_ILLEGAL_IN_DSLOT
        break;
      }

    case OpCode::BR:
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block   = !inst->dslot;
          prev_had_dslot = inst->dslot;
        
          *(inst->dst2) = inst->jmp_target;
          inst->taken_branch = true;
          next_D = inst->dslot;
          if (inst->link) {
            *(inst->dst1) = state.pc + inst->link_offset;
            // Profiling counters
            //
            if (sim_opts.is_call_freq_recording_enabled)  {
              cnt_ctx.call_freq_hist.inc(*(inst->dst2));
            }
            if (sim_opts.is_call_graph_recording_enabled) {
              cnt_ctx.call_graph_multihist.inc(state.pc,*(inst->dst2));
            }
          }          
          // Profiling counters
          //
          if (sim_opts.is_dkilled_recording_enabled
              && !inst->dslot) {
            cnt_ctx.dkilled_freq_hist.inc(state.pc + inst->size);
          }
        END_ILLEGAL_IN_DSLOT
        break;
      }

    case OpCode::BRCC:
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block   = !inst->dslot;
          prev_had_dslot = inst->dslot;
        
          switch (inst->q_field) {
            case BREQ_COND: { inst->taken_branch = (*(inst->src1) == *(inst->src2)); break; }
            case BRNE_COND: { inst->taken_branch = (*(inst->src1) != *(inst->src2)); break; }
            case BRLT_COND: { inst->taken_branch = (static_cast<sint32>(*(inst->src1)) <  static_cast<sint32>(*(inst->src2))); break; }
            case BRGE_COND: { inst->taken_branch = (static_cast<sint32>(*(inst->src1)) >= static_cast<sint32>(*(inst->src2))); break; }
            case BRLO_COND: { inst->taken_branch = (static_cast<uint32>(*(inst->src1)) <  static_cast<uint32>(*(inst->src2))); break; }
            case BRHS_COND: { inst->taken_branch = (static_cast<uint32>(*(inst->src1)) >= static_cast<uint32>(*(inst->src2))); break; }
            default: {
              LOG(LOG_ERROR)
                <<  "**Error: Unexpected BRcc condition ("
                << inst->q_field
                << ") at PC=" << std::hex << std::setw(8) << std::setfill('0')
                << state.pc;
            }
          }

          if (inst->taken_branch) {
            *(inst->dst2) = inst->jmp_target;
            next_D = inst->dslot;
            // Profiling counters ----------------------------------------------
            //
            if (sim_opts.is_dkilled_recording_enabled && !inst->dslot) {
              cnt_ctx.dkilled_freq_hist.inc(state.pc + inst->size);
            }
          } else {
            commit = false;
          }
        END_ILLEGAL_IN_DSLOT
        break;
      }

    case OpCode::BBIT0:
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block = !inst->dslot;
          prev_had_dslot = inst->dslot;
          if ((commit = ((*(inst->src1) & (1 << (*(inst->src2) & 31) ))) == 0))
          {
            *(inst->dst2) = inst->jmp_target;
            next_D = inst->dslot;
            inst->taken_branch = true;
            // Profiling counters ----------------------------------------------
            //
            if (sim_opts.is_dkilled_recording_enabled && !inst->dslot) {
              cnt_ctx.dkilled_freq_hist.inc(state.pc + inst->size);
            }
          }
        END_ILLEGAL_IN_DSLOT
        break;
      }

    case OpCode::BBIT1:
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block = !inst->dslot;
          prev_had_dslot = inst->dslot;
          if ((commit = ((*(inst->src1) & (1 << (*(inst->src2) & 31) ))) != 0))
          {
            *(inst->dst2) = inst->jmp_target;
            next_D = inst->dslot;
            inst->taken_branch = true;
            // Profiling counters ----------------------------------------------
            //
            if (sim_opts.is_dkilled_recording_enabled && !inst->dslot) {
              cnt_ctx.dkilled_freq_hist.inc(state.pc + inst->size);
            }
          }
        END_ILLEGAL_IN_DSLOT
        break;
      }

    case OpCode::JCC_SRC1:
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block   = !inst->dslot;
          prev_had_dslot = inst->dslot;
          if ((commit = eval_cc (inst->q_field) ))
          {
            *(inst->dst2) = *(inst->src1);
            next_D = inst->dslot;
            inst->taken_branch = true;
            if (inst->link) {
              *(inst->dst1) = state.pc + inst->link_offset;
              // Profiling counters
              //
              if (sim_opts.is_call_freq_recording_enabled) {
                cnt_ctx.call_freq_hist.inc(*(inst->dst2));
              }
              if (sim_opts.is_call_graph_recording_enabled) {
                cnt_ctx.call_graph_multihist.inc(state.pc,*(inst->dst2));
              }
            }
            // Profiling counters
            //
            if (sim_opts.is_dkilled_recording_enabled && !inst->dslot) {
              cnt_ctx.dkilled_freq_hist.inc(state.pc + inst->size);
            }
          }
        END_ILLEGAL_IN_DSLOT
        break;
      }

    case OpCode::JCC_SRC2:
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block = !inst->dslot;
          prev_had_dslot = inst->dslot;
          if ((commit = eval_cc (inst->q_field) ))
          {
            *(inst->dst2) = *(inst->src2);
            next_D = inst->dslot;
            inst->taken_branch = true;
            if (inst->link) {
              *(inst->dst1) = state.pc + inst->link_offset;
              // Profiling counters
              //
              if (sim_opts.is_call_freq_recording_enabled) {
                cnt_ctx.call_freq_hist.inc(*(inst->dst2));
              }
              if (sim_opts.is_call_graph_recording_enabled) {
                cnt_ctx.call_graph_multihist.inc(state.pc,*(inst->dst2));
              }
            }
            // Profiling counters
            //
            if (sim_opts.is_dkilled_recording_enabled
                && !inst->dslot) {
              cnt_ctx.dkilled_freq_hist.inc(state.pc + inst->size);
            }
          }
        END_ILLEGAL_IN_DSLOT
        break;
      }

      // Return from IRQ1
      //
    case OpCode::J_F_ILINK1:
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block = true;
          phys_profile_.reset_active_trace_sequence(interrupt_stack.top());
          if ((commit = eval_cc (inst->q_field) ))
          {
            inst->taken_branch = true;
            exit_interrupt (1);
            next_D = state.D;
          }
        END_ILLEGAL_IN_DSLOT
        break;
      }

      // Return from IRQ2
      //
    case OpCode::J_F_ILINK2:
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block = true;        
          phys_profile_.reset_active_trace_sequence(interrupt_stack.top());
          if ((commit = eval_cc (inst->q_field) ))
          {
            inst->taken_branch = true;
            exit_interrupt (2);
            next_D = state.D;
          }
        END_ILLEGAL_IN_DSLOT
        break;
      }

    case OpCode::LPCC:
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block = true;

          // Register mapping from lp_end to lp_start
          //
          lp_end_to_lp_start_map[inst->jmp_target] = state.pc + inst->size;

          if ((commit = eval_cc (inst->q_field)))
          {
            write_aux_register (AUX_LP_START, state.pc + inst->size, true);
            write_aux_register (AUX_LP_END,   inst->jmp_target,      true);
            state.L = 0;
            IF_STEP_TRACE_INSTR(this, trace_loop_inst (0, 0));
          } else {
            inst->taken_branch = true;
            state.next_pc = inst->jmp_target; /* Jump around loop */
            IF_STEP_TRACE_INSTR(this, trace_loop_inst (1, inst->jmp_target));
          }
        END_ILLEGAL_IN_DSLOT
        break;
      }

    // Generic load word operation which does further run-time decoding
    // of the addr_shift and inst->dst1 fields to determine whether a
    // pre-adder address is used and if the effective address should be
    // written back to the first source register. This is here for reference
    // only, and has been superceded by the OpCode::LW_* codes which are specialized
    // for each possible path through the OpCode::LD_WORD semantics.
    //
    case OpCode::LD_WORD:
      {
        uint32 ea = *(inst->src1) + (*(inst->src2) << inst->addr_shift);
        uint32 ma = inst->pre_addr ? *(inst->src1) : ea;
        uint32 rd = 0;
        
        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read32(ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW, ma, rd));
          if (inst->dst1) { /* optional address update */
            *(inst->dst1) = ea;
          }
        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX, ma, rd));
        }
        break;
      }

    // Load word, no address shift, no address update
    //
    case OpCode::LW:
      {
        uint32 ma = *(inst->src1) + *(inst->src2);
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read32(ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW, ma, rd));
        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX, ma, rd));
        }
        break;
      }

    // Load word, no address shift, no address update, use pre-adder EA
    // (probably not needed, but provided for completeness)
    //
    case OpCode::LW_PRE:
      {
        uint32 ma = *(inst->src1);
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read32 (ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW, ma, rd));
        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX, ma, rd));
        }
        break;
      }

    // Load word, 2-bit address shift, no address update
    //
    case OpCode::LW_SH2:
      {
        uint32 ma = *(inst->src1) + (*(inst->src2) << 2);
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read32 (ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW, ma, rd));
        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX, ma, rd));
        }
        break;
      }

    // Load word, 2-bit address shift, no address update, pre-adder address
    // (probably not needed, but provided for completeness)
    //
    case OpCode::LW_PRE_SH2:
      {
        uint32 ma = *(inst->src1);
        //uint32 ea = ma + (*(inst->src2) << 2);
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read32 (ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW, ma, rd));
        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX, ma, rd));
        }
        break;
      }

    // Load word, no address shift, address register update
    //
    case OpCode::LW_A:
      {
        uint32 ma = *(inst->src1) + *(inst->src2);
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) &&  read32(ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW, ma, rd));
          if (inst->dst1) { /* optional address update */
            *(inst->dst1) = ma;
          }          
        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX, ma, rd));
        }
        break;
      }

    // Load word, no address shift, address register update,
    // use pre-adder value as address
    //
    case OpCode::LW_PRE_A:
      {
        uint32 ma = *(inst->src1);
        uint32 ea = ma + *(inst->src2);
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read32 (ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW, ma, rd));
          if (inst->dst1) { /* optional address update */
            *(inst->dst1) = ea;
          }

        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX, ma, rd));
        }
        break;
      }

    // Load word, 2-bit address shift, address register update
    // (probably not needed, but provided for completeness)
    //
    case OpCode::LW_SH2_A:
      {
        uint32 ma = *(inst->src1) + (*(inst->src2) << 2);
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read32 (ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW, ma, rd));
          if (inst->dst1) { /* optional address update */
            *(inst->dst1) = ma;
          }
        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX, ma, rd));
        }
        break;
      }

    // Load word, 2-bit address shift, address register update,
    // use pre-adder value as address. Probably not needed, but
    // provided for completeness.
    //
    case OpCode::LW_PRE_SH2_A:
      {
        uint32 ma = *(inst->src1);
        uint32 ea = ma + (*(inst->src2) << 2);
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read32 (ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW, ma, rd));
          if (inst->dst1) { /* optional address update */
            *(inst->dst1) = ea;
          }          

        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX, ma, rd));
        }
        break;
      }

    case OpCode::LD_HALF_S:
      {
        uint32 ea = *(inst->src1) + (*(inst->src2) << inst->addr_shift);
        uint32 ma = inst->pre_addr ? *(inst->src1) : ea;
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read16_extend (ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LH, ma, rd));
          if (inst->dst1) { /* optional address update */
            *(inst->dst1) = ea;
          }
        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LHX, ma, rd));
        }
        break;
      }

    case OpCode::LD_BYTE_S:
      {
        uint32 ea = *(inst->src1) + (*(inst->src2) << inst->addr_shift);
        uint32 ma = inst->pre_addr ? *(inst->src1) : ea;
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read8_extend (ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LB, ma, rd));
          if (inst->dst1) { /* optional address update */
            *(inst->dst1) = ea;
          }
        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LBX, ma, rd));
        }
        break;
      }

    case OpCode::LD_HALF_U:
      {
        uint32 ea = *(inst->src1) + (*(inst->src2) << inst->addr_shift);
        uint32 ma = inst->pre_addr ? *(inst->src1) : ea;
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read16 (ma, rd)) {
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LH, ma, rd));
          if (inst->dst1) { /* optional address update */
            *(inst->dst1) = ea;
          }
        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LHX, ma, rd));
        }
        break;
      }

    case OpCode::LD_BYTE_U:
      {
        uint32 ea = *(inst->src1) + (*(inst->src2) << inst->addr_shift);
        uint32 ma = inst->pre_addr ? *(inst->src1) : ea;
        uint32 rd = 0;

        MEMORY_ACCESS(ma);

        if (is_stack_check_success_r(ma) && read8 (ma, rd)){
          *(inst->dst2) = rd;
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LB, ma, rd));
          if (inst->dst1) { /* optional address update */
            *(inst->dst1) = ea;
          }          
        } else {
          if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LBX, ma, rd));
        }
        break;
      }

    // 32-bit encoded Load Indexed (LDI) requires a separate semantics
    // as it can be a conditional instruction. Otherwise, it behaves
    // just like a OpCode::LD_WORD
    //
    case OpCode::LDI:
      {
        IF_CC(inst,state)
        {
          uint32 ea = *(inst->src1) + (*(inst->src2) << inst->addr_shift);
          uint32 ma = inst->pre_addr ? *(inst->src1) : ea;
          uint32 rd = 0;

          LOG(LOG_DEBUG4) << "OpCode::LDI: src1 = " << HEX(*(inst->src1))
                          << ", src2 = "            << HEX(*(inst->src2));
          
          MEMORY_ACCESS(ma);

          if (is_stack_check_success_r(ma) && read32 (ma, rd)) {
            *(inst->dst2) = rd;
            IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW, ma, rd));            
            if (inst->dst1) { /* optional address update */
              *(inst->dst1) = ea;
            }            
          } else {
            if (!is_stack_check_success_r(ma)) { // raise exception on stack check failure
              ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
              enter_exception (ecause, state.pc, state.pc);
            }
            IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX, ma, rd));
          }
        }
        break;
      }

    case OpCode::ST_WORD:
      {
        uint32 ea = *(inst->src1) + ((inst->shimm) << inst->addr_shift);
        uint32 ma = inst->pre_addr ? *(inst->src1) : ea;

        MEMORY_ACCESS(ma);
        
#ifdef STEP
        if (check && is_stack_check_success_w(ma)) {
          // Don't write to memory, just check memory against store data
          // (this mode is only used when checking translated code against
          // this interpreted code.
          //
          uint32 rd = 0;
          if (sim_opts.trace_on) {
            char  buf[BUFSIZ];
            int   len = snprintf(buf, sizeof(buf),
                                 ": check sw [%08x] <= %08x", ma, *(inst->src2));
            IS.write(buf, len);
          }

          // Read back data written by translated basic block
          read32 (ma, rd);

          // Check it against the current write data - they should be identical
          if (rd != *(inst->src2))
          {
            if (sim_opts.trace_on) {
              char  buf[BUFSIZ];
              int   len = snprintf(buf, sizeof(buf),
                                   " ERROR: read %08x, wdata = %08x",
                                   rd, *(inst->src2));
              IS.write(buf, len);
            } else {
              LOG(LOG_ERROR)
                << "*** Error @ " << std::hex << std::setw(8) << std::setfill('0')
                << state.pc
                << ", sw [" << std::hex << std::setw(8) << std::setfill('0')
                << ma 
                << "] wrote " << std::hex << std::setw(8) << std::setfill('0')
                << rd
                << ", should be " << std::hex << std::setw(8) << std::setfill('0')
                << *(inst->src2);
            }
          }
          if (inst->dst1) { /* optional address update */
            *(inst->dst1) = ea; 
          }
        }
        else
#endif
        {
          // Do the store operation as normal
          //
          if (is_stack_check_success_w(ma) && write32 (ma, *(inst->src2))) {
#ifdef STEP
            if (sim_opts.cosim) {
              delta->wmask       |= UPKT_WMASK_ST;
              delta->store.pc     = state.pc;
              delta->store.addr   = ma;
              delta->store.mask   = WORD_MASK(ma);
              delta->store.data[0]= *(inst->src2) & delta->store.mask;
            }
            IF_STEP_TRACE_INSTR(this, trace_store(T_FORMAT_SW, ma, *(inst->src2)));
#endif
            if (inst->dst1) { /* optional address update */
              *(inst->dst1) = ea;
            }
          } else {
            if (!is_stack_check_success_w(ma)) { // raise exception on stack check failure
              ecause = ECR(sys_arch.isa_opts.EV_ProtV, StoreTLBfault, sys_arch.isa_opts.PV_StackCheck);
              enter_exception (ecause, state.pc, state.pc);
            }   
            IF_STEP_TRACE_INSTR(this, trace_store(T_FORMAT_SWX, ma, *(inst->src2)));
          }
        }
        break;
      }

    case OpCode::ST_HALF:
      {
        uint32 ea = *(inst->src1) + ((inst->shimm) << inst->addr_shift);
        uint32 ma = inst->pre_addr ? *(inst->src1) : ea;

        MEMORY_ACCESS(ma);

#ifdef STEP
        if (check && is_stack_check_success_w(ma)) {
          // Don't write to memory, just check memory against store data
          //
          uint32 rd = 0;
          read16 (ma, rd);
          
          if (rd != *(inst->src2))
          {
            if (sim_opts.trace_on) {
              char  buf[BUFSIZ];
              int   len = snprintf(buf, sizeof(buf),
                                   " ERROR: mem=%04x, ref=%04x",
                                   rd, *(inst->src2));
              IS.write(buf, len);
            } else {
              LOG(LOG_ERROR)
                << "*** Error @ " << std::hex << std::setw(8) << std::setfill('0')
                << state.pc
                << ", sh [" << std::hex << std::setw(8) << std::setfill('0')
                << ma 
                << "] wrote " << std::hex << std::setw(4) << std::setfill('0')
                << (unsigned short)rd
                << ", should be " << std::hex << std::setw(4) << std::setfill('0')
                << (unsigned short)*(inst->src2);
            }
          }
          if (inst->dst1) {
            *(inst->dst1) = ea; /* optional address update */
          }
        }
        else
#endif
        {
          if (is_stack_check_success_w(ma) && write16(ma, *(inst->src2))) {
#ifdef STEP
            if (sim_opts.cosim) {
              delta->wmask       |= UPKT_WMASK_ST;
              delta->store.pc     = state.pc;
              delta->store.addr   = ma;
              delta->store.mask   = HALF_MASK(ma);
              delta->store.data[0]= (*(inst->src2) << (((ma >> 1) & 0x1) * 16))
                                    & delta->store.mask;
            }
            IF_STEP_TRACE_INSTR(this, trace_store(T_FORMAT_SH, ma, *(inst->src2)));
#endif
            if (inst->dst1) {
              *(inst->dst1) = ea; /* optional address update */
            }
          } else {
            if (!is_stack_check_success_w(ma)) { // raise exception on stack check failure
              ecause = ECR(sys_arch.isa_opts.EV_ProtV, StoreTLBfault, sys_arch.isa_opts.PV_StackCheck);
              enter_exception (ecause, state.pc, state.pc);
            }
            IF_STEP_TRACE_INSTR(this, trace_store(T_FORMAT_SHX, ma, *(inst->src2)));
          }
        }
        break;
      }

    case OpCode::ST_BYTE:
      {
        uint32 ea = *(inst->src1) + ((inst->shimm) << inst->addr_shift);
        uint32 ma = inst->pre_addr ? *(inst->src1) : ea;

        MEMORY_ACCESS(ma);

#ifdef STEP
        if (check && is_stack_check_success_w(ma)) {
          // Don't write to memory, just check memory against store data
          //
          uint32 rd = 0;
          read8 (ma, rd);
          
          if (rd != *(inst->src2))
          {
            if (sim_opts.trace_on) {
              char  buf[BUFSIZ];
              int   len = snprintf(buf, sizeof(buf),
                                   " ERROR: mem=%02x, ref=%02x",
                                   rd, *(inst->src2));
              IS.write(buf, len);
            } else {
              LOG(LOG_ERROR)
                << "*** Error @ " << std::hex << std::setw(8) << std::setfill('0')
                << state.pc
                << ", sb [" << std::hex << std::setw(8) << std::setfill('0')
                << ma 
                << "] wrote " << std::hex << std::setw(2) << std::setfill('0')
                << (unsigned char)rd
                << ", should be " << std::hex << std::setw(2) << std::setfill('0')
                << (unsigned char)*(inst->src2);
            }
          }
          if (inst->dst1) { /* optional address update */
            *(inst->dst1) = ea; 
          }
        }
        else
#endif
        {
          if (is_stack_check_success_w(ma) && write8(ma, *(inst->src2))) {
#ifdef STEP
            if (sim_opts.cosim) {
              delta->wmask       |= UPKT_WMASK_ST;
              delta->store.pc     = state.pc;
              delta->store.addr   = ma;
              delta->store.mask   = BYTE_MASK(ma);
              delta->store.data[0]= (*(inst->src2) << ((ma & 0x3)*8))
                                  & delta->store.mask;
            }
            IF_STEP_TRACE_INSTR(this, trace_store(T_FORMAT_SB, ma, *(inst->src2)));
#endif
            if (inst->dst1) { /* optional address update */
              *(inst->dst1) = ea; 
            }            
          } else {
            if (!is_stack_check_success_w(ma)) { // raise exception on stack check failure
              ecause = ECR(sys_arch.isa_opts.EV_ProtV, StoreTLBfault, sys_arch.isa_opts.PV_StackCheck);
              enter_exception (ecause, state.pc, state.pc);
            }
            IF_STEP_TRACE_INSTR(this, trace_store(T_FORMAT_SBX, ma, *(inst->src2)));
          }
        }
        break;
      }

    case OpCode::LR:
      {
        uint32 c = *(inst->src2);
        uint32 b;

        if (read_aux_register (c, &b, true))
        {
          *(inst->dst1) = b;
          IF_STEP_TRACE_INSTR(this, trace_lr(c, b, 1));
        } else {
          IF_STEP_TRACE_INSTR(this, trace_lr(c, b, 0));
        }
        break;
      }

    case OpCode::SR:
      {

        uint32 b = *(inst->src1);
        uint32 c = *(inst->src2);

        // If the write could be interrupt enabling, then set end_of_block to true.
        //
        end_of_block = true;
        
        if (write_aux_register (c, b, true))
        {
#ifdef STEP
          if (sim_opts.cosim) {
            delta->wmask   |= UPKT_WMASK_AUX;
            delta->aux.a    = c;
            delta->aux.w    = b;
          }
#endif
          IF_STEP_TRACE_INSTR(this, trace_sr(c, b, 1));
        } else {
          IF_STEP_TRACE_INSTR(this, trace_sr(c, b, 0));
        }
        break;
      }

    case OpCode::AEX:
      {
        
        uint32 b = *(inst->src1);
        uint32 c = *(inst->src2);
        uint32 tmp;
        if(read_aux_register(c, &tmp, true))
        {
          IF_STEP_TRACE_INSTR(this, trace_sr(c, tmp, 1));
          // If the write could be interrupt enabling, then
          // set end_of_block to true.
          //
          end_of_block = true;
          IF_CC(inst, state)
          {
            if(write_aux_register(c, b, true)) {
              *(inst->src1) = tmp;
              IF_STEP_TRACE_INSTR(this, trace_sr(c, b, 1));
            } else {
              IF_STEP_TRACE_INSTR(this, trace_sr(c, b, 0));
            }
          }
        }
        break;
    }
    
    case OpCode::SETI:
    {
      LOG(LOG_DEBUG3) << "SETI " << HEX(*(inst->src2));

      if(*(inst->src2) & 0x20) 
      {
        state.E = *(inst->src2) & 0xf;
        state.IE = (*(inst->src2) & 0x10) != 0;
        LOG(LOG_DEBUG3) << "SETI Set IE to " << (uint32)state.IE;
      }
      else
      {
        state.IE = 1;
        LOG(LOG_DEBUG3) << "SETI Constant Set IE to 1";
        if(*(inst->src2) & 0x10) state.E = *(inst->src2) & 0xf;
      }
      break;
    }
    
    case OpCode::CLRI:
    {
      if(inst->info.rf_renb0)
      {
        *(inst->src1) = state.E | (state.IE << 4) | (1 << 5);
      }
      state.IE = 0;
      break;
    }

    case OpCode::TST:
      {
        IF_CC(inst,state)
        {
          uint32 a = *(inst->src1) & *(inst->src2);
          state.Z = (a == 0);
          state.N = ((sint32)a < 0);
        }
        break;
      }

    case OpCode::BTST:
      {
        IF_CC(inst,state)
        {
          uint32 a = *(inst->src1) & (1 << (*(inst->src2) & 0x1f));
          state.Z = (a == 0);
          state.N = ((sint32)a < 0);
        }
        break;
      }

    case OpCode::CMP:
      {
        IF_CC(inst,state)
        {
          __asm__ __volatile__ ("cmpl %5, %4;\n\t"      /* compare */
                                "seto %0;\n\t"          /* set V */
                                "setc %1;\n\t"          /* set C */
                                "sets %2;\n\t"          /* set N */
                                "setz %3;\n\t"          /* set Z */
                                  : "=m"(state.V),      /* output - %0 */
                                    "=m"(state.C),      /* output - %1 */
                                    "=m"(state.N),      /* output - %2 */
                                    "=m"(state.Z)       /* output - %3 */
                                  : "r"(*(inst->src1)), /* input  - %4 */
                                    "r"(*(inst->src2))  /* input  - %5 */
                                  : "cc"                /* clobbered register */
                                );              
        }
        break;
      }

    case OpCode::RCMP:
      {
        IF_CC(inst,state)
        {
          __asm__ __volatile__ ("cmpl %5, %4;\n\t"      /* compare */
                                "seto %0;\n\t"          /* set V */
                                "setc %1;\n\t"          /* set C */
                                "sets %2;\n\t"          /* set N */
                                "setz %3;\n\t"          /* set Z */
                                  : "=m"(state.V),      /* output - %0 */
                                    "=m"(state.C),      /* output - %1 */
                                    "=m"(state.N),      /* output - %2 */
                                    "=m"(state.Z)       /* output - %3 */
                                  : "r"(*(inst->src2)), /* input  - %4 */
                                    "r"(*(inst->src1))  /* input  - %5 */
                                  : "cc"                /* clobbered register */
                                );
        }
        break;
      }

    /* Versions of frequent functions (mov, add, sub, and, or)
     * which execute unconditionally and do not set flags.
     * These functions dominate execution time, so are coded ultra-efficiently.
     */
    case OpCode::MOV: *(inst->dst1) = *(inst->src2);               break;
    case OpCode::ADD: *(inst->dst1) = *(inst->src1) + *(inst->src2); break;
    case OpCode::SUB: *(inst->dst1) = *(inst->src1) - *(inst->src2); break;
    case OpCode::AND: *(inst->dst1) = *(inst->src1) & *(inst->src2); break;
    case OpCode::OR:  *(inst->dst1) = *(inst->src1) | *(inst->src2); break;

    /* Versions of mov, add, sub, and, or that either set flags,
     * or execute conditionally, or both.
     */
    case OpCode::MOV_F:
      {
        IF_CC(inst,state)
        {
          uint32 t1 = *(inst->src2);
          *(inst->dst1) = t1;
          if (inst->flag_enable)
          {
            state.Z = (t1 == 0);
            state.N = ((sint32)t1 < 0);
          }
        }
        break;
      }

    case OpCode::ADD_F:
      {
        IF_CC(inst,state)
        {
          uint32 t2 = *(inst->src2);
          *(inst->dst1) = *(inst->src1);
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("addl %5, %0;\n\t"      /* add */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "r"(t2),            /* input  - %5 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            __asm__ __volatile__ ("addl %1, %0"           /* add */
                                    : "=r"(*(inst->dst1)) /* output - %0 */
                                    : "r"(t2),            /* input  - %1 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"
                                  );
          }
        }
        break;
      }

    case OpCode::SUB_F:
      {
        IF_CC(inst,state)
        {
          uint32 t2 = *(inst->src2);
          *(inst->dst1) = *(inst->src1);
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("subl %5, %0;\n\t"      /* sub */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "r"(t2),            /* input  - %5 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            __asm__ __volatile__ ("subl %1, %0"           /* sub */
                                    : "=r"(*(inst->dst1)) /* output - %0 */
                                    : "r"(t2),            /* input  - %1 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          }
        }
        break;
      }

    case OpCode::AND_F:
      {
        IF_CC(inst,state)
        {
          uint32 t2 = *(inst->src2);
          *(inst->dst1) = *(inst->src1);
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("andl %3, %0;\n\t"      /* andl */
                                  "sets %1;\n\t"          /* set N */
                                  "setz %2;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.N),      /* output - %1 */
                                      "=m"(state.Z)       /* output - %2 */
                                    : "r"(t2),            /* input  - %3 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            __asm__ __volatile__ ("andl %1, %0"           /* andl */
                                    : "=r"(*(inst->dst1)) /* output - %0 */
                                    : "r"(t2),            /* input  - %1 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          }
        }
        break;
      }

    case OpCode::BCLR:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t2 = ~(1 << (*(inst->src2) & 0x1f) );
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("andl %3, %0;\n\t"      /* andl */
                                  "sets %1;\n\t"          /* set N */
                                  "setz %2;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.N),      /* output - %1 */
                                      "=m"(state.Z)       /* output - %2 */
                                    : "r"(t2),            /* input  - %3 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *inst->src1 & ~(1 << (*inst->src2 & 0x1f));
          }
        }
        break;
      }

    case OpCode::BSET:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t2 = (1 << (*(inst->src2) & 0x1f));
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("orl %3, %0;\n\t"       /* orl */
                                  "sets %1;\n\t"          /* set N */
                                  "setz %2;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.N),      /* output - %1 */
                                      "=m"(state.Z)       /* output - %2 */
                                    : "r"(t2),            /* input  - %3 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *inst->src1 | (1 << (*inst->src2 & 0x1f));
          }
        }
        break;
      }

    case OpCode::OR_F:
      {
        IF_CC(inst,state)
        {
          uint32 t2 = *(inst->src2);
          *(inst->dst1) = *(inst->src1);
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("orl %3, %0;\n\t"       /* orl */
                                  "sets %1;\n\t"          /* set N */
                                  "setz %2;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.N),      /* output - %1 */
                                      "=m"(state.Z)       /* output - %2 */
                                    : "r"(t2),            /* input  - %3 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            __asm__ __volatile__ ("orl %1, %0"            /* orl */
                                    : "=r"(*(inst->dst1)) /* output - %0 */
                                    : "r"(t2),            /* input  - %1 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc");              /* clobbered register */
          }
        }
        break;
      }

    case OpCode::RSUB:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t1 = *(inst->src1);
            *(inst->dst1) = *(inst->src2);
            __asm__ __volatile__ ("subl %5, %0;\n\t"      /* subl */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "r"(t1),            /* input  - %5 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *(inst->src2) - *(inst->src1);
          }
        }
        break;
      }

    case OpCode::ADC:
      {
        IF_CC(inst,state)
        {
          uint32 t2 = *(inst->src2);
          *(inst->dst1) = *(inst->src1);
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("bt   $0, %2;\n\t"      /* set carry bit */
                                  "adcl %5, %0;\n\t"      /* adcl */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "r"(t2),            /* input  - %5 */
                                      "0"(*(inst->dst1)), /* input/output  - %0 */
                                      "m"(state.C)        /* input/output  - %2 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            __asm__ __volatile__ ("bt $0, %2;\n\t"          /* set carry bit */
                                  "adcl %1, %0"             /* adcl */
                                    : "=r"(*(inst->dst1))   /* output - %0 */
                                    : "r"(t2),              /* input  - %1 */
                                      "m"(state.C),         /* input  - %2 */
                                      "0"(*(inst->dst1))    /* input/output  - %0 */
                                    : "cc"
                                  );
          }
        }
        break;
      }

    case OpCode::SBC:
      {
        IF_CC(inst,state)
        {
          uint32 t2 = *(inst->src2);
          *(inst->dst1) = *(inst->src1);
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("bt   $0, %2;\n\t"      /* set carry bit */
                                  "sbbl %5, %0;\n\t"      /* sbbl */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "r"(t2),            /* input  - %5 */
                                      "0"(*(inst->dst1)), /* input/output  - %0 */
                                      "m"(state.C)        /* input/output  - %2 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            __asm__ __volatile__ ("bt   $0, %2;\n\t"      /* set carry bit */
                                  "sbbl %1, %0"           /* sbbl */
                                    : "=r"(*(inst->dst1)) /* output - %0 */
                                    : "r"(t2),            /* input  - %1 */
                                      "m"(state.C),       /* input  - %2 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"
                                  );
          }
        }
        break;
      }

    case OpCode::XOR:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t2 = *(inst->src2);
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("xorl %3, %0;\n\t"      /* xorl */
                                  "sets %1;\n\t"          /* set N */
                                  "setz %2;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.N),      /* output - %1 */
                                      "=m"(state.Z)       /* output - %2 */
                                    : "r"(t2),            /* input  - %3 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *inst->src1 ^ *inst->src2;
          }
        }
        break;
      }

    case OpCode::BIC:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t2 = ~*(inst->src2);
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("andl %3, %0;\n\t"      /* andl */
                                  "sets %1;\n\t"          /* set N */
                                  "setz %2;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.N),      /* output - %1 */
                                      "=m"(state.Z)       /* output - %2 */
                                    : "r"(t2),            /* input  - %3 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *inst->src1 & ~(*inst->src2);
          }
        }
        break;
      }

    case OpCode::MAX: // Bug fixed, Nov-07 (fixed setting of C bit)
      {
        IF_CC(inst,state)
        {
          sint32 t1;
          if (inst->flag_enable)
          {
            t1 = *(inst->src1);
            __asm__ __volatile__ ("subl %4, %0;\n\t"      /* subl */
                                  "seto %1;\n\t"          /* set V */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=r"(t1),           /* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "r"(*(inst->src2)), /* input  - %4 */
                                      "0"(t1)             /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
            if (((sint32)*inst->src2) >= ((sint32)*inst->src1))
            {
              *(inst->dst1) = *(inst->src2);
              state.C = true;
            } else {
              *(inst->dst1) = *(inst->src1);
              state.C = false;
            }
          }
          else
          *(inst->dst1) = (((sint32)*(inst->src2)) > ((sint32)*(inst->src1)))
                                ? *(inst->src2)
                                : *(inst->src1);
        }
        break;
      }

    case OpCode::MIN: // Bug fixed, Nov-07 (fixed setting of C bit)
      {
        IF_CC(inst,state)
        {
          sint32 t1;
          if (inst->flag_enable)
          {
            t1 = *(inst->src1);
            __asm__ __volatile__ ("subl %4, %0;\n\t"      /* subl */
                                  "seto %1;\n\t"          /* set V */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=r"(t1),           /* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "r"(*(inst->src2)), /* input  - %4 */
                                      "0"(t1)             /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
            if (((sint32)*inst->src2) > ((sint32)*inst->src1))
            {
              *(inst->dst1) = *(inst->src1);
              state.C = false;
            } else {
              *(inst->dst1) = *(inst->src2);
              state.C = true;
            }
          } else {
            *(inst->dst1) = (((sint32)*(inst->src2)) > ((sint32)*(inst->src1)))?*(inst->src1):*(inst->src2);
          }
        }
        break;
      }

    case OpCode::BXOR: // Bug fixed, Nov-07 (ANDed 0x1f mask to src2)
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t2 = 1 << (*(inst->src2) & 0x1f);
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("xorl %3, %0;\n\t"      /* xorl */
                                  "sets %1;\n\t"          /* set N */
                                  "setz %2;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.N),      /* output - %1 */
                                      "=m"(state.Z)       /* output - %2 */
                                    : "r"(t2),            /* input  - %3 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *inst->src1 ^ (1 << (*inst->src2 & 0x1f));
          }
        }
        break;
      }

    case OpCode::BMSK: // Bug fixed, Nov-07 (modified mask generation)
      {
        IF_CC(inst,state)
        {
          uint32 mask = (uint32)(*(inst->src2)) & 0x1f;
          mask = ((uint32)0xffffffff) >> (31 - (mask & 0x1f));

          if (inst->flag_enable)
          {
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("andl %3, %0;\n\t"      /* andl */
                                  "sets %1;\n\t"          /* set N */
                                  "setz %2;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.N),      /* output - %1 */
                                      "=m"(state.Z)       /* output - %2 */
                                    : "r"(mask),            /* input  - %3 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );            
          } else {
            *(inst->dst1) = *inst->src1 & mask;
          }
        }
        break;
      }

     case OpCode::BMSKN: // ARCv2 only
      {
        IF_CC(inst,state)
        {
          uint32 mask = (uint32)*(inst->src2) & 0x1f;
          mask = ~(((uint32)0xffffffff) >> (31 - (mask & 0x1f)));

          if (inst->flag_enable)
          {
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("andl %3, %0;\n\t"      /* andl */
                                  "sets %1;\n\t"          /* set N */
                                  "setz %2;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.N),      /* output - %1 */
                                      "=m"(state.Z)       /* output - %2 */
                                    : "r"(mask),            /* input  - %3 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );            
          } else {
            *(inst->dst1) = *inst->src1 & mask;
          }
        }
        break;
      }

   case OpCode::ADD1:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t2 = *(inst->src2) << 1;
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("addl %5, %0;\n\t"      /* addl */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "r"(t2),            /* input  - %5 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *inst->src1 + (*inst->src2 << 1);
          }
        }
        break;
      }

    case OpCode::ADD2:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t2 = *(inst->src2) << 2;
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("addl %5, %0;\n\t"      /* addl */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "r"(t2),            /* input  - %5 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *inst->src1 + (*inst->src2 << 2);
          }
        }
        break;
      }

    case OpCode::ADD3:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t2 = *(inst->src2) << 3;
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("addl %5, %0;\n\t"      /* addl */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "r"(t2),            /* input  - %5 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *inst->src1 + (*inst->src2 << 3);
          }
        }
        break;
      }

    case OpCode::SUB1:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t2 = (*(inst->src2) << 1);
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("subl %5, %0;\n\t"      /* subl */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "r"(t2),            /* input  - %5 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *inst->src1 - (*inst->src2 << 1);
          }
        }
        break;
      }

    case OpCode::SUB2:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t2 = (*(inst->src2) << 2);
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("subl %5, %0;\n\t"      /* subl */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "r"(t2),            /* input  - %5 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *inst->src1 - (*inst->src2 << 2);
          }
        }
        break;
      }

    case OpCode::SUB3:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            uint32 t2 = (*(inst->src2) << 3);
            *(inst->dst1) = *(inst->src1);
            __asm__ __volatile__ ("subl %5, %0;\n\t"      /* subl */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "r"(t2),            /* input  - %5 */
                                      "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *inst->src1 - (*inst->src2 << 3);
          }
        }
        break;
      }

    case OpCode::MPY: // Bug fixed, Nov 07 (adjusted state.V update logic)
      {
        IF_CC(inst,state)
        {
          sint64 product = (sint64)((sint32)*(inst->src1));
          product = product * (sint64)((sint32)*(inst->src2));
          *(inst->dst1) = (uint32)product;
          if (inst->flag_enable)
          {
            uint32 bits_31_to_62 = (uint32)(product >> 31);
            uint32 bit_63 = ((uint32)(product >> 63)) & 0x1;

            state.V = ( (bits_31_to_62 != 0 && bit_63 == 0)  ||  ( bits_31_to_62 != 0xFFFFFFFF && bit_63 == 1 ) ); // TBD: this might be improved
            state.N = (product < 0);
            state.Z = (*(inst->dst1) == 0);
          }
        }
        break;
      }

    case OpCode::MPYH: // Bug fixed, Nov 07 (corrected casting of state.N expression)
      {
        IF_CC(inst,state)
        {
          sint64 product = (sint64)((sint32)*(inst->src1));
          product = product * (sint64)((sint32)*(inst->src2));
          *(inst->dst1) = (sint32)(product >> 32);
          if (inst->flag_enable)
          {
            state.V = 0;
            state.N = (((sint32)*(inst->dst1)) < 0);
            state.Z = ((*(inst->dst1)) == 0);
          }
        }
        break;
      }

    case OpCode::MPYHU: // Bug fixed, Nov 07 (used dst1 in state.Z calculation)
      {
        IF_CC(inst,state)
        {
          uint64 product = (uint64)(*(inst->src1));
          product = product * (uint64)(*(inst->src2));
          *(inst->dst1) = (uint32)(product >> 32);
          if (inst->flag_enable)
          {
            state.V = 0;
            state.N = 0;
            state.Z = (*(inst->dst1) == 0);
          }
        }
        break;
      }

    case OpCode::MPYU: // Bug fixed, Nov 07 (used dst1 in state.Z calculation)
      {
        IF_CC(inst,state)
        {
          uint64 product = (uint64)(*(inst->src1));
          product = product * *(inst->src2);
          *(inst->dst1) = (uint32)product;
          if (inst->flag_enable)
          {
            state.V = (*(inst->dst1) != product);
            state.N = 0;
            state.Z = (*(inst->dst1) == 0);
          }
        }
        break;
      }
    
    // -------------------------------------------------------------------------
    // OpCode::MUL64: only available on ARC 600 processor
    //
    case OpCode::MUL64:
      {
        IF_CC(inst,state)
        { 
          sint64 product = ((sint64)((sint32)*(inst->src1))) * ((sint64)((sint32)*(inst->src2)));
          state.gprs[MLO_REG]   = (uint32)(product);
          state.gprs[MMID_REG]  = (uint32)(product >> 16);
          state.gprs[MHI_REG]   = (uint32)(product >> 32);
          IF_STEP_TRACE_INSTR(this, trace_mul64_inst());
        }
        break;
      }
      
    // -------------------------------------------------------------------------
    // OpCode::MULU64: only available on ARC 600 processor
    //
    case OpCode::MULU64:
      {
        IF_CC(inst,state)
        {
          uint64 product = ((uint64)(*(inst->src1))) * ((uint64)(*(inst->src2)));
          state.gprs[MLO_REG]   = (uint32)(product);
          state.gprs[MMID_REG]  = (uint32)(product >> 16);
          state.gprs[MHI_REG]   = (uint32)(product >> 32);
          IF_STEP_TRACE_INSTR(this, trace_mul64_inst());
        }
        break;
      }

    // -------------------------------------------------------------------------
    // OpCode::MPYW: only available on ARC 600 and ARCv2
    //
    case OpCode::MPYW:
      {
        IF_CC(inst,state)
        {
          sint32 product = ((sint32)(*(inst->src1)) << 16) >> 16;
          product = product * ((((sint32)*(inst->src2)) << 16) >> 16);
          *(inst->dst1) = (sint32)product;
          if (inst->flag_enable)
          {
            state.V = 0;              // 16x16y32 can never lead to overflow
            state.N = (product < 0);
            state.Z = (*(inst->dst1) == 0);
          }
        }
        break;
      }
    // -------------------------------------------------------------------------
    // OpCode::MPYWU: only available on ARC 600 and ARCv2
    //
    case OpCode::MPYWU:
      {
        IF_CC(inst,state)
        {
          uint32 product = (uint32)(*(inst->src1) & 0x0000FFFFU);
          product = product * (uint32)(*(inst->src2) & 0x0000FFFFU);
          *(inst->dst1) = (uint32)product;
          if (inst->flag_enable)
          {
            state.V = 0; // 16x16y32 can never lead to overflow
            state.N = 0; // unsigned operation never produced negative result
            state.Z = (*(inst->dst1) == 0);
          }
        }
        break;
      }

    case OpCode::SETEQ:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("cmpl %5, %4;\n\t"      /* compare */
                                  "seto %0;\n\t"          /* set V */
                                  "setc %1;\n\t"          /* set C */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=m"(state.V),      /* output - %0 */
                                      "=m"(state.C),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "r"(*(inst->src1)), /* input  - %4 */
                                      "r"(*(inst->src2))  /* input  - %5 */
                                    : "cc"                /* clobbered register */
                                  );              
          }
          *(inst->dst1) = (*(inst->src1) == *(inst->src2));
        }
        break;
      }

    case OpCode::SETNE:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("cmpl %5, %4;\n\t"      /* compare */
                                  "seto %0;\n\t"          /* set V */
                                  "setc %1;\n\t"          /* set C */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=m"(state.V),      /* output - %0 */
                                      "=m"(state.C),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "r"(*(inst->src1)), /* input  - %4 */
                                      "r"(*(inst->src2))  /* input  - %5 */
                                    : "cc"                /* clobbered register */
                                  );              
          }
          *(inst->dst1) = (*(inst->src1) != *(inst->src2));
        }
        break;
      }

    case OpCode::SETLT:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("cmpl %5, %4;\n\t"      /* compare */
                                  "seto %0;\n\t"          /* set V */
                                  "setc %1;\n\t"          /* set C */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=m"(state.V),      /* output - %0 */
                                      "=m"(state.C),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "r"(*(inst->src1)), /* input  - %4 */
                                      "r"(*(inst->src2))  /* input  - %5 */
                                    : "cc"                /* clobbered register */
                                  );
          }
          *(inst->dst1) = ((sint32)*(inst->src1) < (sint32)*(inst->src2));
        }
        break;
      }

    case OpCode::SETGE:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("cmpl %5, %4;\n\t"      /* compare */
                                  "seto %0;\n\t"          /* set V */
                                  "setc %1;\n\t"          /* set C */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=m"(state.V),      /* output - %0 */
                                      "=m"(state.C),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "r"(*(inst->src1)), /* input  - %4 */
                                      "r"(*(inst->src2))  /* input  - %5 */
                                    : "cc"                /* clobbered register */
                                  );              
          }
          *(inst->dst1) = ((sint32)*(inst->src1) >= (sint32)*(inst->src2));
        }
        break;
      }

    case OpCode::SETLO:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("cmpl %5, %4;\n\t"      /* compare */
                                  "seto %0;\n\t"          /* set V */
                                  "setc %1;\n\t"          /* set C */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=m"(state.V),      /* output - %0 */
                                      "=m"(state.C),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "r"(*(inst->src1)), /* input  - %4 */
                                      "r"(*(inst->src2))  /* input  - %5 */
                                    : "cc"                /* clobbered register */
                                  );              
          }
          *(inst->dst1) = ((uint32)*(inst->src1) < (uint32)*(inst->src2));
        }
        break;
      }

    case OpCode::SETHS:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("cmpl %5, %4;\n\t"      /* compare */
                                  "seto %0;\n\t"          /* set V */
                                  "setc %1;\n\t"          /* set C */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=m"(state.V),      /* output - %0 */
                                      "=m"(state.C),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "r"(*(inst->src1)), /* input  - %4 */
                                      "r"(*(inst->src2))  /* input  - %5 */
                                    : "cc"                /* clobbered register */
                                  );              
          }
          *(inst->dst1) = ((uint32)*(inst->src1) >= (uint32)*(inst->src2));
        }
        break;
      }

    case OpCode::SETLE:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("cmpl %5, %4;\n\t"      /* compare */
                                  "seto %0;\n\t"          /* set V */
                                  "setc %1;\n\t"          /* set C */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=m"(state.V),      /* output - %0 */
                                      "=m"(state.C),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "r"(*(inst->src1)), /* input  - %4 */
                                      "r"(*(inst->src2))  /* input  - %5 */
                                    : "cc"                /* clobbered register */
                                  );
          }
          *(inst->dst1) = ((sint32)*(inst->src1) <= (sint32)*(inst->src2));
        }
        break;
      }

    case OpCode::SETGT:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("cmpl %5, %4;\n\t"      /* compare */
                                  "seto %0;\n\t"          /* set V */
                                  "setc %1;\n\t"          /* set C */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=m"(state.V),      /* output - %0 */
                                      "=m"(state.C),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "r"(*(inst->src1)), /* input  - %4 */
                                      "r"(*(inst->src2))  /* input  - %5 */
                                    : "cc"                /* clobbered register */
                                  );
          }
          *(inst->dst1) = ((sint32)*(inst->src1) > (sint32)*(inst->src2));
        }
        break;
      }

    case OpCode::ASL: // Bug fixed, Nov 07 (added update of state.V)
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            *(inst->dst1) = *(inst->src2);
            __asm__ __volatile__ ("sal $1, %0;\n\t"       /* sal */
                                  "seto %1;\n\t"          /* set V */
                                  "setc %2;\n\t"          /* set C */
                                  "sets %3;\n\t"          /* set N */
                                  "setz %4;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.V),      /* output - %1 */
                                      "=m"(state.C),      /* output - %2 */
                                      "=m"(state.N),      /* output - %3 */
                                      "=m"(state.Z)       /* output - %4 */
                                    : "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = *(inst->src2) << 1;
          }
        }
        break;
      }

    case OpCode::ASR:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            *(inst->dst1) = *(inst->src2);
            __asm__ __volatile__ ("sar $1, %0;\n\t"       /* sar */
                                  "setc %1;\n\t"          /* set C */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.C),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = ((sint32)*(inst->src2)) >> 1;
          }
        }
        break;
      }

    case OpCode::LSR:
      {
        IF_CC(inst,state)
        {
          if (inst->flag_enable)
          {
            *(inst->dst1) = *(inst->src2);
            __asm__ __volatile__ ("shr $1, %0;\n\t"       /* shr */
                                  "setc %1;\n\t"          /* set C */
                                  "sets %2;\n\t"          /* set N */
                                  "setz %3;\n\t"          /* set Z */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.C),      /* output - %1 */
                                      "=m"(state.N),      /* output - %2 */
                                      "=m"(state.Z)       /* output - %3 */
                                    : "0"(*(inst->dst1))  /* input/output  - %0 */
                                    : "cc"                /* clobbered register */
                                  );
          } else {
            *(inst->dst1) = ((uint32)*(inst->src2)) >> 1;
          }
        }
        break;
      }

    case OpCode::ROR: // Bug fixed, Nov 07 (corrected update to state.Z and state.N)
      {
        IF_CC(inst,state)
        {
          *(inst->dst1) = *(inst->src2);
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("ror $1, %0;\n\t"         /* ror */
                                  "setc %1;\n\t"            /* set C */
                                    : "=r"(*(inst->dst1)),  /* output - %0 */
                                      "=m"(state.C)         /* output - %1 */
                                    : "0" (*(inst->dst1))   /* input/output  - %0 */
                                    : "cc"                  /* clobbered register */
                                  );
            state.Z = ((sint32)*(inst->dst1) == 0);
            state.N = ((sint32)*(inst->dst1) < 0);
          } else {
            __asm__ __volatile__ ("ror $1, %0" : "=r"(*(inst->dst1)) : "0"(*(inst->dst1)) : "cc");
          }
        }
        break;
      }

    case OpCode::ROL: // ARCv2 basecase
      {
        IF_CC(inst,state)
        {
          *(inst->dst1) = *(inst->src2);
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("rol $1, %0;\n\t"         /* rol */
                                  "setc %1;\n\t"            /* set C */
                                    : "=r"(*(inst->dst1)),  /* output - %0 */
                                      "=m"(state.C)         /* output - %1 */
                                    : "0" (*(inst->dst1))   /* input/output  - %0 */
                                    : "cc"                  /* clobbered register */
                                  );

            state.Z = ((sint32)*(inst->dst1) == 0);
            state.N = ((sint32)*(inst->dst1) < 0);
          }
          else
            __asm__ __volatile__ ("rol $1, %0"              /* rol */
                                    : "=r"(*(inst->dst1))   /* output - %0 */
                                    : "0" (*(inst->dst1))   /* input/output - %0 */
                                    : "cc"
                                  );
        }
        break;
      }

    case OpCode::RRC: // Bug fixed, Nov 07 (corrected update to state.Z and state.N)
      {
        IF_CC(inst,state)
        {
          *(inst->dst1) = *(inst->src2);
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("bt  $0, %1;\n\t"         /* set carry bit */
                                  "rcr $1, %0;\n\t"         /* rcr */
                                  "setc %1;\n\t"            /* set C */
                                    : "=r"(*(inst->dst1)),  /* output - %0 */
                                      "=m"(state.C)         /* output - %1 */
                                    : "0" (*(inst->dst1)),  /* input/output - %0 */
                                      "m" (state.C)         /* input/output - %1 */
                                    : "cc"
                                  );
            state.Z = ((sint32)*(inst->dst1) == 0);
            state.N = ((sint32)*(inst->dst1) < 0);
          } else {
            __asm__ __volatile__ ("bt  $0, %1;\n\t"         /* set carry bit */
                                  "rcr $1, %0"              /* rcr */
                                    : "=r"(*(inst->dst1))   /* output - %0 */
                                    : "m" (state.C),        /* input - %1 */
                                      "0" (*(inst->dst1))   /* input/output - %0 */
                                    : "cc");
          }
        }
        break;
      }

    case OpCode::RLC: // Note, different behaviour between ARC700 and ARC600.
      {       // '600 sets overflow if flag_enable, '700 does not set overflow
              // Bug fix, Nov 07 (corrected Z and N bit update logic)
        IF_CC(inst,state)
        {
          *(inst->dst1) = *(inst->src2);
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("bt  $0, %1;\n\t"         /* set carry bit */
                                  "rcl $1, %0;\n\t"         /* rcl */
                                  "setc %1;\n\t"            /* set C */
                                    : "=r"(*(inst->dst1)),  /* output - %0 */
                                      "=m"(state.C)         /* output - %1 */
                                    : "0" (*(inst->dst1)),  /* input/output - %0 */
                                      "m" (state.C)         /* input/output - %1 */
                                    : "cc"
                                  );
// ARC600 only  __asm__ __volatile__ ("seto %0" : "=m"(state.V));
            state.Z = ((sint32)*(inst->dst1) == 0);
            state.N = ((sint32)*(inst->dst1) < 0);
          } else {
            __asm__ __volatile__ ("bt  $0, %1;\n\t"         /* set carry bit */
                                  "rcl $1, %0"              /* rcl */
                                    : "=r"(*(inst->dst1))   /* output - %0 */
                                    : "m" (state.C),        /* input - %1 */
                                      "0" (*(inst->dst1))   /* input/output - %0 */
                                    : "cc");
          }
        }
        break;
      }

    case OpCode::SEXBYTE:
      {
        *(inst->dst1) = (char)(*(inst->src2));
        if (inst->flag_enable)
        {
          state.Z = (*(inst->dst1) == 0);
          state.N = (((sint32)*(inst->dst1)) < 0);
        }
        break;
      }

    case OpCode::SEXWORD:
      {
        *(inst->dst1) = (short)(*(inst->src2));
        if (inst->flag_enable)
        {
          state.Z = (*(inst->dst1) == 0);
          state.N = (((sint32)*(inst->dst1)) < 0);
        }
        break;
      }

    case OpCode::EXTBYTE:
      {
        *(inst->dst1) = (unsigned char)(*(inst->src2));
        if (inst->flag_enable)
        {
          state.Z = (*(inst->dst1) == 0);
          state.N = false;
        }
        break;
      }

    case OpCode::EXTWORD:
      {
        *(inst->dst1) = (unsigned short)(*(inst->src2));
        if (inst->flag_enable)
        {
          state.Z = (*(inst->dst1) == 0);
          state.N = false;
        }
        break;
      }

    case OpCode::ABS: // Bug fix, Nov 07 (update to state.V added)
      {
        sint32 b = *(inst->src2);
        *(inst->dst1) = (b < 0) ? -b : b;
        if (inst->flag_enable)
        {
          state.Z = (b == 0);
          state.V = state.N = ((uint32)b == 0x80000000);
          state.C = ((b & 0x80000000) != 0);
        }
        break;
      }

    case OpCode::NOT:
      {
        sint32 b = ~*(inst->src2);
        *(inst->dst1) = (uint32)b;
        if (inst->flag_enable)
        {
          state.Z = (b ==0);
          state.N = (b < 0);
        }
        break;
      }

      // ----------------------------------------------------------------------
      // EX: atomic exchange operation 
      //
    case OpCode::EX: // Bug fix, Nov 07 (only update dst1 if write succeeds)
    {
      const uint32  ma = *(inst->src2);       // target address of EX memory operand
      const uint32  sd = *(inst->dst1);       // sd is required for tracing
      
      MEMORY_ACCESS(ma);
          
      if ((is_stack_check_success_x(ma) && atomic_exchange(ma, inst->dst1)))
      {
#ifdef STEP      
        IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW,  ma, *(inst->dst1)));
        IF_STEP_TRACE_INSTR(this, trace_store(T_FORMAT_SW, ma, sd));
        if (sim_opts.cosim) {
          delta->wmask       |= UPKT_WMASK_ST;
          delta->store.pc     = state.pc;
          delta->store.addr   = ma;
          delta->store.mask   = WORD_MASK(ma);
          delta->store.data[0]= sd & delta->store.mask;
        }
#endif
      } else {
        if (!is_stack_check_success_x(ma)) { // raise exception on stack check failure
          ecause = ECR(sys_arch.isa_opts.EV_ProtV, (StoreTLBfault|LoadTLBfault), sys_arch.isa_opts.PV_StackCheck);
          enter_exception (ecause, state.pc, state.pc);
        }
        IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX,  ma, 0));
        IF_STEP_TRACE_INSTR(this, trace_store(T_FORMAT_SWX, ma, sd));
      }
      break;
    }

      // ----------------------------------------------------------------------
      // LLOCK: Load locked
      //
    case OpCode::LLOCK:
    {
      const uint32  ma = *(inst->src2);
      uint32        rd = 0;

      MEMORY_ACCESS(ma);
          
      if (is_stack_check_success_r(ma) && read32(ma, rd)) {
        *(inst->dst1) = rd;
        
        // lookup physical address and store it in 'state.lock_phys_addr' as
        //  30-bit Lock Physical Address = MA[31:2]
        mmu.lookup_data(ma, state.U, state.lock_phys_addr);  
        // set 1-bit Lock Flag  = 1
        state.lock_phys_addr |= 1;
        IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LW, ma, rd));
      } else {
        // clear 1-bit Lock Flag when error occured (TLB exception, memory exception)
        state.lock_phys_addr = 0;
        
        if (!is_stack_check_success_x(ma)) { // raise exception on stack check failure
          ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
          enter_exception (ecause, state.pc, state.pc);
        }
        IF_STEP_TRACE_INSTR(this, trace_load(T_FORMAT_LWX, ma, rd));
      }
      break;
    }
      // ----------------------------------------------------------------------
      // SCOND: Store conditional
      //
    case OpCode::SCOND:
    {
      commit = state.lock_phys_addr & 1; // extract lock flag indicating whether we commit

      if (commit) {
        const uint32 ma = (*inst->src2);
        MEMORY_ACCESS(ma);
        
        if (is_stack_check_success_x(ma) && write32(ma, *(inst->dst1))) {
          IF_STEP_TRACE_INSTR(this, trace_store(T_FORMAT_SW,  ma, *(inst->dst1)));
        } else {
          if (!is_stack_check_success_x(ma)) { // raise exception on stack check failure
            ecause = ECR(sys_arch.isa_opts.EV_ProtV, StoreTLBfault, sys_arch.isa_opts.PV_StackCheck);
            enter_exception (ecause, state.pc, state.pc);
          }
          IF_STEP_TRACE_INSTR(this, trace_store(T_FORMAT_SWX, ma, *(inst->dst1)));
        }
      }
      state.Z               = commit; // set STATUS32.Z to lock flag
      state.lock_phys_addr &= ~1;     // clear lock flag bit
      break;
    }
      
    case OpCode::ASLM: // Bug fix, Nov 07 (to deal with x86 SAL instruction)
      {
        IF_CC(inst,state)
        {
          char shift = (char)(*inst->src2);
          shift = shift & 0x1f;
          if (inst->flag_enable)
          {
            *(inst->dst1) = *(inst->src1);
            if (shift != 0)
            {
              __asm__ __volatile__ ("mov %5, %%cl;\n\t"     /* move shift into 8bit register */
                                    "sal %%cl, %0;\n\t"     /* sal */
                                    "setc %1;\n\t"          /* set C */
                                    "sets %2;\n\t"          /* set N */
                                    "setz %3;\n\t"          /* set Z */
                                      : "=r"(*(inst->dst1)),/* output - %0 */
                                        "=m"(state.C),      /* output - %1 */
                                        "=m"(state.N),      /* output - %2 */
                                        "=m"(state.Z)       /* output - %3 */
                                      : "0"(*(inst->dst1)), /* input/output  - %0 */
                                        "Q"(shift)          /* input  - %5 */
                                      : "cc","%cl"          /* clobbered register */
                                    );
            } else {
              // NOTE: x86 SAL instruction doesn't update flags for 0 shift!
              //
              __asm__ __volatile__ ("mov %2, %%cl;\n\t"       /* move shift into 8bit register */
                                    "sal %%cl, %0"            /* sal */
                                      : "=r"(*(inst->dst1))   /* output - %0 */
                                      : "0" (*(inst->dst1)),  /* input/output - %0 */
                                        "Q"(shift)            /* input  - %2 */
                                      : "cc","%cl"            /* clobbered register */
                                    );
              state.C = false;
              state.N = ((sint32)(*inst->dst1) < 0);
              state.Z = (*inst->dst1 == 0);
            }
          }
          else
            *(inst->dst1) = *(inst->src1) << shift;
        }
        break;
      }

    case OpCode::LSRM: // Bug fix, Nov 07 (to deal with x86 SAL instruction)
      {
        IF_CC(inst,state)
        {
          char shift = (char)(*inst->src2);
          shift=shift & 0x1f;
          if (inst->flag_enable)
          {
            *(inst->dst1) = *(inst->src1);
            if (shift != 0)
            {
              __asm__ __volatile__ ("mov %5, %%cl;\n\t"     /* move shift value into 8 bit register */
                                    "shr %%cl, %0;\n\t"     /* shr */
                                    "setc %1;\n\t"          /* set C */
                                    "sets %2;\n\t"          /* set N */
                                    "setz %3;\n\t"          /* set Z */
                                      : "=r"(*(inst->dst1)),/* output - %0 */
                                        "=m"(state.C),      /* output - %1 */
                                        "=m"(state.N),      /* output - %2 */
                                        "=m"(state.Z)       /* output - %3 */
                                      : "0"(*(inst->dst1)), /* input/output  - %0 */
                                        "Q"(shift)          /* input - %5 */
                                      : "cc","%cl"          /* clobbered register */
                                    );
            } else  {
              // NOTE: x86 SHR instruction doesn't update flags for 0 shift!
              //
              __asm__ __volatile__ ("mov %2, %%cl;\n\t"     /* move shift value into 8 bit register */
                                    "shr %%cl, %0"          /* shr */
                                      : "=r"(*(inst->dst1)) /* output - %0 */
                                      : "0"(*(inst->dst1)), /* input/output  - %0 */
                                        "Q"(shift)          /* input - %2 */
                                      : "cc","%cl");        /* clobbered register */
              state.C = false;
              state.N = ((sint32)(*inst->dst1) < 0);
              state.Z = (*inst->dst1 == 0);
            }
          } else {
            *(inst->dst1) = ((uint32)*(inst->src1)) >> shift;
          }
        }
        break;
      }

    case OpCode::ASRM: // Bug fix, Nov 07 (to deal with x86 SAL instruction)
      {
        IF_CC(inst,state)
        {
          char shift = (char)(*inst->src2);
          shift=shift & 0x1f;

          if (inst->flag_enable)
          {
            *(inst->dst1) = *(inst->src1);
            if (shift != 0)
            {
              __asm__ __volatile__ ("mov %5, %%cl;\n\t"     /* move shift value into 8 bit register */
                                    "sar %%cl, %0;\n\t"     /* sar */
                                    "setc %1;\n\t"          /* set C */
                                    "sets %2;\n\t"          /* set N */
                                    "setz %3;\n\t"          /* set Z */
                                      : "=r"(*(inst->dst1)),/* output - %0 */
                                        "=m"(state.C),      /* output - %1 */
                                        "=m"(state.N),      /* output - %2 */
                                        "=m"(state.Z)       /* output - %3 */
                                      : "0"(*(inst->dst1)), /* input/output  - %0 */
                                        "Q"(shift)          /* input  - %5 */
                                      : "cc", "%cl"         /* clobbered register */
                                    );
            } else { //the x86 SAR instruction doesn't update flags for 0 shift!
              __asm__ __volatile__ ("mov %2, %%cl;\n\t"     /* move shift value into 8 bit register */
                                    "sar %%cl, %0;"         /* sar */
                                      : "=r"(*(inst->dst1)) /* output - %0 */
                                      : "0"(*(inst->dst1)), /* input/output  - %0 */
                                        "Q"(shift)          /* input  - %2 */
                                      : "cc", "%cl");       /* clobbered register */
              state.C = false;
              state.N = ((sint32)(*inst->dst1) < 0);
              state.Z = (*inst->dst1 == 0);
            }
          } else {
            *(inst->dst1) = ((sint32)*(inst->src1)) >> shift;
          }
        }
        break;
      }

    case OpCode::RORM: // Bug fix, Nov 07 (adjusted state.Z and state.N update)
      {
        IF_CC(inst,state)
        {
          char shift = (char)(*inst->src2);
          *(inst->dst1) = *(inst->src1);
          if (inst->flag_enable)
          {
            __asm__ __volatile__ ("mov %3, %%cl;\n\t"     /* move shift value into 8 bit register */
                                  "ror %%cl, %0;\n\t"     /* ror */
                                  "setc %1;\n\t"          /* set C */
                                    : "=r"(*(inst->dst1)),/* output - %0 */
                                      "=m"(state.C)       /* output - %1 */
                                    : "0"(*(inst->dst1)), /* input/output  - %0 */
                                      "Q"(shift)          /* input  - %3 */
                                    : "cc","%cl"          /* clobbered register */
                                  );
            state.Z = ((sint32)*(inst->dst1) == 0);
            state.N = ((sint32)*(inst->dst1) < 0);
          } else {
            __asm__ __volatile__ ("mov %2, %%cl;\n\t"     /* move shift value into 8 bit register */
                                  "ror %%cl, %0"          /* ror */
                                    : "=r"(*(inst->dst1)) /* output - %0 */
                                    : "0"(*(inst->dst1)), /* input/output  - %0 */
                                      "Q"(shift)          /* input  - %2 */
                                    : "cc", "%cl");       /* clobbered register */
          }
        }
        break;
      }

    /* Extended arithmetic instructions start here */

    case OpCode::ABSS: // Bug fix, Nov 07 (instruction not conditional)
      {
        sint32 b = *(inst->src2);
        bool sat = ((uint32)b == 0x80000000);

        if (sat)
          *(inst->dst1) = 0x7fffffff;
        else
          *(inst->dst1) = (b < 0) ? -b : b;

        if (inst->flag_enable)
        {
          state.Z = (b == 0);
          state.N = ((b & 0x80000000) != 0);
          state.V = sat;
          if (sat)
            state.auxs[AUX_MACMODE] |= 0x00000210;
        }
        break;
      }

    case OpCode::ABSSW: // Bug fix, Nov 07 (instruction not conditional, changed logic too)
      {
        sint32 b = (sint32)(*(inst->src2) & 0x0000ffff);
        bool sat = (b == 0x00008000);

        if (sat) {
          *(inst->dst1) = 0x00007fff;
        } else {
          short c = (short)b;
          *(inst->dst1) = (sint32)((c < 0) ? -c : c) & 0x0000ffff;
        }

        if (inst->flag_enable)
        {
          state.Z = (b == 0);
          state.N = ((b & 0x00008000) != 0);
          state.V = sat;
          if (sat) {
            state.auxs[AUX_MACMODE] |= 0x00000210;
          }
        }
        break;
      }

    case OpCode::ADDS: // Bug fixes, Nov 07 (cast for src1 and dst1)
      {       //  1. cast for src1 in '>' comparison
              //  2. cast for dst1 in state.N expression
              //  3. no update to state.C (apparently)
        IF_CC(inst,state)
        {
          char sat;
          sint32 d = *(inst->src1);
          __asm__ __volatile__ ("addl %2, %0;\n\t"      /* subl */
                                "seto %1"               /* set 'sat' */
                                  : "=r"(d),            /* output - %0 */
                                    "=m"(sat)           /* output - %1 */
                                  : "r"(*(inst->src2)), /* input  - %2 */
                                    "0"(d)              /* input/output - %0 */
                                  : "cc"                /* clobbered register */
                                );

          if (sat) {
            *(inst->dst1) = (((sint32)*(inst->src1)) >= 0) ? 0x7fffffff : 0x80000000;
          } else {
            *(inst->dst1) = d;
          }

          if (inst->flag_enable)
          {
            state.Z = (*(inst->dst1) == 0);
            state.N = ((sint32)*(inst->dst1) < 0);
            state.V = (sat != 0);
            if (sat)
              state.auxs[AUX_MACMODE] |= 0x00000210;
          }
        }
        break;
      }

    case OpCode::SUBS: // Bug fix, Nov 07
      {       //  1. cast src1 before comparison
              //  2. comparison should be >= rather than >
              //  3. no update to state.C
              //  4. cast dst1 before comparison
        IF_CC(inst,state)
        {
          char sat;
          sint32 d = (sint32)*(inst->src1);
          
          __asm__ __volatile__ ("subl %2, %0;\n\t"      /* subl */
                                "seto %1"               /* set 'sat' */
                                  : "=r"(d),            /* output - %0 */
                                    "=m"(sat)           /* output - %1 */
                                  : "r"(*(inst->src2)), /* input  - %2 */
                                    "0"(d)              /* input/output - %0 */
                                  : "cc"                /* clobbered register */
                                );

          /* If overflow during subtraction, then the sign of src1 can be used
           * to determine whether to saturate to maximum positive or the
           * maximum negative number.
           */

          if (sat) {
            *(inst->dst1) = (sint32)*(inst->src1) >= 0 ? 0x7fffffff : 0x80000000;
          } else {
            *(inst->dst1) = d;
          }

          if (inst->flag_enable)
          {
            state.Z = (*(inst->dst1) == 0);
            state.N = (((sint32)*(inst->dst1)) < 0);
            state.V = (sat != 0);
            if (sat)
              state.auxs[AUX_MACMODE] |= 0x00000210;
          }
        }
        break;
      }

    case OpCode::ADDSDW: // ADDSDW and SUBSDW are handled together, as their
    case OpCode::SUBSDW: // semantics are almost identical. They differ only
      {             // in the operator performed (add versus subtract).
        IF_CC(inst,state)
        {
#if defined (__GNUC__) && (__GNUC__ >= 4) 
          if (sim_opts.has_mmx)
          {

            // Exploit the MMX 16-bit SIMD add, sub and saturating
            // versions in order to speed up saturating arithmetic.
            // The detection of saturation is performed by comparing
            // the saturated result with the non-saturated result.

            v4hi b, c, sat_result, ord_result;

            b = *(v4hi*)(inst->src1);
            c = *(v4hi*)(inst->src2);

            if (inst->code == OpCode::ADDSDW)
            {
              sat_result = __builtin_ia32_paddsw (b, c);
              ord_result = __builtin_ia32_paddw (b, c);
            } else { // must be OpCode::SUBSDW
              sat_result = __builtin_ia32_psubsw (b, c);
              ord_result = __builtin_ia32_psubw (b, c);
            }

            *inst->dst1 = *(sint32*)(&sat_result); // bug fixed here Nov 07

            if (inst->flag_enable)
            {
              v4hi vdiff = (__builtin_ia32_pxor(ord_result, sat_result));
              vect4 diffs;
              *(v4hi*)diffs = vdiff;
              bool v1 = (diffs[0] != 0);
              bool v2 = (diffs[1] != 0);
              state.V = v1 || v2;
              state.auxs[AUX_MACMODE] |= ((v1 << 4) | (v2 << 9));
              state.Z = (*(inst->dst1) == 0);
              state.N = ((*(inst->dst1) & 0x80008000) != 0);
            }
          }
          else
#endif
          {
            // Standard C implementation, with no machine dependencies.
            // This performs the arithmetic on each 16-bit sub-word value
            // independently.

            // Perform channel 1 (high) first.
            //
            sint32 a = *(inst->src1) & 0xffff0000;
            sint32 b = *(inst->src2) & 0xffff0000;
            char sat;

            if (inst->code == OpCode::ADDSDW)
            {
              __asm__ __volatile__ ("addl %2, %0;\n\t"    /* addl */
                                    "seto %1"             /* set 'sat' */
                                      : "=r"(a),          /* output - %0 */
                                        "=m"(sat)         /* output - %1 */
                                      : "r"(b),           /* input  - %2 */
                                        "0"(a)            /* input/output - %0 */
                                      : "cc"              /* clobbered register */
                                    );
            } else { // must be OpCode::SUBSDW
              __asm__ __volatile__ ("subl %2, %0;\n\t"    /* subl */
                                    "seto %1"             /* set 'sat' */
                                      : "=r"(a),          /* output - %0 */
                                        "=m"(sat)         /* output - %1 */
                                      : "r"(b),           /* input  - %2 */
                                        "0"(a)            /* input/output - %0 */
                                      : "cc"               /* clobbered register */
                                    );
            }

            if (sat)
            {
              *(inst->dst1) = b > 0 ? 0x7fff0000 : 0x80000000;
              if (inst->flag_enable) {
                state.auxs[AUX_MACMODE] |= 0x00000200;
              }
            } else {
              *(inst->dst1) = a;
            }
            // Perform channel 2 (low) second.
            //
            a = *(inst->src1) << 16;
            b = *(inst->src2) << 16;

            if (inst->code == OpCode::ADDSDW)
            {
              __asm__ __volatile__ ("addl %2, %0;\n\t"    /* addl */
                                    "seto %1"             /* set 'sat' */
                                      : "=r"(a),          /* output - %0 */
                                        "=m"(sat)         /* output - %1 */
                                      : "r"(b),           /* input  - %2 */
                                        "0"(a)            /* input/output - %0 */
                                      : "cc"              /* clobbered register */
                                    );
            } else { // must be OpCode::SUBSDW
              __asm__ __volatile__ ("subl %2, %0;\n\t"    /* subl */
                                    "seto %1"             /* set 'sat' */
                                      : "=r"(a),          /* output - %0 */
                                        "=m"(sat)         /* output - %1 */
                                      : "r"(b),           /* input  - %2 */
                                        "0"(a)            /* input/output - %0 */
                                      : "cc"               /* clobbered register */
                                    );
            }

            if (sat)
            {
              *(inst->dst1) |= b > 0 ? 0x00007fff : 0x00008000;
              if (inst->flag_enable)
                state.auxs[AUX_MACMODE] |= 0x00000010;
            } else {
              *(inst->dst1) |= ((uint32)a >> 16);
            }

            if (inst->flag_enable)
            {
              state.Z = (*(inst->dst1) == 0);
              state.N = ((*(inst->dst1) & 0x80008000) != 0);
            }
          }
        }
        break;
      }

    case OpCode::ASLS: // Bug fix, major changes for ASLS
      {
        IF_CC(inst,state)
        {
          sint64 a = (sint64)((sint32)*(inst->src1));
          sint32 b = (sint32)*(inst->src2);
          bool right = (b < 0);
          bool sat = false;
          bool sat_shift = (b > 31);

          if (a == (sint64)0)
          {
            *(inst->dst1) = 0x00000000;
          } else if ((a > (sint64)0) && sat_shift) {
            //saturated +ve value
            *(inst->dst1) = 0x7fffffff;
            sat = true;
          } else if (sat_shift) {
            //saturated -ve value
            sat = true;
            *(inst->dst1) = 0x80000000;
          } else {
            uint64 result;
            if (right)
            {
              if (b < -31)  { b = 31; }
              else          { b = -b; }
              result = a >> b;
              *(inst->dst1) = (uint32)result;
            } else {
              result = a << b;
              //check for saturation
              if (   (a > (sint64)0)
                  && ((result & 0xFFFFFFFF80000000LL) != 0))
              {
                sat = true;
                *(inst->dst1) = 0x7fffffff;
              } else if (   (a < (sint64)0)
                         && ((result & 0xFFFFFFFF80000000LL) != 0xFFFFFFFF80000000LL))
              {
                sat = true;
                *(inst->dst1) = 0x80000000;
              } else {
                *(inst->dst1) = (uint32)result;
              }
            }
          }

          if (inst->flag_enable)
          {
            state.Z = *(inst->dst1) == 0;
            state.N = (sint32)*(inst->dst1) < 0;
            state.V = sat;
            if (sat)
            state.auxs[AUX_MACMODE] |= 0x00000210;
          }
        }
        break;
      }

    case OpCode::ASRS: // Bug fix, major changes for ASLS
      {
        IF_CC(inst,state)
        {
          sint64 a = (sint64)((sint32)*(inst->src1));
          sint32 b = *(inst->src2);
          bool left = (b < 0);
          bool sat = false;
          bool sat_shift = (b < -31);

          if (a == (sint64)0)
          {
            *(inst->dst1) = 0x00000000;
          } else if ((a > (sint64)0) && sat_shift) {
            //saturated +ve value
            *(inst->dst1) = 0x7fffffff;
            sat = true;
          } else if (sat_shift) {
            //saturated -ve value
            sat = true;
            *(inst->dst1) = 0x80000000;
          } else {
            uint64 result;
            if (!left)
            {
              if (b > 31) { b = 31; }
              result = a >> b;
              *(inst->dst1) = (uint32)result;
            } else {
              result = a << -b;
              //check for saturation
              if (   (a > (sint64)0)
                  && ((result & 0xFFFFFFFF80000000LL) != 0))
              {
                sat = true;
                *(inst->dst1) = 0x7fffffff;
              } else if (   (a < (sint64)0)
                         && ((result & 0xFFFFFFFF80000000LL) != 0xFFFFFFFF80000000LL))
              {
                sat = true;
                *(inst->dst1) = 0x80000000;
              } else {
                *(inst->dst1) = (uint32)result;
              }
            }
          }

          if (inst->flag_enable)
          {
            state.Z = *(inst->dst1) == 0;
            state.N = (sint32)*(inst->dst1) < 0;
            state.V = sat;
            if (sat)
              state.auxs[AUX_MACMODE] |= 0x00000210;
          }
        }
        break;
      }

    case OpCode::DIVAW: // Bug fix, Nov 07
      {
        IF_CC(inst,state)
        {
          sint32 src1 = *(inst->src1);
          // Check for special case of src1 == 0
          if (src1 == 0) {
            *(inst->dst1) = 0;
          } else {
            // Otherwise perform division step
            src1 <<= 1;

            // The comparison in RTL is done on bit 31 of the difference(!)
            sint32 diff = src1 - (sint32)(*(inst->src2));
            if ((diff & 0x80000000) == 0) {
              *(inst->dst1) = (src1 - *(inst->src2)) | 0x01;
            } else {
              *(inst->dst1) = src1;
            }
          }
        }
        break;
      }

    case OpCode::NEG: // Bug fix, Nov 07 (added this instruction for ARC 700)
      {
        *(inst->dst1) = - (sint32)*(inst->src2);
        break;
      }

    case OpCode::NEGS: // Bug fix, Nov 07 (not a conditional instruction)
      {
        bool sat;

        if (*(inst->src2) == 0x80000000UL)
        {
          sat = true;
          *(inst->dst1) = 0x7fffffffUL;
        } else {
          sat = false;
          *(inst->dst1) = - (sint32)*(inst->src2);
        }

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = (sint32)(*(inst->dst1)) < 0;
          state.V = sat;
          if (sat) {
            state.auxs[AUX_MACMODE] |= 0x00000210;
          }
        }
        break;
      }

    case OpCode::NEGSW: // Bug fix, Nov 07 (not a conditional instruction)
      {        // also fixed error in first if-condition
        bool sat;
        short src16 = *(inst->src2) & 0x0000ffff;

        if (src16 == (sint32)0xffff8000)
        {
          sat = true;
          *(inst->dst1) = 0x00007fffUL;
        } else {
          sat = false;
          *(inst->dst1) = (sint32)(-src16);
        }

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = (sint32)(*(inst->dst1)) < 0;
          state.V = sat;
          if (sat) {
            state.auxs[AUX_MACMODE] |= 0x00000210;
          }
        }
        break;
      }

    case OpCode::NORM: // Bug fix, Nov 07 (not a conditional instruction)
      {       // also need to retest 'a' after bsrl insn
        sint32 a = *(inst->src2);
        bool z = (a == 0);
        bool n = (a < 0);

        if (a < 0)
          a = ~a ;

        sint32 d;

        // Intel x86 dependency
        //
        __asm__ __volatile__ ("bsrl %1, %0" : "=r"(d) : "r"(a) : "cc");
        
        *(inst->dst1) = (a == 0) ? 31: 30 - d; // Bug fix, Nov 07

        if (inst->flag_enable)
        {
          state.Z = z;
          state.N = n;
        }
        break;
      }

    case OpCode::NORMW: // Bug fix, Nov 07 (not a conditional instruction)
      {        // also need to retest 'a' after bsrl insn
        sint32 a = ((sint32)*(inst->src2)) << 16;
        bool z = (a == 0);
        bool n = (a < 0);

        if (a < 0) {
          a = ~a ;
        }
        sint32 d;

        // Intel x86 dependency
        //
        __asm__ __volatile__ ("bsrl %1, %0" : "=r"(d) : "r"(a) : "cc");
        *(inst->dst1) = (z || (a == 0x0000ffff)) ? 15: 30 - d; // Bug fix, Nov 07

        if (inst->flag_enable)
        {
          state.Z = z;
          state.N = n;
        }
        break;
      }

    case OpCode::FFS: // ARCv2 only
      {
        sint32 a = *(inst->src2);
        sint32 d = 31;

        if (inst->flag_enable)
        {
          state.Z = (a == 0);
          state.N = (a < 0);
        }

        // Intel x86 dependency
        //
        __asm__ __volatile__ ("bsfl %1, %0" : "+r"(d) : "rm"(a) : "cc");
        *(inst->dst1) = d;
        
        break;
      }

    case OpCode::FLS: // ARCv2 only
      {
        sint32 a = *(inst->src2);
        sint32 d = 0;

        if (inst->flag_enable)
        {
          state.Z = (a == 0);
          state.N = (a < 0);
        }

        // Intel x86 dependency
        //
        __asm__ __volatile__ ("bsrl %1, %0" : "+r"(d) : "rm"(a) : "cc");
        *(inst->dst1) = d;
        
        break;
      }

    case OpCode::RND16: // Bug fix, Nov 07 (not a conditional instruction)
      {
        sint32 a = *(inst->src2);
        bool sat;

        if (a >= 0x7fff8000L)
        {
          sat = true;
          *(inst->dst1) = 0x00007fffL;
        } else {
          sat = false;
          *(inst->dst1) = (a + 0x00008000) >> 16;
        }

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0; // Bug fix, Nov 07
          state.V = sat;
          if (sat)
            state.auxs[AUX_MACMODE] |= 0x00000210;
        }
        break;
      }

    case OpCode::SAT16: // Bug fix, Nov 07 (not a conditional instruction)
      {
        sint32 a   = *(inst->src2);
        bool   sat = false;

        if (a > (sint32)0x00007fff)
        {
          sat = true;
          *(inst->dst1) = 0x00007fffUL;
        } else if (a < (sint32)0xffff8000) {
          sat = true;
          *(inst->dst1) = 0xffff8000UL;
        } else { // Bug fix, Nov 07 - this clause is new
          *(inst->dst1) = a;
        }

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0; // Bug fix, Nov 07
          state.V = sat;
          if (sat) {
            state.auxs[AUX_MACMODE] |= 0x00000210;
          }
        }
        break;
      }

    /* Extended arithmetic instructions finish here. */

    case OpCode::SWAP: // Bug fix, Nov 07 (not a conditional instruction)
      {
        uint32 a = (uint32)*(inst->src2);
        *(inst->dst1) = (a >> 16) | (a << 16);

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0; // Bug fix, Nov 07
        }
        break;
      }

    case OpCode::SWAPE: // ARCv2 only
      {
        uint32 a = (uint32)*(inst->src2);
        a = ((a & 0x00FF00FFU) << 8) | ((a & 0xFF00FF00U) >> 8);
        *(inst->dst1) = (a >> 16) | (a << 16);

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0;
        }
        break;
      }

    case OpCode::LSL16: // ARCv2 only
      {
        uint32 a = (uint32)*(inst->src2);
        *(inst->dst1) = (a << 16);

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0;
        }
        break;
      }

    case OpCode::LSR16: // ARCv2 only
      {
        uint32 a = (uint32)*(inst->src2);
        *(inst->dst1) = (a >> 16);

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0;
        }
        break;
      }

    case OpCode::ASR16: // ARCv2 only
      {
        sint32 a = (sint32)*(inst->src2);
        *(inst->dst1) = (uint32)(a >> 16);

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0;
        }
        break;
      }

    case OpCode::ASR8: // ARCv2 only
      {
        sint32 a = (sint32)*(inst->src2);
        *(inst->dst1) = (uint32)(a >> 8);

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0;
        }
        break;
      }

    case OpCode::LSR8: // ARCv2 only
      {
        uint32 a = (uint32)*(inst->src2);
        *(inst->dst1) = (a >> 8);

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0;
        }
        break;
      }

    case OpCode::LSL8: // ARCv2 only
      {
        uint32 a = (uint32)*(inst->src2);
        *(inst->dst1) = (a << 8);

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0;
        }
        break;
      }

    case OpCode::ROL8: // ARCv2 only
      {
        uint32 a = (uint32)*(inst->src2);
        *(inst->dst1) = (a << 8) | (a >> 24);

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0;
        }
        break;
      }

    case OpCode::ROR8: // ARCv2 only
      {
        uint32 a = (uint32)*(inst->src2);
        *(inst->dst1) = (a >> 8) | (a << 24);

        if (inst->flag_enable)
        {
          state.Z = *(inst->dst1) == 0;
          state.N = ((sint32)*(inst->dst1)) < 0;
        }
        break;
      }

    case OpCode::DIV:  // ARCv2 only
      {
        IF_CC(inst,state)
        {
          sint32 divisor = (sint32)(*(inst->src2));
          
          if (divisor != 0)
          {
            uint32 dividend = (*(inst->src1));
	    
            if ((dividend != 0x80000000UL) || (divisor != -1))
            {	    
              sint32 quotient = (sint32)dividend / divisor;
              *(inst->dst1) = (uint32)quotient;

              if (inst->flag_enable)
              {
                state.V = 0;
                state.N = (quotient < 0);
                state.Z = (quotient == 0);
              }
            } else {
              commit  = 0;
              if (inst->flag_enable)
              {
                state.V = 1;
                state.N = 0;
                state.Z = 0;
              }
            }
          } else {
            if (state.DZ != 0) {
              enter_exception (ECR(sys_arch.isa_opts.EV_DivZero, 0, 0), state.pc, state.pc);
            } else {
              commit = 0;
              if (inst->flag_enable)
              {
                state.V = 1;
                state.N = 0;
                state.Z = 0;
              }
            }            
          }
        }
        break;
      }      

    case OpCode::DIVU:  // ARCv2 only
      {
        IF_CC(inst,state)
        {
          uint32 divisor = (uint32)(*(inst->src2));
          
          if (divisor != 0) {
            uint32 quotient = (uint32)(*(inst->src1)) / divisor;
            *(inst->dst1) = quotient;
            
            if (inst->flag_enable)
            {
              state.V = 0;
              state.N = 0;
              state.Z = (quotient == 0);
            }
          } else {
            // divisor is '0'
            //
            if (state.DZ != 0) {
              enter_exception (ECR(sys_arch.isa_opts.EV_DivZero, 0, 0), state.pc, state.pc);
            } else {
              commit = 0;
              if (inst->flag_enable)
              {
                state.V = 1;
                state.N = 0;
                state.Z = 0;
              }
            }            
          }
        }
        break;
      }

    case OpCode::REM:  // ARCv2 only
      {
        IF_CC(inst,state)
        {
          sint32 divisor = (sint32)(*(inst->src2));
          
          if (divisor != 0)
          {
            sint32 dividend = (sint32)(*(inst->src1));
	    
            if ((dividend != 0x80000000) || (divisor != -1))
            {	    
              sint32 remainder = dividend % divisor;
              *(inst->dst1) = (uint32)remainder;

              if (inst->flag_enable)
              {
                state.V = 0;
                state.N = (remainder < 0);
                state.Z = (remainder == 0);
              }
            } else {
              commit = 0;
              if (inst->flag_enable)
              {
                state.V = 1;
                state.N = 0;
                state.Z = 0;
              }
            }
          } else {
            if (state.DZ != 0) {
              enter_exception (ECR(sys_arch.isa_opts.EV_DivZero, 0, 0), state.pc, state.pc);
            } else {
              commit = 0;
              if (inst->flag_enable)
              {
                state.V = 1;
                state.N = 0;
                state.Z = 0;
              }
            }            
          }
        }
        break;
      }
      

    case OpCode::REMU:  // ARCv2 only
      {
        IF_CC(inst,state)
        {
          uint32 divisor = (uint32)(*(inst->src2));
          
          if (divisor != 0)
          {
            uint32 remainder = (uint32)(*(inst->src1)) % divisor;
            *(inst->dst1) = remainder;
            
            if (inst->flag_enable)
            {
              state.V = 0;
              state.N = 0;
              state.Z = (remainder == 0);
            }
          } else {
            if (state.DZ != 0) {
              enter_exception (ECR(sys_arch.isa_opts.EV_DivZero, 0, 0), state.pc, state.pc);
            } else {
              commit = 0;
              if (inst->flag_enable)
              {
                state.V = 1;
                state.N = 0;
                state.Z = 0;
              }
            }            
          }
        }
        break;
      }

    case OpCode::JLI_S:  // ARCv2 only
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block = true;
          prev_had_dslot = false;
          inst->taken_branch = true;
          *(inst->dst1) = state.pc + inst->link_offset;  // BLINK <= next_seq_PC
          state.next_pc = *(inst->src1) + *(inst->src2); // state.next_pc <= JLI_BASE + (u10<<2)
          // Profiling counters
          //
          if (sim_opts.is_call_freq_recording_enabled) {
            cnt_ctx.call_freq_hist.inc(state.next_pc);
          }
          // FIXME: How shall we best perform call graph recording in this case
          //
        END_ILLEGAL_IN_DSLOT
        break;
      }      

    case OpCode::EI_S:  // ARCv2 only
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block = true;
          prev_had_dslot = false;
          inst->taken_branch = true;
          state.next_pc = *(inst->src1) + *(inst->src2); // state.next_pc <= EI_BASE + (u10<<2)
          *(inst->dst2) = state.pc + 2;                  // BTA <= next sequential PC
          next_E = true;	  
          // Profiling counters
          //
          if (sim_opts.is_call_freq_recording_enabled) {
            cnt_ctx.call_freq_hist.inc(state.next_pc);
          }
          // FIXME: How shall we best perform call graph recording in this case
          //
        END_ILLEGAL_IN_DSLOT
        break;
      }      

    case OpCode::BI:    // ARCv2 only
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block = true;
          prev_had_dslot = false;
          inst->taken_branch = true;

          // state.next_pc <= next_seq_PC + (c << 2)
          //
          state.next_pc = state.pc + inst->size + (*(inst->src2) << 2);
        END_ILLEGAL_IN_DSLOT
        break;
      }
    
    case OpCode::BIH:   // ARCv2 only
      {
        START_ILLEGAL_IN_DSLOT
          end_of_block = true;
          prev_had_dslot = false;
          inst->taken_branch = true;

          // state.next_pc <= next_seq_PC + (c <<2)
          //
          state.next_pc = state.pc + inst->size + (*(inst->src2) << 1);                 
        END_ILLEGAL_IN_DSLOT
        break;
      }
    
    /* ARCv2 instructions finish here. */

    case OpCode::FLAG: // Bug fix, Nov 07 (this insn is conditional)
      {            // flag_enable => kflag instruction for A6K
        IF_CC(inst,state)
        {
          end_of_block = true;
          flag_inst (*(inst->src2), inst->flag_enable);
        }
        break;
      }

    case OpCode::SLEEP:
      {
        end_of_block = true;
        sleep_inst (*(inst->src2));
        // The sleep instruction DOES commit itself.
        break;
      }

    case OpCode::BREAK:
      {
        end_of_block = true;
        LOG(LOG_DEBUG4) << "OpCode::BREAK executing";
#ifdef VERIFICATION_OPTIONS
        if (!sys_arch.isa_opts.ignore_brk_sleep) {
#endif
          break_inst ();
          // The BRK and BRK_S instructions DO NOT commit themselves.
          // Hence, return immediately from the simulation loop
          // without committing any results or updating PC.

#ifdef STEP
          // Copy state-change information into the delta structure
          // This captures the architecturally-visible CPU state
          // changes in a single structure that can be used for
          // simulator verification, or for state-wise comparisons
          // between this simulator and (for example) an RTL simulation.
          //
          // Currently, the delta structure does not include aux register
          // side effects, and Flag updates.
          //
          if (sim_opts.cosim) {
            delta->pc     = state.pc;
            delta->inst   = inst->info.ir;
            delta->limm   = inst->limm;
            if (commit && inst->info.rf_wenb0) { delta->wmask |= UPKT_WMASK_RF0; }
            if (commit && inst->info.rf_wenb1) { delta->wmask |= UPKT_WMASK_RF1; }
            delta->rf[0].a= inst->info.rf_wa0;
            delta->rf[1].a= inst->info.rf_wa1;
            if (inst->dst1) { delta->rf[0].w = *(inst->dst1); }
            if (inst->dst2) { delta->rf[1].w = *(inst->dst2); }
            delta->status32 = BUILD_STATUS32(state);
          }
#endif /* STEP */
#ifdef VERIFICATION_OPTIONS
        }
        
        if (sys_arch.isa_opts.ignore_brk_sleep) {
          // Behave like the RTL testbench and replace the
          // BRK/BRK_S instruction with a NOP/NOP_S instructions
          // and then re-execute the instruction.
          //
          LOG(LOG_DEBUG4) << "OpCode::BREAK calling clear_breakpoint()";
          bool brk_s = ((inst->size == 2) || (inst->size == 6));
          
          // FIXME: '0x78e0'  '0x264a7000' stand for NOP_S/NOP, turn them into
          //        constants
          clear_breakpoint(state.pc, ((brk_s) ? 0x78e0 : 0x264a7000), brk_s);
          state.next_pc = state.pc;
          next_D = state.D;
          next_E = state.ES;
          state.next_lpc = state.gprs[LP_COUNT];
          break;
        } else {
          return_before_commit = true;
          break;
        }   
#else
        // Non-verification mode
        //
        return_before_commit = true;
        break;
#endif
      }
    case OpCode::AP_BREAK:
      {
        // An Actionpoints breakpoint has been triggered, and this may lead
        // to either an exception or a break, depending on the Action setting
        // of the Actionpoint that was triggered. If the AA field == 0 then
        // a breakpoint halt will take place, otherwise a Privilege Violation
        // exception will be taken, with cause code set to ActionPointHit (2).
        // The AA field of a matching instruction Actionpoint will have been
        // assigned to the shimm field of the Dcode object.
        //
        end_of_block = true;
        
        aps.take_breakpoint(inst->aps_inst_matches, state.pc, inst->info.ir);
        
        // Insert the ASR[7:0] bits into the DEBUG auxiliary register
        //
        state.auxs[AUX_DEBUG] =   (state.auxs[AUX_DEBUG] & 0xfffff807UL) 
                                | ((aps.aps_matches & 0xffUL) << 3);
        
        if (inst->shimm == 0) {
          // The Action for this Actionpoint is to Halt the processor,
          // and this is implemented using the same mechanism as a BRK instruction.
          //
#ifdef VERIFICATION_OPTIONS
          if (!sys_arch.isa_opts.ignore_brk_sleep) {
#endif
            break_inst ();

#ifdef STEP
            // Copy state-change information into the delta structure
            // This captures the architecturally-visible CPU state
            // changes in a single structure that can be used for
            // simulator verification, or for state-wise comparisons
            // between this simulator and (for example) an RTL simulation.
            //
            // Currently, the delta structure does not include aux register
            // side effects, and Flag updates.
            //
            if (sim_opts.cosim) {
              delta->pc     = state.pc;
              delta->inst   = inst->info.ir;
              delta->limm   = inst->limm;
              if (commit && inst->info.rf_wenb0) { delta->wmask |= UPKT_WMASK_RF0; }
              if (commit && inst->info.rf_wenb1) { delta->wmask |= UPKT_WMASK_RF1; }
              delta->rf[0].a= inst->info.rf_wa0;
              delta->rf[1].a= inst->info.rf_wa1;
              if (inst->dst1) { delta->rf[0].w = *(inst->dst1); }
              if (inst->dst2) { delta->rf[1].w = *(inst->dst2); }
              delta->status32 = BUILD_STATUS32(state);
            }
#endif /* STEP */
#ifdef VERIFICATION_OPTIONS
          }
        
          // An Actionpoint break DOES NOT commit, but returns
          // immediately from the simulation loop if such termination
          // if not disabled by the ignore_brk_sleep option
          //
          if (sys_arch.isa_opts.ignore_brk_sleep) {
            state.next_pc  = state.pc;
            next_D         = state.D;
            next_E         = state.ES;
            state.next_lpc = state.gprs[LP_COUNT];
            break;
          } else {
            return_before_commit = true;
            break;
          }
#else
          // Non-verification mode
          //
          return_before_commit = true;
          break;
#endif
        } else {
          // The action for this Actionpoint is to raise a software
          // exception, specifically a Privilege Violation with cause
          // code 2.
          //
          int apn = 0;
          uint32 aps = inst->aps_inst_matches & 0xffUL;

          LOG(LOG_DEBUG4) << "[APS] Raising Breakpoint exception with aps_inst_matches = " 
                          << std::hex << std::setw(8) << std::setfill('0')
                          << inst->aps_inst_matches;

          // find first bit set in aps, assigning bit number to apn
          //
          while (aps && ((aps & 0x1) == 0)) {
            ++apn;
            aps = aps >> 1;
          }

          // Set the AP_WP_PC aux register to the current PC
          //
          state.auxs[AUX_AP_WP_PC] = state.pc;

          uint32 ecr_value = ECR(sys_arch.isa_opts.EV_PrivilegeV, ActionPointHit, apn);
          LOG(LOG_DEBUG4) << "[APS] Raising exception with ECR " 
                          << std::hex << std::setw(8) << std::setfill('0')
                          << ecr_value;
                    
          enter_exception (ecr_value, state.pc, state.pc);

#ifdef STEP
          // Copy state-change information into the delta structure
          // This captures the architecturally-visible CPU state
          // changes in a single structure that can be used for
          // simulator verification, or for state-wise comparisons
          // between this simulator and (for example) an RTL simulation.
          //
          // Currently, the delta structure does not include aux register
          // side effects, and Flag updates.
          //
          if (sim_opts.cosim) {
            delta->pc     = state.pc;
            delta->inst   = inst->info.ir;
            delta->limm   = inst->limm;
            if (commit && inst->info.rf_wenb0) { delta->wmask |= UPKT_WMASK_RF0; }
            if (commit && inst->info.rf_wenb1) { delta->wmask |= UPKT_WMASK_RF1; }
            delta->rf[0].a= inst->info.rf_wa0;
            delta->rf[1].a= inst->info.rf_wa1;
            if (inst->dst1) { delta->rf[0].w = *(inst->dst1); }
            if (inst->dst2) { delta->rf[1].w = *(inst->dst2); }
            delta->status32 = BUILD_STATUS32(state);
          }
#endif
          break;
        }
        break;
      }

    // -------------------------------------------------------------------------
    // Software interrupts and trap instructions
    //
    case OpCode::SWI:
    {
      phys_profile_.reset_active_trace_sequence(interrupt_stack.top());
      end_of_block = true;

      if (sys_arch.isa_opts.is_isa_a600()) { // A600 semantics
        ecause = ECR(sys_arch.isa_opts.EV_InstructionError,IllegalInstruction, 0); // instruction error level interrupt
        efa    = state.pc;                                    // exception fault address
      } else { // A700 and ARCompact V2 semantics
        // The SWI and SWI_S instructions DO NOT commit themselves (i.e. they
        // do not commit any results and do not update the PC). The check if
        // pre-commit exceptions have been raised happens just before PC is
        // updated.
        //
        enter_exception (*(inst->src2), state.pc, state.pc);
      }
      
#ifdef STEP
      // Copy state-change information into the delta structure
      // This captures the architecturally-visible CPU state
      // changes in a single structure that can be used for
      // simulator verification, or for state-wise comparisons
      // between this simulator and (for example) an RTL simulation.
      //
      // Currently, the delta structure does not include aux register
      // side effects, and Flag updates.
      //
      if (sim_opts.cosim) {
        delta->pc     = state.pc;
        delta->inst   = inst->info.ir;
        delta->limm   = inst->limm;
        if (commit && inst->info.rf_wenb0) { delta->wmask |= UPKT_WMASK_RF0; }
        if (commit && inst->info.rf_wenb1) { delta->wmask |= UPKT_WMASK_RF1; }
        delta->rf[0].a= inst->info.rf_wa0;
        delta->rf[1].a= inst->info.rf_wa1;
        if (inst->dst1) { delta->rf[0].w = *(inst->dst1); }
        if (inst->dst2) { delta->rf[1].w = *(inst->dst2); }
        delta->status32 = BUILD_STATUS32(state);
      }
      
#endif
      break;
    }

    case OpCode::TRAP0:
      {

        // The TRAP and TRAP_S instructions set the local ecause and efa
        // variables for use after the instruction has committed all state changes.
        // The trap exception, or the emulated exception, will be taken later.
        //
        ecause       = *(inst->src2); // fully-decoded ECR value       
        efa          = state.pc;      // exception fault address
        phys_profile_.reset_active_trace_sequence(interrupt_stack.top());
        end_of_block = true;

        // If we are already in an exception we will raise a machine check exception,
        // instead of a trap. Call enter_exception() here causing a pre-commit exception.
        //
        if (state.AE)
          enter_exception (ecause, efa, state.pc);
        
        break;
      }

    case OpCode::RTIE:
      {
        START_ILLEGAL_IN_DSLOT
          phys_profile_.reset_active_trace_sequence(interrupt_stack.top());
          end_of_block = true;
          exit_exception ();
          next_D = state.D;
          next_E = state.ES;
          IF_STEP_TRACE_INSTR(this, trace_rtie());
        END_ILLEGAL_IN_DSLOT
        break;
      }

    // -------------------------------------------------------------------------
      
    case OpCode::SYNC:
      break;

    case OpCode::FMUL:
    case OpCode::FADD:
    case OpCode::FSUB:
      {
        IF_CC(inst,state) {
          spfp_emulation(inst->code,inst->dst1,*(inst->src1),*(inst->src2),inst->flag_enable);
        }
        break;
      }

    case OpCode::DMULH11:
    case OpCode::DMULH12:
    case OpCode::DMULH21:
    case OpCode::DMULH22:
    case OpCode::DADDH11:
    case OpCode::DADDH12:
    case OpCode::DADDH21:
    case OpCode::DADDH22:
    case OpCode::DSUBH11:
    case OpCode::DSUBH12:
    case OpCode::DSUBH21:
    case OpCode::DSUBH22:
    case OpCode::DRSUBH11:
    case OpCode::DRSUBH12:
    case OpCode::DRSUBH21:
    case OpCode::DRSUBH22: {
      IF_CC(inst,state) {
        dpfp_emulation(inst->code,inst->dst1,*(inst->src1),*(inst->src2),inst->flag_enable);
      }
      break;
    }

    case OpCode::DEXCL1:
    case OpCode::DEXCL2: {
      IF_CC(inst,state) { // register D1|D2 exchange
        dexcl_emulation(inst->code, inst->dst1, *(inst->src1), *(inst->src2));
      }
      break;
    }

    case OpCode::NOP:
      break;
      
      // -----------------------------------------------------------------------
      // ENTER instruction overloads the following fields:
      //   shimm   - number of general regs starting r13 to save/restore
      //   link    - 1 => save BLINK,  0 => don't save BLINK
      //   dslot   - 1 => save FP,     0 => don't save FP
      //
    case OpCode::ENTER: {
      START_ILLEGAL_IN_DSLOT
        
        uint32       rd, ra, rn;
        bool         success = true;
        const sint32 saves   = inst->shimm + (inst->link ? 1 : 0) + (inst->dslot ? 1 : 0);
        sint32       offset  = (-saves) << 2;
        
        if (saves && addr_is_misaligned(state.gprs[SP_REG], 3, state.pc)) { 
          break; // bail out if SP address is misaligned and should be saved
        }
      
#ifdef STEP
        UpdatePacket* cur_uop = delta;
        const uint32 s32  = BUILD_STATUS32(state); // Not changed by enter_s
        if (sim_opts.cosim) {
          cur_uop->pc       = state.pc;
          cur_uop->inst     = inst->info.ir;
          cur_uop->status32 = s32;
          cur_uop->wmask    = 0;
        }
#endif /* STEP */
      
        if (saves != 0) { // ENTER is encoded to save some values

          const uint32 old_sp = state.gprs[SP_REG]; // store SP to enable restore in case of failure

          // save BLINK register if 'link' is set ----------------------------
          if (inst->link) {
            const uint32 ma = state.gprs[SP_REG] + offset;
            if(state.SC && (state.stack_base <= ma || state.stack_top > ma))
            { // Stack checking ENABLED and FAILED
              MEMORY_ACCESS(ma);
              ecause = ECR(sys_arch.isa_opts.EV_ProtV, StoreTLBfault, sys_arch.isa_opts.PV_StackCheck);
              enter_exception (ecause, state.pc, state.pc);
              success = false;
            } else {
              EXEC_UOP_ST_R(BLINK, offset);
#ifdef STEP
                if (sim_opts.cosim) { // pack result delta of uop instruction: ST c,[sp,s9]
                  INIT_UOP_DELTA((0x1c003000UL
                                  | ((offset & 0x000000ffUL) << 16)
                                  | ((offset & 0x80000000UL) >> 16)
                                  | ((BLINK  & 0x3fUL) << 6)), 0U);
                  cur_uop->store.pc     = cur_uop->pc;
                  cur_uop->store.addr   = ra;
                  cur_uop->store.mask   = 0xffffffffUL;
                  cur_uop->store.data[0]= rd;
                  cur_uop->wmask       |= UPKT_WMASK_ST;
                }
              TRACE_UOP_ST_R();
#endif /* STEP */    
              offset += 4;
            }
          }
            
          // 'shimm' encodes number of general regs starting from r13 to save
          for (int i = 0; (i < inst->shimm) && success; ++i) {
            const uint32 ma = state.gprs[SP_REG] + offset;
            if(state.SC && (state.stack_base <= ma || state.stack_top > ma))
            { // Stack checking ENABLED and FAILED
              MEMORY_ACCESS(ma);
              ecause = ECR(sys_arch.isa_opts.EV_ProtV, StoreTLBfault, sys_arch.isa_opts.PV_StackCheck);
              enter_exception(ecause, state.pc, state.pc);
              success = false;
            } else {
              EXEC_UOP_ST_R(13+i, offset);
#ifdef STEP
              if (sim_opts.cosim) { // pack result delta of uop instruction: ST c,[sp,s9]
                INIT_UOP_DELTA(( 0x1c003000UL | ((offset & 0x000000ffUL) << 16)
                                | ((offset & 0x80000000UL) >> 16)
                                | ((13+i   & 0x3fUL) << 6)), 0U);
                cur_uop->store.pc     = cur_uop->pc;
                cur_uop->store.addr   = ra;
                cur_uop->store.mask   = 0xffffffffUL;
                cur_uop->store.data[0]= rd;
                cur_uop->wmask       |= UPKT_WMASK_ST;
              }
              TRACE_UOP_ST_R();
#endif /* STEP */    
              offset += 4;
            }
          } // END FOR - storing general regs starting from r13
            
          // save FP is 'dlsot' is set ---------------------------------------
          if (inst->dslot && success) {
            const uint32 ma = state.gprs[SP_REG] + offset;
            if(state.SC && (state.stack_base <= ma || state.stack_top > ma))
            { // Stack checking ENABLED and FAILED
              MEMORY_ACCESS(ma);
              ecause = ECR(sys_arch.isa_opts.EV_ProtV, StoreTLBfault, sys_arch.isa_opts.PV_StackCheck);
              enter_exception (ecause, state.pc, state.pc);
              success = false;
            } else {
              EXEC_UOP_ST_R(FP_REG, offset);
#ifdef STEP
              if (sim_opts.cosim) { // pack result delta of uop instruction: ST c,[sp,s9]
                INIT_UOP_DELTA((  0x1c003000UL
                                | ((offset & 0x000000ffUL) << 16)
                                | ((offset & 0x80000000UL) >> 16)
                                | ((FP_REG   & 0x3fUL) << 6)), 0U);
                cur_uop->store.pc     = cur_uop->pc;
                cur_uop->store.addr   = ra;
                cur_uop->store.mask   = 0xffffffffUL;
                cur_uop->store.data[0]= rd;
                cur_uop->wmask       |= UPKT_WMASK_ST;
              }
              TRACE_UOP_ST_R();
#endif /* STEP */        
              offset += 4;
            }
          } // END IF - save FP
          
          // If previous memory operations succeeded we modify registers ------
          
          if (success) {
            EXEC_UOP_SUB_S(saves<<2);
#ifdef STEP
            if (sim_opts.cosim) { // pack result delta of uop instruction: SUB_S sp,sp,u7
              INIT_UOP_DELTA((0xc1a00000UL | (((saves<<2) & 0x7fUL) << 14)), 0U);
              cur_uop->rf[0].a = SP_REG;
              cur_uop->rf[0].w = rd;
              cur_uop->wmask  |= UPKT_WMASK_RF0;
            }
            TRACE_UOP_REG_OP();
#endif /* STEP */
              
            if (inst->dslot) {
              EXEC_UOP_MOV_FP();
#ifdef STEP
              if (sim_opts.cosim) { // pack result delta of uop instruction: MOV fp, sp
                INIT_UOP_DELTA(0x23ca3700UL, 0U);
                cur_uop->rf[0].a = FP_REG;
                cur_uop->rf[0].w = state.gprs[SP_REG];
                cur_uop->wmask  |= UPKT_WMASK_RF0;
              }
              TRACE_UOP_REG_OP();
#endif /* STEP */
            }
          }
          
          if (!success) { // restore stack pointer in case of failure
            state.gprs[SP_REG] = old_sp;
          }
          
        } // END (saves != 0)
      
      END_ILLEGAL_IN_DSLOT
      break;
    }

      // -----------------------------------------------------------------------
      // LEAVE instruction overloads the following fields:
      //   shimm         - number of general regs starting r13 to save/restore
      //   link          - 1 => restore BLINK, 0 => don't restore BLINK
      //   dslot         - 1 => restore FP,    0 => don't restore FP
      //   info.isReturn - 1 => jump to blink after restoring context
      //
    case OpCode::LEAVE: {
      START_ILLEGAL_IN_DSLOT
        
        uint32  rd, ra, rn;
        bool    success    = true;
        const sint32 saved = inst->shimm + (inst->link ? 1 : 0) + (inst->dslot ? 1 : 0);

        const uint32 old_sp = state.gprs[SP_REG];
      
        if (saved && addr_is_misaligned(state.gprs[SP_REG], 3, state.pc)) {
          break; // bail out if SP address is misaligned and should be saved
        }
        
#ifdef STEP
        UpdatePacket* cur_uop = delta;
        const uint32 s32      = BUILD_STATUS32(state); // not changed by leave_s
        if (sim_opts.cosim) {
          cur_uop->pc       = state.pc;
          cur_uop->inst     = inst->info.ir;
          cur_uop->status32 = s32;
          cur_uop->wmask    = 0;
        }
#endif /* STEP */
          
        if (saved != 0) { // LEAVE is encoded to save some values             
          sint32 offset = 0;
          
          // restore FP if 'dslot' is set, sp <= fp --------------------------
          if (inst->dslot) {
            EXEC_UOP_MOV_SP();
#ifdef STEP
            if (sim_opts.cosim) { // Pack result delta of uop instruction: MOV sp, fp
              INIT_UOP_DELTA(0x24ca36c0UL, 0U);
              cur_uop->rf[0].a = SP_REG;
              cur_uop->rf[0].w = state.gprs[FP_REG];
              cur_uop->wmask  |= UPKT_WMASK_RF0;
            }
            TRACE_UOP_REG_OP();
#endif /* STEP */
          }
          
          // restore BLINK register if 'link' is set -------------------------
          if (inst->link) {    
            const uint32 ma = state.gprs[SP_REG] + offset;
            if(state.SC && (state.stack_base <= ma || state.stack_top > ma))
            { // Stack checking ENABLED and FALED
              MEMORY_ACCESS(ma);
              ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
              enter_exception (ecause, state.pc, state.pc);
              success = false;
            } else {
              EXEC_UOP_LD_R(BLINK, offset);
#ifdef STEP
              if (sim_opts.cosim) { // pack result delta of uop instruction: LD a,[sp,s9]
                INIT_UOP_DELTA(( 0x14003000UL
                                | ((offset & 0x000000ffUL) << 16)
                                | ((offset & 0x80000000UL) >> 16)
                                | (BLINK   & 0x0000003fUL)), 0U);
                cur_uop->rf[1].a = rn;
                cur_uop->rf[1].w = rd;
                cur_uop->wmask  |= UPKT_WMASK_RF1;
              }
              TRACE_UOP_LD_R();
#endif /* STEP */
              offset += 4;
            }
          }
          
          // 'shimm' encodes number of general regs starting from r13 to restore
          for (int i = 0; i < inst->shimm && success; ++i) {
            const uint32 ma = state.gprs[SP_REG] + offset;
            if(state.SC && (state.stack_base <= ma || state.stack_top > ma))
            { // Stack checking ENABLED and FAILED
              MEMORY_ACCESS(ma);
              ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
              enter_exception (ecause, state.pc, state.pc);
              success = false;
            } else {
              EXEC_UOP_LD_R(13+i, offset);
#ifdef STEP
              if (sim_opts.cosim) { // pack result delta of uop instruction: LD a,[sp,s9]
                INIT_UOP_DELTA(( 0x14003000UL
                                | (((offset) & 0x000000ffUL) << 16)
                                | (((offset) & 0x80000000UL) >> 16)
                                | ((13+i)  & 0x0000003fUL)), 0U);
                cur_uop->rf[1].a = rn;
                cur_uop->rf[1].w = rd;
                cur_uop->wmask  |= UPKT_WMASK_RF1;
              }
              TRACE_UOP_LD_R();
#endif /* STEP */
              offset += 4;
            }
          }
          
          // restore FP is 'dlsot' is set ------------------------------------
          if (inst->dslot && success) {
            const uint32 ma = state.gprs[SP_REG] + offset;
            if(state.SC && (state.stack_base <= ma || state.stack_top > ma))
            {  // Stack checking ENABLED and FAILED
              MEMORY_ACCESS(ma);
              ecause = ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck);
              enter_exception (ecause, state.pc, state.pc);
              success = false;
            } else {
              EXEC_UOP_LD_R(FP_REG, offset);
#ifdef STEP
              if (sim_opts.cosim) { // pack result delta of uop instruction: LD a,[sp,s9]
                INIT_UOP_DELTA(( 0x14003000UL
                                | ((offset & 0x000000ffUL) << 16)
                                | ((offset & 0x80000000UL) >> 16)
                                | (FP_REG  & 0x0000003fUL)), 0U);
                cur_uop->rf[1].a = rn;
                cur_uop->rf[1].w = rd;
                cur_uop->wmask  |= UPKT_WMASK_RF1;
              }
              TRACE_UOP_LD_R();
#endif /* STEP */                
              offset += 4;
            }
          }
          
          // If previous memory operations succeeded we modify registers ------
          
          if (success) {
            // jump to blink after restoring context when 'info.isReturn' is set
            if (inst->info.isReturn) {
              EXEC_UOP_ADD_S(saved<<2);
#ifdef STEP 
              if (sim_opts.cosim) {
                // pack result delta of uop instruction: J_S.D [blink]
                INIT_UOP_DELTA(0x7fe00000UL, 0UL);
                // pack result delta of uop instruction: ADD_S sp,sp,u7
                INIT_UOP_DELTA((0xc0a00000UL | (((saved<<2) & 0x7fUL) << 14)), 0U);
                cur_uop->rf[0].a = SP_REG;
                cur_uop->rf[0].w = rd;
                cur_uop->wmask  |= UPKT_WMASK_RF0;
              }
              TRACE_UOP_REG_OP();
#endif /* STEP */
              EXEC_UOP_J_SD();  // Note this is moved to the end
            } else {
              EXEC_UOP_ADD_S(saved<<2);
#ifdef STEP 
              if (sim_opts.cosim) {
                // pack result delta of uop instruction: J_S.D [blink]
                INIT_UOP_DELTA(0x7fe00000UL, 0UL);
                // pack result delta of uop instruction: ADD_S sp,sp,u7
                INIT_UOP_DELTA((0xc0a00000UL | (((saved<<2) & 0x7fUL) << 14)), 0U);
                cur_uop->rf[0].a = SP_REG;
                cur_uop->rf[0].w = rd;
                cur_uop->wmask  |= UPKT_WMASK_RF0;
              }
              TRACE_UOP_REG_OP();
#endif /* STEP */
            }
          }
          if (!success) {  // restore stack pointer in case of failure
            state.gprs[SP_REG] = old_sp;
          }
        }
        // Only jump to BLINK without saving
        //
        else if (inst->info.isReturn) {
            EXEC_UOP_J_S();
#ifdef STEP // Pack result delta of uop instruction: J_S [blink]
          if (sim_opts.cosim) { INIT_UOP_DELTA(0x7ee00000UL, 0UL); } 
#endif
        }
              
      END_ILLEGAL_IN_DSLOT
      break;
    }

    // -------------------------------------------------------------------------
    // EIA Extension Instructions
    //
    case OpCode::EIA_ZOP: {
      // Setup arguments
      // 
      ise::eia::EiaBflags bflags_in = {state.Z,  state.N,  state.C,  state.V };
      ise::eia::EiaXflags xflags_in = {state.X3, state.X2, state.X1, state.X0};

      // Call EIA instruction method evaluation
      //
      *inst->dst1 = inst->eia_inst->eval_zero_opd(bflags_in, xflags_in);
      
      break;
    }
    case OpCode::EIA_ZOP_F: {
      // Setup arguments
      //
      ise::eia::EiaBflags bflags = {state.Z,  state.N,  state.C,  state.V };
      ise::eia::EiaXflags xflags = {state.X3, state.X2, state.X1, state.X0};
      
      // Call EIA instruction method evaluation
      //
      *inst->dst1 = inst->eia_inst->eval_zero_opd(bflags, xflags, &bflags, &xflags);
      
      if (inst->flag_enable)
      { // Propagate flags
        //
        state.Z  = bflags.Z;
        state.N  = bflags.N;
        state.C  = bflags.C;
        state.V  = bflags.V;
        state.X3 = xflags.X3;
        state.X2 = xflags.X2;
        state.X1 = xflags.X1;
        state.X0 = xflags.X0;
      }
      break;
    }
    case OpCode::EIA_SOP: {
      // Setup arguments
      // 
      ise::eia::EiaBflags bflags_in = {state.Z,  state.N,  state.C,  state.V };
      ise::eia::EiaXflags xflags_in = {state.X3, state.X2, state.X1, state.X0};
      
      // Call EIA instruction method evaluation
      //
      *inst->dst1 = inst->eia_inst->eval_single_opd(*inst->src2, bflags_in, xflags_in);
      
      break;
    }
    case OpCode::EIA_SOP_F: {
      // Setup arguments
      // 
      ise::eia::EiaBflags bflags = {state.Z,  state.N,  state.C,  state.V };
      ise::eia::EiaXflags xflags = {state.X3, state.X2, state.X1, state.X0};
      
      // Call EIA instruction method evaluation
      //
      *inst->dst1 = inst->eia_inst->eval_single_opd(*inst->src2, bflags, xflags, &bflags, &xflags);
      
      if (inst->flag_enable)
      { // Propagate flags
        //
        state.Z  = bflags.Z;
        state.N  = bflags.N;
        state.C  = bflags.C;
        state.V  = bflags.V;
        state.X3 = xflags.X3;
        state.X2 = xflags.X2;
        state.X1 = xflags.X1;
        state.X0 = xflags.X0;
      }
      break;
    }
    case OpCode::EIA_DOP: {
      // Setup arguments
      // 
      ise::eia::EiaBflags bflags_in = {state.Z,  state.N,  state.C,  state.V };
      ise::eia::EiaXflags xflags_in = {state.X3, state.X2, state.X1, state.X0};
      
      // Call EIA instruction method evaluation
      //
      *inst->dst1 = inst->eia_inst->eval_dual_opd(*inst->src1, *inst->src2, bflags_in, xflags_in);
      
      break;
    }
    case OpCode::EIA_DOP_F: {
      // Setup arguments
      // 
      ise::eia::EiaBflags bflags = {state.Z,  state.N,  state.C,  state.V };
      ise::eia::EiaXflags xflags = {state.X3, state.X2, state.X1, state.X0};
      
      // Call EIA instruction method evaluation
      //
      *inst->dst1 = inst->eia_inst->eval_dual_opd(*inst->src1, *inst->src2,
                                                  bflags,  xflags,
                                                  &bflags, &xflags);
      if (inst->flag_enable)
      { // Propagate flags
        //
        state.Z  = bflags.Z;
        state.N  = bflags.N;
        state.C  = bflags.C;
        state.V  = bflags.V;
        state.X3 = xflags.X3;
        state.X2 = xflags.X2;
        state.X1 = xflags.X1;
        state.X0 = xflags.X0;
      }      
      break;
    }

    case OpCode::EXCEPTION:
    default: {

#ifdef DEBUG_EXCEPTIONS
        LOG(LOG_DEBUG)
          << "EXCEPTION: PC = '0x" << HEX(state.pc)
          << "', OPCODE = '"       << std::dec << (OpCode)inst->code
          << "', IR = '0x"         << HEX(inst->info.ir)
          << "', INSTR = '"        << arcsim::isa::arc::Opcode::to_string(static_cast<OpCode>(inst->code))
          << "'";
        
#endif /* DEBUG_EXCEPTIONS */
        if (    inst
             && inst->illegal_in_dslot
             && (state.D || state.ES) 
             && (*(inst->src2) != ECR(sys_arch.isa_opts.EV_InstructionError, IllegalInstruction, 0))
           ) {
          // This is an Illegal Instruction Sequence exception, which must
          // assume priority over all exceptions other than IllegalInstruction.
          // We therefore raise IllegelSequence at this point.
          //
          enter_exception (ECR(sys_arch.isa_opts.EV_InstructionError, IllegalSequence, 0), state.pc, state.pc);
        }
        else if (inst && inst->src2) {
          // This is an exception that has been determined by decoding the
          // instruction.
          //
          enter_exception (*(inst->src2), state.pc, state.pc);
        } else {
          // This is an exception with a ECR == 0, in which case we halt the
          // simulation.
          //
          enter_exception (0, state.pc, state.pc);
          sim_opts.halt_simulation = 1;
        }
        break;
      }
  } /* END switch (inst->code) */
  //
  // ---------------------------------------------------------------------------
  
  
  
  // ---------------------------------------------------------------------------
  // Instructions like BRK might set this variable to true, causing us to return
  // early without committing state
  //
  if (return_before_commit) {
    IF_STEP_TRACE_INSTR(this, trace_string("\n"));
    return !state.H;
  }
  
  // ---------------------------------------------------------------------------
  // Update memory models when memory model simulation is enabled
  //
  if (sim_opts.memory_sim) {
    ASSERT(mem_model && "Memory model NOT instantiated but memory simulation enabled!");

    // -------------------------------------------------------------------------
    // Instruction Fetch
    //
    // Models the fetching of one instruction from memory. Each instruction may
    // require from 1 to 3 distinct 32-bit words to be accessed, as instructions
    // can be from 2 to 8 bytes in size, and can be aligned to any 2-byte boundary.
    // The processor is assumed to have a 2-byte alignment buffer, allowing a
    // residue from the previous fetch to be used as the first 2 bytes of the current
    // fetch. A 4-byte fetch that is not aligned to a 4-byte boundary will have
    // inst->fetches == 2, as will an 8-byte fetch that is aligned to a 4-byte
    // boundary. A misaligned 8-byte fetch will have inst->fetches == 3.
    //
#ifdef CYCLE_ACC_SIM
  #define FETCH_ASSIGN(_i_,_cycles_) _i_->fet_cycles  = (_cycles_)
  #define FETCH_ADD(_i_,_cycles_)    _i_->fet_cycles += (_cycles_)
#else
  #define FETCH_ASSIGN(_i_,_cycles_) (_cycles_)
  #define FETCH_ADD(_i_,_cycles_)    (_cycles_)
#endif
    switch (inst->fetches) {
      case 1: { // 1. FETCH
        FETCH_ASSIGN(inst,   mem_model->fetch(inst->fetch_addr[0], state.pc));
        state.ibuff_addr = (state.pc) >> 2;        
        break;
      }
      case 2: { // 2. FETCHES
        if (state.ibuff_addr != (state.pc >> 2)) {
          FETCH_ASSIGN(inst, mem_model->fetch(inst->fetch_addr[0], state.pc));
          FETCH_ADD(inst,    mem_model->fetch(inst->fetch_addr[1], state.pc));
        } else {
          FETCH_ASSIGN(inst, mem_model->fetch(inst->fetch_addr[1], state.pc));
        }
        state.ibuff_addr  = inst->fetch_addr[1] >> 2;
        break;
      }
      case 3: { // 3. FETCHES
        if (state.ibuff_addr != (state.pc >> 2)) {
          FETCH_ASSIGN(inst, mem_model->fetch(inst->fetch_addr[0], state.pc));
          FETCH_ADD(inst,    mem_model->fetch(inst->fetch_addr[1], state.pc));
        } else {
          FETCH_ASSIGN(inst, mem_model->fetch(inst->fetch_addr[1], state.pc));
        }
        FETCH_ADD(inst, mem_model->fetch(inst->fetch_addr[2], state.pc));
        state.ibuff_addr  = inst->fetch_addr[2] >> 2;
        break;
      }
      default: break;
    }
    
    // -------------------------------------------------------------------------
    // Check for committed memory instructions, if so perform necessary memory
    // model operations
    //
#ifdef CYCLE_ACC_SIM
  #define MEM_ASSIGN(_i_,_cycles_) _i_->mem_cycles  = (_cycles_)
  #define MEM_ADD(_i_,_cycles_)    _i_->mem_cycles += (_cycles_)
#else
  #define MEM_ASSIGN(_i_,_cycles_) (_cycles_)
  #define MEM_ADD(_i_,_cycles_)    (_cycles_)
#endif

    if (inst->is_memory_kind_inst() && commit) {

      switch (inst->kind)
      {
        case arcsim::isa::arc::Dcode::kMemLoad: {
          ASSERT(!mem_model->addr_queue.empty() && "Memory kind instruction did not record memory access address.");
          MEM_ASSIGN(inst, mem_model->read(mem_model->addr_queue.front(),
                                                  state.pc,
                                                  inst->cache_byp));
#ifdef COSIM_SIM
          if (inst->cache_byp
              && mem_model->dcache_enabled
              && mem_model->is_dirty_dc_hit(mem_model->addr_queue.front())) {
            fprintf (stdout, "**** Error: uncached load from address that is modified in data cache: ");
            fprintf (stdout, "at 0x%08x, an uncached load from 0x%08x, is a dirty dcache hit.\n",
                     state.pc,
                     mem_model->addr_queue.front());

          }
#endif
          mem_model->addr_queue.pop();
          break;          
        }
        case arcsim::isa::arc::Dcode::kMemStore: {
          ASSERT(!mem_model->addr_queue.empty() && "Memory kind instruction did not record memory access address.");
          MEM_ASSIGN(inst, mem_model->write(mem_model->addr_queue.front(),
                                                   state.pc,
                                                   inst->cache_byp));
#ifdef COSIM_SIM
          if (inst->cache_byp
              && mem_model->dcache_enabled
              && mem_model->is_dc_hit(mem_model->addr_queue.front())) {
            fprintf (stdout, "**** Error: uncached store to address that is in the data cache: ");
            fprintf (stdout, "at 0x%08x, an uncached store to 0x%08x, is a dcache hit.\n",
                     state.pc,
                     mem_model->addr_queue.front());
            
          }
#endif
          mem_model->addr_queue.pop();
          break;
        }
        case arcsim::isa::arc::Dcode::kMemExchg: {
          ASSERT(!mem_model->addr_queue.empty() && "Memory kind instruction did not record memory access address.");
          MEM_ASSIGN(inst, mem_model->read (mem_model->addr_queue.front(),
                                                   state.pc,
                                                   inst->cache_byp));
          MEM_ADD(inst, mem_model->write(mem_model->addr_queue.front(),
                                                state.pc,
                                                inst->cache_byp));
          mem_model->addr_queue.pop();
          break;
        }
        case arcsim::isa::arc::Dcode::kMemEnterLeave: {
          MEM_ASSIGN(inst, 0);
          if (inst->code == OpCode::ENTER) {  // ENTER instruction
            while (!mem_model->addr_queue.empty()) {
              MEM_ADD(inst, mem_model->write (mem_model->addr_queue.front(),
                                                     state.pc,
                                                     false));
              mem_model->addr_queue.pop();                
            }
          } else {                        // LEAVE instruction
            while (!mem_model->addr_queue.empty()) {
              MEM_ADD(inst, mem_model->read (mem_model->addr_queue.front(),
                                                    state.pc,
                                                    false));
              mem_model->addr_queue.pop();
            }
          }
          break;
        }
        default: 
          UNIMPLEMENTED1("switch (inst->kind): Unknown memory model instruction!");
          break;
      }
    } // if (inst->is_memory_kind_inst())
    
#ifdef CYCLE_ACC_SIM
    // -------------------------------------------------------------------------
    // Processor pipeline update
    //    
    if (sim_opts.cycle_sim)
      pipeline->update_pipeline(*this);

#endif // CYCLE_ACC_SIM
    
    // If this instruction marks the end of a basic block, then invalidate the
    // fetch buffer to ensure the next fetch does not assume it can use any
    // stale prefetched information.
    //
    if (end_of_block && inst->taken_branch)
      state.ibuff_addr = 0x80000000; // bit 31 indicates invalid buffer entry

}  
  
#ifdef REGTRACK_SIM
  // ---------------------------------------------------------------------------
  // Register tracking simulation
  //
  if (sim_opts.track_regs) {
    if ( inst->dst1_stats && inst->dst1_stats->last) {
      ++inst->dst1_stats->write;
      uint64 temp = instructions() - inst->dst1_stats->last;
      if (temp) {
        inst->dst1_stats->arith += temp;
        inst->dst1_stats->geom  += log(temp);
      }
    }
    if ( inst->dst2_stats && inst->dst2_stats->last) {
      ++inst->dst2_stats->write;
      uint64 temp = instructions() - inst->dst2_stats->last;
      if (temp) {
        inst->dst2_stats->arith += temp;
        inst->dst2_stats->geom  += log(temp);
      }
    }
    if ( inst->src1_stats && inst->src1_stats->last) {
      ++inst->src1_stats->read;
      uint64 temp = instructions() - inst->src1_stats->last;
      if (temp) {
        inst->src1_stats->arith += temp;
        inst->src1_stats->geom  += log(temp);
      }
    }
    if ( inst->src2_stats && inst->src2_stats->last) {
      ++inst->src2_stats->read;
      uint64 temp = instructions() - inst->src2_stats->last;
      if (temp) {
        inst->src2_stats->arith += temp;
        inst->src2_stats->geom  += log(temp);
      }
    }
    if ( inst->dst1_stats ) inst->dst1_stats->last = instructions();
    if ( inst->dst2_stats ) inst->dst2_stats->last = instructions();
    if ( inst->src1_stats ) inst->src1_stats->last = instructions();
    if ( inst->src2_stats ) inst->src2_stats->last = instructions();
  }
#endif /* REGTRACK_SIM */

  
  // ---------------------------------------------------------------------------
  // Check to see if any pre-commit exception was raised by the current
  // instruction. We don't update state if any pre-commit exception was raised.
  //
  if (state.raise_exception) {
    IF_STEP_TRACE_INSTR(this, trace_exception());
    return !state.H;
  }

#ifdef STEP
  // Copy state-change information into the delta structure
  // This captures the architecturally-visible CPU state
  // changes in a single structure that can be used for
  // simulator verification, or for state-wise comparisons
  // between this simulator and (for example) an RTL simulation.
  //
  // Currently, the delta structure does not include memory
  // updates, aux register side effects, and Flag updates.
  //
  if (sim_opts.cosim) {
    delta->pc    = state.pc;
    delta->inst  = inst->info.ir;
    delta->limm  = inst->limm;

    if (commit && inst->info.rf_wenb0)  { delta->wmask |= UPKT_WMASK_RF0; }
    if (commit && inst->info.rf_wenb1)  { delta->wmask |= UPKT_WMASK_RF1; }
    delta->rf[0].a  = inst->info.rf_wa0;
    delta->rf[1].a  = inst->info.rf_wa1;
    if (inst->dst1) { delta->rf[0].w = *(inst->dst1); }
    if (inst->dst2) { delta->rf[1].w = *(inst->dst2); }
    delta->status32 = BUILD_STATUS32(state);
  }
#endif /* STEP */

  // SmaRT push_branch if branch is taken, or delay-slot jump to BTA
  // takes place, always providing that a post-commit exception is
  // not also going to happen after this instruction.
  //
  if (smt.enabled() && !ecause) {
    if (inst->taken_branch && !next_D) {
      LOG(LOG_DEBUG1) << "SMT enabled on taken branch without dslot";
      smt.push_branch (state.pc, state.next_pc);
    } else if (state.D || state.ES) {
      LOG(LOG_DEBUG1) << "SMT enabled on delayed slot branch";
      smt.push_branch (state.pc, state.next_pc);
    } else if (loop_back) {
      LOG(LOG_DEBUG1) << "SMT enabled on zero-overhead loop-back";
      smt.push_branch (state.auxs[AUX_LP_END], state.next_pc);
    } else if (inst->code == OpCode::RTIE) {
      LOG(LOG_DEBUG1) << "SMT enabled on RTIE control flow instruction";
      smt.push_branch(state.pc, state.next_pc);
    }
  }

  // ---------------------------------------------------------------------------
  // Profiling counters
  //
  if (sim_opts.is_killed_recording_enabled && !commit)
    cnt_ctx.killed_freq_hist.inc(state.pc);
    
  if (sim_opts.is_pc_freq_recording_enabled)
    cnt_ctx.pc_freq_hist.inc(state.pc);
  
  if (sim_opts.is_limm_freq_recording_enabled && inst->has_limm)
    cnt_ctx.limm_freq_hist.inc(state.pc);

  if (sim_opts.show_profile) {
    cnt_ctx.opcode_freq_hist.inc(inst->code);
    
    if (state.D) // count delay slot instructions
      cnt_ctx.dslot_inst_count.inc();
    
    if ((inst->q_field != 0) // count stalls due to flag def-use dependencies
        && ((cnt_ctx.interp_inst_count.get_value() - t_set_flags) < 2))
      cnt_ctx.flag_stall_count.inc();
    
    if (inst->flag_enable) // set flag-def time if this insn modifies flags
      t_set_flags = cnt_ctx.interp_inst_count.get_value();
  }

  // ---------------------------------------------------------------------------
  // COMMIT state and update PC
  //
  state.pc             = state.next_pc & state.pc_mask;   // update PC value
  state.D              = next_D;
  state.ES             = next_E;
  state.gprs[LP_COUNT] = state.next_lpc & state.lpc_mask;
  state.gprs[PCL_REG]  = state.pc & 0xfffffffc;           // keep the PCL register in step with PC
  cnt_ctx.interp_inst_count.inc();                        // increment interpreted instructions

#ifdef STEP
  // Gather together all trace-related activity after completion of current instruction.
  IF_STEP_TRACE_INSTR(this, trace_write_back(inst->info.rf_wa0, inst->info.rf_wenb0, inst->dst1,
                                    inst->info.rf_wa1, inst->info.rf_wenb1, inst->dst2));
  // If we are in a ZOL trace loop back and loop count
  if (loop_back)        { IF_STEP_TRACE_INSTR(this, trace_loop_back());   }
  if (trace_loop_count) { IF_STEP_TRACE_INSTR(this, trace_loop_count());  }
#endif /* STEP */

  // Check for post-commit exceptions, caused by a TRAP or TRAP_S
  //
  if (ecause) {
    if (sim_opts.emulate_traps && // Is this is a TRAP when traps are emulated?
            (ECR_VECTOR(ecause) == sys_arch.isa_opts.EV_Trap))
    {
      emulate_trap();
    } else {
      // NOTE: state.pc points to NEXT PC at this point in time
      enter_exception (ecause, efa, state.pc);  
      IF_STEP_TRACE_INSTR(this, trace_exception());
    }
  }

  IF_STEP_TRACE_INSTR(this, trace_commit(commit)); // trace instruction commit
  
  // ---------------------------------------------------------------------------
  // AboutToExecuteInstructionIPT and HandleBeginBasicBlockInstructionIPT check
  //
  if (ipt_mgr.is_enabled()) {
    
    if (ipt_mgr.is_enabled(arcsim::ipt::IPTManager::kIPTAboutToExecuteInstruction)
        && ipt_mgr.is_about_to_execute_instruction(state.pc))
    {
      // NOTE: state.pc points to NEXT PC at this point in time
      if (ipt_mgr.exec_about_to_execute_instruction_ipt_handlers(state.pc)) {
        set_pending_action(kPendingAction_IPT);
        return !state.H;
      }
    }
    
    if (end_of_block
        && ipt_mgr.is_enabled(arcsim::ipt::IPTManager::kIPTBeginBasicBlockInstruction))
    {
      // NOTE: state.pc points to NEXT PC at this point in time, in this particular
      //       case it denotes the PC of the next basic block entry instruction
      ipt_mgr.notify_begin_basic_block_instruction_execution_ipt_handlers(state.pc);
    }
  }
  
  // TBD - check the DEBUG.IS bit and self-halt the processor if
  // single-stepping is enabled.

  return !state.H; // return true if processor is not halted
}

