//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2006 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
//  This file implements the dynamic binary translation code generator
//  mapping ARCompact instructions onto C code.
//
// N.B.  Any translations that call simEnterException() MUST pass the
//       pc_cur value for the translated instruction that is causing the
//       exception. The value of s->pc may have been modified by that
//       stage, and cannot be relied upon to be up-to-date.
//
// N.B.  Calls from translated code into the simulator, using cpuXXX (...)
//       functions, must pass the actual PC value of the instruction
//       who's translation is making the call, if the cpuXXX (...) function
//       may possibly raise an exception when executed.
//
// N.B.  Any instruction that causes a premature exit of a translated block
//       must arrange that s->pc is updated correctly before the exit.
//       Branch and Jump instructions explicitly set s->pc, as well as
//       AUX_BTA (for delayed-branches). Other instructions, such as
//       SLEEP, BRK, or Flag with bit 0 set, must explicitly synchronise
//       s->pc with pc_cur.
//
// N.B. During JIT code generation we MUST NOT access inst.{dst|src}{1|2}_stats
//      when register tracking simulation is enabled, because these pointers are
//      not available.
//
// =====================================================================

#include <cstdlib>
#include <iomanip>
#include <cstdio>
#include <list>
#include <sstream>

#include "Assertion.h"

#include "isa/arc/DcodeConst.h"
#include "isa/arc/Opcode.h"
#include "isa/arc/Dcode.h"
#include "isa/arc/Disasm.h"

#include "ipt/IPTManager.h"

#include "system.h"
#include "exceptions.h"
#include "sys/cpu/processor.h"
#include "sim_types.h"

#include "uarch/processor/ProcessorPipelineInterface.h"

#include "profile/BlockEntry.h"

#include "translate/TranslationRuntimeApi.h"
#include "translate/TranslationEmit.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/TranslationWorker.h"

#include "util/CodeBuffer.h"
#include "util/Log.h"
#include "util/Counter.h"
#include "util/Histogram.h"
#include "util/MultiHistogram.h"

namespace arcsim {
  namespace internal {
    namespace translate {

      
      // -----------------------------------------------------------------------
      // Symbol constants
      //
      
      // Standard registers "s->gprs[REGNUM]"
#define _CR(_x_) QUOTEME(s->gprs[_x_])
#define _XR(_x_) QUOTEME(*(s->xregs[_x_]))
      static const char *R[GPR_BASE_REGS] = {
        _CR(0),  _CR(1),  _CR(2),  _CR(3),  _CR(4),  _CR(5),  _CR(6),  _CR(7),
        _CR(8),  _CR(9),  _CR(10), _CR(11), _CR(12), _CR(13), _CR(14), _CR(15),
        _CR(16), _CR(17), _CR(18), _CR(19), _CR(20), _CR(21), _CR(22), _CR(23),
        _CR(24), _CR(25), _CR(26), _CR(27), _CR(28), _CR(29), _CR(30), _CR(31),
        _XR(32), _XR(33), _XR(34), _XR(35), _XR(36), _XR(37), _XR(38), _XR(39),
        _XR(40), _XR(41), _XR(42), _XR(43), _XR(44), _XR(45), _XR(46), _XR(47),
        _XR(48), _XR(49), _XR(50), _XR(51), _XR(52), _XR(53), _XR(54), _XR(55),
        _XR(56), _XR(57), _XR(58), _XR(59), _CR(60), _CR(61), _CR(62), _CR(63)
      };
      
      // Operand availability time for each target register
#define _RC(_x_) QUOTEME(s->gprs_avail[_x_])
      static const char *RC[GPR_BASE_REGS] = {
        _RC(0),  _RC(1),  _RC(2),  _RC(3),  _RC(4),  _RC(5),  _RC(6),  _RC(7),
        _RC(8),  _RC(9),  _RC(10), _RC(11), _RC(12), _RC(13), _RC(14), _RC(15),
        _RC(16), _RC(17), _RC(18), _RC(19), _RC(20), _RC(21), _RC(22), _RC(23),
        _RC(24), _RC(25), _RC(26), _RC(27), _RC(28), _RC(29), _RC(30), _RC(31),
        _RC(32), _RC(33), _RC(34), _RC(35), _RC(36), _RC(37), _RC(38), _RC(39),
        _RC(40), _RC(41), _RC(42), _RC(43), _RC(44), _RC(45), _RC(46), _RC(47),
        _RC(48), _RC(49), _RC(50), _RC(51), _RC(52), _RC(53), _RC(54), _RC(55),
        _RC(56), _RC(57), _RC(58), _RC(59), _RC(60), _RC(61), _RC(62), _RC(63)
      };
      
      static const char *kSymZero        = "0";
      static const char *kSymT1          = "t1";
      static const char *kSymT2          = "t2";
      static const char *kSymReg1        = "reg1";
      static const char *kSymMemAddr     = "maddr";
      static const char *kSymPc          = "s->pc";
      static const char *kSymBta         = "s->auxs[" QUOTEME(AUX_BTA) "]";
      static const char *kSymCpuContext  = "s->cpu_ctx";
      static const char *kSymStateIgnore = "s->ignore";

      // -----------------------------------------------------------------------
      // MACROS
      //

#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_
      
// Emit MACRO
// 
// NOTE: The `##' token paste operator has a special meaning when placed between
// a comma and a variable argument. If no variable arguments are specified the
// comma before the `##' will be deleted
//
#define E(fmt, ...) buf.append(fmt, ##__VA_ARGS__ )

// Emit MACRO - for ASM and C level comments 
//
#define E_COMMENT(...)   { if (sim_opts.fast_enable_debug) E(__VA_ARGS__); }

// These macros are for translating conditional instructions, they are SLIGHTLY
// tricky to understand so please have a good look at them!
//
#define E_IF_CC(_inst)                                           \
  { /* OPEN SCOPE */                                             \
    if (_inst.q_field != 0) {                                    \
      is_conditional = true;                                     \
      flag_conditional(buf, _inst, is_conditional_result_stored);\
      E("\t{\n");\
    }                                                            \
    { /* OPEN SCOPE */

#define ELSE_CC(_inst)                                           \
  } /* CLOSE SCOPE */                                            \
  if (_inst.q_field != 0) {                                      \
    E("\t} else {\n");

#define E_ENDIF_CC(_inst)                                        \
    } /* CLOSE SCOPE - either from E_IF_CC or from ELSE_CC*/     \
  if (_inst.q_field != 0) { E("\t}\n"); }                        \
  } /* CLOSE SCOPE */

// emit code to set the commit variable, when tracing is enabled
#define E_COMMIT(_value_)                                         \
{ is_conditional = true; if (is_conditional_result_stored) E("\tcommit = %d;\n", _value_); }


// -----------------------------------------------------------------------------

// simple macro to ensure that PC is always written with a 16-bit aligned value
// and that PC is masked to be of size PC_SIZE bits.
#define PC_ALIGN(_loc_) ((_loc_) & work_unit.cpu->state.pc_mask)

// emit code that updates BLINK register if required
#define E_BLINK_UPDATE(_pc_) { if (inst.link) E("\t%s = 0x%08x;\n", R[BLINK], _pc_); }

      
// The E_DSLOT_UPDATE macro is instantiated at the translation
// of any jump or branch instruction that may possibly have a delay slot.
// It emits the code to set the Status32.DE bit on completion of the branch
// and might also update the PC. It also sets the is_in_dslot flag, so the
// translation of the following instruction knows to place a dynamic check
// for s->D being set to 1 (which raises IllegalInstructionSequence exception).
//
#define E_DSLOT_UPDATE(_tgt_reg_, _tgt_addr_)                                   \
  if (inst.dslot) {                                                             \
    E("\ts->D = 1;\n");                                                         \
    if (sim_opts.show_profile) {                                                \
      E_COMMENT("\t// -- DSLOT COUNTER UPDATE\n");                              \
      E("\t++(*((uint64 * const)(%#p)));\n", work_unit.cpu->cnt_ctx.dslot_inst_count.get_ptr());\
    }                                                                           \
    is_in_dslot = true;                                                         \
  }                                                                             \
  if (!inst.dslot && (_tgt_reg_ != kSymPc)) {                               \
    E("\t%s = 0x%08x;\n", kSymPc, PC_ALIGN(_tgt_addr_));                    \
  }

// The E_CHECK_ILLEGAL macro is instantiated at the translation of any instruction
// that is deemed to be illegal when the state.D bit is set to 1.
// It first checks to see if the instruction is placed in the delay slot of a
// previously-translated branch/jump instruction, or is the first instruction
// of the block (in which case we have no static information regarding the state
// of the D bit). In either of these two cases, we emit a dynamic check for
// (s->D != 0), and if true we raise an IllegalInstructionSequence exception).
// NOTE: state.pc is set in the call to 'cpuEnterException'
//
#define E_CHECK_ILLEGAL                                                         \
  if (is_in_dslot || is_first_insn) {                                           \
    E("\tif (s->D == 1) {\n");                                                  \
      E("\t\tcpuEnterException (%s, 0x%08x, 0x%08x, 0x%08x);\n",                \
        kSymCpuContext,                                                         \
        ECR(work_unit.cpu->sys_arch.isa_opts.EV_InstructionError, IllegalSequence, 0),                              \
        pc_cur,                                                                 \
        pc_cur);                                                                \
      if (sim_opts.trace_on) E("\t\tcpuTraceException (%s, 0);\n", kSymCpuContext);\
      E_TRANS_INSNS_UPDATE(block_insns-1)                                       \
      E_PIPELINE_COMMIT                                                         \
      E("\t\treturn;\n");                                                       \
    E("\t}\n");                                                                 \
  }

// ZOL end test and loopback logic 
//
#define E_ZOL_END_TEST                                                          \
  pc_updated       = true;                                                      \
  loopback_checked = true;                                                      \
  loopback_end_pc  = pc_nxt;                                                    \
  INDIRECT_CONTROL_TRANSFER_INST                                                \
  emit_zero_overhead_loop_back(buf, sim_opts, work_unit, pc_nxt, true, last_zol_inst_modifies_lp_count);

#define E_OPEN_SCOPE  E("\t{\n");
#define E_CLOSE_SCOPE E("\t}\n");


#define CHECK_FOR_PC_UPDATE if (!inst.dslot) pc_updated = true;

// DIRECT_CONTROL_TRANSFER_INST identifies those blocks with a Direct Control Transfer Inst.
//
#define DIRECT_CONTROL_TRANSFER_INST(_fall_through_, _jmp_tgt_)                 \
    is_direct_block_control_transfer_inst         = true;                              \
    direct_block_control_transfer_fall_through_addr = (_fall_through_);                  \
    direct_block_control_transfer_target_addr  = (_jmp_tgt_);

// INDIRECT_CONTROL_TRANSFER_INST identifies those blocks with an Indirect CTI
//
#define INDIRECT_CONTROL_TRANSFER_INST                                          \
    is_indirect_block_control_transfer_inst = true;


// Macro updating memory models
//
#define E_MEMORY_MODEL_ACCESS(_inst_,_addr_str_)                                \
  { if (sim_opts.memory_sim) {                                                  \
      if (sim_opts.cycle_sim) {  E("\tmc = ");  }                               \
      work_unit.cpu->mem_model->jit_emit_instr_memory_access(buf,_inst_, _addr_str_);\
    }\
  }
      
// Macros to perform cycle accurate cache simulation
//
#ifdef CYCLE_ACC_SIM

#define SRC1_EXPR ((inst.info.rf_renb0) ? RC[inst.info.rf_ra0] : kSymZero )
#define SRC2_EXPR ((inst.info.rf_renb1) ? RC[inst.info.rf_ra1] : kSymZero )

#define DST1_EXPR ((inst.info.rf_wenb0) ? RC[inst.info.rf_wa0] : kSymStateIgnore )
#define DST2_EXPR ((inst.info.rf_wenb1) ? RC[inst.info.rf_wa1] : kSymStateIgnore )

// At the end of each block we commit the full pipeline state
//
#define E_PIPELINE_COMMIT                                                       \
  if (sim_opts.cycle_sim) {                                                     \
    pipeline.jit_emit_block_end(buf, work_unit.cpu->cnt_ctx, sim_opts, work_unit.cpu->sys_arch.isa_opts);\
  }

// Emitted after each translated instruction
//
#define E_PIPELINE_UPDATE                                                 \
  if (sim_opts.cycle_sim) {                                               \
    pipeline.jit_emit_instr_pipeline_update(buf, inst,                    \
                                            SRC1_EXPR, SRC2_EXPR,         \
                                            DST1_EXPR, DST2_EXPR);        \
    pipeline_updated = true;                                              \
  }

// Emitted after cycle-approximate update, when a branch has been taken
//
#define E_TAKEN_BRANCH(_pc)                                               \
  if (sim_opts.cycle_sim) {                                               \
    pipeline.jit_emit_instr_branch_taken(buf, inst, _pc);                 \
  }

// Emitted after cycle-approximate update, when a branch has been not-taken
//
#define E_NON_TAKEN_BRANCH(_pc)                                           \
  if (sim_opts.cycle_sim) {                                               \
    pipeline.jit_emit_instr_branch_not_taken(buf,inst, _pc);              \
  }

#else

#define E_PIPELINE_UPDATE
#define E_PIPELINE_COMMIT
#define E_TAKEN_BRANCH(_pc)
#define E_NON_TAKEN_BRANCH(_pc)

#endif /* END !defined(CYCLE_ACC_SIM) */ 

// Emit macros to support ARCompact-V2 ENTER_S and LEAVE_S instructions
//
#define E_TRACE_UOP_REG_OP(_reg_)                                       \
  do {                                                                  \
    E("\tcpuTraceWriteBack(%s,%d,%d,&(%s),%d,%d,&(%s));\n",             \
      kSymCpuContext, (_reg_), 1, R[(_reg_)], 62, 0, R[62]);            \
  } while (0)

#define E_TRACE_UOP_LD_R(_reg_,_offset_)                                \
  do {                                                                  \
    E("\tcpuTraceLoad(%s, %d, maddr+%d, %s);\n",                        \
       kSymCpuContext, T_FORMAT_LW, (_offset_), R[(_reg_)]);            \
  } while (0)

#define E_TRACE_UOP_ST_R(_reg_,_offset_)                                \
  do {                                                                  \
    E("\tcpuTraceStore (%s, %d, maddr+%d, %s);\n",                      \
       kSymCpuContext, T_FORMAT_SW, (_offset_), R[(_reg_)]);            \
    E("\tcpuTraceString (%s,\" (r%d)\");\n", kSymCpuContext, _reg_);    \
  } while (0)

#define E_UOP_MOV(_dst_,_src_)                                          \
  do {                                                                  \
    E("\ts->gprs[%d] = s->gprs[%d];\n",(_dst_),(_src_));                \
    if (sim_opts.trace_on) { E_TRACE_UOP_REG_OP(_dst_); }               \
  } while (0)

#define E_UOP_ADD_SP(_u7_)                                              \
  do {                                                                  \
    E("\ts->gprs[%d] += %d;\n",                                         \
      SP_REG,(uint32)((uint32)(_u7_) & 0x7fUL));                        \
    if (sim_opts.trace_on) { E_TRACE_UOP_REG_OP(SP_REG); }              \
  } while (0)

#define E_UOP_SUB_SP(_u7_)                                              \
  do {                                                                  \
    E("\ts->gprs[%d] -= %d;\n",                                         \
      SP_REG,(uint32)((uint32)(_u7_) & 0x7fUL));                        \
    if (sim_opts.trace_on) { E_TRACE_UOP_REG_OP(SP_REG); }              \
  } while (0)

#define E_UOP_LD_R(_reg_,_offset_)                                      \
  do {                                                                  \
    E("\tif (!mem_read_word(s,0x%08x,maddr+%d,%d)) {\n",            \
       pc_cur, (_offset_), (_reg_));                                    \
    E_TRANS_INSNS_UPDATE(block_insns-1)                                 \
    E_PIPELINE_COMMIT                                                   \
    if (sim_opts.trace_on) { E("\tcpuTraceCommit(%s,0);\n", kSymCpuContext); }\
    E("\t\treturn;\n");                                                 \
    E("\t}\n");                                                         \
    if (sim_opts.trace_on) {                                            \
      E_TRACE_UOP_LD_R((_reg_),(_offset_));                             \
      E("\tcpuTraceWriteBack(%s,%d,%d,&(%s),%d,%d,&(%s));\n",           \
        kSymCpuContext, 62, 0, R[62], (_reg_), 1, R[(_reg_)]);          \
    }                                                                   \
  } while (0)

#define E_UOP_ST_R(_reg_,_offset_)                                      \
  do {                                                                  \
    E("\tif (!mem_write_word(s,0x%08x,maddr+%d,%s)) {\n",             \
       pc_cur, (_offset_), R[(_reg_)]);                                 \
    E_TRANS_INSNS_UPDATE(block_insns-1)                                 \
    E_PIPELINE_COMMIT                                                   \
    if (sim_opts.trace_on) { E("\tcpuTraceCommit(%s,0);\n", kSymCpuContext); }\
    E("\t\treturn;\n");                                                 \
    E("\t}\n");                                                         \
    if (sim_opts.trace_on) { E_TRACE_UOP_ST_R((_reg_),(_offset_)); }    \
  } while (0)

#define E_UOP_J_SD()                                                    \
    do {                                                                \
      E_PIPELINE_UPDATE                                                 \
      E("\t%s = %s = s->gprs[%d] & 0x%08x;\n",                          \
        kSymPc, kSymBta, BLINK, work_unit.cpu->state.pc_mask);          \
      pc_updated = true;                                                \
    } while(0)

#define E_UOP_J_S()                                                     \
    do {                                                                \
      E_PIPELINE_UPDATE                                                 \
      E("\t%s = s->gprs[%d] & 0x%08x;\n",                               \
         kSymPc, BLINK, work_unit.cpu->state.pc_mask);                  \
      pc_updated = true;                                                \
    } while(0)

// Logic that determines if inline assembly should be used for emitted code
//
#define IF_INLINE_ASM   if (sim_opts.fast_use_inline_asm)
#define ELSE_INLINE_ASM else
#define END_INLINE_ASM

// -----------------------------------------------------------------------------
// Histogram and counter updates
//
#define E_DKILLED_FREQ_HIST_UPDATE(_pc)                                         \
  if (sim_opts.is_dkilled_recording_enabled && !inst.dslot) {                   \
      E("\t++(*((uint32 * const)(%#p)));\n",                                    \
        work_unit.cpu->cnt_ctx.dkilled_freq_hist.get_value_ptr_at_index(_pc));  \
  }

#define E_CALL_GRAPH_MULTIHIST_UPDATE(_pc, _jmp_tgt)                            \
  if (sim_opts.is_call_graph_recording_enabled && inst.link) {                  \
    E("\t++(*((uint32 * const)(%#p)));\n",                                      \
      (work_unit.cpu->cnt_ctx.call_graph_multihist.get_hist_ptr_at_index(_pc))->get_value_ptr_at_index(_jmp_tgt));\
  }

#define E_CALL_GRAPH_MULTIHIST_UPDATE_API_CALL(_pc, _jmp_tgt)                   \
  if (sim_opts.is_call_graph_recording_enabled && inst.link) {                  \
    E("\t\tcpuHistogramInc((void*)(%#p),%s);\n",                                \
      work_unit.cpu->cnt_ctx.call_graph_multihist.get_hist_ptr_at_index(_pc),   \
      _jmp_tgt);                                                                \
  }

#define E_CALL_FREQ_HIST_UPDATE(_pc)                                            \
  if (sim_opts.is_call_freq_recording_enabled && inst.link) {                   \
      E("\t++(*((uint32 * const)(%#p)));\n",                                    \
        work_unit.cpu->cnt_ctx.call_freq_hist.get_value_ptr_at_index(_pc));     \
  }

#define E_CALL_FREQ_HIST_UPDATE_API_CALL(_pc_val)                               \
  if (sim_opts.is_call_freq_recording_enabled && inst.link) {                   \
      E("\tcpuHistogramInc((void*)(%#p),%s);\n",                                \
        &(work_unit.cpu->cnt_ctx.call_freq_hist), _pc_val);                     \
  }      
      
#define E_TRANS_INSNS_UPDATE(_count)                                            \
  { if (_count) {                                                               \
      E("\t*((uint64 * const)(%#p)) += %u;\n",                                  \
                  work_unit.cpu->cnt_ctx.native_inst_count.get_ptr() , _count); \
    }                                                                           \
  }

// =====================================================================
// Prototypes
// =====================================================================

static void emit_zero_overhead_loop_back(arcsim::util::CodeBuffer& buf,
                                         SimOptions& opts,
                                         const TranslationWorkUnit& work_unit,
                                         uint32  next_pc,
                                         bool    update_to_next_pc,
                                         bool    last_zol_updates_lp_count);
      
static void flag_conditional (arcsim::util::CodeBuffer& buf,
                              const arcsim::isa::arc::Dcode& inst,
                              bool        tracing);
static void emit_set_ZN_noasm(arcsim::util::CodeBuffer& buf,
                              const char *var, 
                              const bool set_Z=true,
                              const bool set_N=true);
static void emit_set_CV_noasm(arcsim::util::CodeBuffer& buf,
                              const char *var,
                              const bool set_C=true,
                              const bool set_V=false,
                              const char *op1=0,
                              const char *op2=0,
                              const bool is_substraction=false);
static void emit_commutative_op(SimOptions&,
                                arcsim::util::CodeBuffer& buf,
                                const arcsim::isa::arc::Dcode&,
                                const char,
                                const char *,
                                const char *,
                                const char *,
                                const char *,
                                bool,
                                bool);
static void emit_noncommutative_op(SimOptions&,
                                   arcsim::util::CodeBuffer& buf,
                                   const arcsim::isa::arc::Dcode&,
                                   const char,
                                   const char *,
                                   const char *,
                                   const char *,
                                   const char *,
                                   bool,
                                   bool);



// ----------------------------------------------------------------------------
// Methods & Functions
//

bool
TranslationWorker::translate_work_unit_to_c (const TranslationWorkUnit& work_unit)
{
  ASSERT(code_buf_ && "Code buffer not instantiated!");
  arcsim::util::CodeBuffer& buf = *code_buf_;
  bool success                  = true;     // true if successfully translated
  const bool check_inst_timer   =  work_unit.cpu->inst_timer_enabled && !sim_opts.cycle_sim;
  ProcessorPipelineInterface& pipeline = *work_unit.cpu->pipeline;

  // blocks contained in this translation work unit
  const std::list<TranslationBlockUnit*>& blocks = work_unit.blocks;
  
  // OPEN SCOPE - kCompilationModePageControlFlowGraph
  //
  if (sim_opts.fast_trans_mode == kCompilationModePageControlFlowGraph) {
    // Emit header for translation function
    TranslationEmit::block_signature(buf, blocks.front()->entry_.virt_addr);
    // Emit local variables in translation function
    E("\tuint32 t1, t2, reg1, maddr, wdata0, wdata1, pc_t, mask, commit; char shift;\n");
#ifdef CYCLE_ACC_SIM
    if (sim_opts.cycle_sim) {
      pipeline.jit_emit_block_begin(buf,work_unit.cpu->cnt_ctx, sim_opts, work_unit.cpu->sys_arch.isa_opts);      
    }
#endif /* CYCLE_ACC_SIM */

    // JUMP TABLE: @see: http://blog.llvm.org/2010/01/address-of-label-and-indirect-branches.html#more
    //
    if (blocks.size() > 1) {
      E("\tswitch (s->pc) {\n");
        for (std::list<TranslationBlockUnit*>::const_iterator I = blocks.begin(), E = blocks.end();
             I != E; ++I) {
          E("\t\tcase 0x%08x: goto BLK_0x%08x;\n", (*I)->entry_.virt_addr, (*I)->entry_.virt_addr);
        }
      E("\t}\n");
    }
  }
    
  // -------------------------------------------------------------------------
  // Iterate over all blocks
  //
  for (std::list<TranslationBlockUnit*>::const_iterator I = blocks.begin(), E = blocks.end();
       I != E && success; ++I)
  {
    const TranslationBlockUnit& block = *(*I);
    
    // OPEN SCOPE - kCompilationModeBasicBlock
    //
    if (sim_opts.fast_trans_mode == kCompilationModeBasicBlock) {
      // Emit header for translation function
      TranslationEmit::block_signature(buf,block.entry_.virt_addr);
      // Emit local variables in translation function
      E("\tuint32 t1, t2, reg1, maddr, wdata0, wdata1, pc_t, mask, commit; char shift;\n");
#ifdef CYCLE_ACC_SIM
      if (sim_opts.cycle_sim) {
        pipeline.jit_emit_block_begin(buf, work_unit.cpu->cnt_ctx, sim_opts, work_unit.cpu->sys_arch.isa_opts);
      }
#endif /* CYCLE_ACC_SIM */
    }

    uint32 pc_cur = kInvalidPcAddress;   // cur PC value
    uint32 pc_nxt = kInvalidPcAddress;   // next PC value
    
    char   limm_buf[16];
    
    bool   pc_updated        = false;  // true if block updates PC at exit
    bool   inst_has_dslot    = false;  // true if waiting to process dslot
    bool   loopback_checked  = false;  // true if loop-back check code has been emitted
    uint32 loopback_end_pc   = 0x0;    // if loopback_checked is true, this will be set to
                                       // last the lp_end address we checked

    bool   is_direct_block_control_transfer_inst  = false;   // true if direct cti
    bool   is_indirect_block_control_transfer_inst = false;   // true if indirect cti
    uint32 direct_block_control_transfer_fall_through_addr;    // dcti not taken addr
    uint32 direct_block_control_transfer_target_addr;     // dcti taken addr
    
    bool   is_conditional;                            // true if commit is conditional
    bool   is_conditional_result_stored;              // true if commit result should be
                                                      // stored in generated code
    bool   is_in_dslot   = false;                     // true if previous insn had dslot
    bool   is_first_insn = true;                      // true if this is the first in block
    
    const char *src1, *src2, *reg1;                   // current operand strings
    const char *target_reg;
    
    bool  last_zol_inst_modifies_lp_count   = false;  // true if last ZOL inst
                                                      // modifies LP_COUNT

    int    block_insns = 0;                           // number of insns in block
    
    // Emit block start label
    //
    E_COMMENT("\t// -- BEGIN BLOCK BLK_0x%08x\n", block.entry_.virt_addr);
    E("BLK_0x%08x:\n", block.entry_.virt_addr);
    
    // Assign PC to be start of block
    //
    pc_cur = block.entry_.virt_addr;
    
    // -------------------------------------------------------------------------
    // Iterate over all instructions in this block
    //
    for (std::list<TranslationInstructionUnit*>::const_iterator TI = block.begin(), TE = block.end();
         TI != TE; ++TI) 
    {
      const TranslationInstructionUnit* iunit = *TI;
      const arcsim::isa::arc::Dcode&    inst  = iunit->inst;
      bool  pipeline_updated                  = false;       // true if instruction updated pipeline
      bool  check_mem_aux_watchpoints         = false;       // true if watchpoint check required

      is_conditional                          = false;       // by default an instruction is not conditional
      
      // By the default, the evaluation of the conditional for conditional
      // instructions is not stored, unless instruction tracing is turned on.
      //
      is_conditional_result_stored = (sim_opts.trace_on || sim_opts.is_killed_recording_enabled);
      
      // increment the translator's PC value by inst.size
      pc_nxt = PC_ALIGN((pc_cur + inst.size));
              
      // calculate block insns
      ++block_insns;
      
      if (sim_opts.trace_on) {
        E("\tcpuTraceInstruction (%s, 0x%08x, 0x%08x, 0x%08x);\n",
          kSymCpuContext, pc_cur, inst.info.ir, inst.limm);
      }
      
      if (sim_opts.fast_enable_debug) {
        // dis-assemble instruction and print as comment
        arcsim::isa::arc::Disasm disasm(work_unit.cpu->sys_arch.isa_opts,
                                        work_unit.cpu->eia_mgr,
                                        inst.info.ir,
                                        inst.limm);
        E_COMMENT("\t// -- [%08x] %s\n", pc_cur, disasm.buf);
      }    

      // ---------------------------------------------------------------------
      // Instrumentation PoinT hooks
      //
      
      if (work_unit.cpu->ipt_mgr.is_enabled()) {
        // AboutToExecuteInstructionIPT
        //
        if (work_unit.cpu->ipt_mgr.is_enabled(arcsim::ipt::IPTManager::kIPTAboutToExecuteInstruction)
            && work_unit.cpu->ipt_mgr.is_about_to_execute_instruction(pc_cur))
        {
          E_COMMENT("\t// -- AboutToExecuteInstructionIPT\n");
          E("\t%s = 0x%08x;\n", kSymPc, pc_cur); // commit PC before calling runtime API
          E("\tif (cpuIptAboutToExecuteInstr(%s) || (s->pending_actions != kPendingAction_NONE)) {\n", kSymCpuContext);
            if (is_first_insn) {
              // When IPT triggers for first basic block instr we need to destroy
              // this translation in order to avoid hitting the IPT again upon continuation.
              //
              E("\t\tcpuRegisterTranslationForRemoval(%s,0x%08x);\n", kSymCpuContext, pc_cur);
            }
            E_TRANS_INSNS_UPDATE(block_insns-1)
            E_PIPELINE_COMMIT
            E("\t\treturn;\n");
          E("\t}\n");
        }
        // BeginInstructionExecutionIPT
        //
        if (work_unit.cpu->ipt_mgr.is_enabled(arcsim::ipt::IPTManager::kIPTBeginInstruction)) {
          E_COMMENT("\t// -- BeginInstructionExecutionIPT\n");
          E("\tcpuIptNotifyBeginInstrExecution(%s,0x%08x,%d);\n", kSymCpuContext, pc_cur, inst.size);
        }
      }
      
      // ---------------------------------------------------------------------
      // Maintain various performance counters
      //
      
      // Record instruction kind frequencies
      //
      if (sim_opts.show_profile) {
        E_COMMENT("\t// -- OPCODE FREQUENCY HIST UPDATE\n");
        E("\t++(*((uint32 * const)(%#p)));\n",
          work_unit.cpu->cnt_ctx.opcode_freq_hist.get_value_ptr_at_index(inst.code));
      }

      // Record PC frequencies
      //
      if (sim_opts.is_pc_freq_recording_enabled) {
        E_COMMENT("\t// -- PC FREQUENCY HIST UPDATE\n");
        E("\t++(*((uint32 * const)(%#p)));\n",
          work_unit.cpu->cnt_ctx.pc_freq_hist.get_value_ptr_at_index(pc_cur));
      }
      
      // Record LIMM frequencies
      //
      if (sim_opts.is_limm_freq_recording_enabled && inst.has_limm) {
        E_COMMENT("\t// -- LIMM FREQUENCY HIST UPDATE\n");
        E("\t++(*((uint32 * const)(%#p)));\n",
          work_unit.cpu->cnt_ctx.limm_freq_hist.get_value_ptr_at_index(pc_cur));
      }
      
      // ---------------------------------------------------------------------
      // If register tracking simulation is enabled we need to account for reg usage
      //
#ifdef REGTRACK_SIM
      if (sim_opts.track_regs) {
        if (inst.info.rf_wenb0) { E("\tcpuRegisterWrite(%s,%d);\n", kSymCpuContext, inst.info.rf_wa0); }
        if (inst.info.rf_wenb1) { E("\tcpuRegisterWrite(%s,%d);\n", kSymCpuContext, inst.info.rf_wa1); }
        if (inst.info.rf_renb0) { E("\tcpuRegisterRead(%s,%d);\n",  kSymCpuContext, inst.info.rf_ra0); }
        if (inst.info.rf_renb1) { E("\tcpuRegisterRead(%s,%d);\n",  kSymCpuContext, inst.info.rf_ra1); }
      }
#endif
      
      // ---------------------------------------------------------------------
      // 'Decode' operands - after decoding 'op1' and 'op2' will refer to the
      //  right operand.
      //

      // If the current instruction has a delay slot, then it is a branch or
      // jump, and it's target register must be BTA rather than PC. The
      // 'target_reg' variable is defined here as one of those two register
      // names, depending on the dslot value.
      //
      target_reg = (inst.dslot) ? kSymBta : kSymPc;

      // 1. Select first operand
      //
      if (inst.info.rf_renb0 && (inst.info.rf_ra0 == PCL_REG)) {
        E("\tt1 = (uint32)0x%08x;\n", (pc_cur & 0xfffffffc));
        src1 = kSymT1;
      } else {
        bool limm_r0 = inst.src1 == &(inst.limm);
        if (limm_r0) {
          snprintf (limm_buf, sizeof(limm_buf), "0x%08x", inst.limm);
          src1 = limm_buf;
        } else {
          src1 = inst.info.rf_renb0 ? R[inst.info.rf_ra0] : kSymZero;
        }
      }
      
      // 2. Select second operand
      //
      if (inst.info.rf_renb1 && (inst.info.rf_ra1 == PCL_REG)) {
        E("\treg1 = (uint32)0x%08x;\n", (pc_cur & 0xfffffffc));
        reg1 = kSymReg1;
      } else {
        bool limm_r1 = inst.src2 == &(inst.limm);
        if (limm_r1) {
          snprintf (limm_buf, sizeof(limm_buf), "0x%08x", inst.limm);
          reg1 = limm_buf;    
        } else {
          reg1 = inst.info.rf_renb1 ? R[inst.info.rf_ra1] : kSymZero;
        }
      }
      
      // Load/Store instructions can have a 'shimm'.
      //
      if (inst.src2 == &(inst.shimm)) {
        // For LOADS we can detect if a shimm is set by checking for:
        //  - 'inst.src2 == &(inst.shimm)'
        //
        E("\tt2 = (uint32)%d;\n", inst.shimm);
        src2 = kSymT2;
      } else if (inst.src2 == &(inst.jmp_target)) {
        // For STORES we can detect if a shimm is set by checking for:
        //  - 'inst.src2 == &(inst.jmp_target)' the jmp_target field is overloaded
        //    in this case and used to store the 'shimm'
        //
        E("\tt2 = (uint32)%d;\n", inst.jmp_target);
        src2 = kSymT2;          
      } else {
        src2 = reg1;
      }

      // ---------------------------------------------------------------------
      // Check if this is the last instruction and the destination is LP_COUNT
      // and it is the last instruction of a zero overhead loop
      //
      if ((iunit == block.get_last_instruction())                     // last instruction?
          &&
          (   (inst.info.rf_wenb0 && (inst.info.rf_wa0 == LP_COUNT))  // dst is LP_COUNT?
            ||(inst.info.rf_wenb1 && (inst.info.rf_wa1 == LP_COUNT))
           )
          &&
          (work_unit.cpu->is_end_of_zero_overhead_loop(work_unit.lp_end_to_lp_start_map,
                                                       pc_nxt))       // end of ZOL?
         )
      {    
        // Because writing LP_COUNT from the last instruction in the loop will
        // take effect in the next loop iteration, we will store the current LP_COUNT
        // in 's->next_lpc' so the last instruction can safely modify s->gprs[LP_COUNT]
        // and we will take care of this special case on loop back.
        //
        E("\ts->next_lpc = s->gprs[%d]; commit = 1;\n", LP_COUNT);
        
        // Store the fact that this instruction is the last one in a ZOL and
        // modifies LP_COUNT.
        //
        last_zol_inst_modifies_lp_count = true;
        
        // Because last instruction of a ZOL might be conditional we need it
        // to store the outcome of the condition to know which value to update
        // upon loop-back for this special case.
        //
        is_conditional_result_stored = true;
      }
      
      if (sim_opts.memory_sim){
        
        // Variable to store fetch latency when cycle accurate simulation is enabled
        //
        if (sim_opts.cycle_sim) { E("\tfc = "); }

        // Emit code that fetches instruction
        work_unit.cpu->mem_model->jit_emit_instr_memory_fetch(buf,inst,pc_cur);
        
#ifdef CYCLE_ACC_SIM
        if (sim_opts.cycle_sim) {
          // Emit code at the beginning of an instruction
          pipeline.jit_emit_instr_begin(buf, inst, pc_cur, work_unit.cpu->cnt_ctx, sim_opts);      
        }
#endif /* CYCLE_ACC_SIM */
        
      }
      
      // ---------------------------------------------------------------------
      // Translate each instruction within a block
      //
      using namespace arcsim::isa::arc;
      switch (inst.code)
      {    
        case OpCode::BCC: {
          uint32 next_linear_pc = PC_ALIGN((pc_cur + inst.link_offset));
          E_CHECK_ILLEGAL
          E_PIPELINE_UPDATE
          E_IF_CC (inst) {
            E_TAKEN_BRANCH(pc_cur)
            E("\t%s = 0x%08x;\n", target_reg, inst.jmp_target);
            E_DSLOT_UPDATE(target_reg, inst.jmp_target)
            E_BLINK_UPDATE(next_linear_pc)
            E_CALL_FREQ_HIST_UPDATE(inst.jmp_target)
            E_CALL_GRAPH_MULTIHIST_UPDATE(pc_cur, inst.jmp_target)
            E_DKILLED_FREQ_HIST_UPDATE(next_linear_pc)
          } ELSE_CC (inst) {
            E_NON_TAKEN_BRANCH(pc_cur)
            if (inst.dslot) { E("\t%s = 0x%08x;\n", kSymPc, next_linear_pc);    }
            else            { E("\t%s = 0x%08x;\n", target_reg, next_linear_pc); }
            // If direct control transfer did not occur check for ZOL loop-back
            //
            if (!inst.dslot
                && work_unit.cpu->is_end_of_zero_overhead_loop(work_unit.lp_end_to_lp_start_map,
                                                               pc_nxt)) {          
              E_ZOL_END_TEST
            }     
          } E_ENDIF_CC (inst)
          CHECK_FOR_PC_UPDATE
          DIRECT_CONTROL_TRANSFER_INST(next_linear_pc, inst.jmp_target)
          break;
        }
          
        case OpCode::BR: {
          uint32 next_linear_pc = PC_ALIGN((pc_cur + inst.link_offset));
          E_CHECK_ILLEGAL
          E_PIPELINE_UPDATE
          E_TAKEN_BRANCH(pc_cur)
          E("\t%s = 0x%08x;\n", target_reg, inst.jmp_target);
          E_DSLOT_UPDATE(target_reg, inst.jmp_target)
          E_BLINK_UPDATE(next_linear_pc)
          E_CALL_FREQ_HIST_UPDATE(inst.jmp_target)
          E_CALL_GRAPH_MULTIHIST_UPDATE(pc_cur, inst.jmp_target)
          E_DKILLED_FREQ_HIST_UPDATE(next_linear_pc)
          CHECK_FOR_PC_UPDATE
          DIRECT_CONTROL_TRANSFER_INST(kInvalidPcAddress, inst.jmp_target)
          break;
        }
          
        case OpCode::BRCC: {
          uint32 next_linear_pc = PC_ALIGN((pc_cur + inst.link_offset));
          const char* cmp;
          switch (inst.q_field) {
            case BREQ_COND: cmp = "\tif (%s == %s) {\n"; break;
            case BRNE_COND: cmp = "\tif (%s != %s) {\n"; break;
            case BRLT_COND: cmp = "\tif ((signed)%s <  (signed)%s) {\n"; break;
            case BRGE_COND: cmp = "\tif ((signed)%s >= (signed)%s) {\n"; break;
            case BRLO_COND: cmp = "\tif ((unsigned)%s <  (unsigned)%s) {\n"; break;
            case BRHS_COND: cmp = "\tif ((unsigned)%s >= (unsigned)%s) {\n"; break;
            default:
              LOG(LOG_ERROR) << "[JIT] Unexpected BRCC condition '" << (uint32)inst.q_field << "'.";
              success = false;
              cmp = "/* ERROR: JIT OpCode::BRCC */ {\n";
              break ;
          }
          E_CHECK_ILLEGAL
          E_PIPELINE_UPDATE
          E(cmp, src1, src2); { // BRANCH TAKEN
            E_COMMIT(1)
            E_TAKEN_BRANCH(pc_cur)
            E("\t\t%s = 0x%08x;\n", target_reg, inst.jmp_target);
            E_DSLOT_UPDATE(target_reg, inst.jmp_target)
            E_DKILLED_FREQ_HIST_UPDATE(next_linear_pc);
          } E("\t} else {\n"); { // BRANCH NOT TAKEN
            E_COMMIT(0)
            E_NON_TAKEN_BRANCH(pc_cur)
            E("\t\t%s = 0x%08x;\n", kSymPc, next_linear_pc);
            // If direct control transfer did not occur check for ZOL loop-back
            //
            if (!inst.dslot
                && work_unit.cpu->is_end_of_zero_overhead_loop(work_unit.lp_end_to_lp_start_map,
                                                               pc_nxt)) {          
              E_ZOL_END_TEST
            }
            E("\t}\n");
          }
          CHECK_FOR_PC_UPDATE
          DIRECT_CONTROL_TRANSFER_INST(next_linear_pc, inst.jmp_target)
          break;
        }
          
        case OpCode::BBIT0: {
          uint32 next_linear_pc = PC_ALIGN((pc_cur + inst.link_offset));
          E_CHECK_ILLEGAL
          E_PIPELINE_UPDATE
          E("\tif ((%s & (1<<(%s & 31))) == 0) {\n", src1, src2); { // BRANCH TAKEN
            E_COMMIT(1)
            E_TAKEN_BRANCH(pc_cur)
            E("\t\t%s = 0x%08x;\n", target_reg, inst.jmp_target);          
            E_DSLOT_UPDATE(target_reg, inst.jmp_target)
            E_DKILLED_FREQ_HIST_UPDATE(next_linear_pc);
          } E("\t} else {\n"); { // BRANCH NOT TAKEN
            E_COMMIT(0)
            E_NON_TAKEN_BRANCH(pc_cur)
            E("\t\t%s = 0x%08x;\n", kSymPc, next_linear_pc);
            // If direct control transfer did not occur check for ZOL loop-back
            //
            if (!inst.dslot
                && work_unit.cpu->is_end_of_zero_overhead_loop(work_unit.lp_end_to_lp_start_map,
                                                               pc_nxt)) {          
              E_ZOL_END_TEST
            }
            E("\t}\n");
          }
          CHECK_FOR_PC_UPDATE
          DIRECT_CONTROL_TRANSFER_INST(next_linear_pc, inst.jmp_target)
          break;
        }
          
        case OpCode::BBIT1: {
          uint32 next_linear_pc = PC_ALIGN((pc_cur + inst.link_offset));
          E_CHECK_ILLEGAL
          E("\tif ((%s & (1<<(%s & 31))) != 0) {\n", src1, src2); { // BRANCH TAKEN
            E_COMMIT(1)
            E_TAKEN_BRANCH(pc_cur)
            E("\t\t%s = 0x%08x;\n", target_reg, inst.jmp_target);
            E_DSLOT_UPDATE(target_reg, inst.jmp_target)
            E_DKILLED_FREQ_HIST_UPDATE(next_linear_pc);
          } E("\t} else {\n"); { // BRANCH NOT TAKEN
            E_COMMIT(0)
            E_NON_TAKEN_BRANCH(pc_cur)
            E("\t\t%s = 0x%08x;\n", kSymPc, next_linear_pc);
            // If direct control transfer did not occur check for ZOL loop-back
            //
            if (!inst.dslot
                && work_unit.cpu->is_end_of_zero_overhead_loop(work_unit.lp_end_to_lp_start_map,
                                                               pc_nxt)) {          
              E_ZOL_END_TEST
            }
            E("\t}\n");
          }
          CHECK_FOR_PC_UPDATE
          DIRECT_CONTROL_TRANSFER_INST(next_linear_pc, inst.jmp_target)
          break;
        }
          
        case OpCode::JCC_SRC1: {
          uint32 next_linear_pc = PC_ALIGN((pc_cur + inst.link_offset));
          E_CHECK_ILLEGAL
          const char* jcc_str = inst.info.rf_renb0 ? R[inst.info.rf_ra0] : kSymZero;
          uint32 pc_mask = work_unit.cpu->state.pc_mask;
          
          E_PIPELINE_UPDATE
          E_IF_CC (inst) {
            E_TAKEN_BRANCH(pc_cur)
            E("\t%s = %s & 0x%08x;\n", target_reg, jcc_str, pc_mask);
            if (inst.dslot) {
              E("\ts->D = 1;\n");
              if (sim_opts.show_profile) {
                E_COMMENT("\t// -- DSLOT COUNTER UPDATE\n")
                E("\t++(*((uint64 * const)(%#p)));\n", work_unit.cpu->cnt_ctx.dslot_inst_count.get_ptr());
              }
            } else {
              if (target_reg != kSymPc)
                E("\t%s = %s & 0x%08x;\n", kSymPc, jcc_str, pc_mask);
              pc_updated = true;
            }        
            E_BLINK_UPDATE(next_linear_pc)
            E_CALL_FREQ_HIST_UPDATE_API_CALL(kSymPc)
            E_CALL_GRAPH_MULTIHIST_UPDATE_API_CALL(pc_cur, kSymPc)
            E_DKILLED_FREQ_HIST_UPDATE(next_linear_pc)
          } ELSE_CC (inst) {
            E_NON_TAKEN_BRANCH(pc_cur)
            E("\t%s = 0x%08x;\n", kSymPc, next_linear_pc);
            pc_updated = true;
          } E_ENDIF_CC (inst)
          CHECK_FOR_PC_UPDATE
          INDIRECT_CONTROL_TRANSFER_INST
          break;
        }
          
        case OpCode::JCC_SRC2: {
          uint32 next_linear_pc = PC_ALIGN((pc_cur + inst.link_offset));
          uint32 pc_mask        = work_unit.cpu->state.pc_mask;
          E_CHECK_ILLEGAL
          E_PIPELINE_UPDATE
          E_IF_CC (inst) {
            E_TAKEN_BRANCH(pc_cur)
            E("\t%s = %s & 0x%08x;\n", target_reg, src2, pc_mask);
            if (inst.dslot) {
              E("\ts->D = 1;\n");
              if (sim_opts.show_profile) {
                E_COMMENT("\t// -- DSLOT COUNTER UPDATE\n")
                E("\t++(*((uint64 * const)(%#p)));\n", work_unit.cpu->cnt_ctx.dslot_inst_count.get_ptr());
              }
            } else {
              if (target_reg != kSymPc)
                E("\t%s = %s & 0x%08x;\n", kSymPc, src2, pc_mask);
              pc_updated = true;
            }
            E_BLINK_UPDATE(next_linear_pc)
            E_CALL_FREQ_HIST_UPDATE_API_CALL(kSymPc)
            E_CALL_GRAPH_MULTIHIST_UPDATE_API_CALL(pc_cur, kSymPc)
            E_DKILLED_FREQ_HIST_UPDATE(next_linear_pc)
          } ELSE_CC (inst) {
            E_NON_TAKEN_BRANCH(pc_cur)
            E("\t%s = 0x%08x;\n", kSymPc, next_linear_pc);
            pc_updated = true;              
          } E_ENDIF_CC (inst)
          CHECK_FOR_PC_UPDATE
          INDIRECT_CONTROL_TRANSFER_INST
          break;
        }
         
        case OpCode::J_F_ILINK1: {
          E_CHECK_ILLEGAL
          E_PIPELINE_UPDATE
          E_IF_CC (inst) {
            pc_updated = true;
            // exit from interrupt
            E("\tcpuExitInterrupt(%s,1);\n", kSymCpuContext);
            // set PC to NEXT_PC after a call to cpuExitInterrupt()
            E("\t%s = s->next_pc & s->pc_mask;\n", kSymPc);
          } E_ENDIF_CC(inst)
          INDIRECT_CONTROL_TRANSFER_INST
          break;
        }
        
        case OpCode::J_F_ILINK2: {
          E_CHECK_ILLEGAL
          E_PIPELINE_UPDATE
          E_IF_CC (inst) {
            pc_updated = true;
            // exit from interrupt
            E("\tcpuExitInterrupt(%s,2);\n", kSymCpuContext);
            // set PC to NEXT_PC after a call to cpuExitInterrupt()
            E("\t%s = s->next_pc & s->pc_mask;\n", kSymPc);
          } E_ENDIF_CC(inst)
          INDIRECT_CONTROL_TRANSFER_INST
          break;          
        }
                            
        case OpCode::JLI_S: {
          uint32 next_linear_pc = PC_ALIGN((pc_cur + inst.link_offset));
          uint32 pc_mask        = work_unit.cpu->state.pc_mask;
          E_CHECK_ILLEGAL
          E_PIPELINE_UPDATE
          E_TAKEN_BRANCH(pc_cur)
          E("\t%s = (s->auxs[0x%03x] + %s) & 0x%08x;\n", kSymPc, AUX_JLI_BASE, src2, pc_mask);
          E("\t%s = 0x%08x;\n", R[BLINK], next_linear_pc);
          E_CALL_FREQ_HIST_UPDATE_API_CALL(kSymPc)
          // FIXME: How shall we best perform call graph recording in this case
          //
          // E_CALL_GRAPH_MULTIHIST_UPDATE_API_CALL(pc_cur, kSymPc)
          pc_updated = true;
          CHECK_FOR_PC_UPDATE
          INDIRECT_CONTROL_TRANSFER_INST
          break;
        }
         
        case OpCode::BI: {
          uint32 pc_mask = work_unit.cpu->state.pc_mask;
          E_CHECK_ILLEGAL
          E_PIPELINE_UPDATE
          E_TAKEN_BRANCH(pc_cur)
          E("\t%s = (0x%08x + ((%s)<<2)) & 0x%08x;\n", kSymPc, pc_nxt, src2, pc_mask);      
          CHECK_FOR_PC_UPDATE
          INDIRECT_CONTROL_TRANSFER_INST
          break;
        }

        case OpCode::BIH: {
          uint32 pc_mask = work_unit.cpu->state.pc_mask;
          E_CHECK_ILLEGAL
          E_PIPELINE_UPDATE
          E_TAKEN_BRANCH(pc_cur)
          E("\t%s = (0x%08x + ((%s)<<1)) & 0x%08x;\n", kSymPc, pc_nxt, src2, pc_mask);      
          CHECK_FOR_PC_UPDATE
          INDIRECT_CONTROL_TRANSFER_INST
          break;
        }

        case OpCode::LPCC: {
          E_CHECK_ILLEGAL
          pc_updated = true;
          E("\tcpuRegisterLpEndJmpTarget(%s,0x%08x,0x%08x);\n", kSymCpuContext, inst.jmp_target, pc_nxt); // Register LP_END
          E_PIPELINE_UPDATE
          E_IF_CC (inst)
            E("\tcpuWriteAuxReg(%s, 0x%03x, 0x%08x);\n", kSymCpuContext, AUX_LP_START, pc_nxt); // LP_START
            E("\tcpuWriteAuxReg(%s, 0x%03x, 0x%08x);\n", kSymCpuContext, AUX_LP_END, inst.jmp_target); // LP_END
            E("\t%s = 0x%08x;\n", kSymPc, pc_nxt);
            E("\ts->L = 0;\n");
            if (sim_opts.trace_on) E("\tcpuTraceLpInst(%s, 0, 0x%08x);\n", kSymCpuContext, inst.jmp_target);   
          ELSE_CC (inst) {
            E_TAKEN_BRANCH(pc_cur)
            if (sim_opts.trace_on) E("\tcpuTraceLpInst(%s, 1, 0x%08x);\n", kSymCpuContext, inst.jmp_target);            
            E("\t%s = 0x%08x;\n", kSymPc, inst.jmp_target);
          } E_ENDIF_CC (inst)
          DIRECT_CONTROL_TRANSFER_INST(pc_nxt, inst.jmp_target)
          break;
        }
        
        // -------------------------------------------------------------------
        // MEMORY LOAD - code generation
        //
        case OpCode::LD_HALF_S:
        case OpCode::LD_BYTE_S:
        case OpCode::LD_HALF_U:
        case OpCode::LD_BYTE_U:
        case OpCode::LW:
        case OpCode::LW_PRE:
        case OpCode::LW_SH2:
        case OpCode::LW_PRE_SH2:
        case OpCode::LW_A:
        case OpCode::LW_PRE_A:
        case OpCode::LW_SH2_A:
        case OpCode::LW_PRE_SH2_A:
        case OpCode::LD_WORD: 
        { // determine 'load' implementation and trace format
          const char                      *fun_sym;
          InstructionTraceLoadStoreFormat trace_fmt, trace_fmt_excptn;
          if (inst.code == OpCode::LD_HALF_S) {
            fun_sym       = "mem_read_half_signed";
            trace_fmt     = T_FORMAT_LH; trace_fmt_excptn = T_FORMAT_LHX;
          } else if (inst.code == OpCode::LD_BYTE_S) {
            fun_sym       = "mem_read_byte_signed";
            trace_fmt     = T_FORMAT_LB; trace_fmt_excptn = T_FORMAT_LBX;
          } else if (inst.code == OpCode::LD_HALF_U) {
            fun_sym       = "mem_read_half_unsigned";
            trace_fmt     = T_FORMAT_LH; trace_fmt_excptn = T_FORMAT_LHX;
          } else if (inst.code == OpCode::LD_BYTE_U) {
            fun_sym         = "mem_read_byte_unsigned";
            trace_fmt     = T_FORMAT_LB; trace_fmt_excptn = T_FORMAT_LBX;
          } else {
            fun_sym       = "mem_read_word";
            trace_fmt     = T_FORMAT_LW; trace_fmt_excptn = T_FORMAT_LWX;
          }
          
          // store PC previously computed by delayed branch
          if (inst_has_dslot)  { E("\tpc_t = %s;\n", kSymPc); }            
          if (inst.addr_shift) { E("\t%s = %s + (%s << %d);\n", kSymMemAddr, src1, src2, inst.addr_shift); }
          else                 { E("\t%s = %s + %s;\n", kSymMemAddr, src1, src2); }
          // memory address symbol to use
          const char * addr = (inst.pre_addr) ? src1 : kSymMemAddr;
                      
          E_MEMORY_MODEL_ACCESS(inst, addr)
          E_PIPELINE_UPDATE
          
          E("\tif (!%s(s,0x%08x,%s,%d)) {\n", fun_sym, pc_cur, addr, inst.info.rf_wa1);
            if (sim_opts.trace_on) {
              E("\t\tcpuTraceLoad(%s,%d,%s,0);\n", kSymCpuContext, trace_fmt_excptn, addr);
              E("\t\tcpuTraceException(%s,0);\n", kSymCpuContext);
            }
            E_TRANS_INSNS_UPDATE(block_insns-1)
            E_PIPELINE_COMMIT
            E("\t\treturn;\n");
          E("\t}\n");

          if (sim_opts.trace_on) { // emit trace before the optional address update
            E("\tcpuTraceLoad (%s,%d,%s,%s);\n", kSymCpuContext, trace_fmt, addr, R[inst.info.rf_wa1]);
          }

          if (inst.info.rf_wenb0) { // optional address update
            E("\t%s = %s;\n", R[inst.info.rf_wa0], kSymMemAddr);
          }
          // If there are any Watchpoints on load operations, then a check must be
          // made after each load instruction to see if any Watchpoint may have been
          // triggered. The code for this examines the state.pending_actions flag
          // and returns from the translated block if it is != kPendingAction_NONE.
          //
          if (work_unit.cpu->aps.has_ld_aps())
            check_mem_aux_watchpoints = true;
          
          // Re-store PC previously computed by delayed branch, after successful memory call
          if (inst_has_dslot) {
            E("\t%s = pc_t;\n", kSymPc);
          }
          break;
        }
          
        // -------------------------------------------------------------------
        // MEMORY STORE - code generation
        //
        case OpCode::ST_WORD:
        case OpCode::ST_HALF:
        case OpCode::ST_BYTE:
        { // determine 'store' implementation and trace format
          const char                      *fun_sym;
          InstructionTraceLoadStoreFormat trace_fmt, trace_fmt_excptn;
          if (inst.code == OpCode::ST_HALF) {
            fun_sym   = "mem_write_half";
            trace_fmt = T_FORMAT_SH; trace_fmt_excptn = T_FORMAT_SHX;
          } else if (inst.code == OpCode::ST_BYTE) {
            fun_sym   = "mem_write_byte";
            trace_fmt = T_FORMAT_SB; trace_fmt_excptn = T_FORMAT_SBX;
          } else { // (inst.code == OpCode::ST_WORD)
            fun_sym   = "mem_write_word";
            trace_fmt = T_FORMAT_SW; trace_fmt_excptn = T_FORMAT_SWX;
          }
          
          // store PC previously computed by delayed branch
          if (inst_has_dslot) {
            E("\tpc_t = %s;\n", kSymPc);
          }
          const uint32 offset = (inst.shimm) << inst.addr_shift;
          E("\t%s = %s + %d;\n", kSymMemAddr, src1, offset);
          // memory address symbol to use
          const char * addr = (inst.pre_addr) ? src1 : kSymMemAddr;
                      
          E_MEMORY_MODEL_ACCESS(inst, addr)
          E_PIPELINE_UPDATE
          
          // perform memory store
          E("\tif (!%s(s,0x%08x,%s,%s)) {\n", fun_sym, pc_cur, addr, src2);
            if (sim_opts.trace_on) {
              E("\t\tcpuTraceStore(%s,%d,%s,%s);\n", kSymCpuContext, trace_fmt_excptn, addr, src2);
              E("\t\tcpuTraceException(%s,0);\n", kSymCpuContext);
            }
            E_TRANS_INSNS_UPDATE(block_insns-1)
            E_PIPELINE_COMMIT
            E("\t\treturn;\n");
          E("\t}\n");

          if (sim_opts.trace_on) { // emit trace before the optional address update
            E("\tcpuTraceStore(%s,%d,%s,%s);\n", kSymCpuContext, trace_fmt, addr, src2);
          }
          if (inst.info.rf_wenb0) { // optional address update
            E("\t%s = %s;\n", R[inst.info.rf_wa0], kSymMemAddr);
          }
          // If there are any Watchpoints on store operations, then a check must be
          // made after each store instruction to see if any Watchpoint may have been
          // triggered. The code for this examines the state.pending_actions flag
          // and returns from the translated block if it is != kPendingAction_NONE.
          //
          if (work_unit.cpu->aps.has_st_aps())
            check_mem_aux_watchpoints = true;
          
          // Re-store PC previously computed by delayed branch, after successful memory call
          if (inst_has_dslot) {
            E("\t%s = pc_t;\n", kSymPc);
          }
          break;
        }
          
        case OpCode::LR: {
          // Each LR could raise an exception. This is handled by the call
          // to read_aux_register, but we must return from the translated
          // block immediately on detecting a null return code from that
          // function - indicating an exception has been raised.
          // However, the most common aux register to be read is STATUS32,
          // so we handle that one inline without calling cpuReadAuxReg
          // provided there are no LR-based Watchpoints enabled currently.
          //
          if (    (inst.src2 == &(inst.shimm)) && (inst.shimm == AUX_STATUS32)
               && !work_unit.cpu->aps.has_lr_aps() )
          { // FAST PATH - read of STATUS32 register
            //
            E("\t%s = (s->Z  << 11) | (s->N  << 10) | (s->C  <<  9) | (s->V  <<  8)", 
                 R[inst.info.rf_wa0]);
            
            if (block.entry_.mode == KERNEL_MODE)
            {
              if (work_unit.cpu->sys_arch.isa_opts.is_isa_a6k()) {
                E(" | (s->DZ << 13)");
              }
              E(" | (s->L  << 12) | (s->U  <<  7) | (s->D  <<  6) | (s->AE <<  5) \
                  | (s->A2 <<  4) | (s->A1 <<  3) | (s->E2 <<  2) | (s->E1 <<  1) | s->H");
            }
            
            E(";\n");
            E_PIPELINE_UPDATE
            
            if (sim_opts.trace_on) E("\tcpuTraceLR (%s, 0x%08x, %s, 1);\n", kSymCpuContext, AUX_STATUS32, R[inst.info.rf_wa0]);
          } else {
            // SLOW PATH - call cpuReadAuxReg()
            //
            E_PIPELINE_UPDATE
            E("\tif (!cpuReadAuxReg(%s, %s, &(%s))) {\n", kSymCpuContext, src2, R[inst.info.rf_wa0]);
              // LR failed, hence we must return
              //
              E("\t\t%s = 0x%08x;\n", kSymPc, pc_cur);
              E_TRANS_INSNS_UPDATE(block_insns-1)
              E_PIPELINE_COMMIT
              if (sim_opts.trace_on) {
                E("\t\tcpuTraceLR(%s,%s,%s,0);\n", kSymCpuContext, src2, R[inst.info.rf_wa0]);
                E("\tcpuTraceCommit(%s,0);\n", kSymCpuContext);
              }
              E("\t\treturn;\n");
            E("\t}\n");
            if (sim_opts.trace_on) E("\tcpuTraceLR (%s, %s, %s, 1);\n", kSymCpuContext, src2, R[inst.info.rf_wa0]);
          }
                      
          // If there are any Watchpoints on LR operations, then a check must be
          // made after each LR instruction to see if any Watchpoint may have been
          // triggered. The code for this examines the state.pending_actions flag
          // and returns from the translated block if it is != kPendingAction_NONE.
          //
          if (work_unit.cpu->aps.has_lr_aps())
            check_mem_aux_watchpoints = true;
          
          break;
        }
          
        case OpCode::SR:
        {
          E_PIPELINE_UPDATE
          E("\tif (!cpuWriteAuxReg(%s,%s,%s)) {\n", kSymCpuContext, src2, src1);
            E("\t\t%s = 0x%08x;\n", kSymPc, pc_cur);
            E_TRANS_INSNS_UPDATE(block_insns-1)
            E_PIPELINE_COMMIT
            if (sim_opts.trace_on) {
              E("\t\tcpuTraceSR (%s, %s, %s, 0);\n", kSymCpuContext, src2, src1);
              E("\tcpuTraceCommit(%s,0);\n", kSymCpuContext);
            }
            E("\t\treturn;\n");
          E("\t}\n");
          if (sim_opts.trace_on) E("\tcpuTraceSR (%s, %s, %s, 1);\n", kSymCpuContext, src2, src1);
                      
          // If there are any Watchpoints on SR operations, then a check must be
          // made after each SR instruction to see if any Watchpoint may have been
          // triggered. The code for this examines the state.pending_actions flag
          // and returns from the translated block if it is != kPendingAction_NONE.
          //
          if (work_unit.cpu->aps.has_sr_aps())
            check_mem_aux_watchpoints = true;
          
          break;
        }
          
        case OpCode::TST:
        { 
          E_IF_CC (inst)
          E("\twdata0 = %s & %s;\n", src1, src2);
          E("\ts->Z = (wdata0 == 0);\n");
          E("\ts->N = ((sint32)wdata0 < 0);\n");
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::BTST:
        {
          E_IF_CC (inst)
          E("\twdata0 = %s & (1 << (%s & 0x1f));\n", src1, src2);
          E("\ts->Z = (wdata0 == 0);\n");
          E("\ts->N = ((sint32)wdata0 < 0);\n");
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::CMP:
        {
          E_IF_CC (inst)
          IF_INLINE_ASM {
            E("\t__asm__ __volatile__ (\"cmpl %%1, %%0\" : : \"r\"(%s), \"r\"(%s) : \"cc\");\n", src1, src2);
            E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
            E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
            E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
            E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
          } ELSE_INLINE_ASM {
            E("\t{\n");
            E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src1, src2);
            emit_set_ZN_noasm(buf, "data64");
            emit_set_CV_noasm(buf, "data64", true, true, src1, src2, true);
            E("\t}\n");
          } END_INLINE_ASM;            
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::RCMP:
        {
          E_IF_CC (inst)     
          IF_INLINE_ASM {
            E("\t__asm__ __volatile__ (\"cmpl %%1, %%0\" : : \"r\"(%s), \"r\"(%s) : \"cc\");\n", src2, src1);
            E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
            E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
            E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
            E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
          } ELSE_INLINE_ASM {
            E("\t{\n");
            E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src2, src1);
            emit_set_ZN_noasm(buf, "data64");
            emit_set_CV_noasm(buf, "data64", true, true, src2, src1, true);
            E("\t}\n");
          } END_INLINE_ASM;
          E_ENDIF_CC (inst)
          break;
        }
          
          // Versions of frequent functions (mov, add, sub, and, or)
          // which execute unconditionally and do not set flags. 
          // These functions dominate execution time, so are coded ultra-efficiently.
          //
        case OpCode::MOV: E("\t%s = %s;\n",      R[inst.info.rf_wa0], src2);      break;
        case OpCode::ADD: E("\t%s = %s + %s;\n", R[inst.info.rf_wa0], src1, src2); break;
        case OpCode::SUB: E("\t%s = %s - %s;\n", R[inst.info.rf_wa0], src1, src2); break;
        case OpCode::AND: E("\t%s = %s & %s;\n", R[inst.info.rf_wa0], src1, src2); break;
        case OpCode::OR:  E("\t%s = %s | %s;\n", R[inst.info.rf_wa0], src1, src2); break;
          
          // Versions of mov, add, sub, and, or that either set flags, 
          // or execute conditionally, or both.
          //
        case OpCode::MOV_F:
        {
          E_IF_CC (inst)
          E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", src2);
            E("\ts->N = ((sint32)(%s) < 0);\n", src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::ADD_F:
        {
          E_IF_CC (inst)
          emit_commutative_op (sim_opts, buf, inst, '+', "addl",
                               R[inst.info.rf_wa0], src1, src2, false, 
                               inst.flag_enable);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::SUB_F:
        {
          E_IF_CC (inst)
          emit_noncommutative_op (sim_opts, buf, inst, '-', "subl",
                                  R[inst.info.rf_wa0], src1, src2, false, 
                                  inst.flag_enable);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::AND_F:
        {
          E_IF_CC (inst)
          emit_commutative_op (sim_opts, buf, inst, '&', "andl",
                               R[inst.info.rf_wa0], src1, src2, false, 
                               inst.flag_enable);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::OR_F:
        {
          E_IF_CC (inst)
          emit_commutative_op (sim_opts, buf, inst, '|', "orl",
                               R[inst.info.rf_wa0], src1, src2, false, 
                               inst.flag_enable);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::BCLR:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {                  
                E("\tuint32 temp2 = ~(1 << (%s & 0x1f));\n", src2);
                E("\t%s = %s;\n", R[inst.info.rf_wa0], src1);
                E("\t__asm__ __volatile__ (\"andl %%1, %%0\" : \"=r\"(%s) : \"r\"(temp2), \"0\"(%s) : \"cc\");\n",
                  R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
                E("\t%s = %s & ~(1 << (%s & 0x1f));\n", R[inst.info.rf_wa0], src1, src2);
                emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s & ~(1 << (%s & 0x1f));\n", R[inst.info.rf_wa0], src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::BSET:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tuint32 temp2 = (1 << (%s & 0x1f));\n", src2);
                E("\t%s = %s;\n", R[inst.info.rf_wa0], src1);
                E("\t__asm__ __volatile__ (\"orl %%1, %%0\" : \"=r\"(%s) : \"r\"(temp2), \"0\"(%s) : \"cc\");\n",
                  R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
                E("\t%s = %s | (1 << (%s & 0x1f));\n", R[inst.info.rf_wa0], src1, src2);
                emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s | (1 << (%s & 0x1f));\n", R[inst.info.rf_wa0], src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::RSUB:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tuint32 temp2 = %s;\n", src2);
                E("\t__asm__ __volatile__ (\"subl %%1, %%0\" : \"=r\"(temp2) : \"r\"(%s), \"0\"(temp2) : \"cc\");\n", src1);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
                E("\t%s = temp2;\n", R[inst.info.rf_wa0]);
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
              E("\t{\n");
              E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src2, src1);
              emit_set_ZN_noasm(buf, "data64");
              emit_set_CV_noasm(buf, "data64", true, true, src2, src1, true);
              E("\t%s = (uint32)data64;\n", R[inst.info.rf_wa0]);
              E("\t}\n");                
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s - %s;\n", R[inst.info.rf_wa0], src2, src1);
          }
          E_ENDIF_CC (inst)
          break;  
        }
          
        case OpCode::ADC:
        {
          E_IF_CC (inst)
          emit_commutative_op (sim_opts, buf, inst, '+', "adcl",
                               R[inst.info.rf_wa0], src1, src2, true, true);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::SBC:
        {
          E_IF_CC (inst)
          emit_noncommutative_op (sim_opts, buf, inst, '-', "sbbl",
                                  R[inst.info.rf_wa0], src1, src2, true, true);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::XOR:
        {
          E_IF_CC (inst)
          emit_commutative_op (sim_opts, buf, inst, '^', "xorl",
                               R[inst.info.rf_wa0], src1, src2, false, 
                               inst.flag_enable);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::BIC:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tuint32 temp2 = ~(%s);\n", src2);
                E("\t%s = %s;\n", R[inst.info.rf_wa0], src1);
                E("\t__asm__ __volatile__ (\"andl %%1, %%0\" : \"=r\"(%s) : \"r\"(temp2), \"0\"(%s) : \"cc\");\n",
                   R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
              E("\t%s = %s & ~(%s);\n", R[inst.info.rf_wa0], src1, src2);
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s & ~(%s);\n", R[inst.info.rf_wa0], src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::MAX:
        { // Modified - Dec 2007
          // success = false; break;
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tsint32 temp1;\n");
                E("\ttemp1 = (sint32)%s;\n", src1);
                E("\t__asm__ __volatile__ (\"subl %%1, %%0\" : \"=r\"(temp1) : \"r\"((sint32)%s), \"0\"(temp1) : \"cc\");\n", src2);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
                E("\tif( (sint32)%s >= ((sint32)%s) )\n", src2, src1);
                E("\t{\n");
                E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
                E("\ts->C=1;\n");
                E("\t}\nelse{\n");
                E("\t%s = %s;\n", R[inst.info.rf_wa0], src1);
                E("\ts->C=0;\n");
                E("\t}\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
              // Set Z, N, and V flags
              E("\t\t{\n");
              E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src1, src2);
              emit_set_ZN_noasm(buf, "data64");
              emit_set_CV_noasm(buf, "data64", false, true, src1, src2, true);
              E("\t\t}\n");
              // Set dest and carry bit
              E("\tif( (sint32)%s >= ((sint32)%s) )\n", src2, src1);
              E("\t{\n");
              E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
              E("\ts->C=1;\n");
              E("\t}\nelse{\n");
              E("\t%s = %s;\n", R[inst.info.rf_wa0], src1);
              E("\ts->C=0;\n");
              E("\t}\n");
            } END_INLINE_ASM;
          } else {
            E("\t%s = ((sint32)%s > (sint32)%s) ? %s : %s;\n",
               R[inst.info.rf_wa0], src1, src2, src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::MIN:
        { // Modified - Dec 2007
          // success = false; break;
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tsint32 temp1;\n");
                E("\ttemp1 = (sint32)%s;\n", src1);
                E("\t__asm__ __volatile__ (\"subl %%1, %%0\" : \"=r\"(temp1) : \"r\"(%s), \"0\"(temp1) : \"cc\");\n", src2);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
                E("\tif( (sint32)%s > ((sint32)%s) )\n", src2, src1);
                E("\t{\n");
                E("\t%s = %s;\n", R[inst.info.rf_wa0], src1);
                E("\ts->C=0;\n");
                E("\t}\nelse{\n");
                E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
                E("\ts->C=1;\n");
                E("\t}\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
              // Set Z, N, and V flags
              E("\t\t{\n");
              E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src1, src2);
              emit_set_ZN_noasm(buf, "data64");
              emit_set_CV_noasm(buf, "data64", false, true, src1, src2, true);
              E("\t\t}\n");
              // Set dest and carry flag
              E("\tif( (sint32)%s > ((sint32)%s) )\n", src2, src1);
              E("\t{\n");
              E("\t%s = %s;\n", R[inst.info.rf_wa0], src1);
              E("\ts->C=0;\n");
              E("\t}\nelse{\n");
              E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
              E("\ts->C=1;\n");
              E("\t}\n");
            } END_INLINE_ASM;
          } else {
            E("\t%s = ((sint32)%s < (sint32)%s) ? %s : %s;\n",
               R[inst.info.rf_wa0], src1, src2, src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::BXOR:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tuint32 temp2 = 1 << (%s & 0x1f);\n",src2);
                E("\t%s = %s;\n", R[inst.info.rf_wa0], src1);
                E("\t__asm__ __volatile__ (\"xorl %%1, %%0\" : \"=r\"(%s) : \"r\"(temp2), \"0\"(%s) : \"cc\");\n",
                  R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
              E("\t%s = %s ^ (1 << (%s & 0x1f));\n", R[inst.info.rf_wa0], src1, src2);
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s ^ (1 << (%s & 0x1f));\n", R[inst.info.rf_wa0], src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::BMSK:
        {
          E_IF_CC (inst)
          E("\tmask = ((uint32)0xffffffff) >> (31 - (%s & 0x1f));\n", src2);
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E("\t%s = %s;\n", R[inst.info.rf_wa0], src1);
              E("\t__asm__ __volatile__ (\"andl %%1, %%0\" : \"=r\"(%s) : \"r\"(mask), \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
              E("__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
            } ELSE_INLINE_ASM {
              E("\t%s = mask & %s;\n", R[inst.info.rf_wa0], src1);
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            } END_INLINE_ASM;
          } else {
            E("\t%s = mask & %s;\n", R[inst.info.rf_wa0], src1);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::BMSKN:
        {
          E_IF_CC (inst)
          E("\tmask = ~(((uint32)0xffffffff) >> (31 - (%s & 0x1f)));\n", src2);
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E("\t%s = %s;\n", R[inst.info.rf_wa0], src1);
              E("\t__asm__ __volatile__ (\"andl %%1, %%0\" : \"=r\"(%s) : \"r\"(mask), \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
              E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
            } ELSE_INLINE_ASM {
              E("\t%s = mask & %s;\n", R[inst.info.rf_wa0], src1);
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            } END_INLINE_ASM;
          } else {
            E("\t%s = mask & %s;\n", R[inst.info.rf_wa0], src1);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::ADD1:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tuint32 temp2 = (%s) << 1;\n", src2);
                E("\t%s = (%s);\n", R[inst.info.rf_wa0], src1);
                E("\t__asm__ __volatile__ (\"addl %%1, %%0\" : \"=r\"(%s) : \"r\"(temp2), \"0\"(%s) : \"cc\");\n",
                     R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\t\tconst uint32 temp2 = (%s) << 1;\n", src2);
                E("\t\tconst uint64 data64 = (uint64)(%s) + temp2;\n", src1);
                emit_set_ZN_noasm(buf, "data64");
                emit_set_CV_noasm(buf, "data64", true, true, src1, "temp2");
                E("\t\t%s = (uint32)data64;\n", R[inst.info.rf_wa0]);
              } E_CLOSE_SCOPE
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s + ((%s) << 1);\n", R[inst.info.rf_wa0], src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::ADD2:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tuint32 temp2 = (%s) << 2;\n", src2);
                E("\t%s = (%s);\n", R[inst.info.rf_wa0], src1);
                E("\t__asm__ __volatile__ (\"addl %%1, %%0\" : \"=r\"(%s) : \"r\"(temp2), \"0\"(%s) : \"cc\");\n",
                  R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\t\tconst uint32 temp2 = (%s) << 2;\n", src2);
                E("\t\tconst uint64 data64 = (uint64)(%s) + temp2;\n", src1);
                emit_set_ZN_noasm(buf, "data64");
                emit_set_CV_noasm(buf, "data64", true, true, src1, "temp2");
                E("\t\t%s = (uint32)data64;\n", R[inst.info.rf_wa0]);
              } E_CLOSE_SCOPE                
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s + ((%s) << 2);\n", R[inst.info.rf_wa0], src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::ADD3:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tuint32 temp2 = (%s) << 3;\n", src2);
                E("\t%s = (%s);\n", R[inst.info.rf_wa0], src1);
                E("\t__asm__ __volatile__ (\"addl %%1, %%0\" : \"=r\"(%s) : \"r\"(temp2), \"0\"(%s) : \"cc\");\n",
                  R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\t\tconst uint32 temp2 = (%s) << 3;\n", src2);
                E("\t\tconst uint64 data64 = (uint64)(%s) + temp2;\n", src1);
                emit_set_ZN_noasm(buf, "data64");
                emit_set_CV_noasm(buf, "data64", true, true, src1, "temp2");
                E("\t\t%s = (uint32)data64;\n", R[inst.info.rf_wa0]);
              } E_CLOSE_SCOPE                
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s + ((%s) << 3);\n", R[inst.info.rf_wa0], src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::SUB1:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tuint32 temp2 = (%s) << 1;\n", src2);
                E("\t%s = (%s);\n", R[inst.info.rf_wa0], src1);
                E("\t__asm__ __volatile__ (\"subl %%1, %%0\" : \"=r\"(%s) : \"r\"(temp2), \"0\"(%s) : \"cc\");\n",
                  R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\t\tconst uint32 temp2 = (%s) << 1;\n", src2);
                E("\t\tconst uint64 data64 = (uint64)(%s) - temp2;\n", src1);
                emit_set_ZN_noasm(buf, "data64");
                emit_set_CV_noasm(buf, "data64", true, true, src1, "temp2", true);
                E("\t\t%s = (uint32)data64;\n", R[inst.info.rf_wa0]);
              } E_CLOSE_SCOPE
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s - ((%s) << 1);\n", R[inst.info.rf_wa0], src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::SUB2:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tuint32 temp2 = (%s) << 2;\n", src2);
                E("\t%s = (%s);\n", R[inst.info.rf_wa0], src1);
                E("\t__asm__ __volatile__ (\"subl %%1, %%0\" : \"=r\"(%s) : \"r\"(temp2), \"0\"(%s) : \"cc\");\n",
                  R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\t\tconst uint32 temp2 = (%s) << 2;\n", src2);
                E("\t\tconst uint64 data64 = (uint64)(%s) - temp2;\n", src1);
                emit_set_ZN_noasm(buf, "data64");
                emit_set_CV_noasm(buf, "data64", true, true, src1, "temp2", true);
                E("\t\t%s = (uint32)data64;\n", R[inst.info.rf_wa0]);
              } E_CLOSE_SCOPE
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s - ((%s) << 2);\n", R[inst.info.rf_wa0], src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::SUB3:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\tuint32 temp2 = (%s) << 3;\n", src2);
                E("\t%s = (%s);\n", R[inst.info.rf_wa0], src1);
                E("\t__asm__ __volatile__ (\"subl %%1, %%0\" : \"=r\"(%s) : \"r\"(temp2), \"0\"(%s) : \"cc\");\n",
                  R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } E_CLOSE_SCOPE
            } ELSE_INLINE_ASM {
              E_OPEN_SCOPE {
                E("\t\tconst uint32 temp2 = (%s) << 3;\n", src2);
                E("\t\tconst uint64 data64 = (uint64)(%s) - temp2;\n", src1);
                emit_set_ZN_noasm(buf, "data64");
                emit_set_CV_noasm(buf, "data64", true, true, src1, "temp2", true);
                E("\t\t%s = (uint32)data64;\n", R[inst.info.rf_wa0]);
              } E_CLOSE_SCOPE                
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s - ((%s) << 3);\n", R[inst.info.rf_wa0], src1, src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::MPY:
        {
          E_OPEN_SCOPE {
            E_IF_CC (inst)
            if (inst.flag_enable) {
              E("\tsint64 x = (sint64)((sint32)%s), y = (sint64)((sint32)%s);\n", src1, src2);
              E("\tx *= y;\n");
              E("\t%s = (uint32)x;\n", R[inst.info.rf_wa0]);
              //              
              E("\tuint32 bits_32_to_62 = (uint32)(x>>31);\n");
              E("\tuint32 bit_63 = (uint32)(x>>63) & 0x1;\n");
              E("\ts->V = ( (bits_32_to_62 != 0 && bit_63==0) || (bits_32_to_62 != 0xFFFFFFFF && bit_63==1) );\n");
              E("\ts->N = ((sint64)x < 0);\n");
              E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            }
            else {
              E("\tsint32 x = ((sint32)%s), y = ((sint32)%s);\n", src1, src2);
              E("\tx *= y;\n");
              E("\t%s = (uint32)x;\n", R[inst.info.rf_wa0]);
            }
            E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }
          
        case OpCode::MPYH:
        {
          E_OPEN_SCOPE {
            E_IF_CC (inst)
            E("\tsint64 x = (sint64)((sint32)%s), y = (sint64)((sint32)%s);\n", src1, src2);
            E("\tx *= y;\n");
            E("\t%s = (sint32)(x >> 32);\n", R[inst.info.rf_wa0]);
            if (inst.flag_enable) {
              E("\ts->V = 0;\n");
              E("\ts->N = (((sint32)%s) < 0);\n", R[inst.info.rf_wa0]);
              E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            }
            E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }
          
        case OpCode::MPYHU:
        {
          E_OPEN_SCOPE {
            E_IF_CC (inst)
            E("\tuint64 x = (uint64)(%s), y = (uint64)(%s);\n", src1, src2);
            E("\tx *= y;\n");
            E("\t%s = (uint32)(x >> 32);\n", R[inst.info.rf_wa0]);
            
            if (inst.flag_enable) {
              E("\ts->V = 0;\n");
              E("\ts->N = 0;\n");
              E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            }
            E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }
          
        case OpCode::MPYU:
        {
          E_OPEN_SCOPE {
            E_IF_CC (inst)
            if (inst.flag_enable) {
              E("\tuint64 x = (uint64)(%s), y = (uint64)(%s);\n", src1, src2);
              E("\tx *= y;\n");
              E("\t%s = (uint32)x;\n", R[inst.info.rf_wa0]);
              //
              E("\ts->V = (%s != x);\n", R[inst.info.rf_wa0]);
              E("\ts->N = 0;\n");
              E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            } else {
              E("\t%s = (%s) * (%s);\n", R[inst.info.rf_wa0], src1, src2);
            }
            E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }
          
        case OpCode::MUL64:
        {
          E_OPEN_SCOPE {
            E_IF_CC (inst)
              E("\tsint64 x = ((sint64)((sint32)(%s))) * ((sint64)((sint32)(%s)));\n", src1, src2);
              E("\ts->gprs[%d] = (uint32)x;\n",       MLO_REG);
              E("\ts->gprs[%d] = (uint32)(x>>16);\n", MMID_REG);
              E("\ts->gprs[%d] = (uint32)(x>>32);\n", MHI_REG);
              if (sim_opts.trace_on) E("\tcpuTraceMul64Inst(%s);\n", kSymCpuContext);
            E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }
          
        case OpCode::MULU64:
        {
          E_OPEN_SCOPE {
            E_IF_CC (inst)
              E("\tuint64 x = ((uint64)(%s)) * ((uint64)(%s));\n", src1, src2);
              E("\ts->gprs[%d] = (uint32)x;\n",       MLO_REG);
              E("\ts->gprs[%d] = (uint32)(x>>16);\n", MMID_REG);
              E("\ts->gprs[%d] = (uint32)(x>>32);\n", MHI_REG);
              if (sim_opts.trace_on) E("\tcpuTraceMul64Inst(%s);\n", kSymCpuContext);
            E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE        
          break;
        }
          
        case OpCode::ASL:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
              E("\t__asm__ __volatile__ (\"sal $1, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
              E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
              E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
            } ELSE_INLINE_ASM {
              E("\t\t{\n");
              E("\t\tconst uint64 data64 = (uint64)%s + %s;\n", src2, src2);
              emit_set_ZN_noasm(buf, "data64");
              emit_set_CV_noasm(buf, "data64", true, true, src2, src2);
              E("\t\t%s = (uint32)data64;\n", R[inst.info.rf_wa0]);
              E("\t\t}\n");
            } END_INLINE_ASM;
          } else {
            E("\t%s = %s << 1;\n", R[inst.info.rf_wa0], src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::ASR:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
              E("\t__asm__ __volatile__ (\"sar $1, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
              E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
              E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
            } ELSE_INLINE_ASM {
              E("\ts->C = %s & 1;\n", src2); // C_flag = src[0]
              E("\t%s = (sint32)(%s) >> 1;\n", R[inst.info.rf_wa0], src2);
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            } END_INLINE_ASM;
          } else {
            E("\t%s = (sint32)(%s) >> 1;\n", R[inst.info.rf_wa0], src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::LSR:
        {
          E_IF_CC (inst)
          if (inst.flag_enable) {
            IF_INLINE_ASM {
              E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
              E("\t__asm__ __volatile__ (\"shr $1, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
              E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
              E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
            } ELSE_INLINE_ASM {
              E("\ts->C = %s & 1;\n", src2); // C_flag = src[0]                
              E("\t%s = (uint32)(%s) >> 1;\n", R[inst.info.rf_wa0], src2);
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            } END_INLINE_ASM;
          } else {
            E("\t%s = (uint32)(%s) >> 1;\n", R[inst.info.rf_wa0], src2);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::ROR:
        {
          E_IF_CC (inst)
          IF_INLINE_ASM {
            E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
            if (inst.flag_enable) {
              E("\t__asm__ __volatile__ (\"ror $1, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
              E("\ts->Z = ((sint32)%s==0);\n", R[inst.info.rf_wa0]);
              E("\ts->N = ((sint32)%s < 0);\n", R[inst.info.rf_wa0]);
            } else
              E("\t__asm__ __volatile__ (\"ror $1, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                   R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
          } ELSE_INLINE_ASM {
            if (inst.flag_enable) {
              E("\ts->C = %s & 1;\n", src2); // C_flag = src[0]
            }
            E("\t%s = ((uint32)%s>>1) | ((uint32)%s<<31);\n", R[inst.info.rf_wa0], src2, src2);
            if (inst.flag_enable) {
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            }
          } END_INLINE_ASM;
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::ROL:
        {
          E_IF_CC (inst)
          IF_INLINE_ASM {
            E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
            if (inst.flag_enable) {
              E("\t__asm__ __volatile__ (\"rol $1, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                 R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
              E("\ts->Z = ((sint32)%s==0);\n", R[inst.info.rf_wa0]);
              E("\ts->N = ((sint32)%s < 0);\n", R[inst.info.rf_wa0]);
            } else
              E("\t__asm__ __volatile__ (\"rol $1, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
          } ELSE_INLINE_ASM {
            if (inst.flag_enable) {
              E("\ts->C = %s & 31;\n", src2); // C_flag = src[31]
            }
            E("\t%s = ((uint32)%s>>31) | ((uint32)%s<<1);\n", R[inst.info.rf_wa0], src2, src2);
            if (inst.flag_enable) {
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            }
          } END_INLINE_ASM;
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::RRC:
        {
          E_IF_CC (inst)
          IF_INLINE_ASM {
            E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
            if (inst.flag_enable) {
              E("\t__asm__ __volatile__ (\"bt $0, %%0\" : : \"m\"(s->C) : \"cc\");\n");
              E("\t__asm__ __volatile__ (\"rcr $1, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
              E("\ts->Z = ((sint32)%s==0);\n",R[inst.info.rf_wa0]);
              E("\ts->N = ((sint32)%s<0);\n", R[inst.info.rf_wa0]);
            } else {
              E("\t__asm__ __volatile__ (\"bt $0, %%0\" : : \"m\"(s->C) : \"cc\");\n");
              E("\t__asm__ __volatile__ (\"rcr $1, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);   
            }
          } ELSE_INLINE_ASM {
            if (inst.flag_enable) {
              // determine new carry (need to do this here because op2 could be same as destination!)
              E("\tt1 = (uint32)%s & 1;\n", src2); // C_flag = src[0]
            }
            
            E("\t%s = ((uint32)%s >> 1) | ((uint32)s->C << 31);\n", R[inst.info.rf_wa0], src2);
            
            if (inst.flag_enable) {
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
              E("\ts->C = t1;\n");
            }
          } END_INLINE_ASM;
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::RLC:
        {
          E_IF_CC (inst)
          IF_INLINE_ASM {
            E("\t%s = %s;\n", R[inst.info.rf_wa0], src2);
            if (inst.flag_enable) {
              E("\t__asm__ __volatile__ (\"bt $0, %%0\" : : \"m\"(s->C) : \"cc\");\n");
              E("\t__asm__ __volatile__ (\"rcl $1, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
              E("\ts->Z = ((sint32)%s==0);\n",R[inst.info.rf_wa0]);
              E("\ts->N = ((sint32)%s<0);\n", R[inst.info.rf_wa0]);
            } else {
              E("\t__asm__ __volatile__ (\"bt $0, %%0\" : : \"m\"(s->C) : \"cc\");\n");
              E("\t__asm__ __volatile__ (\"rcl $1, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);   
            }
          } ELSE_INLINE_ASM {
            if (inst.flag_enable) {
              // determine new carry (need to do this here because op2 could be same as destination!)
              E("\tt1 = ((uint32)%s >> 31) & 1;\n", src2); // C_flag = src[31]
            }              
            
            E("\t%s = ((uint32)%s << 1) | (uint32)s->C;\n", R[inst.info.rf_wa0], src2);
            
            if (inst.flag_enable) {
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
              E("\ts->C = t1;\n");
            }
          } END_INLINE_ASM;
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::SEXBYTE:
        {
          E("\t%s = (char)(%s);\n", R[inst.info.rf_wa0], src2);
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);
          }
          break;
        }
          
        case OpCode::SEXWORD:
        {
          E("\t%s = (short)(%s);\n", R[inst.info.rf_wa0], src2);
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);
          }
          break;
        }
          
        case OpCode::EXTBYTE:
        {
          E("\t%s = (unsigned char)(%s);\n", R[inst.info.rf_wa0], src2);
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = 0;\n");
          }
          break;
        }
          
        case OpCode::EXTWORD:
        {
          E("\t%s = (unsigned short)(%s);\n", R[inst.info.rf_wa0], src2);
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = 0;\n");
          }
          break;
        }
          
        case OpCode::ABS:
        {
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", src2);
            E("\ts->V = s->N = (%s == 0x80000000);\n", src2);
            E("\ts->C = (%s & 0x80000000) != 0;\n", src2);
          }
          E("\t%s = ((sint32)(%s) < 0) ? -(%s) : %s;\n", 
               R[inst.info.rf_wa0], src2, src2, src2);
          break;
        }
          
        case OpCode::NOT:
        {
          E("\t%s = ~(%s);\n", R[inst.info.rf_wa0], src2);
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);
          }
          break;
        }
          
        case OpCode::EX:
        {
          E("\t%s = 0x%08x;\n", kSymPc, pc_cur); 
          E("\t%s = %s;\n", kSymMemAddr, src2);
          
          E_MEMORY_MODEL_ACCESS(inst, kSymMemAddr)
          E_PIPELINE_UPDATE
          
          if (sim_opts.trace_on) { // preserve 'R[inst.info.rf_wa0]' for correct tracing
            E("\t%s = %s;\n", kSymT1, R[inst.info.rf_wa0]);
          }
          
          E("\tif (!cpuAtomicExchange(%s,%s,&(%s))) {\n", kSymCpuContext, kSymMemAddr, R[inst.info.rf_wa0]);
            if (sim_opts.trace_on) {
              E("\t\tcpuTraceLoad(%s,%d,%s,0);\n", kSymCpuContext, T_FORMAT_LWX, kSymMemAddr);
              E("\t\tcpuTraceStore(%s,%d,%s,%s);\n", kSymCpuContext, T_FORMAT_SWX, kSymMemAddr, R[inst.info.rf_wa0]);
              E("\t\tcpuTraceException(%s,0);\n", kSymCpuContext);
            }
            E_TRANS_INSNS_UPDATE(block_insns-1)
            E_PIPELINE_COMMIT
            E("\t\treturn;\n");
          E("\t}\n");
          
          if (sim_opts.trace_on) {
            E("\t\tcpuTraceLoad(%s,%d,%s,%s);\n", kSymCpuContext, T_FORMAT_LW, kSymMemAddr, R[inst.info.rf_wa0]);
            E("\t\tcpuTraceStore(%s,%d,%s,%s);\n", kSymCpuContext, T_FORMAT_SW, kSymMemAddr, kSymT1);
          }
          break;
        }
        
        case OpCode::LLOCK:
        {
          // if we are in a delay slot store previously computed PC
          if (inst_has_dslot) {
            E("\tpc_t = %s;\n", kSymPc);
          }

          E("\t%s = 0x%08x;\n", kSymPc, pc_cur); 
          E("\t%s = %s;\n", kSymMemAddr, src2);
          
          E_MEMORY_MODEL_ACCESS(inst, kSymMemAddr)
          E_PIPELINE_UPDATE

          E("\tif (!mem_read_word(s,0x%08x,%s,%d)) {\n", pc_cur, kSymMemAddr, inst.info.rf_wa0);
            // clear 1-bit Lock Flag when error occured (TLB exception, memory exception)
            E("\ts->lock_phys_addr = 0;\n");
            if (sim_opts.trace_on) {
              E("\t\tcpuTraceLoad(%s,%d,%s,0);\n", kSymCpuContext, T_FORMAT_LWX, kSymMemAddr);
              E("\t\tcpuTraceException(%s,0);\n",  kSymCpuContext);
            }
            E_TRANS_INSNS_UPDATE(block_insns-1)
            E_PIPELINE_COMMIT
            E("\t\treturn;\n");
          E("\t}\n");
          
          if (sim_opts.trace_on) {
            E("\tcpuTraceLoad (%s,%d,%s,%s);\n", kSymCpuContext, T_FORMAT_LW, kSymMemAddr, R[inst.info.rf_wa0]);
          }

          // Re-store PC previously computed by delayed branch, after successful memory call
          if (inst_has_dslot) {
            E("\t%s = pc_t;\n", kSymPc);
          }
          // set 30-bit Lock Physical Address = MA[31:2] and 1-bit Lock Flag  = 1
          E("\ts->lock_phys_addr = cpuTranslateVirtual(%s,%s) | 1;\n", kSymCpuContext, kSymMemAddr);
          
          break;
        }

        case OpCode::SCOND:
        {
          E("\tcommit = s->lock_phys_addr & 1;\n"); // extract lock flag indicating wether we commit
          
          E("\tif (commit) {\n"); // conditional store
            // if we are in a delay slot store previously computed PC
            if (inst_has_dslot) {
              E("\tpc_t = %s;\n", kSymPc);
            }
            E("\t%s = %s;\n", kSymMemAddr, src2);
            
            E_MEMORY_MODEL_ACCESS(inst, kSymMemAddr)
            E_PIPELINE_UPDATE
            
            // perform memory store
            E("\tif (!mem_write_word(s,0x%08x,%s,%s)) {\n", pc_cur, kSymMemAddr, R[inst.info.rf_wa0]);
            if (sim_opts.trace_on) {
              E("\t\tcpuTraceStore(%s,%d,%s,%s);\n", kSymCpuContext, T_FORMAT_SWX, kSymMemAddr,R[inst.info.rf_wa0]);
              E("\t\tcpuTraceException(%s,0);\n", kSymCpuContext);
            }
            E_TRANS_INSNS_UPDATE(block_insns-1)
            E_PIPELINE_COMMIT
            E("\t\treturn;\n");
            E("\t}\n");
            
            if (sim_opts.trace_on) { // emit trace before the optional address update
              E("\tcpuTraceStore(%s,%d,%s,%s);\n", kSymCpuContext, T_FORMAT_SW, kSymMemAddr,R[inst.info.rf_wa0]);
            }

            // Re-store PC previously computed by delayed branch, after successful memory call
            if (inst_has_dslot) {
              E("\t%s = pc_t;\n", kSymPc);
            }
          E("\t}\n"); // end of conditional store
          
          E("\ts->Z = commit;\n"); // set STATUS32.Z to lock flag
          E("\ts->lock_phys_addr &= ~1;\n"); // clear lock flag bit
          
          break;
        }
          
          
        case OpCode::ASLM:
        {
          E_IF_CC (inst)
          E("\tshift = (char)%s & 0x1f;\n", src2);
          if (inst.flag_enable) {
            E("\tif (shift != 0) {\n");
            
            IF_INLINE_ASM {
              E("\t\t%s = %s;\n", R[inst.info.rf_wa0], src1);
              E("\t\t__asm__ __volatile__ (\"mov %%0, %%%%cl\" : : \"Q\"(shift) : \"%%cl\");\n");
              E("\t\t__asm__ __volatile__ (\"sal %%%%cl, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("\t\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
              E("\t\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
              E("\t\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
            } ELSE_INLINE_ASM {
              E("\ts->C = (%s >> 32-shift) & 1;\n", src1);
              E("\t%s = %s << shift;\n", R[inst.info.rf_wa0], src1);
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            } END_INLINE_ASM;
            
            E("\t} else {\n");
          
            E("\t\t %s = %s;\n", R[inst.info.rf_wa0], src1);
            E("\t\t s->C=0;\n");
            E("\t\t s->N= ( (sint32)%s < 0);\n", R[inst.info.rf_wa0]);
            E("\t\t s->Z= ( (sint32)%s == 0 );\n", R[inst.info.rf_wa0]);
          
            E("\t}\n");
          } else {
            E("\t%s = %s << shift;\n", R[inst.info.rf_wa0], src1);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::LSRM:
        {
          E_IF_CC (inst)
          E("\tshift = (char)%s & 0x1f;\n", src2);
          if (inst.flag_enable) {
            E("\tif (shift != 0) {\n");
            
            IF_INLINE_ASM {
              E("\t\t%s = %s;\n", R[inst.info.rf_wa0], src1);
              E("\t\t__asm__ __volatile__ (\"mov %%0, %%%%cl\" : : \"Q\"(shift) : \"%%cl\");\n");
              E("\t\t__asm__ __volatile__ (\"shr %%%%cl, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("\t\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
              E("\t\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
              E("\t\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
            } ELSE_INLINE_ASM {
              E("\ts->C = (%s >> shift-1) & 1;\n", src1);
              E("\t%s = ((uint32)%s) >> shift;\n", R[inst.info.rf_wa0], src1);
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
            } END_INLINE_ASM;
            
            E("\t} else {\n");
            
            E("\t\t%s = %s;\n", R[inst.info.rf_wa0], src1);
            E("\t\ts->C=0;\n");
            E("\t\ts->N= ( (sint32)%s < 0);\n", R[inst.info.rf_wa0]);
            E("\t\ts->Z= ( (sint32)%s == 0 );\n", R[inst.info.rf_wa0]);
            
            E("\t}\n");
          } else {
            E("\t%s = (uint32)%s >> shift;\n", R[inst.info.rf_wa0], src1);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::ASRM:
        {
          E_IF_CC (inst)
          E("\tshift = (char)%s & 0x1f;\n", src2);
          if (inst.flag_enable)
          {
            E("\tif (shift != 0) {\n");
            
            IF_INLINE_ASM {
              E("\t%s = %s;\n", R[inst.info.rf_wa0], src1);
              E("\t\t__asm__ __volatile__ (\"mov %%0, %%%%cl\" : : \"Q\"(shift) : \"%%cl\");\n");
              E("\t\t__asm__ __volatile__ (\"sar %%%%cl, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              E("\t\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
              E("\t\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
              E("\t\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
            } ELSE_INLINE_ASM {
              E("\ts->C = (%s >> shift-1) & 1;\n", src1);
              E("\t%s = ((sint32)%s) >> shift;\n", R[inst.info.rf_wa0], src1);
              emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]); 
            } END_INLINE_ASM;
            
            E("\t} else {\n");
            
            E("\t\t%s = %s;\n", R[inst.info.rf_wa0], src1);
            E("\t\ts->C=0;\n");
            E("\t\ts->N= ( (sint32)%s < 0);\n", R[inst.info.rf_wa0]);
            E("\t\ts->Z= ( (sint32)%s == 0 );\n", R[inst.info.rf_wa0]);
            
            E("\t}\n");
          }
          else
            E("\t%s = ((sint32)%s) >> shift;\n", R[inst.info.rf_wa0], src1);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::RORM:
        {
          E_IF_CC (inst) {
            E("\tshift = (char)%s;\n", src2);
            IF_INLINE_ASM {
              if (inst.info.rf_wa0 != inst.info.rf_ra0) { // if (dst1 != src1)
                E("\t__asm__ __volatile__ (\"mov %%1, %%0\" : \"=r\"(%s) : \"r\"(%s));\n", R[inst.info.rf_wa0], src1);
              }
              E("\t__asm__ __volatile__ (\"mov %%0, %%%%cl\" : : \"Q\"(shift) : \"%%cl\");\n");
              E("\t__asm__ __volatile__ (\"ror %%%%cl, %%0\" : \"=r\"(%s) : \"0\"(%s) : \"cc\");\n",
                R[inst.info.rf_wa0], R[inst.info.rf_wa0]);
              if (inst.flag_enable) {
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\ts->N= ( (sint32)%s < 0);\n", R[inst.info.rf_wa0]);
                E("\ts->Z= ( (sint32)%s == 0 );\n", R[inst.info.rf_wa0]);
              }
            } ELSE_INLINE_ASM {
              if (inst.flag_enable) {
                E("\ts->C = ((uint32)%s >> shift-1) & 1;\n", src1); // C_flag = src[shift-1]
              }
              E("\t%s = ((uint32)%s>>shift) | ((uint32)%s << 32-shift);\n", R[inst.info.rf_wa0], src1, src1);
              if (inst.flag_enable) {
                emit_set_ZN_noasm(buf, R[inst.info.rf_wa0]);
              }
            } END_INLINE_ASM;
          } E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::ABSS:
        {
          E("\tshift = ((wdata0 = %s) == 0x80000000);\n", src2);
          E("\t%s = (shift) ? 0x7fffffff : (wdata0 < 0 ? -wdata0 : wdata0);\n", R[inst.info.rf_wa0]);
          if (inst.flag_enable) {
            E("\ts->Z = (wdata0 == 0);\n");
            E("\ts->N = (wdata0 & 0x80000000) != 0);\n");
            E("\ts->V = shift;\n");
            E("\tif (shift) s->auxs[%d] |= 0x00000210;\n", AUX_MACMODE);
          }
          break;
        }
          
        case OpCode::ABSSW:
        {
          E("\tshift = ((wdata0 = ((%s) & 0x0000ffff)) == 0x00008000);\n", src2);
          E("\t%s = (shift) ? 0x00007fff : (wdata0 < 0 ? -wdata0 : wdata0);\n", R[inst.info.rf_wa0]);
          if (inst.flag_enable) {
            E("\ts->Z = (wdata0 == 0);\n");
            E("\ts->N = (wdata0 & 0x00008000) != 0);\n");
            E("\ts->V = shift;\n");
            E("\tif (shift) s->auxs[%d] |= 0x00000210;\n", AUX_MACMODE);
          }
          break;
        }
          
        case OpCode::ADDS:
        {
          E_IF_CC (inst)
          
          IF_INLINE_ASM {
            E("\twdata0 = %s;\n", src1);
            E("\t__asm__ __volatile__ (\"addl %%1, %%0\" : \"=r\"(wdata0) : \"r\"(%s), \"0\"(wdata0) : \"cc\");\n", src2);
            E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(shift));\n");
          } ELSE_INLINE_ASM {
            E("\twdata0 = (sint32)%s + (sint32)%s;\n", src1, src2);
            E_OPEN_SCOPE {
              E("\t\tt1 = s->V;\n"); // save s->V
              E("\t\tconst uint64 data64 = (uint64)(%s) + %s;\n", src1, src2);
              emit_set_CV_noasm(buf, "data64", false, true, src1, src2);                
              E("\t\tshift = s->V;\n");
              E("\t\ts->V = t1;\n"); // restore s->V
            } E_CLOSE_SCOPE;
          } END_INLINE_ASM;
          
          E("\t%s = shift ? (%s > 0 ? 0x7fffffff : 0x80000000) : wdata0;\n", R[inst.info.rf_wa0], src1);
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = (%s < 0);\n",  R[inst.info.rf_wa0]);
            E("\ts->V = shift;\n");
            E("\tif (shift) s->auxs[%d] |= 0x00000210;\n", AUX_MACMODE);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::SUBS:
        {
          E_IF_CC (inst)
          
          IF_INLINE_ASM {
            E("\twdata0 = %s;\n", src1);
            E("\t__asm__ __volatile__ (\"subl %%1, %%0\" : \"=r\"(wdata0) : \"r\"(%s), \"0\"(wdata0) : \"cc\");\n", src2);
            E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(shift));\n");
          } ELSE_INLINE_ASM {
            E("\twdata0 = (sint32)%s - (sint32)%s;\n", src1, src2);
            E_OPEN_SCOPE {
              E("\t\tt1 = s->V;\n"); // save s->V
              E("\t\tconst uint64 data64 = (uint64)(%s) - %s;\n", src1, src2);
              emit_set_CV_noasm(buf, "data64", false, true, src1, src2, false);                
              E("\t\tshift = s->V;\n");
              E("\t\ts->V = t1;\n"); // restore s->V
            } E_CLOSE_SCOPE;              
          } END_INLINE_ASM;
          
          E("\t%s = shift ? (%s > 0 ? 0x7fffffff : 0x80000000) : wdata0;\n", R[inst.info.rf_wa0], src1);
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = (%s < 0);\n", R[inst.info.rf_wa0]);
            E("\ts->V = shift;\n");
            E("\tif (shift) s->auxs[%d] |= 0x00000210;\n", AUX_MACMODE);
          }
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::ADDSDW:
        case OpCode::SUBSDW:
        {
          // As translation is x86-specific, we can assume availability of MMX
          // The code for both ADDSDW and SUBSDW is identical, except for the use
          // of 'paddsw' versus 'psubsw'.
          //
          const char* mmx_op = inst.code == OpCode::ADDSDW ? "add" : "sub";
          const char c_op = inst.code == OpCode::ADDSDW ? '+' : '-';
          
          E_IF_CC (inst)

          if (inst.flag_enable) {
            E("\t{\n\t\tint v1, v2;\n");
            
            IF_INLINE_ASM {
              E("\t\t__asm__ __volatile__ (\n");
              E("\t\t\"movd %%2, %%%%mm0\\n\\t\"\n");
              E("\t\t\"movd %%3, %%%%mm1\\n\\t\"\n");
              E("\t\t\"movd %%2, %%%%mm2\\n\\t\"\n");
              E("\t\t\"movd %%3, %%%%mm3\\n\\t\"\n");
              E("\t\t\"p%ssw %%%%mm1, %%%%mm0\\n\\t\"\n", mmx_op);
              E("\t\t\"p%sw %%%%mm3, %%%%mm2\\n\\t\"\n", mmx_op);
              E("\t\t\"movd %%%%mm0, %%0\\n\\t\"\n");
              E("\t\t\"movd %%%%mm1, %%1\\n\\t\"\n");
              E("\t\t: \"=m\" (%s), \"=m\" (wdata0)\n", R[inst.info.rf_wa0]);
              E("\t\t: \"m\" (%s), \"m\" (%s)\n", src1, src2);
              E("\t\t: \"memory\", \"cc\"\n");
              E("\t\t);\n");
            } ELSE_INLINE_ASM {
              // Inspired by mmx-emu library - this code needs testing!
              E("\t\tsint16 *op1ptr = (sint16 *) &%s;\n", src1);
              E("\t\tsint16 *op2ptr = (sint16 *) &%s;\n", src2);
              E("\t\tsint16 *dstptr = (sint16 *) &%s;\n", R[inst.info.rf_wa0]);
              E("\t\tsint16 *wdata0ptr = (sint16 *) &wdata0;\n");
              E("\t\tv1 = (sint32)op1ptr[0] %c (sint32)op2ptr[0];\n",c_op);
              E("\t\tv2 = (sint32)op1ptr[1] %c (sint32)op2ptr[1];\n",c_op);
              // Saturate both words
              E("\t\tif (v1>(sint32)(sint16)0x7FFF) { dstptr[0] = (sint16)0x7FFF; }\n" \
                   "\t\t\telse if (v1<(sint32)(sint16)0x8000) { dstptr[0] = (sint16)0x8000; }\n" \
                   "\t\t\telse { dstptr[0] = (sint16)v1; }\n");
              E("\t\tif (v2>(sint32)(sint16)0x7FFF) { dstptr[1] = (sint16)0x7FFF; }\n" \
                   "\t\t\telse if (v2<(sint32)(sint16)0x8000) { dstptr[1] = (sint16)0x8000; }\n" \
                   "\t\t\telse { dstptr[1] = (sint16)v2; }\n");
              E("\t\twdata0ptr[0] = (sint16)v1;\n");
              E("\t\twdata0ptr[1] = (sint16)v2;\n");
            } END_INLINE_ASM;
            
            E("\t\tv2 = (wdata0 & 0xffff0000) != 0;\n");
            E("\t\tv1 = (wdata0 & 0x0000ffff) != 0;\n");
            E("\t\ts->V = v2 || v1;\n");
            E("\t\ts->auxs[0x%03x] |= ((v1 << 4) | (v2 << 9));\n", AUX_MACMODE);
            E("\t\ts->N = (((%s) & 0x80008000) != 0);\n", R[inst.info.rf_wa0]);
            
            //E("printf(\"%cSDW: op1=%%08x op2=%%08x dst=%%08x v1=%%08x v2=%%08x\n\",%s,%s,%s,v1,v2);", c_op, op1, op2, R[inst.info.rf_wa0]);
            E("\t}\n");   
          } else {
            IF_INLINE_ASM {
              E("\t__asm__ __volatile__ (\n");
              E("\t\"movd %%1, %%%%mm0\\n\\t\"\n");
              E("\t\"movd %%2, %%%%mm1\\n\\t\"\n");
              E("\t\"movd %%1, %%%%mm2\\n\\t\"\n");
              E("\t\"movd %%2, %%%%mm3\\n\\t\"\n");
              E("\t\"p%ssw %%%%mm1, %%%%mm0\\n\\t\"\n", mmx_op);
              E("\t\"movd %%%%mm0, %%0\\n\\t\"\n");
              E("\t: \"=m\" (%s)\n", R[inst.info.rf_wa0]);
              E("\t: \"m\" (%s), \"m\" (%s)\n", src1, src2);
              E("\t: \"memory\", \"cc\"\n");
              E("\t);\n");
              //E("printf(\"%cSDW: op1=%%08x op2=%%08x dst=%%08x v1=-- v2=--\n\",%s,%s,%s);", c_op, op1, op2, R[inst.info.rf_wa0]);
            } ELSE_INLINE_ASM {
              E("\t{\n\t\tint v1, v2;\n");
              // Inspired by mmx-emu library - this code needs testing!
              E("\t\tsint16 *op1ptr = (sint16 *) &%s;\n", src1);
              E("\t\tsint16 *op2ptr = (sint16 *) &%s;\n", src2);
              E("\t\tsint16 *dstptr = (sint16 *) &%s;\n", R[inst.info.rf_wa0]);
              E("\t\tv1 = (sint32)op1ptr[0] %c (sint32)op2ptr[0];\n", c_op);
              E("\t\tv2 = (sint32)op1ptr[1] %c (sint32)op2ptr[1];\n", c_op);
              // Saturate both words
              E("\t\tif (v1>(sint32)(sint16)0x7FFF) { dstptr[0] = (sint16)0x7FFF; }\n" \
                   "\t\t\telse if (v1<(sint32)(sint16)0x8000) { dstptr[0] = (sint16)0x8000; }\n" \
                   "\t\t\telse { dstptr[0] = (sint16)v1; }\n");
              E("\t\tif (v2>(sint32)(sint16)0x7FFF) { dstptr[1] = (sint16)0x7FFF; }\n" \
                   "\t\t\telse if (v2<(sint32)(sint16)0x8000) { dstptr[1] = (sint16)0x8000; }\n" \
                   "\t\t\telse { dstptr[1] = (sint16)v2; }\n");
              E("\t}\n");
              //E("printf(\"%cSDW: op1=%%08x op2=%%08x dst=%%08x v1=%%08x v2=%%08x\n\",%s,%s,%s,v1,v2);", c_op, op1, op2, R[inst.info.rf_wa0]);
            } END_INLINE_ASM;
          }
          
          E_ENDIF_CC (inst)
          break;
        }
          
          
        case OpCode::ASLS:
        case OpCode::ASRS:
        {
          E_IF_CC (inst)
          
          E("\t{\n\tint, sat_shift, sat;\n");
          
          if (inst.flag_enable) {
            E("\t\ts->Z = (%s) == 0;\n", src1);
            E("\t\ts->N = (sint32)(%s) < 0;\n", src1);
          }
          
          E("\t\tdir = ((shift = %s) < 0);\n", src2);
          
          if (inst.code == OpCode::ASLS) {
            E("\t\tsat_shift = (shift < -31);\n");
            E("\t\tshift = dir ? (sat_shift ? 31 : -shift) : shift;\n");
            E("\t\tsat = sat_shift || ((shift & 0x1fUL) != 0);\n");
            E("\t\t%s = (dir ? (%s >> shift) : (%s << shift);\n", R[inst.info.rf_wa0], src1, src1);
          } else {
            E("\t\tsat_shift = (shift > 31);\n");
            E("\t\tshift = dir ? -shift : (sat_shift ? 31 : shift);\n");
            E("\t\t%s = (dir ? (%s >> shift) : (%s << shift);\n", R[inst.info.rf_wa0], src1, src1);
          }
          
          if (inst.flag_enable) {
            E("\t\tsat = sat_shift || ((shift & 0x1fUL) != 0);\n");
            E("\t\ts->V = (sat != 0);\n");
            E("\t\tif (sat) s->auxs[0x%03x] |= 0x00000210;\n", AUX_MACMODE);
          }
          
          E("\t}\n");   
          
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::DIVAW:
        {
          E_OPEN_SCOPE {
            E_IF_CC (inst)
            E("\tsint32 src1 = (%s);\n", src1);
            E("\tif (src1 == 0)\n");
            E("\t%s = 0;\n", R[inst.info.rf_wa0]);
            E("\telse{\n");
            E("\t src1 <<= 1;\n");
            E("\tsint32 diff = src1 -((sint32)%s);\n", src2);
            E("\tif( (diff & 0x80000000) == 0)\n");
            E("\t%s = (src1 - (sint32)%s) | 0x1;\n", R[inst.info.rf_wa0], src2);
            E("\telse{\n");
            E("\t%s = src1 ;\n", R[inst.info.rf_wa0]);
            E("\t}\n");
            E("\t}\n");
            E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }
          
        case OpCode::NEG:
        {            
          E_IF_CC (inst)
          E("\t%s = -(%s);\n", R[inst.info.rf_wa0], src2);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::NEGS:
        case OpCode::NEGSW:
        {
          const char *ovfl_test;
          const char *sat_val;
          
          if (inst.code == OpCode::NEGS) {
            ovfl_test = "== 0x80000000L";
            sat_val   = "0x7fffffffL";
          } else {
            ovfl_test = "<= 0xffff8000L";
            sat_val   = "0x00007fffL";
          }
          
          E_IF_CC (inst)
          if (inst.flag_enable) {
            E("\tif ((sint32)(%s) %s) {\n", src2, ovfl_test);
            E("\t\t%s = %s;\n", R[inst.info.rf_wa0], sat_val);
            E("\t\ts->V = 1;\n");
            E("\t\ts->auxs[0x%03x] |= 0x00000210UL;\n", AUX_MACMODE);
            E("\t} else\n");
            E("\t\t%s = -(sint32)(%s);\n", R[inst.info.rf_wa0], src2);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
          } else
            E("\t%s = ((sint32)%s %s) ? %s : -(sint32)(%s);\n",
              R[inst.info.rf_wa0], src2, ovfl_test, sat_val, src2);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::NORM:
        {
          E("\t\tt2 = (sint32)%s;\n", src2);

          if (inst.flag_enable) {
            E("\t\ts->Z = (t2 == 0);\n");
            E("\t\ts->N = ((sint32)t2 < 0);\n");
          }

          E("\t\tif ((sint32)t2 < 0) t2 = ~(sint32)t2;\n");
          
          IF_INLINE_ASM {
            E("\t\t__asm__ __volatile__ (\n");         
            E("\t\t\t\"bsrl %%1, %%0\\n\\t\" : \"=r\" (t1) : \"m\" (t2) : \"cc\" );\n");
          } ELSE_INLINE_ASM {
            E("\t\tt1 = 31;\n");
            E("\t\twhile (t1 <= 31) {\n"); // t1 is unsigned, so it wraps around!
            E("\t\t\tif (t2 & 0x80000000) break;\n");
            E("\t\t\tt1--; t2 <<= 1;\n");
            E("\t\t}\n");
          } END_INLINE_ASM;
          
          E("\t\t%s = (t2 == 0) ? 31 : 30 - (sint32)t1;\n", R[inst.info.rf_wa0]);
          break;
        }
          
        case OpCode::NORMW:
        {
          E("\t\tt2 = %s;\n", src2);
          
          if (inst.flag_enable) {
            E("\t\ts->Z = (t2 == 0);\n");
            E("\t\ts->N = ((sint32)t2 < 0);\n");
          }
          
          E("\t\tif ((sint32)t2 < 0) t2 = ~t2;\n");
          
          IF_INLINE_ASM {
            E("\t\t__asm__ __volatile__ (\n");         
            E("\t\t\t\"bsrl %%1, %%0\\n\\t\" : \"=r\" (t1) : \"m\" (t2) : \"cc\" );\n"); 
          } ELSE_INLINE_ASM {
            E("\t\tt1 = 31;\n");
            E("\t\twhile (t1 <= 31) {\n"); // t1 is unsigned, so it wraps around!
            E("\t\t\tif (t2 & 0x80000000) break;\n");
            E("\t\t\tt1--; t2 <<= 1;\n");
            E("\t\t}\n");
          } END_INLINE_ASM;
            
          E("\t\t%s = (s->Z || (t2 == 0x0000ffff)) ? 15: 30 - t1;\n", R[inst.info.rf_wa0]);
          break;
        }
          
        case OpCode::FFS:
        {
        
          E("\t\tt2 = (sint32)%s;\n", src2);

          if (inst.flag_enable) {
            E("\t\ts->Z = (t2 == 0);\n");
            E("\t\ts->N = ((sint32)t2 < 0);\n");
          }
          
          IF_INLINE_ASM {
            E("\t\tt1 = 31;\n");
            E("\t\t__asm__ __volatile__ (\n");         
            E("\t\t\t\"bsfl %%1, %%0\\n\\t\" : \"+r\" (t1) : \"rm\" (t2) : \"cc\" );\n");
          } ELSE_INLINE_ASM {
            E("\t\tt1 = 0;\n");
            E("\t\twhile (t1 < 31) {\n");
            E("\t\t\tif (t2 & 0x1) break;\n");
            E("\t\t\tt1++; t2 >>= 1;\n");
            E("\t\t}\n");
          } END_INLINE_ASM;
          
          E("\t\t%s = t1;\n", R[inst.info.rf_wa0]);
          break;
        }

        case OpCode::FLS:
        {
        
          E("\t\tt2 = (sint32)%s;\n", src2);

          if (inst.flag_enable) {
            E("\t\ts->Z = (t2 == 0);\n");
            E("\t\ts->N = ((sint32)t2 < 0);\n");
          }
          
          IF_INLINE_ASM {
            E("\t\tt1 = 0;\n");
            E("\t\t__asm__ __volatile__ (\n");         
            E("\t\t\t\"bsrl %%1, %%0\\n\\t\" : \"+r\" (t1) : \"rm\" (t2) : \"cc\" );\n");
          } ELSE_INLINE_ASM {
            E("\t\tt1 = 31;\n");
            E("\t\twhile (t1 > 0) {\n");
            E("\t\t\tif (t2 & 0x80000000) break;\n");
            E("\t\t\tt1--; t2 <<= 1;\n");
            E("\t\t}\n");
          } END_INLINE_ASM;
          
          E("\t\t%s = t1;\n", R[inst.info.rf_wa0]);
          break;
        }

        case OpCode::RND16:
        {
          E("\tt2 = %s;\n", src2);
          
          if (inst.flag_enable) {
            E("\tif ((sint32)t2 >= 0x7fff8000L) {\n");
            E("\t\t%s = 0x7fffL;\n", R[inst.info.rf_wa0]);
            E("\t\ts->V = 1;\n");
            E("\t\ts->auxs.[0x%03x] |= 0x00000210UL;\n", AUX_MACMODE);
            E("\telse %s = (t2 + 0x8000L) >> 16;\n", R[inst.info.rf_wa0]);
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);
          } else {
            E("\t%s = ((sint32)t2 >= 0x7fff8000L) ? 0x7fffL : (t2+0x8000L)>>16;\n",
              R[inst.info.rf_wa0]);
          }
          break;   
        }
          
        case OpCode::SAT16:
        {
          E("\tif (%s > (sint32)0x00007fff) {\n", src2);
            E("\t\t%s = 0x00007fffUL; t2 = 1;\n", R[inst.info.rf_wa0]);
          E("\t} else if (%s < (sint32)0xffff8000) {\n", src2);
            E("\t\t%s = 0xffff8000UL; t2 = 1;\n", R[inst.info.rf_wa0]);
          E("\t} else { %s = %s; t2 = 0; }\n", R[inst.info.rf_wa0], src2);
          
          if (inst.flag_enable) {
            E("\t\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\t\ts->N = (((sint32)%s) < 0);\n", R[inst.info.rf_wa0]);
            E("\t\ts->V = t2;\n");
            E("\t\tif (t2) { s->auxs[%d] |= 0x00000210UL; }\n", AUX_MACMODE);
          }
          break;   
        }
          
        case OpCode::SWAP:
        {
          E("\tt2 = %s;\n", src2);
          E("\t%s = (t2 >> 16) | (t2 << 16);\n", R[inst.info.rf_wa0]);
          
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);    
          }
          break;
        }
          
        case OpCode::SWAPE:
        {
          E("\tt2 = %s;\n", src2);
          E("\tt2 = ((t2 & 0x00FF00FFU) << 8) | ((t2 & 0xFF00FF00U) >> 8);\n");
          E("\t%s = (t2 >> 16) | (t2 << 16);\n", R[inst.info.rf_wa0]);
          
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);    
          }
          break;
        }
        
        case OpCode::LSL16:
        {
          E("\tt2 = %s;\n", src2);
          E("\t%s = (t2 << 16);\n", R[inst.info.rf_wa0]);
          
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);    
          }
          break;
        }
          
        case OpCode::LSR16:
        {
          E("\tt2 = %s;\n", src2);
          E("\t%s = (t2 >> 16);\n", R[inst.info.rf_wa0]);
          
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);    
          }
          break;
        }
          
        case OpCode::ASR16:
        {
          E("\t%s = ((sint32)%s) >> 16;\n", R[inst.info.rf_wa0], src2);
          
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);    
          }
          break;
        }
          
        case OpCode::ASR8:
        {
          E("\t%s = ((sint32)%s) >> 8;\n", R[inst.info.rf_wa0], src2);
          
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);
          }
          break;
        }
          
        case OpCode::LSR8:
        {
          E("\tt2 = %s;\n", src2);
          E("\t%s = (t2 >> 8);\n", R[inst.info.rf_wa0]);
          
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);    
          }
          break;
        }
          
        case OpCode::ROL8:
        {
          E("\tt2 = %s;\n", src2);
          E("\t%s = ((t2 << 8) | (t2 >> 24));\n", R[inst.info.rf_wa0]);
          
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);    
          }
          break;
        }
          
        case OpCode::ROR8:
        {
          E("\tt2 = %s;\n", src2);
          E("\t%s = ((t2 >> 8) | (t2 << 24));\n", R[inst.info.rf_wa0]);
          
          if (inst.flag_enable) {
            E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            E("\ts->N = ((sint32)(%s) < 0);\n", R[inst.info.rf_wa0]);    
          }
          break;
        }
          
        case OpCode::FLAG:
        {
          E_IF_CC (inst)
            // As a FLAG instruction could halt the CPU, we must terminate
            // the translated block at this instruction.
            // N.B. we do not update PC explicitly, thereby allowing next
            // pc to be assigned to state.pc just before the translated
            // block is exited. This ensures the FLAG instruction commits.
            //
            E_PIPELINE_UPDATE
            E("\tif (!(s->U)) {\n");
              E("\tif ((%s & 1) == 0) {\n", src2);
                E("\t\ts->E2 = (%s >> 2) & 1UL;\n", src2);
                E("\t\ts->E1 = (%s >> 1) & 1UL;\n", src2);
            
            if (inst.flag_enable) {
                E("\t\ts->AE = (%s >> 5) & 1UL;\n", src2);
                E("\t\ts->A2 = (%s >> 4) & 1UL;\n", src2);
                E("\t\ts->A1 = (%s >> 3) & 1UL;\n", src2);            
            }
            
                E("\t\ts->auxs[%d] &= 0x9fffffff;\n", AUX_DEBUG);
                E("\t\ts->pending_actions |= kPendingAction_CPU;\n");
              E("\t} else {\n");
                E("\t\tcpuHalt(%s);\n", kSymCpuContext);
                E_TRANS_INSNS_UPDATE(block_insns)
                E_PIPELINE_COMMIT
                if (sim_opts.trace_on) { E("\tcpuTraceCommit(%s,0);\n", kSymCpuContext); }
                E("\t\treturn;\n");
              E("\t}\n");
            E("\t}\n");
            E("\ts->Z = (%s >> 11) & 1UL;\n", src2);
            E("\ts->N = (%s >> 10) & 1UL;\n", src2);
            E("\ts->C = (%s >> 9)  & 1UL;\n", src2);
            E("\ts->V = (%s >> 8)  & 1UL;\n", src2);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::SLEEP:
        {
          // Note: we do not update PC as part of the sleep semantics.
          // This is to allow pc_cur to move onto the instruction after
          // the sleep instruction so that the sleep instruction is
          // effectively committed.
          //
          E("\t\tcpuSleep(%s,%s);\n", kSymCpuContext, src2);
          break;
        }
          
        case OpCode::BREAK:
        {
          // BREAK instruction is not counted when PC frequency recording is enabled
          //
          if (sim_opts.is_pc_freq_recording_enabled) {
            E_COMMENT("\t// -- PC FREQUENCY HIST UPDATE\n");
            E("\t--(*((uint32 * const)(%#p)));\n",
              work_unit.cpu->cnt_ctx.pc_freq_hist.get_value_ptr_at_index(pc_cur));
          }
          
          E("\t%s = 0x%08x;\n", kSymPc, pc_cur); 
          E("\tcpuBreak(%s);\n", kSymCpuContext);
          
          // We also do not update the pipeline for this instruction by setting
          // the following variable to true
          //
          pipeline_updated = true;
          
          // Note: we do update PC with the current value of pc_cur, which points
          // at the BRK instruction. This prevents any further update to state.pc,
          // and this means that the BRK instruction has not actually committed.
          //
          pc_updated = true;

          break;
        }
                    
        case OpCode::AP_BREAK:
        {
          E("\t%s = 0x%08x;\n", kSymPc, pc_cur); 
          
          // The required action is given by the short-immediate operand.
          //
          if (inst.shimm == 0) {
             // The action for this Breakpoint is to Halt the processor,
             // as if this were a BRK instruction.
             //
            E("\tcpuBreak (%s);\n", kSymCpuContext);
            E("\treturn;\n");
          } else {
            // The action for this Breakpoint is to raise an exception,
            // as if this were a SWI instruction.
            //
            E("\tcpuEnterException(%s,0x%08x,0x%08x,0x%08x);\n",
                 kSymCpuContext, ECR(work_unit.cpu->sys_arch.isa_opts.EV_PrivilegeV, ActionPointHit, 0), pc_cur, pc_cur);
            if (sim_opts.trace_on) {
              E("\tcpuTraceException(%s,0);\n", kSymCpuContext);
            }
          }
          
          // Note: we update PC with the current value of pc_cur, which points
          // at the instruction that triggered this  breakpoint. This prevents
          // any further update to state.pc, meaning that we stop before executing
          // the instruction at the breakpoint address.
          //
          pc_updated = true;

          break;
        }
          
        case OpCode::SWI:
        {
          if (work_unit.cpu->sys_arch.isa_opts.is_isa_a600()) { // A600 semantics
            E("\tcpuEnterException(%s,0x%08x,0x%08x,0x%08x);\n", kSymCpuContext,
              ECR(work_unit.cpu->sys_arch.isa_opts.EV_InstructionError,IllegalInstruction, 0), pc_cur, pc_nxt);
          } else { // A700 and ARCompact v2
            E("\tcpuEnterException(%s,%s,0x%08x,0x%08x);\n", kSymCpuContext, src2, pc_cur, pc_cur);
          }
          if (sim_opts.trace_on) { E("\tcpuTraceException(%s,0);\n", kSymCpuContext); }
          pc_updated = true;
          break;
        }

        case OpCode::TRAP0:
        {
          // If we are already in an exception we will raise a machine check exception,
          // instead of a trap; this necessitates calling enter_exception here,
          // causing a pre-commit exception.
          //
          E("\tif(s->AE){\n");
            E("\t\tcpuEnterException(%s,%s,0x%08x,0x%08x);\n", kSymCpuContext, src2, pc_cur, pc_cur);
            if (sim_opts.trace_on) E("\t\tcpuTraceException (%s, 0);\n", kSymCpuContext);
            E_TRANS_INSNS_UPDATE(block_insns-1)
            E_PIPELINE_COMMIT
            E("\t\treturn;\n");
          E("\t}\n");
          if (sim_opts.emulate_traps) {
            E("\t%s = 0x%08x;\n", kSymPc, pc_cur);
            E("\tcpuEmulateTrap (%s);\n", kSymCpuContext);
          } else {
            pc_updated = true;
            E("\tcpuEnterException(%s,%s,0x%08x,0x%08x);\n", kSymCpuContext, src2, pc_cur, pc_nxt);
            if (sim_opts.trace_on) { E("\tcpuTraceException(%s,0);\n", kSymCpuContext); }
          }
          break;
        }
          
        case OpCode::RTIE:
        {
          E_CHECK_ILLEGAL
          pc_updated = true;
                    
          E("\tcpuExitException(%s);\n", kSymCpuContext);
          // When we return from from an exception we need to set PC to NEXT_PC
          // as the call to cpuExitException() sets next_pc correctly
          //
          E("\t%s = s->next_pc & s->pc_mask;\n", kSymPc);
          
          if (sim_opts.trace_on) E("\tcpuTraceRTIE (%s);\n", kSymCpuContext);
          break;
        }
        
        case OpCode::SYNC:
          // FIXME: this needs special handling with respect to the performance
          //        model!
          break;

        case OpCode::FMUL:
        case OpCode::FADD:
        case OpCode::FSUB: {
          E_IF_CC (inst)
          E("\tcpuSPFP (%s,%d,&(%s),%s,%s,%d);\n",
            kSymCpuContext, inst.code, R[inst.info.rf_wa0], src1, src2, inst.flag_enable);
          E_ENDIF_CC (inst)
          break;   
        }
          
        case OpCode::DMULH11:  case OpCode::DMULH12:
        case OpCode::DMULH21:  case OpCode::DMULH22:
        case OpCode::DADDH11:  case OpCode::DADDH12:
        case OpCode::DADDH21:  case OpCode::DADDH22:
        case OpCode::DSUBH11:  case OpCode::DSUBH12:
        case OpCode::DSUBH21:  case OpCode::DSUBH22:
        case OpCode::DRSUBH11: case OpCode::DRSUBH12:
        case OpCode::DRSUBH21: case OpCode::DRSUBH22: {
          E_IF_CC (inst)
          E("\tcpuDPFP(%s,%d,&(%s),%s,%s,%d);\n",
            kSymCpuContext, inst.code, R[inst.info.rf_wa0], src1, src2, inst.flag_enable);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::DEXCL1:
        case OpCode::DEXCL2: {
          E_IF_CC (inst)
          E("\tcpuDEXCL(%s,%d,&(%s),%s,%s,%d);\n",
            kSymCpuContext, inst.code, R[inst.info.rf_wa0], src1, src2, inst.flag_enable);
          E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::NOP: break;
          
        // -------------------------------------------------------------------
        // Translations for ARCompact V2 instructions
        //

        case OpCode::SETEQ:
        {
          E_IF_CC (inst)
            E("\t%s = ((%s) == (%s));\n", R[inst.info.rf_wa0], src1, src2);
            if (inst.flag_enable)
            {
              IF_INLINE_ASM {
                E("\t__asm__ __volatile__ (\"cmpl %%1, %%0\" : : \"r\"(%s), \"r\"(%s) : \"cc\");\n", src1, src2);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } ELSE_INLINE_ASM {
                E_OPEN_SCOPE {
                  E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src1, src2);
                  emit_set_ZN_noasm(buf, "data64");
                  emit_set_CV_noasm(buf, "data64", true, true, src1, src2, true);
                } E_CLOSE_SCOPE
              } END_INLINE_ASM;
            }
            E_ENDIF_CC (inst)
          break;
        }
          
        case OpCode::SETNE:
        {
          E_IF_CC (inst)
            E("\t%s = ((%s) != (%s));\n", R[inst.info.rf_wa0], src1, src2);
            if (inst.flag_enable)
            {
              IF_INLINE_ASM {
                E("\t__asm__ __volatile__ (\"cmpl %%1, %%0\" : : \"r\"(%s), \"r\"(%s) : \"cc\");\n", src1, src2);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } ELSE_INLINE_ASM {
                E_OPEN_SCOPE {
                  E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src1, src2);
                  emit_set_ZN_noasm(buf, "data64");
                  emit_set_CV_noasm(buf, "data64", true, true, src1, src2, true);
                } E_CLOSE_SCOPE
              } END_INLINE_ASM;
            }
            E_ENDIF_CC (inst)
          break;
        }
        
        case OpCode::SETLT:
        {
          E_IF_CC (inst)
            E("\t%s = (((sint32)%s) < ((sint32)%s));\n", R[inst.info.rf_wa0], src1, src2);
            if (inst.flag_enable)
            {
              IF_INLINE_ASM {
                E("\t__asm__ __volatile__ (\"cmpl %%1, %%0\" : : \"r\"(%s), \"r\"(%s) : \"cc\");\n", src1, src2);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } ELSE_INLINE_ASM {
                E_OPEN_SCOPE {
                  E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src1, src2);
                  emit_set_ZN_noasm(buf, "data64");
                  emit_set_CV_noasm(buf, "data64", true, true, src1, src2, true);
                } E_CLOSE_SCOPE
              } END_INLINE_ASM;
            }
            E_ENDIF_CC (inst)
          break;
        }
        
        case OpCode::SETGE:
        {
          E_IF_CC (inst)
            E("\t%s = (((sint32)%s) >= ((sint32)%s));\n", R[inst.info.rf_wa0], src1, src2);
            if (inst.flag_enable)
            {
              IF_INLINE_ASM {
                E("\t__asm__ __volatile__ (\"cmpl %%1, %%0\" : : \"r\"(%s), \"r\"(%s) : \"cc\");\n", src1, src2);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } ELSE_INLINE_ASM {
                E_OPEN_SCOPE {
                  E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src1, src2);
                  emit_set_ZN_noasm(buf, "data64");
                  emit_set_CV_noasm(buf, "data64", true, true, src1, src2, true);
                } E_CLOSE_SCOPE
              } END_INLINE_ASM;
            }
            E_ENDIF_CC (inst)
          break;
        }
        
        case OpCode::SETLO:
        {
          E_IF_CC (inst)
            E("\t%s = (((uint32)%s) < ((uint32)%s));\n", R[inst.info.rf_wa0], src1, src2);
            if (inst.flag_enable)
            {
              IF_INLINE_ASM {
                E("\t__asm__ __volatile__ (\"cmpl %%1, %%0\" : : \"r\"(%s), \"r\"(%s) : \"cc\");\n", src1, src2);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } ELSE_INLINE_ASM {
                E_OPEN_SCOPE {
                  E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src1, src2);
                  emit_set_ZN_noasm(buf, "data64");
                  emit_set_CV_noasm(buf, "data64", true, true, src1, src2, true);
                } E_CLOSE_SCOPE
              } END_INLINE_ASM;
            }
            E_ENDIF_CC (inst)
          break;
        }
        
        case OpCode::SETHS:
        {
          E_IF_CC (inst)
            E("\t%s = (((uint32)%s) >= ((uint32)%s));\n", R[inst.info.rf_wa0], src1, src2);
            if (inst.flag_enable)
            {
              IF_INLINE_ASM {
                E("\t__asm__ __volatile__ (\"cmpl %%1, %%0\" : : \"r\"(%s), \"r\"(%s) : \"cc\");\n", src1, src2);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } ELSE_INLINE_ASM {
                E_OPEN_SCOPE {
                  E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src1, src2);
                  emit_set_ZN_noasm(buf, "data64");
                  emit_set_CV_noasm(buf, "data64", true, true, src1, src2, true);
                } E_CLOSE_SCOPE
              } END_INLINE_ASM;
            }
            E_ENDIF_CC (inst)
          break;
        }
        
        case OpCode::SETLE:
        {
          E_IF_CC (inst)
            E("\t%s = (((sint32)%s) <= ((sint32)%s));\n", R[inst.info.rf_wa0], src1, src2);
            if (inst.flag_enable)
            {
              IF_INLINE_ASM {
                E("\t__asm__ __volatile__ (\"cmpl %%1, %%0\" : : \"r\"(%s), \"r\"(%s) : \"cc\");\n", src1, src2);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } ELSE_INLINE_ASM {
                E_OPEN_SCOPE {
                  E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src1, src2);
                  emit_set_ZN_noasm(buf, "data64");
                  emit_set_CV_noasm(buf, "data64", true, true, src1, src2, true);
                } E_CLOSE_SCOPE
              } END_INLINE_ASM;
            }
            E_ENDIF_CC (inst)
          break;
        }
        
        case OpCode::SETGT:
        {
          E_IF_CC (inst)
            E("\t%s = (((sint32)%s) > ((sint32)%s));\n", R[inst.info.rf_wa0], src1, src2);
            if (inst.flag_enable)
            {
              IF_INLINE_ASM {
                E("\t__asm__ __volatile__ (\"cmpl %%1, %%0\" : : \"r\"(%s), \"r\"(%s) : \"cc\");\n", src1, src2);
                E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
                E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
                E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
                E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
              } ELSE_INLINE_ASM {
                E_OPEN_SCOPE {
                  E("\t\tconst uint64 data64 = (uint64)%s - %s;\n", src1, src2);
                  emit_set_ZN_noasm(buf, "data64");
                  emit_set_CV_noasm(buf, "data64", true, true, src1, src2, true);
                } E_CLOSE_SCOPE
              } END_INLINE_ASM;
            }
            E_ENDIF_CC (inst)
          break;
        }

        case OpCode::MPYW:
        {
          E_OPEN_SCOPE {
          E_IF_CC (inst)
            E("\tsint32 x = ((sint32)(%s) << 16) >> 16;\n", src1);
            E("\tx *= ((((sint32)%s) << 16) >> 16);\n", src2);
            E("\t%s = (sint32)x;\n", R[inst.info.rf_wa0]);
            
            if (inst.flag_enable) {
              E("\ts->V = 0;\n");
              E("\ts->N = (x < 0);\n");
              E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            }
          E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }           

        case OpCode::MPYWU:
        {
          E_OPEN_SCOPE {
          E_IF_CC (inst) {
            E("\tuint32 x = (uint32)((%s) & 0x0000FFFFU);\n", src1);
            E("\tx *= (uint32)((%s) & 0x0000FFFFU);\n", src2);
            E("\t%s = (uint32)x;\n", R[inst.info.rf_wa0]);
          
            if (inst.flag_enable) {
              E("\ts->V = 0;\n");
              E("\ts->N = 0;\n");
              E("\ts->Z = (%s == 0);\n", R[inst.info.rf_wa0]);
            }
          } E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }           

        case OpCode::DIV:
        {
          E_OPEN_SCOPE {
          E_IF_CC (inst)
          E("\tsint32 d = ((sint32)(%s));\n", src2);
          E("\tif (d != 0) {\n");
            E("\t\tsint32 q = (sint32)(%s);\n", src1);
            E("\t\tif ((d != -1) || (q != 0x80000000)) {\n");
              E_COMMIT(1)
              E("\t\t\tq = q / d;\n");
              E("\t\t\t%s = (sint32)q;\n", R[inst.info.rf_wa0]);
              if (inst.flag_enable) {
                E("\t\t\ts->V = 0;\n");
                E("\t\t\ts->N = (q < 0);\n");
                E("\t\t\ts->Z = (q == 0);\n");
              }
            E("\t\t} else {\n");
              if (inst.flag_enable) {
                E_COMMIT(0)
                E("\t\t\ts->V = 1;\n");
            }
            E("\t\t}\n");
          E("\t} else {\n");
            E("\t\tif (s->DZ != 0) {\n");
              E("\t\tcpuEnterException (%s, 0x%08x, 0x%08x, 0x%08x);\n",
                kSymCpuContext, ECR(work_unit.cpu->sys_arch.isa_opts.EV_DivZero, 0, 0), pc_cur, pc_cur);
              if (sim_opts.trace_on) E("\tcpuTraceException (%s, 0);\n", kSymCpuContext);
            E("\t}\n");
            if (inst.flag_enable) {
              E("\telse {\n");
                E_COMMIT(0)
                E("\t\ts->V = 1;\n");
                E("\t\ts->N = 0;\n");
                E("\t\ts->Z = 0;\n");                
              E("\t}\n");               
            }              
          E("\t}\n");               
          E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }
        
        case OpCode::DIVU:
        {
          E_OPEN_SCOPE {
          E_IF_CC (inst)
            E("\tuint32 d = ((uint32)(%s));\n", src2);
            E("\tif (d != 0) {\n");
              E_COMMIT(1)
              E("\t\tuint32 q = (uint32)(%s) / d;\n", src1);
              E("\t\t%s = (uint32)q;\n", R[inst.info.rf_wa0]);
              if (inst.flag_enable) {
                E("\t\ts->V = 0;\n");
                E("\t\ts->N = (q < 0);\n");
                E("\t\ts->Z = (q == 0);\n");                
              }
            E("\t} else {\n");
              // divisor is '0'
              //
              E("\t\tif (s->DZ != 0) {\n");
                E("\t\tcpuEnterException (%s, 0x%08x, 0x%08x, 0x%08x);\n",
                  kSymCpuContext, ECR(work_unit.cpu->sys_arch.isa_opts.EV_DivZero, 0, 0), pc_cur, pc_cur);
                if (sim_opts.trace_on) E("\tcpuTraceException (%s, 0);\n", kSymCpuContext);
              E("\t}\n");
                if (inst.flag_enable) {
                  E("\telse {\n");
                    E_COMMIT(0)
                    E("\t\ts->V = 1;\n");
                    E("\t\ts->N = 0;\n");
                    E("\t\ts->Z = 0;\n");
                  E("\t}\n");
                }
            E("\t}\n");
          E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }
        
        case OpCode::REM:
        {
          E_OPEN_SCOPE {
          E_IF_CC (inst)
          E("\tsint32 d = ((sint32)(%s));\n", src2);
          E("\tif (d != 0) {\n");
            E("\t\tsint32 r = (sint32)(%s);\n", src1);
            E("\t\tif ((d != -1) || (r != 0x80000000)) {\n");
              E_COMMIT(1)
              E("\t\t\tr = r %% d;\n");
              E("\t\t\t%s = (sint32)r;\n", R[inst.info.rf_wa0]);
              if (inst.flag_enable) {
                E("\t\t\ts->V = 0;\n");
                E("\t\t\ts->N = (r < 0);\n");
                E("\t\t\ts->Z = (r == 0);\n");
              }
            E("\t\t} else {\n");
              if (inst.flag_enable) {
                E_COMMIT(0)
                E("\t\t\ts->V = 1;\n");
            }
            E("\t\t}\n");
          E("\t} else {\n");
            E("\t\tif (s->DZ != 0) {\n");
              E("\t\tcpuEnterException (%s, 0x%08x, 0x%08x, 0x%08x);\n",
                kSymCpuContext, ECR(work_unit.cpu->sys_arch.isa_opts.EV_DivZero, 0, 0), pc_cur, pc_cur);
              if (sim_opts.trace_on) E("\tcpuTraceException (%s, 0);\n", kSymCpuContext);
            E("\t}\n");
            if (inst.flag_enable) {
              E("\telse {\n");
                E_COMMIT(0)
                E("\t\ts->V = 1;\n");
                E("\t\ts->N = 0;\n");
                E("\t\ts->Z = 0;\n");                
              E("\t}\n");               
            }              
          E("\t}\n");               
          E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }

        case OpCode::REMU:
        {
          E_OPEN_SCOPE {
            E_IF_CC (inst)
            E("\tuint32 d = ((uint32)(%s));\n", src2);
            E("\tif (d != 0) {\n");
              E("\t\tuint32 r = (uint32)(%s) %% d;\n", src1);
              E("\t\t%s = (uint32)r;\n", R[inst.info.rf_wa0]);
              E_COMMIT(1)
              if (inst.flag_enable) {
                E("\t\ts->V = 0;\n");
                E("\t\ts->N = (r < 0);\n");
                E("\t\ts->Z = (r == 0);\n");                
              }
            E("\t} else {\n");
              E("\t\tif (s->DZ != 0) {\n");
                E("\t\tcpuEnterException (%s, 0x%08x, 0x%08x, 0x%08x);\n",
                  kSymCpuContext, ECR(work_unit.cpu->sys_arch.isa_opts.EV_DivZero, 0, 0), pc_cur, pc_cur);              
              if (sim_opts.trace_on) {
                E("\tcpuTraceException (%s, 0);\n", kSymCpuContext);
              }
              E("\t}\n");
              if (inst.flag_enable) {
                E("\telse {\n");
                  E_COMMIT(0)
                  E("\t\ts->V = 1;\n");
                  E("\t\ts->N = 0;\n");
                  E("\t\ts->Z = 0;\n");                
                E("\t}\n");               
              }              
            E("\t}\n");               
            E_ENDIF_CC (inst)
          } E_CLOSE_SCOPE
          break;
        }

        case OpCode::ENTER:
        {
          E_CHECK_ILLEGAL
          int     saves  = inst.shimm + (inst.link ? 1 : 0) + (inst.dslot ? 1 : 0);
          sint32  offset = (-saves) << 2;
          sint32  trace_offset = offset;
          
          if (saves != 0) { 
            // If the enter instruction is in a delay slot we need to SAVE
            // the already computed target pc.
            //
            if (inst_has_dslot) { E("\tpc_t = %s;\n", kSymPc); }

            // Get the current stack pointer to use as base address
            //
            E("\tmaddr = s->gprs[%d];\n", SP_REG); // SP == r28

            if (inst.link) {
              E_UOP_ST_R(BLINK, offset);
              offset += 4;
              trace_offset += 4;
            }
            
            // Save gprs from register 13 to 13+inst.shimm-1
            //
            if (inst.shimm != 0) {
              E("\tif (   !mem_write_word(s,0x%08x,maddr+%d,%s)", pc_cur, offset, R[13]);
              offset += 4;
              for (int i = 1; i < inst.shimm; i++) {
                E("\n\t    || !mem_write_word(s,0x%08x,maddr+%d,%s)", pc_cur, offset, R[13+i]);
                offset += 4;
              }
              E("\t) {\n");
                E_TRANS_INSNS_UPDATE(block_insns-1)
                E_PIPELINE_COMMIT
                if (sim_opts.trace_on) { E("\tcpuTraceCommit(%s,0);\n", kSymCpuContext); }
                E("\t\treturn;\n");
              E("\t}\n");                
            }
            if (sim_opts.trace_on) {
              for (int i = 0; i < inst.shimm; i ++)
                E_TRACE_UOP_ST_R(i+13, trace_offset+(4*i));
            }
            
            if (inst.dslot) {
              E_UOP_ST_R(FP_REG, offset);
            }
            
            E_UOP_SUB_SP(saves<<2);

            if (inst.dslot) {
              E_UOP_MOV(FP_REG, SP_REG);
            }
            
            // If the enter was in a delay slot we need to RESTORE the saved
            // target PC as it might have changed during memory access.
            //
            if (inst_has_dslot) { E("\t%s = pc_t;\n", kSymPc); }
          }
          break;
        }

        case OpCode::LEAVE:
        {
          E_CHECK_ILLEGAL
          int saved  = inst.shimm + (inst.link ? 1 : 0) + (inst.dslot ? 1 : 0);
          
          if (saved != 0) { 
            sint32  offset = 0;
            sint32  link_offset = 0;
            
            // If the leave_s instruction is in a delay slot we need to SAVE
            // the already computed target pc.
            //
            if (inst_has_dslot) { E("\tpc_t = %s;\n", kSymPc); }

            if (inst.dslot) {   // if frame-pointer saved, then sp <= fp
              E_UOP_MOV(SP_REG, FP_REG);
            }
            
            // Get the current stack pointer to use as base address
            //
            E("\tmaddr = s->gprs[%d];\n", SP_REG); // SP == r28

            if (inst.link) {    // if link-bit set, then restore blink
              E_UOP_LD_R(BLINK, offset);
              offset += 4;
              link_offset = 4;
            }
            
            // Restore gprs from register 13 to 13+inst.shimm-1
            //
            if (inst.shimm != 0) {
              E("\tif (   !mem_read_word(s,0x%08x,maddr+%d,%d)", pc_cur, offset, 13);
              offset += 4;
              for (int i = 1; i < inst.shimm; i++) {
                E("\n\t    || !mem_read_word (s,0x%08x,maddr+%d,%d)", pc_cur, offset, 13+i);
                offset += 4;
              }
              E("\t) {\n");
                E_TRANS_INSNS_UPDATE(block_insns-1)
                E_PIPELINE_COMMIT
                if (sim_opts.trace_on) { E("\tcpuTraceCommit(%s,0);\n", kSymCpuContext); }
                E("\t\treturn;\n");
              E("\t}\n");                
            }
            if (sim_opts.trace_on) {
              for (int i = 0; i < inst.shimm; i ++) {
                E_TRACE_UOP_LD_R(i+13, link_offset+(4*i));
                E("\tcpuTraceWriteBack(%s,%d,%d,&(%s),%d,%d,&(%s));\n",
                  kSymCpuContext, 62, 0, R[62], i+13, 1, R[i+13]);
              }
            }
            
            if (inst.dslot) {  //  if frame-pointer saved then restore fp
              E_UOP_LD_R(FP_REG, offset);
            }
            
            E_UOP_ADD_SP(saved<<2); // adjust stack pointer as required
            
            if (inst.info.isReturn) { // perform return jump if required
              E_UOP_J_SD();
              INDIRECT_CONTROL_TRANSFER_INST
            }

            // If the leave_s was in a delay slot we need to RESTORE the saved
            // target PC as it might have changed during memory access.
            //
            if (inst_has_dslot) { E("\t%s = pc_t;\n", kSymPc); }
            
          } else if (inst.info.isReturn) {
            E_UOP_J_S();
            INDIRECT_CONTROL_TRANSFER_INST
          }
          break;
        }
        
        // -------------------------------------------------------------------------
        // EIA Extension Instructions
        //
        case OpCode::EIA_DOP_F:
        {
          E_IF_CC (inst)
          E("\tcpuEiaDopF (%s, (void*)%#p, &(%s), %s, %s, %d);\n",
             kSymCpuContext, (void*)(inst.eia_inst), R[inst.info.rf_wa0], src1, src2, inst.flag_enable);
          E_ENDIF_CC (inst)
          break;
        }
        
        case OpCode::EIA_DOP:
        {
          E_IF_CC (inst)
          E("\tcpuEiaDop (%s, (void*)%#p, &(%s), %s, %s);\n",
             kSymCpuContext, (void*)(inst.eia_inst), R[inst.info.rf_wa0], src1, src2);
          E_ENDIF_CC (inst)
          break;
        }
        
        case OpCode::EIA_SOP_F:
        {
          E_IF_CC (inst)
          E("\tcpuEiaSopF (%s, (void*)%#p, &(%s), %s, %d);\n",
             kSymCpuContext, (void*)(inst.eia_inst), R[inst.info.rf_wa0], src2, inst.flag_enable);
          E_ENDIF_CC (inst)
          break;
        }
        
        case OpCode::EIA_SOP:
        {
          E_IF_CC (inst)
          E("\tcpuEiaSop (%s, (void*)%#p, &(%s), %s);\n",
             kSymCpuContext, (void*)(inst.eia_inst), R[inst.info.rf_wa0], src2);
          E_ENDIF_CC (inst)
          break;
        }
            
        case OpCode::EIA_ZOP_F:
        {
          E_IF_CC (inst)
          E("\tcpuEiaZopF (%s, (void*)%#p, &(%s), %d);\n",
             kSymCpuContext, (void*)(inst.eia_inst), R[inst.info.rf_wa0], inst.flag_enable);
          E_ENDIF_CC (inst)
          break;
        }
        
        case OpCode::EIA_ZOP:
        {
          E_IF_CC (inst)
          E("\tcpuEiaZop (%s, (void*)%#p, &(%s));\n",
             kSymCpuContext, (void*)(inst.eia_inst), R[inst.info.rf_wa0]);
          E_ENDIF_CC (inst)
          break;
        }

        case OpCode::EXCEPTION:
        default:
        {
          LOG(LOG_WARNING)
            << "[CODE-GEN] PC = '0x" << HEX(pc_cur)
            << "', OPCODE = '" << OpCode::to_string(static_cast<OpCode::Op>(inst.code)) 
            << "', IR = '0x"   << HEX(inst.info.ir) << "'.";
          success = false;
          break;
        }
      } /* END switch (inst.code) */
      
      // #########################################################################
      
#ifdef CYCLE_ACC_SIM
      // Cycle Approx Pipeline
      //
      if (sim_opts.cycle_sim) {
        if (!pipeline_updated) {
          E_PIPELINE_UPDATE
        }
        
        // Call cycle accurate end of instruction hook
        //
        pipeline.jit_emit_instr_end(buf, inst, pc_cur, work_unit.cpu->cnt_ctx, sim_opts);      
      }
#endif /* CYCLE_ACC_SIM */
      
      // Record killeds frequencies
      //
      if (sim_opts.is_killed_recording_enabled) {
        if (is_conditional) {
          E("\tif (!commit) { ++(*((uint32 * const)(%#p))); };\n",
            work_unit.cpu->cnt_ctx.killed_freq_hist.get_value_ptr_at_index(pc_cur));
        }
      }
      
      // End of instruction translation
      //
      E_COMMENT("\t// -- END INSTRUCTION\n");
      
      // Trace code
      //
      if (sim_opts.trace_on) {
        // Emit code to synchronize the timing model prior to next trace
        //
        E_PIPELINE_COMMIT

        // Emit code to trace the register writes from this instruction
        //
        E("\tcpuTraceWriteBack(%s,%d,%d,&(%s),%d,%d,&(%s));\n",
             kSymCpuContext, 
             inst.info.rf_wa0, inst.info.rf_wenb0, R[inst.info.rf_wa0], 
             inst.info.rf_wa1, inst.info.rf_wenb1, R[inst.info.rf_wa1]);
          
        // Don't emit trace code for the last instruction, this happens a bit
        // later in order to handle ZOL loopback and lp_count tracing correctly
        //
        if (distance(TI, TE) > 1) {
          if (is_conditional) {
            E("\tcpuTraceCommit(%s,commit);\n", kSymCpuContext);
          } else {
            E("\tcpuTraceCommit(%s,1);\n", kSymCpuContext);
          }
        }
      }
      
      // Now set the 'inst_has_dslot' state variable in preparation for
      // the next instruction based on whether this instruction is a
      // delayed-branch. The ENTER and LEAVE instructions re-use the dslot
      // bit, so they must be checked for explicitly.
      //
      if (!inst_has_dslot) inst_has_dslot =  inst.dslot 
                                            && (inst.code != OpCode::ENTER)
                                            && (inst.code != OpCode::LEAVE)
                                            ;
      
      // Move on to next instruction in the block
      //
      pc_cur  = pc_nxt;

      // Clear the flag that indicates this is the first instruction in the block
      //
      is_first_insn = false;
      
      // If we have translated a load, store, LR or SR instruction, and there
      // exist watchpoints on such operations, then check_mem_aux_watchpoints
      // will be true, and we must then check to see if they triggered.
      // If external parameter watchpoints are defined, we will emit a check 
      // for pending_actions being non-zero after every instruction.
      // In both cases we return immediately if any action is required.
      //
      if (   check_mem_aux_watchpoints
          || work_unit.cpu->aps.has_extparam_aps()) {
        E("\tif (s->pending_actions & kPendingAction_WATCHPOINT) return;\n");
      }        
    } /* END ITERATE OVER ALL INSTRUTIONS IN BLOCK */
    
    ASSERT(pc_nxt != kInvalidPcAddress);
    
    // -----------------------------------------------------------------------
    // FIXUP PROGRAM COUNTER
    //
    // At the end of a block we need to compute the next PC value. This is
    // slightly complicated by the presence of delayed branches and ZOL.
    //
    if (inst_has_dslot)
    {
      // ---------------------------------------------------------------------
      // BLOCK HAS A DELAY SLOT - In this case we must emit code that does the
      // following:
      //
      //  - Clear D bit if branch is TAKEN and set PC to BTA correctly.
      //  - Decrement LP_COUNT iff this is the last instruction of a ZOL and
      //    loop-back is overriden by delayed branch.
      //  - If branch is NOT TAKEN and this is the last instruction of a ZOL
      //    emit appropriate code for loop-back.
      //
      pc_updated = true;
      // If branch was TAKEN (i.e. Status.DE bit is set) this code clears the 
      // Status.DE bit and copies BTA to PC. 
      //
      E("\tif (s->D == 1) {\n");
      E("\t\ts->D = 0;\n");
      E("\t\t%s = %s;\n", kSymPc, kSymBta);
      // Decrement LP_COUNT if this is the last instruction of a ZOL and
      // loop-back is overriden by delayed branch
      //
      if (work_unit.cpu->is_end_of_zero_overhead_loop(work_unit.lp_end_to_lp_start_map,
                                                      pc_nxt))
      {
        E("\t\t--s->next_lpc;\n");
        E("\t\t--s->gprs[%d];\n", LP_COUNT);
        if (sim_opts.trace_on) { E("\t\tcpuTraceLpCount(%s);\n", kSymCpuContext); }
      }
      
      E("\t}");
      // If branch was NOT TAKEN and this instruction is the end of a ZOL, we
      // need to emit the appropriate loop-back logic.
      //
      if (work_unit.cpu->is_end_of_zero_overhead_loop(work_unit.lp_end_to_lp_start_map,
                                                      pc_nxt))
      {
        loopback_checked = true;
        loopback_end_pc  = pc_nxt;
        
        E(" else {\n");
          emit_zero_overhead_loop_back(buf,   sim_opts, work_unit, pc_nxt,
                                       false, last_zol_inst_modifies_lp_count);
        E("\t}\n");
        INDIRECT_CONTROL_TRANSFER_INST
      } else {
        E("\n");
      }
    } else {
      // ---------------------------------------------------------------------
      // BLOCK HAS NO DELAY SLOT - In this case we only need to check if last
      // instruction is the end of a ZOL in which case we need to emit appropriate
      // loop-back logic.
      //
      if (!loopback_checked
          && work_unit.cpu->is_end_of_zero_overhead_loop(work_unit.lp_end_to_lp_start_map,
                                                         pc_nxt)) {          
        E_ZOL_END_TEST
      }
    }
    
    // ---------------------------------------------------------------------------
    // After block has been translated we might have to take care of direct/indirect
    // control transfer.
    //

    // If block terminates without explicitly setting state.pc
    // then it must be assigned with the next linear pc value
    // that has just been assigned to pc_cur
    //
    if (!pc_updated) E("\t%s = 0x%08x;\n", kSymPc, pc_cur);
          
    // Update translated instruction count
    //
    E_TRANS_INSNS_UPDATE(block_insns);
    
    // Now we can emit trace code for last instruction
    //
    if (sim_opts.trace_on) {
      if (is_conditional) {
        E("\tcpuTraceCommit(%s,commit);\n", kSymCpuContext);
      } else {
        E("\tcpuTraceCommit(%s,1);\n", kSymCpuContext);
      }
    }
    
    // ---------------------------------------------------------------------
    // Instrumentation PoinT hooks
    //
    if (work_unit.cpu->ipt_mgr.is_enabled()) {
      // BeginBasicBlockInstructionIPT
      //
      if (work_unit.cpu->ipt_mgr.is_enabled(arcsim::ipt::IPTManager::kIPTBeginBasicBlockInstruction)) {
        E_COMMENT("\t// -- kIPTBeginBasicBlockInstruction\n");
        E("\tcpuIptNotifyBeginBasicBlockExecution(%s,%s);\n", kSymCpuContext, kSymPc);
      }
    }
    
    // ---------------------------------------------------------------------------
    // If timers are implemented using instruction count as a proxy for cycles,
    // then check whether the number of instructions has exceeded the timer
    // expiry limit. If so, then call cpuTimerSync, which will in turn set
    // s->pending_actions if an interrupt has been raised. This code only runs
    // 
    // Timer expiry check, used only when cycle-accurate mode is disabled.
    //
    if (check_inst_timer) {
      E("\tif (((*((uint64 * const)(%#p))) + (*((uint64 * const)(%#p)))) >= s->timer_expiry) cpuTimerSync(%s);\n",
        work_unit.cpu->cnt_ctx.native_inst_count.get_ptr(),
        work_unit.cpu->cnt_ctx.interp_inst_count.get_ptr(),
        kSymCpuContext);
    }
    
    // -------------------------------------------------------------------------
    // DETERMINE WHERE TO GO NEXT
    // 
    //
    switch (sim_opts.fast_trans_mode) {
        
      // kCompilationModePageControlFlowGraph
      //    We need to determine where to go after this block. This means
      //    we need to emit code that performs the necessary jumps to target
      //    blocks for direct and indirect control transfers if the jumps
      //    are within this translation unit.
      //
      case kCompilationModePageControlFlowGraph:
      { 
        // Emit dynamic check that determines if we should leave native mode
        // and return to the interpreter by conducting the following checks: 
        //    1. decrement block iteration count
        //    2. check for interrupts of any kind and return
        //
        E("\tif ((!--s->iterations) || (s->pending_actions != kPendingAction_NONE)) {\n");
          E_PIPELINE_COMMIT
          E("\t\treturn;\n");          
        E("\t}\n");
        
        // ---------------------------------------------------------------------
        // DIRECT - Block Control Transfer within translation unit
        //
        if (is_direct_block_control_transfer_inst) {
          E_COMMENT("\t// == DIRECT CONTROL TRANSFER\n");
          for (std::list<TranslationBlockUnit*>::const_iterator I = blocks.begin(), E = blocks.end();
               I != E; ++I)
          {
            const arcsim::profile::BlockEntry& b = (*I)->entry_;
            
            // check if fall-through or branch-target block is present
            if (   b.virt_addr == direct_block_control_transfer_target_addr
                || b.virt_addr == direct_block_control_transfer_fall_through_addr)
            {
              E("\tif (%s == 0x%08x) { goto BLK_0x%08x; }\n",
                kSymPc, b.virt_addr, b.virt_addr);
            }
          }            
        }
        
        // ---------------------------------------------------------------------
        // INDIRECT - block control transfer within translation unit from current block
        //
        if (is_indirect_block_control_transfer_inst) {
          if (!block.edges_.empty()) {
            E_COMMENT("\t// == INDIRECT CONTROL TRANSFER\n");
            // edges from this block 
            for (std::list<arcsim::profile::BlockEntry*>::const_iterator I = block.edges_.begin(),
                  E = block.edges_.end(); I != E; ++I)
            {
              E("\tif (%s == 0x%08x) { goto BLK_0x%08x; }\n",
                kSymPc, (*I)->virt_addr, (*I)->virt_addr);
            } 
          }
        }
        
        // ---------------------------------------------------------------------
        // IMPLICIT - indirect block control transfer
        //
        if (loopback_checked) {
          // check if ZOL loop-back address is within the same work unit
          std::map<uint32,uint32>::const_iterator ZOL = work_unit.lp_end_to_lp_start_map.find(loopback_end_pc);
          
          if (ZOL != work_unit.lp_end_to_lp_start_map.end()) {
            for (std::list<TranslationBlockUnit*>::const_iterator I = blocks.begin(), E = blocks.end();
                 I != E; ++I)
            {
              const arcsim::profile::BlockEntry& b = (*I)->entry_;
              if (b.virt_addr == ZOL->second) {
                E_COMMENT("\t// == ZOL LOOPBACK CHECK\n");
                // emit direct jump if ZOL loop back PC is within translation work unit
                E("\tif (%s == 0x%08x) { goto BLK_0x%08x; }\n",
                  kSymPc, b.virt_addr, b.virt_addr);
                // return early if loop back pc has been found
                break;  
              }
            }              
          }
        }
        
        // Update Pipeline
        //
        E_PIPELINE_COMMIT
        
        // End of block return - if the previously emitted code didn't figure out where
        // to jump to, then return to main simulation loop.
        //
        E("\treturn;\n");

        break;
      }
        
      // kCompilationModeBasicBlock
      //
      case kCompilationModeBasicBlock:
      {  
        // Decrement iteration count
        //
        E("\t--s->iterations;\n");

        // Update Pipeline
        //
        E_PIPELINE_COMMIT
        break;
      }
    }
    
    // CLOSE SCOPE - kCompilationModeBasicBlock
    //
    if (sim_opts.fast_trans_mode == kCompilationModeBasicBlock) {
      E("}\n");
    }      

  } /* END ITERATE OVER ALL BLOCKS IN PATH */

  // CLOSE SCOPE - kCompilationModePageControlFlowGraph
  //
  if (sim_opts.fast_trans_mode == kCompilationModePageControlFlowGraph) {
    E("}\n");
  }      
  
  // Check for code buffer overflow
  if (buf.is_full()) {
    LOG(LOG_ERROR) << "[TW" << worker_id << "] Maximum tranlsation unit size of '"
                   << buf.get_size() << "' bytes exceeded, skipping translation of trace.'";
    return false;
  }
  
  return success;
}

/**
 * Emit Zero Overhead Loop logic
 *
 */
static void
emit_zero_overhead_loop_back(arcsim::util::CodeBuffer&  buf,
                             SimOptions&                sim_opts,
                             const TranslationWorkUnit& work_unit,
                             uint32                     next_pc,
                             bool                       update_to_next_pc,
                             bool                       last_zol_updates_lp_count)
{
  E_COMMENT("\t// -- LOOP END TEST\n"); 
  
  // IF (ZOL LOOPBACK ENABLED && THIS IS 
  //
  E("\tif (!s->L && (s->lp_end == 0x%08x)) {\n", next_pc);
  
  // IF (LP_COUNT != 0) THEN 'loop back'
  //
  // We 'loop back' by setting 's->pc' to 's->lp_start'.
  //
  if (last_zol_updates_lp_count) { E("\t\tif (--s->next_lpc != 0) {\n"); }
  else                           { E("\t\tif (--s->gprs[%d] != 0) {\n", LP_COUNT); }
  
  // Set 's->pc' to 's->lp_start'
  //
  E("\t\t\t%s = s->lp_start;\n", kSymPc);
    
  if (sim_opts.trace_on) { E("\t\t\tcpuTraceLpBack(%s);\n", kSymCpuContext); }
  
  // ELSE 'fall through'
  //
  // We 'fall through' by setting 's->pc' to 'next_pc' but only if the next PC
  // has not been set before which is indicated by 'update_to_next_pc'.
  //
  if (update_to_next_pc) {
    E("\t\t} else {\n");
      E("\t\t\t%s = 0x%08x;\n", kSymPc, PC_ALIGN(next_pc)); 
    E("\t\t}\n");
  } else {
    E("\t\t}\n");
  }

  if (sim_opts.trace_on) { E("\t\tcpuTraceLpCount(%s);\n", kSymCpuContext); }
  
  // In the special case where the last instruction of a ZOL modified LP_COUNT
  // (i.e. 'last_zol_updates_lp_count' is true) and the instruction did NOT
  // committ its value to LP_COUNT, we need to set LP_COUNT to 's->next_lpc'
  //
  if (last_zol_updates_lp_count) {
    E("\t\tif (!commit) { s->gprs[%d] = s->next_lpc; }\n", LP_COUNT);
  }
  
  // Finally, if ZOL loop-back is DISABLED, we need to set 's->pc' but only if it
  // has not been set before
  //
  if (update_to_next_pc) {
    E("\t} else {\n");
      E("\t\t\t%s = 0x%08x;\n", kSymPc, PC_ALIGN(next_pc)); 
    E("\t\t}\n");
  } else {
    E("\t}\n");
  }
}

/**
 * Emit an if-condition based on the q-field in cc and the state of the current flags.
 * If tracing, then emit the code to assign the boolean result to 'cond'.
 */
static void
flag_conditional (arcsim::util::CodeBuffer& buf,
                  const arcsim::isa::arc::Dcode& inst,
                  bool tracing)
{
  const char *cs1 = tracing ? "((commit=" : "";
  const char *cs2 = tracing ? "))" : "";
  
  switch ( inst.q_field ) {
    case 0:  E("\t"); break;                        // true
    case 1:  E("\tif %s(s->Z)%s ", cs1, cs2);    break;
    case 2:  E("\tif %s(!s->Z)%s ", cs1, cs2);   break;
    case 3:  E("\tif %s(!s->N)%s ", cs1, cs2);   break;
    case 4:  E("\tif %s(s->N)%s ", cs1, cs2);    break;  
    case 5:  E("\tif %s(s->C)%s ", cs1, cs2);    break;
    case 6:  E("\tif %s(!s->C)%s ", cs1, cs2);   break;
    case 7:  E("\tif %s(s->V)%s ", cs1, cs2);    break;
    case 8:  E("\tif %s(!s->V)%s ", cs1, cs2);   break;
    case 9:  E("\tif %s((s->N && s->V && !s->Z) || (!s->N && !s->V && !s->Z))%s ", cs1, cs2); break;
    case 10: E("\tif %s((s->N && s->V) || (!s->N && !s->V))%s ", cs1, cs2); break;
    case 11: E("\tif %s(s->N != s->V)%s ", cs1, cs2); break;
    case 12: E("\tif %s(s->Z || (s->N != s->V))%s ", cs1, cs2); break;
    case 13: E("\tif %s(!s->C && !s->Z)%s ", cs1, cs2); break;
    case 14: E("\tif %s(s->C || s->Z)%s ", cs1, cs2);   break;
    case 15: E("\tif %s(!s->N && !s->Z)%s ", cs1, cs2); break;
    default: {
      if (inst.eia_cond) {
        E("\tif %s(cpuEiaCond (%s, (void*)%#p, %d))%s ",
          kSymCpuContext, cs1, (void*)inst.eia_cond, inst.q_field, cs2);
      } else {
        if (inst.q_field == 16) {
          E("\tif %s((s->auxs[0x%03x] & 0x00000210) != 0)%s ", cs1, AUX_MACMODE, cs2);
        } else if (inst.q_field == 17) {
          E("\tif %s((s->auxs[0x%03x] & 0x00000210) == 0)%s ", cs1, AUX_MACMODE, cs2);
        }
      }
    } 
  }
}

// Detect zero and MSB set
//
static void
emit_set_ZN_noasm(arcsim::util::CodeBuffer& buf,
                  const char *var,   // variable based on which to set flags
                  const bool set_Z,  // set Z flag
                  const bool set_N)  // set N flag
{
  if (set_Z) { E("\t\ts->Z = ((uint32)%s == 0);\n", var); }// need the uint32 cast for the cases where 'var' is a uint64
  if (set_N) { E("\t\ts->N = ((sint32)%s < 0);\n", var); }
}

// Detect overflow and carry
// WARNING: Make sure 'var' is not the same as 'op1' or 'op2' because that would mean that you've
//          already overwritten one of the operands!
//
static void
emit_set_CV_noasm(arcsim::util::CodeBuffer& buf,
                  const char *dst,   // variable based on which to set flags (must be 64bit)
                  const bool set_C,  // set C flag
                  const bool set_V,  // set V flag
                  const char *op1,   //  ... operand 1 for overflow calculation
                  const char *op2,   //  ... operand 2 for overflow calculation
                  const bool is_subtraction)
{
  // Code inspired by http://teaching.idallen.com/cst8281/10w/notes/040_overflow.txt
  //
  if (set_V) {
    if (!is_subtraction) {
      // Addition overflow has occured under the following conditions:
      //  MSBop1=1(-), MSBop2=1(-), MSBdst=0(+) or MSBop1=0(+), MSBop2=0(+), MSBdst=1(-)
      E("\t\ts->V = ((((%s) & (%s) & ~((uint32)(%s))) | (~(%s) & ~(%s) & (uint32)(%s))) & 0x80000000) != 0;\n",
           op1, op2, dst, op1, op2, dst);
    } else {
      // Subtraction overflow has occured under the following conditions:
      //  MSBop1=0(+), MSBop2=1(-), MSBdst=1(-) or MSBop1=1(-), MSBop2=0(+), MSBdst=0(+)
      E("\t\ts->V = (((~(%s) & (%s) & (uint32)(%s)) | ((%s) & ~(%s) & ~((uint32)(%s)))) & 0x80000000) != 0;\n",
           op1, op2, dst, op1, op2, dst);
    }
  }
  
  if (set_C) E("\t\ts->C = (%s > 0xFFFFFFFF);\n", dst);  
}
                  
static void
emit_commutative_op(SimOptions& sim_opts,  // Simulation Options
                    arcsim::util::CodeBuffer& buf,
                    const arcsim::isa::arc::Dcode &i,        // ARC instruction
                    const char c_op,       // C-language operator (i.e. '&')
                    const char *x86_op,    // x86 operator (i.e. 'andl')
                    const char *dst,       // pointer to destination
                    const char *op1,       // pointer to first operand
                    const char *op2,       // pointer to second operand
                    const bool carry_in,   // add carry (turns an ADD into ADC)
                    const bool flag_enabled)
{ 
  if (flag_enabled) {
    IF_INLINE_ASM {
      if (carry_in)
        E("\t__asm__ __volatile__ (\"bt $0, %%0\" : : \"m\"(s->C) : \"cc\");\n"); 
      
      if ((i.info.rf_renb0) && (i.info.rf_wa0 == i.info.rf_ra0) && (i.info.rf_wa0 != LIMM_REG)) {
        // dst == src1
        //
        E("\t__asm__ __volatile__ (\"%s %%1, %%0\" : \"=m\"(%s) : \"r\"(%s) : \"cc\", \"memory\");\n",
             x86_op, dst, op2);
        if (i.z_wen) E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
        if (i.n_wen) E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
        if (i.c_wen) E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
        if (i.v_wen) E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
      } else if ((i.info.rf_renb1) && (i.info.rf_wa0 == i.info.rf_ra1) && (i.info.rf_wa0 != LIMM_REG)) {
        // dst == src2
        //
        E("\t__asm__ __volatile__ (\"%s %%1, %%0\" : \"=m\"(%s) : \"r\"(%s) : \"cc\", \"memory\");\n",
             x86_op, dst, op1);   
        if (i.z_wen) E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
        if (i.n_wen) E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
        if (i.c_wen) E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
        if (i.v_wen) E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
      } else {
        // dst != src1 and dst != src2
        //      
        E("\t__asm__ __volatile__ (\"%s %%1, %%0\" : \"=r\"(t1) : \"r\"(%s), \"0\"(t1) : \"cc\");\n",
             x86_op, op2);
        if (i.z_wen) E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
        if (i.n_wen) E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
        if (i.c_wen) E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
        if (i.v_wen) E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
        // Store value from 't1' back to 'dst'
        //
        E("\t%s = t1;\n", dst);
      }
    } ELSE_INLINE_ASM {
      if (carry_in) {
        if (i.c_wen || i.v_wen) {
          // Two steps: 1. handle the carry in, 2. do the regular op
          //
          E("\t\t{\n");
            // data64 = src1 OP C;
            //
            E("\t\tuint64 data64 = (uint64)%s %c s->C;\n", op1, c_op);
          
            // Check whether we have overflow, if yes, do not overwrite this flag
            // in the next op
            //
            emit_set_CV_noasm(buf, "data64", false, i.v_wen, op1, "s->C");
            
            // data64 OP= src2
            //
            E("\t\t\tdata64 %c= %s\n;", c_op, op2);
          
            // No overflow so far, check whether we have it now
            //
            E("\t\tif(!s->V) {\n"); 
            emit_set_CV_noasm(buf, "data64", false, i.v_wen, R[i.info.rf_wa0], op2);
            E("\t\t}\n");
            
            emit_set_CV_noasm(buf, "data64", i.c_wen);
            emit_set_ZN_noasm(buf, "data64", i.z_wen, i.n_wen);
            
            // Write result back into 'dest'
            //
            E("\t\t%s = (uint32)data64;\n", R[i.info.rf_wa0]);
          
          E("\t\t}\n");
        } else {
          E("\t%s = %s %c %s %c s->C;\n", R[i.info.rf_wa0], op1, c_op, op2, c_op);
          emit_set_ZN_noasm(buf, R[i.info.rf_wa0], i.z_wen, i.n_wen);
        }        
      } else { /* no carry in */
        if (i.c_wen || i.v_wen) {
          E("\t\t{\n");

          // data64 = src1 OP src2;
          //
          E("\t\tconst uint64 data64 = (uint64)%s %c %s;\n", op1, c_op, op2);
          emit_set_ZN_noasm(buf, "data64", i.z_wen, i.n_wen);
          emit_set_CV_noasm(buf, "data64", i.c_wen, i.v_wen, op1, op2);

          // Write result back into 'dest'
          //
          E("\t\t%s = (uint32)data64;\n", R[i.info.rf_wa0]);
          E("\t\t}\n");
        } else {
          // dest = src1 OP src2;
          //
          E("\t%s = %s %c %s;\n", R[i.info.rf_wa0], op1, c_op, op2);
          emit_set_ZN_noasm(buf, R[i.info.rf_wa0], i.z_wen, i.n_wen);
        }        
      }
    } END_INLINE_ASM;
  } else {
    E("\t%s = %s %c %s;\n", R[i.info.rf_wa0], op1, c_op, op2);
  }
  
}

static void
emit_noncommutative_op(SimOptions& sim_opts,
                       arcsim::util::CodeBuffer& buf,
                       const arcsim::isa::arc::Dcode &i, // ARC instruction
                       const char c_op,         // C-operator (i.e. '/')
                       const char *x86_op,      // x86 operator
                       const char *dst,         // pointer to destination
                       const char *op1,         // pointer to first operand
                       const char *op2,         // pointer to second operand
                       const bool carry_in,
                       const bool flag_enabled)
{
  if (flag_enabled) {
    IF_INLINE_ASM {
      if (carry_in) 
        E("\t__asm__ __volatile__ (\"bt $0, %%0\" : : \"m\"(s->C) : \"cc\");\n");
      
      if ((i.info.rf_renb0) && (i.info.rf_wa0 == i.info.rf_ra0) && (i.info.rf_wa0 != LIMM_REG)) {
        // dst == src1
        //
        E("\t__asm__ __volatile__ (\"%s %%1, %%0\" : \"=m\"(%s) : \"r\"(%s) : \"cc\", \"memory\");\n",
             x86_op, dst, op2);
        if (i.z_wen) E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
        if (i.n_wen) E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
        if (i.c_wen) E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
        if (i.v_wen) E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");   
      } else {
        // dst != src1
        //
        E("\tt1 = %s;\n", op1);
        E("\t__asm__ __volatile__ (\"%s %%1, %%0\" : \"=r\"(t1) : \"r\"(%s), \"0\"(t1) : \"cc\");\n",
             x86_op, op2);
        if (i.z_wen) E("\t__asm__ __volatile__ (\"setz %%0\" : \"=m\"(s->Z));\n");
        if (i.n_wen) E("\t__asm__ __volatile__ (\"sets %%0\" : \"=m\"(s->N));\n");
        if (i.c_wen) E("\t__asm__ __volatile__ (\"setc %%0\" : \"=m\"(s->C));\n");
        if (i.v_wen) E("\t__asm__ __volatile__ (\"seto %%0\" : \"=m\"(s->V));\n");
        // Store value from 't1' back to 'dst'
        //
        E("\t%s = t1;\n", dst);
      }
    } ELSE_INLINE_ASM {
      if (carry_in) {
        if (i.c_wen || i.v_wen) {
          E("\t\t{\n");
            // Two steps: 1) do the regular op, 2) handle the carry in
            //
          
            // data64 = src1 OP src2;
            //
            E("\t\tuint64 data64 = (uint64)%s %c %s;\n", op1, c_op, op2);
          
            // Check whether we have overflow
            //
            emit_set_CV_noasm(buf, "data64", false, i.v_wen, op1, op2, true);
            
            // data64 OP= C;
            //
            E("\t\t\tdata64 %c= s->C\n;", c_op);
          
            // Check for overflow
            //
            E("\t\tif(!s->V) {\n");
            emit_set_CV_noasm(buf, "data64", false, i.v_wen,
                              R[i.info.rf_wa0], "s->C", true);
            E("\t\t}\n");
            
            emit_set_CV_noasm(buf, "data64", i.c_wen);
            emit_set_ZN_noasm(buf, "data64", i.z_wen, i.n_wen);
              
            // write back to 'dest'
            //
            E("\t\t%s = (uint32)data64;\n", R[i.info.rf_wa0]);
          
          E("\t\t}\n");
        } else {
          E("\t%s = %s %c %s %c s->C;\n", R[i.info.rf_wa0], op1, c_op, op2, c_op);
          emit_set_ZN_noasm(buf, R[i.info.rf_wa0], i.z_wen, i.n_wen);
        }        
      } else {
        if (i.c_wen || i.v_wen) {
          E("\t\t{\n");
          E("\t\tconst uint64 data64 = (uint64)%s %c %s;\n", op1, c_op, op2);
          emit_set_ZN_noasm(buf, "data64", i.z_wen, i.n_wen);
          emit_set_CV_noasm(buf, "data64", i.c_wen, i.v_wen, op1, op2, true);
          E("\t\t%s = (uint32)data64;\n", R[i.info.rf_wa0]);
          E("\t\t}\n");
        } else {
          E("\t%s = %s %c %s;\n", R[i.info.rf_wa0], op1, c_op, op2);
          emit_set_ZN_noasm(buf, R[i.info.rf_wa0], i.z_wen, i.n_wen);
        }
      }
    } END_INLINE_ASM;
  } else {
    E("\t%s = %s %c %s;\n", R[i.info.rf_wa0], op1, c_op, op2);
  }
}

} } } // namespace arcsim::internal::translate

