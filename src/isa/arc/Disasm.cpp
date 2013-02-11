//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2004 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  This file defines member functions and various supporting structures
//  to disassemble ARCompact code.
//
// =====================================================================


// =====================================================================
// HEADERS
// =====================================================================

#include "Assertion.h"

#include "isa/arc/DcodeConst.h"
#include "isa/arc/Disasm.h"

#include "ise/eia/EiaInstructionInterface.h"

#include "sys/cpu/processor.h"
#include "sys/cpu/EiaExtensionManager.h"

// =====================================================================
// MACROS
// =====================================================================

#define DISASM_ALL 0

#define DISASM_TASK(_t_) \
  inline static void _t_(Disasm& d)

#define DISASM_TASK1(_t_,_a_) \
  inline static void _t_(Disasm& d, _a_)

#define DISASM_TASK2(_t_,_a_,_b_) \
  inline static void _t_(Disasm& d, _a_, _b_)

#define DISASM_TASK3(_t_,_a_,_b_,_c_) \
  inline static void _t_(Disasm& d, _a_, _b_, _c_)

#define DISASM_TASK4(_t_,_a_,_b_,_c_,_d_) \
  inline static void _t_(Disasm& d, _a_, _b_, _c_, _d_)

#define REG_A(r)   d.reg_a = reg_names[r];
#define REG_B(r)   d.reg_b = reg_names[r];
#define REG_C(r)   d.reg_c = reg_names[r];
#define DSLOT(v,b) if (BITSEL((v),(b))) d.dslot = dbit_str
#define QFIELD(q)  d.cc_test = cc_names[(q & 0x1f)];
#define BR_COND(q) d.cc_test = br_cond_names[(q & 0x1f)];
#define DST(r)     (r == limm_reg ? zero_str : r)
#define SRC(r)     (has_limm |= (r == limm_reg), (r == limm_reg) ? limm_str : r)

#define SET(_v_) _v_ = true
#define CLEAR(_v_) _v_ = false

#define COND_CODE_COUNT   32

// =====================================================================
// TYPES
// =====================================================================

namespace arcsim {
  namespace isa {
    namespace arc {


// Instruction mnemonic tag indices. The order in which these
// tags appear in this enumerator must match the order in which
// their corresponding disassembly mnemonics appear in op_name[],
// declared in disasm.cpp
//
enum INS_STR_TAGS {
  LD_STR,      ST_STR,     LR_STR,      SR_STR,
  MOV_STR,     ADD_STR,    ADC_STR,     SUB_STR,
  SUBC_STR,    RSUB_STR,   ADD1_STR,    ADD2_STR,
  ADD3_STR,    SUB1_STR,   SUB2_STR,    SUB3_STR,
  EXTB_STR,    SEXB_STR,   TST_STR,     CMP_STR,
  RCMP_STR,    MIN_STR,    MAX_STR,     ABS_STR, 
  NOT_STR,     AND_STR,    OR_STR,      BIC_STR, 
  XOR_STR,     ASL_STR,    LSR_STR,     ASR_STR, 
  ROR_STR,     RRC_STR,    RLC_STR,     BCLR_STR,
  BMSK_STR,    BSET_STR,   BTST_STR,    BXOR_STR,
  BCC_STR,     BRCC_STR,   BLCC_STR,    BBIT0_STR,
  BBIT1_STR,   JCC_STR,    JLCC_STR,    LP_STR,  
  FLAG_STR,    SLEEP_STR,  TRAP0_STR,   LD_S_STR, 
  ST_S_STR,    PUSH_S_STR, POP_S_STR,   MOV_S_STR, 
  ADD_S_STR,   SUB_S_STR,  NEG_S_STR,   ADD1_S_STR,
  ADD2_S_STR,  ADD3_S_STR, EXTW_STR,    SEXW_STR, 
  ABS_S_STR,   NOT_S_STR,  AND_S_STR,   OR_S_STR,  
  XOR_S_STR,   ASL_S_STR,  LSR_S_STR,   ASR_S_STR, 
  BCLR_S_STR,  BMSK_S_STR, BSET_S_STR,  BTST_S_STR,
  BCC_S_STR,   BRCC_S_STR, BL_S_STR,    J_S_STR,  
  JEQ_S_STR,   JNE_S_STR,  JL_S_STR,    BRK_S_STR,
  NOP_S_STR,   CMP_S_STR,  BIC_S_STR,   TST_S_STR,
  MUL_S_STR,   SEXB_S_STR, SEXW_S_STR,  EXTB_S_STR,
  EXTW_S_STR,  TRAP_S_STR, RTIE_STR,    EX_STR,  
  MPYLO_STR,   MPYHI_STR,  MPYLOU_STR,  MPYHIU_STR,
  //
  // Extended arithmetic tags (ARC 700)
  //
  ABSS_STR,    ABSSW_STR,  ADDS_STR,    ADDSDW_STR,
  DIVAW_STR,   ASLS_STR,   ASRS_STR,    NEGS_STR,  
  NEGSW_STR,   NORM_STR,   NORMW_STR,   RND16_STR, 
  SAT16_STR,   SUBS_STR,   SUBSDW_STR,  SWAP_STR,  
  //
  // ARC6000 instruction tags 
  //
  DIV_STR,     DIVU_STR,   REM_STR,     REMU_STR,
  //
  // MUL64 and MULU64 tags (ARC 600)
  //
  MUL64_STR,   MULU64_STR,
  //
  SWAPE_STR,   BMSKN_STR,  LSL16_STR,   LSR16_STR,
  ASR16_STR,   ASR8_STR,   LSR8_STR,    LSL8_STR,
  ROL8_STR,    ROR8_STR,   FFS_STR,     FLS_STR,
  JLI_S_STR,   LDI_STR,    LDI_S_STR,   EI_S_STR, 
  ROL_STR,     NOP_STR,    AEX_STR,     SETI_STR,
  CLRI_STR,
  // 
  SETEQ_STR,   SETNE_STR,  SETLT_STR,   SETGE_STR,
  SETLO_STR,   SETHS_STR,  SETLE_STR,   SETGT_STR,
  //
  MPYW_STR,    MPYWU_STR,  ENTER_S_STR, LEAVE_S_STR,
  LLOCK_STR,   SCOND_STR,  BI_STR,      BIH_STR,
  //
  INVALID_STR, SYNC_S_STR,
  //
  MPY_S_STR,   MPYW_S_STR, MPYUW_S_STR,
  //
  UNIMP_S_STR,
  //
  SWI_S_STR,   SWI_STR,    KFLAG_STR,
  // END MARKER
  OP_NAME_COUNT
};

// FPX instruction tag indices
////
enum INS_FPX_STR_TAGS {
  FMUL_STR,      FADD_STR,     FSUB_STR,     DMULH11_STR=8,
  DMULH12_STR,   DMULH21_STR,  DMULH22_STR,  DADDH11_STR,
  DADDH12_STR,   DADDH21_STR,  DADDH22_STR,  DSUBH11_STR,
  DSUBH12_STR,   DSUBH21_STR,  DSUBH22_STR,  DRSUBH11_STR,
  DRSUBH12_STR,  DRSUBH21_STR, DRSUBH22_STR, DEXCL1_STR,
  DEXCL2_STR,
  FP_OP_NAME_COUNT
};

// List of operand formats defined by the ARCompact ISA
//
//  These are used by the disassembler to determine how to
//  format the operand strings after emitting the mnemonic
//
// Non load/store formats
// (any format with a _SRC suffix has only source operands)
enum OPD_FORMATS {
  FMT_B_IND,     FMT_C_IND,      FMT_IMM,        FMT_B,
  FMT_B_C,       FMT_B_C_SRC,    FMT_B_IMM,      FMT_B_IMM_SRC,
  FMT_B_B_IMM,   FMT_A_B_C,      FMT_A_B_IMM,    FMT_B_B_C,
  FMT_B_B_B,     FMT_B_IMM_OFF,  FMT_B_C_OFF,    FMT_B_0_OFF,
  FMT_C_B,       FMT_C_B_SRC,    FMT_OFF,        FMT_C_B_IMM,
  FMT_B_C_IND,   FMT_B_IMM_IND,  FMT_ZOP,        FMT_C,
  // Load formats
  FMT_L_A_B_C,   FMT_L_A_B_IMM,  FMT_L_A_B,      FMT_L_B_C_IMM,
  FMT_L_C_B_IMM, FMT_L_C_B,
  // Store formats
  FMT_S_B_C_IMM, FMT_S_C_B_IMM,  FMT_S_C_B,      FMT_S_S6_B, 
  // enter_s/leave_s format
  FMT_MACRO,
  // ARCompact v2 formats
  FMT_H_H_S3,    FMT_HD_S3,    FMT_HS_S3
};



// =====================================================================
// GLOBAL VARIABLES
// =====================================================================
static const char *reg_names[GPR_BASE_REGS] = {
  "r0",   "r1",   "r2",   "r3",   "r4",   "r5",     "r6",     "r7",
  "r8",   "r9",   "r10",  "r11",  "r12",  "r13",    "r14",    "r15",
  "r16",  "r17",  "r18",  "r19",  "r20",  "r21",    "r22",    "r23",
  "r24",  "r25",  "gp",   "fp",   "sp",   "ilink1", "ilink2", "blink",
  "r32",  "r33",  "r34",  "r35",  "r36",  "r37",    "r38",    "r39",
  "r40",  "r41",  "r42",  "r43",  "r44",  "r45",    "r46",    "r47",
  "r48",  "r49",  "r50",  "r51",  "r52",  "r53",    "r54",    "r55",
  "r56",  "mlo",  "mmid", "mhi",  "r60",  "r61",    "limm",   "pcl"
};

static const char *limm_reg = reg_names[LIMM_REG];

static const char *br_cond_names[COND_CODE_COUNT] = {
  "",       "eq",     "ne",     "pl",
  "mi",     "cs",     "cc",     "vs",
  "vc",     "gt",     "ge",     "lt",
  "le",     "hi",     "ls",     "pnz",
  "ss",     "sc",     "<c18>",  "<c19>",
  "<c20>",  "<c21>",  "<c22>",  "<c23>",
  "<c24>",  "<c25>",  "<c26>",  "<c27>",
  "<c28>",  "<c29>",  "<c30>",  "<c31>" 
};

static const char *cc_names[COND_CODE_COUNT] = {
  "",      ".eq",   ".ne",    ".pl",
  ".mi",   ".cs",   ".cc",    ".vs",
  ".vc",   ".gt",   ".ge",    ".lt",
  ".le",   ".hi",   ".ls",    ".pnz",
  ".ss",   ".sc",   ".<c18>", ".<c19>",
  ".<c20>",".<c21>",".<c22>", ".<c23>",
  ".<c24>",".<c25>",".<c26>", ".<c27>",
  ".<c28>",".<c29>",".<c30>", ".<c31>"
};

// These names have %s fields to allow optional extension fields to
// be inserted using calls to snprintf.
//
static const char *op_name[OP_NAME_COUNT] = {
  "ld%s%s%s%s",   "st%s%s%s%s",   "lr%s%s",     "sr%s%s",
  "mov%s%s",      "add%s%s",      "adc%s%s",    "sub%s%s",
  "sbc%s%s",      "rsub%s%s",     "add1%s%s",   "add2%s%s",
  "add3%s%s",     "sub1%s%s",     "sub2%s%s",   "sub3%s%s",
  "extb%s%s",     "sexb%s%s",     "tst%s%s",    "cmp%s%s",
  "rcmp%s%s",     "min%s%s",      "max%s%s",    "abs%s%s",
  "not%s%s",      "and%s%s",      "or%s%s",     "bic%s%s",
  "xor%s%s",      "asl%s%s",      "lsr%s%s",    "asr%s%s",
  "ror%s%s",      "rrc%s%s",      "rlc%s%s",    "bclr%s%s",
  "bmsk%s%s",     "bset%s%s",     "btst%s%s",   "bxor%s%s",
  "b%s%s",        "br%s%s",       "bl%s%s",     "bbit0%s%s",
  "bbit1%s%s",    "j%s%s",        "jl%s%s",     "lp%s%s",
  "flag%s%s",     "sleep%s%s",    "trap0%s%s",  "ld%s%s%s%s_s",
  "st%s%s%s%s_s", "push_s%s%s",   "pop_s%s%s",  "mov_s%s%s",
  "add_s%s%s",    "sub_s%s%s",    "neg_s%s%s",  "add1_s%s%s",
  "add2_s%s%s",   "add3_s%s%s",   "extw%s%s",   "sexw%s%s",
  "abs_s%s%s",    "not_s%s%s",    "and_s%s%s",  "or_s%s%s",
  "xor_s%s%s",    "asl_s%s%s",    "lsr_s%s%s",  "asr_s%s%s",
  "bclr_s%s%s",   "bmsk_s%s%s",   "bset_s%s%s", "btst_s%s%s",
  "b%s_s%s",      "br%s_s%s",     "bl%s_s%s",   "j%s_s%s",
  "j%s_s%s",      "j%s_s%s",      "jl%s_s%s",   "brk%s%s_s",
  "nop_s%s%s",    "cmp_s%s%s",    "bic_s%s%s",  "tst_s%s%s",
  "mul_s%s%s",    "sexb_s%s%s",   "sexw_s%s%s", "extb_s%s%s",
  "extw_s%s%s",   "trap_s%s%s",   "rtie%s%s",   "ex%s%s",
  "mpylo%s%s",    "mpyhi%s%s",    "mpylou%s%s", "mpyhiu%s%s",
  "abss%s%s",     "abssw%s%s",    "adds%s%s",   "addsdw%s%s",
  "divaw%s%s",    "asls%s%s",     "asrs%s%s",   "negs%s%s",
  "negsw%s%s",    "norm%s%s",     "normw%s%s",  "rnd16%s%s",
  "sat16%s%s",    "subs%s%s",     "subsdw%s%s", "swap%s%s",
  "div%s%s",      "divu%s%s",     "rem%s%s",    "remu%s%s",
  "mul64%s%s",    "mulu64%s%s",
  "swape%s%s",    "bmskn%s%s",    "lsl16%s%s",  "lsr16%s%s",
  "asr16%s%s",    "asr8%s%s",     "lsr8%s%s",   "lsl8%s%s",
  "rol8%s%s",     "ror8%s%s",     "ffs%s%s",    "fls%s%s",
  "jli_s%s%s",    "ldi%s%s",      "ldi_s%s%s",  "ei_s%s%s",    
  "rol%s%s",      "nop%s%s",      "aex%s%s"  ,  "seti%s%s",
  "clri%s%s",     "seteq%s%s",
  "setne%s%s",
  "setlt%s%s",    "setge%s%s",    "setlo%s%s",  "seths%s%s",
  "setle%s%s",    "setgt%s%s",    "mpyw%s%s",   "mpywu%s%s",
  "enter_s {%s,%s,%s}",           "leave_s {%s,%s,%s,%s}",
  "llock%s%s",    "scond%s%s",    "bi%s%s",     "bih%s%s",
  "invalid-instr%s%s",            "sync%s%s",
  "mpy_s%s%s",    "mpyw_s%s%s",   "mpyuw_s%s%s",
  "unimp_s%s%s",  "swi_s%s%s",    "swi%s%s",    "kflag%s%s"
};

static const char *fp_op_name[FP_OP_NAME_COUNT] = {
    "fmul%s%s",     "fadd%s%s",     "fsub%s%s",     "???",
    "???",          "???",          "???",          "???",
    "dmulh11%s%s",  "dmulh12%s%s",  "dmulh21%s%s",  "dmulh22%s%s",
    "daddh11%s%s",  "daddh12%s%s",  "daddh21%s%s",  "daddh22%s%s",
    "dsubh11%s%s",  "dsubh12%s%s",  "dsubh21%s%s",  "dsubh22%s%s",
    "drsubh11%s%s", "drsubh12%s%s", "drsubh21%s%s", "drsubh22%s%s",
    "dexcl1%s%s",   "dexcl2%s%s"
};

static const char *null_str = "";
static const char *dbit_str = ".d";
static const char *half_str = "w";
static const char *byte_str = "b";
static const char *fbit_str = ".f";
static const char *zero_str = "0";
static const char *aw_str   = ".a";
static const char *ab_str   = ".ab";
static const char *as_str   = ".as";
static const char *ext_str  = ".x";
static const char *cenb_str = ".di";

static const char *enter_leave_regs_str[16] = {
  "",         "r13",      "r13-r14",  "r13-r15",
  "r13-r16",  "r13-r17",  "r13-r18",  "r13-r19",
  "r13-r20",  "r13-r21",  "r13-r22",  "r13-r23",
  "r13-r24",  "r13-r25",  "r13-r26",  "r13-r27"
};


// =====================================================================
//
// DISASSEMBLY TASK DEFINITIONS
//
// =====================================================================

DISASM_TASK(clear_task)
{
  d.buf[0]    = '\0';
  d.len       = 0;
  d.fmt       = 0;
  d.opcode    = null_str;
  d.cc_test   = null_str;
  d.dslot     = null_str;
  d.reg_a     = null_str;
  d.reg_b     = null_str;
  d.reg_c     = null_str;
  d.f_bit     = null_str;
  d.abs_val   = 0;
  d.int_val   = 0;
  d.flags[0]  = '-';
  d.flags[1]  = '-';
  d.flags[2]  = '-';
  d.flags[3]  = '-';
  d.cc_op     = false;
  d.invld_ins = false;
  d.has_limm  = false;
  d.is_16bit  = false;
  d.size_suffix = null_str;
  d.extend_mode = null_str;
  d.write_back_mode = null_str;
  d.cache_byp_mode  = null_str;
}


DISASM_TASK (inst_error_task) {
  d.invld_ins = true;
}

DISASM_TASK (init_local_regs_task) {
  d.flag_enable   = false;
  d.z_write   = false;
  d.n_write   = false;
  d.c_write   = false;
  d.v_write   = false;
}

DISASM_TASK (flag_enable_task) {
  if (d.z_write & d.flag_enable) d.flags[3] = 'Z';
  if (d.n_write & d.flag_enable) d.flags[2] = 'N';
  if (d.c_write & d.flag_enable) d.flags[1] = 'C';
  if (d.v_write & d.flag_enable) d.flags[0] = 'V';
}

/////////////////////////////////////////////////////////////
//
// Register operand and general format disassembly tasks
//
/////////////////////////////////////////////////////////////

DISASM_TASK (f_bit_task) {
  d.flag_enable = BITSEL(d.inst,15);
  if (d.flag_enable)
    d.f_bit = fbit_str;
}

DISASM_TASK (regs_q_32_task) {
  uint8 q_field = UNSIGNED_BITS(d.inst,4,0);
  d.cc_test     = cc_names[q_field];
  
  if (q_field > 15) {
    uint32 eia_cc_num = q_field - 16;
    if (d.eia_mgr.eia_cc_names[eia_cc_num]) {
      d.cc_test = d.eia_mgr.eia_pred_names[eia_cc_num]->c_str();
    } else {
      if (!d.isa_opts.sat_option || (q_field > 17)) {
        d.invld_ins = true;
      }
    }
  }
}

DISASM_TASK (cond_q_32_task) {
  uint8 q_field = UNSIGNED_BITS(d.inst,4,0);
  d.cc_test     = br_cond_names[q_field];
  
  if (q_field > 15) {
    uint32 eia_cc_num = q_field - 16;
    if (d.eia_mgr.eia_cc_names[eia_cc_num]) {
      d.cc_test = d.eia_mgr.eia_cc_names[eia_cc_num]->c_str();
    } else {
      if (!d.isa_opts.sat_option || (q_field > 17)) {
        d.invld_ins = true;
      }
    }
  }
}

DISASM_TASK (regs_a_32_task) {
  REG_A(UNSIGNED_BITS(d.inst,5,0));
}

DISASM_TASK (regs_c_32_task) {
  REG_C(UNSIGNED_BITS(d.inst,11,6));
}

DISASM_TASK (regs_cq_32_task) {
  regs_c_32_task (d);
  regs_q_32_task (d);
}

DISASM_TASK (regs_s12_32_task) {
  d.abs_val = (UNSIGNED_BITS(d.inst,5,0)<<6 | UNSIGNED_BITS(d.inst,11,6));
}

DISASM_TASK (regs_u6_32_task) {
  d.abs_val = UNSIGNED_BITS(d.inst,11,6);
}

DISASM_TASK (regs_b_32_task) {
  REG_B((UNSIGNED_BITS(d.inst,14,12)<<3 | UNSIGNED_BITS(d.inst,26,24)));
}

DISASM_TASK (regs_bu6_32_task) {
  regs_b_32_task (d);
  regs_u6_32_task (d);
}

DISASM_TASK (regs_bq_32_task) {
  regs_b_32_task (d);
  regs_q_32_task (d);
  f_bit_task (d);
}

DISASM_TASK (regs_bbq_32_task) {
  regs_b_32_task (d);
  regs_bq_32_task (d);
}

DISASM_TASK (regs_bu6q_32_task) {
  regs_bu6_32_task (d);    
  regs_q_32_task (d);
  f_bit_task (d);
}

DISASM_TASK (regs_bc_32_task) {
  regs_b_32_task (d);
  regs_c_32_task (d);;
}

DISASM_TASK (regs_abc_32_task) {
  regs_a_32_task (d);
  regs_bc_32_task (d);
  f_bit_task (d);
}

DISASM_TASK (regs_mov_bs12_task) {
  regs_b_32_task (d);
  d.abs_val = (SIGNED_BITS(d.inst,5,0)<<6 | UNSIGNED_BITS(d.inst,11,6));
  f_bit_task (d);
}

DISASM_TASK (regs_bs12_32_task) {
  regs_b_32_task (d);
  d.abs_val = (SIGNED_BITS(d.inst,5,0)<<6 | UNSIGNED_BITS(d.inst,11,6));
  f_bit_task (d);
}

DISASM_TASK (regs_bbs12_32_task) {
  regs_b_32_task (d);
  regs_bs12_32_task (d);
}

DISASM_TASK (regs_sop_bc_32_task) {
  regs_bc_32_task (d);
  f_bit_task (d);
}

DISASM_TASK (regs_sop_bu6_32_task) {
  regs_bu6_32_task (d);
  f_bit_task (d);
}

DISASM_TASK (regs_bcq_32_task) {
  regs_bc_32_task (d);
  regs_q_32_task (d);
  f_bit_task (d);
}

DISASM_TASK (regs_abu6_32_task) {
  regs_a_32_task (d);
  regs_bu6_32_task (d);
  f_bit_task (d);
}

DISASM_TASK (regs_mov_bc_task) {
  regs_bc_32_task (d);
}

DISASM_TASK (regs_mov_bu6_task) {
  regs_bu6_32_task (d);
}

DISASM_TASK (regs_lpcc_s12_task) {
  d.int_val = (SIGNED_BITS(d.inst,5,0)<<7 | UNSIGNED_BITS(d.inst,11,6)) << 1;
  d.fmt = FMT_OFF;
}

DISASM_TASK (regs_lpcc_u6_task) {
  d.int_val = (UNSIGNED_BITS(d.inst,11,6)) << 1;
  d.fmt = FMT_OFF;
}

DISASM_TASK (regs_lpcc_u6q_task) {
  d.int_val = UNSIGNED_BITS(d.inst,11,6) << 1;
  cond_q_32_task (d);
  d.fmt = FMT_OFF;
}

DISASM_TASK (regs_b_16_task) {
  REG_B((BITSEL(d.inst,26)<<3 | UNSIGNED_BITS(d.inst,26,24)));
}

DISASM_TASK (regs_c_16_task) {
  REG_C((BITSEL(d.inst,23)<<3 | UNSIGNED_BITS(d.inst,23,21)));
}
DISASM_TASK (regs_bbc_16_task) {
  regs_b_16_task (d);
  regs_c_16_task (d);
}

DISASM_TASK (regs_h_16_task) {
  if (d.isa_opts.is_isa_a6k()) {
    int r = (UNSIGNED_BITS(d.inst,17,16)<<3 | UNSIGNED_BITS(d.inst,23,21));
    
    if (r == 30)
      r = 62;
    
    REG_C(r);
  }
  else {
    REG_C((UNSIGNED_BITS(d.inst,18,16)<<3 | UNSIGNED_BITS(d.inst,23,21)));
  }
}

DISASM_TASK (regs_g_16_task) {
  int r = (UNSIGNED_BITS(d.inst,20,19)<<3) | UNSIGNED_BITS(d.inst,26,24);
    
  if (r == 30)
    r = 62;
    
  REG_B(r);
}

DISASM_TASK (regs_abc_16_task) {
  REG_A((BITSEL(d.inst,18)<<3 | UNSIGNED_BITS(d.inst,18,16)));
  regs_b_16_task (d);
  regs_c_16_task (d);
}

DISASM_TASK (regs_cbu3_16_task) {
  regs_c_16_task (d);
  regs_b_16_task (d);
  d.abs_val = UNSIGNED_BITS(d.inst,18,16);
}

DISASM_TASK (regs_bbh_16_task) {
  regs_b_16_task (d);
  regs_h_16_task (d);
}

DISASM_TASK (regs_hb_16_task) {
  regs_h_16_task (d);
  regs_b_16_task (d);
}

// -----

DISASM_TASK (regs_s3_16_task) {
  d.int_val = UNSIGNED_BITS(d.inst,26,24);
  if (d.int_val == 7)
    d.int_val = -1;
}

DISASM_TASK (regs_hs3_16_task) {
  regs_h_16_task (d);
  regs_s3_16_task(d);
}

// -----

DISASM_TASK (regs_bspu7_16_task) {
  regs_b_16_task (d);
  REG_C(28);
  d.abs_val = UNSIGNED_BITS(d.inst,20,16) << 2;
}

DISASM_TASK (regs_r0gps9_16_task) {
  REG_A(0);
  REG_B(26);
  d.abs_val = UNSIGNED_BITS(d.inst,24,16);
}

DISASM_TASK (regs_bbu5_16_task) {
  regs_b_16_task (d);
  d.abs_val = UNSIGNED_BITS(d.inst,20,16);
}

DISASM_TASK (regs_bu7_16_task) {
  regs_b_16_task (d);
  d.abs_val = UNSIGNED_BITS(d.inst,22,16);
}

DISASM_TASK (regs_bbu7_16_task) {
  regs_b_16_task (d);
  d.abs_val = UNSIGNED_BITS(d.inst,22,16);
}

DISASM_TASK (regs_mov_bu8_16_task) {
  regs_b_16_task (d);
  d.abs_val = UNSIGNED_BITS(d.inst,23,16);
}

DISASM_TASK (zero_operand_task) {
  clear_task (d);
}

/////////////////////////////////////////////////////////////
//
// Flag update decode tasks
//
/////////////////////////////////////////////////////////////

DISASM_TASK (enable_all_flag_writes) {
  SET (d.z_write);
  SET (d.n_write);
  SET (d.c_write);
  SET (d.v_write);
}

DISASM_TASK (enable_znc_flag_writes) {
  SET (d.z_write);
  SET (d.n_write);
  SET (d.c_write);
}

DISASM_TASK (enable_zn_flag_writes) {
  SET (d.z_write);
  SET (d.n_write);
}

/////////////////////////////////////////////////////////////
//
// Arithmetic instruction disassembly tasks
//
/////////////////////////////////////////////////////////////

DISASM_TASK (add_task) {
  d.opcode = op_name[ADD_STR];
  enable_all_flag_writes(d);
}

DISASM_TASK (adc_task) {
  d.opcode = op_name[ADC_STR];
  enable_all_flag_writes(d);
}

DISASM_TASK (sub_task) {
  d.opcode = op_name[SUB_STR];
  enable_all_flag_writes(d);
}

DISASM_TASK (sbc_task) {
  d.opcode = op_name[SUBC_STR];
  enable_all_flag_writes(d);
}

DISASM_TASK (and_task) {
  d.opcode = op_name[AND_STR];
  enable_zn_flag_writes(d);
}

DISASM_TASK (or_task) {
  d.opcode = op_name[OR_STR];
  enable_zn_flag_writes(d);
}

DISASM_TASK (xor_task) {
  d.opcode = op_name[XOR_STR];
  enable_zn_flag_writes(d);
}

DISASM_TASK (abs_task) {
  d.opcode = op_name[ABS_STR];
  enable_all_flag_writes(d);
}

DISASM_TASK (min_task) {
  d.opcode = op_name[MIN_STR];
  enable_all_flag_writes(d);
}

DISASM_TASK (max_task) {
  d.opcode = op_name[MAX_STR];
  enable_all_flag_writes(d);
}

DISASM_TASK (bic_task) {
  d.opcode = op_name[BIC_STR];
}

DISASM_TASK (mov_task) {
  d.opcode = op_name[MOV_STR];
  if (d.inst == 0x264a7000UL) {
    d.opcode = op_name[NOP_STR];
    d.fmt    = FMT_ZOP;
  }
}

DISASM_TASK (tst_task) {
  d.opcode = op_name[TST_STR];
  enable_zn_flag_writes(d);
}

DISASM_TASK (cmp_task) {
  d.opcode = op_name[CMP_STR];
  enable_all_flag_writes(d);
}

DISASM_TASK (rcmp_task) {
  d.opcode = op_name[RCMP_STR];
  enable_all_flag_writes(d);
}

DISASM_TASK (rsub_task) {
  d.opcode = op_name[RSUB_STR];
  enable_all_flag_writes(d);
}

DISASM_TASK (bset_task) {
  d.opcode = op_name[BSET_STR];
}

DISASM_TASK (bclr_task) {
  d.opcode = op_name[BCLR_STR];
}

DISASM_TASK (btst_task) {
  d.opcode = op_name[BTST_STR];
}

DISASM_TASK (bxor_task) {
  d.opcode = op_name[BXOR_STR];
}

DISASM_TASK (bmsk_task) {
  d.opcode = op_name[BMSK_STR];
}

DISASM_TASK (bmskn_task) {
  d.opcode = op_name[BMSKN_STR];
}

DISASM_TASK (add1_task) {
  d.opcode = op_name[ADD1_STR];
}

DISASM_TASK (add2_task) {
  d.opcode = op_name[ADD2_STR];
}

DISASM_TASK (add3_task) {
  d.opcode = op_name[ADD3_STR];
}

DISASM_TASK (sub1_task) {
  d.opcode = op_name[SUB1_STR];
}

DISASM_TASK (sub2_task) {
  d.opcode = op_name[SUB2_STR];
}

DISASM_TASK (sub3_task) {
  d.opcode = op_name[SUB3_STR];
}

DISASM_TASK (sub_s_ne_task) {
  d.opcode = op_name[SUB_S_STR];
  QFIELD(2);          // NE condition
  d.fmt = FMT_B_B_B;
}

DISASM_TASK1 (gen_sop_task, int opc) {
  d.opcode = op_name[opc];
}

DISASM_TASK (mpylo_task) {
  d.opcode = op_name[MPYLO_STR];
}

DISASM_TASK (mpyhi_task) {
  d.opcode = op_name[MPYHI_STR];
}

DISASM_TASK (mpylou_task) {
  d.opcode = op_name[MPYLOU_STR];
}

DISASM_TASK (mpyhiu_task) {
  d.opcode = op_name[MPYHIU_STR];
}

DISASM_TASK (mpyw_task) {
  d.opcode = op_name[MPYW_STR];
}

DISASM_TASK (mpywu_task) {
  d.opcode = op_name[MPYWU_STR];
}

DISASM_TASK (adds_task) {
  d.opcode = op_name[ADDS_STR];
}

DISASM_TASK (subs_task) {
  d.opcode = op_name[SUBS_STR];
}

DISASM_TASK (addsdw_task) {
  d.opcode = op_name[ADDSDW_STR];
}

DISASM_TASK (subsdw_task) {
  d.opcode = op_name[SUBSDW_STR];
}

DISASM_TASK (asls_task) {
  d.opcode = op_name[ASLS_STR];
}

DISASM_TASK (asrs_task) {
  d.opcode = op_name[ASRS_STR];
}

DISASM_TASK (divaw_task) {
  d.opcode = op_name[DIVAW_STR];
}

DISASM_TASK (div_task) {
  d.opcode = op_name[DIV_STR];
}

DISASM_TASK (mul64_task) {
  d.opcode = op_name[MUL64_STR];
}
      
DISASM_TASK (divu_task) {
  d.opcode = op_name[DIVU_STR];
}

DISASM_TASK (mulu64_task) {
  d.opcode = op_name[MUL64_STR];
}

DISASM_TASK (rem_task) {
  d.opcode = op_name[REM_STR];
}

DISASM_TASK (remu_task) {
  d.opcode = op_name[REMU_STR];
}

DISASM_TASK1 (setcc_task, int op) {
  d.opcode = op_name[op];
  enable_all_flag_writes(d);
}

/////////////////////////////////////////////////////////////
//
// Memory load / store instruction disassembly tasks
//
/////////////////////////////////////////////////////////////

DISASM_TASK1 (ld_s_rr_task, const char *sz) {
  d.opcode = op_name[LD_S_STR];
  d.fmt = FMT_L_A_B_C;
  d.size_suffix = sz;
}

DISASM_TASK (load_rr_32_task) {
  d.opcode = op_name[LD_STR];
  regs_abc_32_task (d);
  d.fmt = FMT_L_A_B_C;
  
  switch (UNSIGNED_BITS(d.inst,18,17)) {
    case 1: d.size_suffix = byte_str; break;
    case 2: d.size_suffix = half_str; break;
  }
  
  switch (UNSIGNED_BITS(d.inst,23,22)) {
    case 1: d.write_back_mode = aw_str; break;
    case 2: d.write_back_mode = ab_str; break;
    case 3: d.write_back_mode = as_str; break;
  }
  
  if (BITSEL(d.inst,16) == 1)
    d.extend_mode = ext_str;
  
  if (BITSEL(d.inst,15) == 1)
    d.cache_byp_mode = cenb_str;
}

DISASM_TASK (load_32_task) {
  d.opcode = op_name[LD_STR];
  regs_a_32_task (d);
  regs_b_32_task (d);
  
  d.int_val = (   (SIGNED_BITS(d.inst,15,15)<<8)
             | (UNSIGNED_BITS(d.inst,23,16))
             );
  
  d.fmt = (d.int_val == 0) ? FMT_L_A_B : FMT_L_A_B_IMM;
  
  switch (UNSIGNED_BITS(d.inst,8,7)) {
    case 1: d.size_suffix = byte_str; break;
    case 2: d.size_suffix = half_str; break;
  }
  
  switch (UNSIGNED_BITS(d.inst,10,9)) {
    case 1: d.write_back_mode = aw_str; break;
    case 2: d.write_back_mode = ab_str; break;
    case 3: d.write_back_mode = as_str; break;
  }
  
  if (BITSEL(d.inst,6) == 1)
    d.extend_mode = ext_str;
  
  if (BITSEL(d.inst,11) == 1)
    d.cache_byp_mode = cenb_str;
}

DISASM_TASK (store_32_task) {
  d.opcode = op_name[ST_STR];
  regs_c_32_task (d);
  regs_b_32_task (d);
  
  d.int_val = (  (SIGNED_BITS(d.inst,15,15)<<8)
             | (UNSIGNED_BITS(d.inst,23,16)));
  
  if (BITSEL(d.inst,0) == 1) 
  {
    d.abs_val = SIGNED_BITS(d.inst,11,6);
    d.fmt     = FMT_S_S6_B;
  }
  else 
  {
    regs_c_32_task (d);
    d.fmt = (d.int_val == 0) ? FMT_S_C_B : FMT_S_C_B_IMM;
  }
  
  
  switch (UNSIGNED_BITS(d.inst,2,1)) {
    case 1: d.size_suffix = byte_str; break;
    case 2: d.size_suffix = half_str; break;
  }
  
  switch (UNSIGNED_BITS(d.inst,4,3)) {
    case 1: d.write_back_mode = aw_str; break;
    case 2: d.write_back_mode = ab_str; break;
    case 3: d.write_back_mode = as_str; break;
  }
  
  if (BITSEL(d.inst,5) == 1)
    d.cache_byp_mode = cenb_str;
}

DISASM_TASK2 (store_16_task, const char *sz, int shift) {
  d.opcode = op_name[ST_S_STR];
  d.fmt = FMT_S_C_B_IMM;
  
  regs_c_16_task (d);
  regs_b_16_task (d);
  
  d.size_suffix = sz;
  d.int_val = UNSIGNED_BITS(d.inst,20,16) << shift;
}

DISASM_TASK3 (mem_sp_16_task, int op, const char *sz, int f) {
  d.opcode = op_name[op];
  d.size_suffix = sz;
  d.fmt = f;
  
  regs_b_16_task (d);
  REG_C(SP_REG);
  d.int_val = UNSIGNED_BITS(d.inst,20,16) << 2;
}

DISASM_TASK4 (r0_gp_16_task, int op, const char *sz, int f, int shift) {
  d.opcode = op_name[op];
  d.size_suffix = sz;
  d.fmt = f;
  
  REG_A(0);      // always r0
  REG_B(GP_REG); // always gp
  d.int_val = SIGNED_BITS(d.inst,24,16) << shift;
}

DISASM_TASK (load_pcl_16_task) {
  d.opcode = op_name[LD_S_STR];
  d.fmt = FMT_L_B_C_IMM;
  regs_b_16_task (d);
  REG_C(PCL_REG);
  d.int_val = UNSIGNED_BITS(d.inst,23,16) << 2;
}

DISASM_TASK (add_sp_16_task) {
  d.opcode = op_name[ADD_S_STR];
  d.fmt = FMT_A_B_IMM;
  REG_A(UNSIGNED_BITS(d.inst,26,24));
  REG_B(SP_REG);
  d.abs_val = UNSIGNED_BITS(d.inst,20,16) << 2;
}

DISASM_TASK1 (arith_sp_sp_task, int op) {
  d.opcode = op_name[op];
  
  d.fmt = FMT_B_B_IMM;
  REG_B(SP_REG);
  d.abs_val = UNSIGNED_BITS(d.inst,20,16) << 2;
}

DISASM_TASK1 (stack_b_task, int op) {
  regs_b_16_task (d);
  d.opcode = op_name[op];
  
  d.fmt = FMT_B;
}

DISASM_TASK1 (stack_blink_task, int op) {
  REG_B(BLINK);
  d.opcode = op_name[op];
  
  d.fmt = FMT_B;
}

/////////////////////////////////////////////////////////////
//
// Miscellaneous tasks
//
/////////////////////////////////////////////////////////////

DISASM_TASK (enter_s_task) {
  if (d.isa_opts.density_option) {
    d.opcode = op_name[ENTER_S_STR];
    // The following fields are overloaded to carry enter_s
    // specific strings
    //  size_suffix     - blink string
    //  extend_mode     - string for normal registers to be saved
    //  cache_byp_mode  - fp string
    //
    d.size_suffix   = BITSEL(d.inst,ENTER_LEAVE_LINK_BIT)
    ? "blink" : null_str;
    d.extend_mode   = enter_leave_regs_str[UNSIGNED_BITS(d.inst, 20, 17)];
    d.cache_byp_mode= BITSEL(d.inst,ENTER_LEAVE_FP_BIT)
    ? "fp" : null_str;
    d.fmt           = FMT_MACRO;
  } else {
    d.invld_ins     = true;
  }
}

DISASM_TASK (leave_s_task) {
  if (d.isa_opts.density_option) {
    d.opcode = op_name[LEAVE_S_STR];
    // The following fields are overloaded to carry leave_s
    // specific strings
    //  size_suffix     - blink string
    //  extend_mode     - string for normal registers to be saved
    //  cache_byp_mode  - fp string
    //  write_back_mode - pcl string (for jump)
    //
    d.size_suffix     = BITSEL(d.inst,ENTER_LEAVE_LINK_BIT)
    ? "blink" : null_str;
    d.extend_mode     = enter_leave_regs_str[UNSIGNED_BITS(d.inst, 20, 17)];
    d.cache_byp_mode  = BITSEL(d.inst,ENTER_LEAVE_FP_BIT)
    ? "fp" : null_str;
    d.write_back_mode = BITSEL(d.inst,ENTER_LEAVE_JMP_BIT)
    ? "pcl" : null_str;
    d.fmt             = FMT_MACRO;
  } else {
    d.invld_ins     = true;
  }
}

DISASM_TASK (jli_s_task) {
  if (d.isa_opts.density_option) {
    d.opcode = op_name[JLI_S_STR];
    d.abs_val= UNSIGNED_BITS(d.inst,25,16);
    d.fmt    = FMT_IMM;
  } else {
    d.invld_ins     = true;
  }
}

DISASM_TASK (ei_s_task) {
  if (d.isa_opts.density_option > 1) {
    d.opcode = op_name[EI_S_STR];
    d.abs_val= UNSIGNED_BITS(d.inst,25,16);
    d.fmt    = FMT_IMM;
  } else {
    d.invld_ins     = true;
  }
}

DISASM_TASK (ldi_s_task) {
  if (d.isa_opts.density_option > 1) {
    d.opcode = op_name[LDI_S_STR];
    REG_B ((BITSEL(d.inst,26)<<3) | UNSIGNED_BITS(d.inst,26,24));
    d.abs_val = ((UNSIGNED_BITS(d.inst,23,20)<<3) | UNSIGNED_BITS(d.inst,18,16));
    d.fmt    = FMT_B_IMM_IND;
  } else {
    d.invld_ins     = true;
  }
}

DISASM_TASK (ldi_task) {
  if (d.isa_opts.density_option > 1) {
    d.opcode = op_name[LDI_STR];
    switch (UNSIGNED_BITS(d.inst,23,22))
    {
      case 0: d.fmt = FMT_B_C_IND;   break;
      case 1: d.fmt = FMT_B_IMM_IND; break;
      case 2: d.fmt = FMT_B_IMM_IND; break;
      case 3: d.fmt = FMT_B_IMM_IND; break;
    }
  } else {
    d.invld_ins     = true;
  }
}

DISASM_TASK (bi_task) {
  if (d.isa_opts.density_option) {
    d.opcode = (BITSEL(d.inst,16) == 1) ? op_name[BIH_STR] : op_name[BI_STR];
    regs_c_32_task (d);
    d.fmt    = FMT_C_IND;
  } else {
    d.invld_ins     = true;
  }
}

DISASM_TASK (sleep_task) {
  d.opcode = op_name[SLEEP_STR];
  d.abs_val= UNSIGNED_BITS(d.inst,11,6); // SLEEP has a u6 field
  d.fmt    = FMT_IMM;
}

DISASM_TASK(seti_task) {
  switch ( UNSIGNED_BITS(d.inst,23,22) )
  {
    case REG_U6IMM_FMT:
      d.fmt = FMT_IMM;
      regs_u6_32_task(d);
      break;
    case REG_REG_FMT:
      regs_c_32_task(d);
      d.fmt = FMT_C;
      break;
  }
  
  d.opcode = op_name[SETI_STR];
}

DISASM_TASK(clri_task) {
  switch ( UNSIGNED_BITS(d.inst,23,22) )
  {
    case REG_U6IMM_FMT:
      d.fmt = FMT_IMM;
      regs_u6_32_task(d);
      break;
    case REG_REG_FMT:
      regs_c_32_task(d);
      d.fmt = FMT_C;
      break;
  }
  
  d.opcode = op_name[CLRI_STR];
}

DISASM_TASK (rol_task) {
  d.opcode = op_name[ROL_STR];
  enable_znc_flag_writes (d);
}

DISASM_TASK (scond_task) {
  d.opcode = op_name[SCOND_STR];
  d.fmt = FMT_B_C_IND;  
}

DISASM_TASK (llock_task) {
  d.opcode = op_name[LLOCK_STR];
  d.fmt = FMT_B_C_IND;
}      
      
DISASM_TASK (ex_task) {
  d.opcode = op_name[EX_STR];
  d.fmt = FMT_B_C_IND;
  
  if (BITSEL(d.inst,15))
    d.f_bit = cenb_str;
}

DISASM_TASK (rol_ex_task) {
  if (d.isa_opts.is_isa_a6k())
    rol_task (d);
  else
    ex_task (d);
}

DISASM_TASK (trap0_task) {
  d.opcode = op_name[TRAP0_STR];
  d.fmt    = FMT_ZOP;
}

DISASM_TASK (swi_task) {
  d.opcode = op_name[SWI_STR];
  d.fmt    = FMT_ZOP;
}

/////////////////////////////////////////////////////////////
//
// Shift and logical operation tasks
//
/////////////////////////////////////////////////////////////

DISASM_TASK (asl_task) {
  d.opcode = op_name[ASL_STR];
  enable_znc_flag_writes (d);
}

DISASM_TASK (asr_task) {
  d.opcode = op_name[ASR_STR];
  enable_znc_flag_writes (d);
}

DISASM_TASK (lsr_task) {
  d.opcode = op_name[LSR_STR];
  enable_znc_flag_writes (d);
}

DISASM_TASK (rlc_task) {
  d.opcode = op_name[RLC_STR];
  enable_znc_flag_writes (d);
}

DISASM_TASK (ror_task) {
  d.opcode = op_name[ROR_STR];
  enable_znc_flag_writes (d);
}

DISASM_TASK (rrc_task) {
  d.opcode = op_name[RRC_STR];
  enable_znc_flag_writes (d);
}

DISASM_TASK (sexb_task) {
  d.opcode = op_name[SEXB_STR];
  enable_zn_flag_writes (d);
}

DISASM_TASK (sexw_task) {
  d.opcode = op_name[SEXW_STR];
  enable_zn_flag_writes (d);
}

DISASM_TASK (extb_task) {
  d.opcode = op_name[EXTB_STR];
  enable_zn_flag_writes (d);
}

DISASM_TASK (extw_task) {
  d.opcode = op_name[EXTW_STR];
  enable_zn_flag_writes (d);
}

DISASM_TASK (add_s_task) {
  d.opcode = op_name[ADD_S_STR];
}

DISASM_TASK (not_task) {
  d.opcode = op_name[NOT_STR];
  enable_zn_flag_writes (d);
}

DISASM_TASK (neg_task) {
  d.opcode = op_name[NEG_S_STR];
}

DISASM_TASK (sub_s_task) {
  d.opcode = op_name[SUB_S_STR];
}

DISASM_TASK (asl_s_task) {
  d.opcode = op_name[ASL_S_STR];
}

DISASM_TASK (asr_s_task) {
  d.opcode = op_name[ASR_S_STR];
}

DISASM_TASK (mov_s_task) {
  d.opcode = op_name[MOV_S_STR];
}

DISASM_TASK (mov_s_ne_task) {
  d.opcode = op_name[MOV_S_STR];
  QFIELD(2);
}

DISASM_TASK (cmp_s_task) {
  d.opcode = op_name[CMP_S_STR];
}

DISASM_TASK (and_s_task) {
  d.opcode = op_name[AND_S_STR];
}

DISASM_TASK (or_s_task) {
  d.opcode = op_name[OR_S_STR];
}

DISASM_TASK (bic_s_task) {
  d.opcode = op_name[BIC_S_STR];
}

DISASM_TASK (xor_s_task) {
  d.opcode = op_name[XOR_S_STR];
}

DISASM_TASK (tst_s_task) {
  d.opcode = op_name[TST_S_STR];
}

DISASM_TASK (mul64_s_task) {
  d.opcode = op_name[MUL_S_STR];
}

DISASM_TASK (mpy_s_task) {
  d.opcode = op_name[MPY_S_STR];
}

DISASM_TASK (mpyw_s_task) {
  d.opcode = op_name[MPYW_S_STR];
}

DISASM_TASK (mpyuw_s_task) {
  d.opcode = op_name[MPYUW_S_STR];
}
      
DISASM_TASK (sexb_s_task) {
  d.opcode = op_name[SEXB_S_STR];
}

DISASM_TASK (sexw_s_task) {
  d.opcode = op_name[SEXW_S_STR];
}

DISASM_TASK (extb_s_task) {
  d.opcode = op_name[EXTB_S_STR];
}

DISASM_TASK (extw_s_task) {
  d.opcode = op_name[EXTW_S_STR];
}

DISASM_TASK (abs_s_task) {
  d.opcode = op_name[ABS_S_STR];
}

DISASM_TASK (not_s_task) {
  d.opcode = op_name[NOT_S_STR];
}

DISASM_TASK (neg_s_task) {
  d.opcode = op_name[NEG_S_STR];
}

DISASM_TASK (add1_s_task) {
  d.opcode = op_name[ADD1_S_STR];
}

DISASM_TASK (add2_s_task) {
  d.opcode = op_name[ADD2_S_STR];
}

DISASM_TASK (add3_s_task) {
  d.opcode = op_name[ADD3_S_STR];
}

DISASM_TASK (lsr_s_task) {
  d.opcode = op_name[LSR_S_STR];
}

DISASM_TASK (trap_s_task) {
  d.opcode = op_name[TRAP_S_STR];
  d.abs_val= UNSIGNED_BITS(d.inst,26,21); // TRAP_S has a u6 field
  d.fmt    = FMT_IMM;
}

DISASM_TASK (brk_s_task) {
  d.opcode = op_name[BRK_S_STR];
  d.fmt    = FMT_ZOP;
}

DISASM_TASK (nop_s_task) {
  d.opcode = op_name[NOP_S_STR];
  d.fmt    = FMT_ZOP;
}

DISASM_TASK (unimp_s_task) {
  d.opcode = op_name[UNIMP_S_STR];
  d.fmt    = FMT_ZOP;
}

DISASM_TASK (swi_s_task) {
  d.opcode = op_name[SWI_S_STR];
  d.fmt    = FMT_ZOP;
}

DISASM_TASK (sync_s_task) {
  d.opcode = op_name[SYNC_S_STR];
  d.fmt    = FMT_ZOP;
}

DISASM_TASK (bset_s_task) {
  d.opcode = op_name[BSET_S_STR];
}

DISASM_TASK (bclr_s_task) {
  d.opcode = op_name[BCLR_S_STR];
}


DISASM_TASK (bmsk_s_task) {
  d.opcode = op_name[BMSK_S_STR];
}


DISASM_TASK (btst_s_task) {
  d.opcode = op_name[BTST_S_STR];
}

/////////////////////////////////////////////////////////////
//
// Load and Store Aux Regs, and related tasks
//
/////////////////////////////////////////////////////////////

DISASM_TASK (lr_task) {
  if ((BITSEL(d.inst,15) == 1) || (UNSIGNED_BITS(d.inst,23,22) == 3))
    d.invld_ins = true;

  d.opcode = op_name[LR_STR];
}

DISASM_TASK (sr_task) {
  if ((BITSEL(d.inst,15) == 1) || (UNSIGNED_BITS(d.inst,23,22) == 3))
    d.invld_ins = true;

  d.opcode = op_name[SR_STR];
}

DISASM_TASK(aex_task)
{
  d.opcode = op_name[AEX_STR];
}

DISASM_TASK (flag_task) {
  if (d.isa_opts.is_isa_a6k() && BITSEL(d.inst,15))
  {
    d.opcode = op_name[KFLAG_STR];
  }
  else
  {  
    d.opcode = op_name[FLAG_STR];
  }
}

/////////////////////////////////////////////////////////////
//
// Jump and branch micro-code tasks
//
/////////////////////////////////////////////////////////////

DISASM_TASK (jcc_task) {
  d.opcode = op_name[JCC_STR];
  d.cc_op = true;
  DSLOT(d.inst,16);
}

DISASM_TASK (j_s_task) {
  d.opcode = op_name[J_S_STR];
  DSLOT(d.inst,21);
}

DISASM_TASK (jeq_s_task) {
  d.opcode = op_name[JEQ_S_STR];
  REG_B(31);   // select BLINK as jump target source
  BR_COND(1);  // select the EQ condition
}

DISASM_TASK (jne_s_task) {
  d.opcode = op_name[JNE_S_STR];
  REG_B(31);   // select BLINK as jump target source
  BR_COND(2);  // select the NE condition
}

DISASM_TASK (j_blink_task) {
  d.opcode = op_name[J_S_STR];
  REG_B(31);   // select BLINK as jump target source
  DSLOT(d.inst,24);  // get dslot indicator
}

DISASM_TASK (jlcc_task) {
  d.opcode = op_name[JLCC_STR];
  d.cc_op = true;
  DSLOT(d.inst,16);   // get dslot indicator
}

DISASM_TASK (jl_s_task) {
  d.opcode = op_name[JL_S_STR];
  DSLOT(d.inst,21);   // get dslot indicator
}

DISASM_TASK (jl_task) {
  d.opcode = op_name[JLCC_STR];
  DSLOT(d.inst,16);   // get dslot indicator
}

DISASM_TASK (lpcc_task) {
  d.opcode = op_name[LP_STR];
  d.cc_op = true;
}

DISASM_TASK (rtie_task) {
  d.opcode = op_name[RTIE_STR];
  d.fmt    = FMT_ZOP;
}

DISASM_TASK (br_cond_task) {
  d.fmt = FMT_OFF;
  d.opcode = op_name[BCC_STR];
  cond_q_32_task (d);
  d.cc_op = true;
  DSLOT(d.inst,5);
  d.int_val = (   (SIGNED_BITS(d.inst,15,6) << 11)
             | (UNSIGNED_BITS(d.inst,26,17) << 1)
             );
}

DISASM_TASK2 (bcc_s_task, sint32 offset, uint32 q_bits) {
  d.fmt = FMT_OFF;
  d.opcode = op_name[BCC_S_STR];
  BR_COND(q_bits);
  d.cc_op = true;
  d.int_val = offset;
}

DISASM_TASK (br_ucond_task) {
  d.fmt = FMT_OFF;
  d.opcode = op_name[BCC_STR];
  d.cc_op = true;
  BR_COND(0);
  DSLOT(d.inst,5);
  d.int_val = (   (SIGNED_BITS(d.inst,3,0) << 21)
             | (UNSIGNED_BITS(d.inst,15,6) << 11)
             | (UNSIGNED_BITS(d.inst,26,17) << 1)
             );
}

DISASM_TASK (bl_cond_task) {
  d.fmt = FMT_OFF;
  d.opcode = op_name[BLCC_STR];
  cond_q_32_task (d);
  d.cc_op = true;
  DSLOT(d.inst,5);
  d.int_val = (   (SIGNED_BITS(d.inst,15,6) << 11)
             | (UNSIGNED_BITS(d.inst,26,18) << 2)
             );
}

DISASM_TASK (bl_ucond_task) {
  d.fmt = FMT_OFF;
  d.opcode = op_name[BLCC_STR];
  BR_COND(0);
  d.cc_op = true;
  DSLOT(d.inst,5);
  d.int_val = (   (SIGNED_BITS(d.inst,3,0) << 21)
             | (UNSIGNED_BITS(d.inst,15,6) << 11)
             | (UNSIGNED_BITS(d.inst,26,18) << 2)
             );
}

DISASM_TASK (bl_s_ucond_task) {
  d.fmt = FMT_OFF;
  d.opcode = op_name[BLCC_STR];
  BR_COND(0);
  d.int_val = SIGNED_BITS(d.inst,26,16) << 2;
}

DISASM_TASK2 (brcc_bbit_task, bool is_bbit, uint32 q_bits) {
  DSLOT(d.inst,5);
  d.invld_ins = BITSEL(d.inst,5) & (d.reg_b == d.limm_str);
  d.int_val = SIGNED_BITS(d.inst,23,17) << 1;
  if (is_bbit)
    if (q_bits == 1)
      d.opcode = op_name[BBIT0_STR];
    else
      d.opcode = op_name[BBIT1_STR];
    else {
      d.opcode = op_name[BRCC_STR];
      BR_COND(q_bits);
      d.cc_op = true;
    }
}

DISASM_TASK1 (brcc_s_task, uint32 q_bits) {
  BR_COND(q_bits);
  d.int_val = SIGNED_BITS(d.inst,22,16) << 1;
  d.opcode = op_name[BRCC_S_STR];
  d.cc_op = true;
}

DISASM_TASK1 (fpx_task, int op_index) {
  d.opcode = fp_op_name[op_index];
}

DISASM_TASK (ld_as_16_task)
{
  d.fmt = FMT_L_A_B_C;
  d.opcode = op_name[LD_S_STR];
  REG_A((BITSEL(d.inst,18)<<3) | UNSIGNED_BITS(d.inst,18,16));
}

DISASM_TASK (ld_rr_u5_16_task)
{
  d.fmt = FMT_L_B_C_IMM;
  d.opcode = op_name[LD_S_STR];
  d.int_val = ((BITSEL(d.inst,26)<<4) | (UNSIGNED_BITS(d.inst,20,19)<<2));
  REG_B(UNSIGNED_BITS(d.inst,25,24));
}

DISASM_TASK (add_r01_u6_task)
{
  d.fmt = FMT_A_B_IMM;
  d.opcode = op_name[ADD_S_STR];
  d.abs_val = ((UNSIGNED_BITS(d.inst,22,20)<<3) | UNSIGNED_BITS(d.inst,18,16));
  REG_A (BITSEL(d.inst,23));
}

DISASM_TASK (mem_r01_gp_s9_task)
{
  uint32 memop = BITSEL(d.inst,20);
  REG_C (GP_REG);
  if (memop == 0) {
    d.opcode = op_name[LD_S_STR];
    d.fmt = FMT_L_B_C_IMM;
    d.int_val = ((SIGNED_BITS(d.inst,26,21)<<5) | (UNSIGNED_BITS(d.inst,18,16)<<2));
    REG_B (1); // R1 is the load destination
  } else {
    d.opcode = op_name[ST_S_STR];
    d.fmt = FMT_S_B_C_IMM;
    d.int_val = ((SIGNED_BITS(d.inst,26,21)<<5) | (UNSIGNED_BITS(d.inst,18,16)<<2));
    REG_B (0); // R0 provides the store data
  }
}

DISASM_TASK (group8_task)
{
  if (d.isa_opts.density_option > 1)
  {
    regs_h_16_task (d);
    if (BITSEL(d.inst,18)) {
      // ld rr,[h,u5]
      ld_rr_u5_16_task (d);
    } else {
      // mov_s g,h
      regs_g_16_task (d);
      mov_s_task (d);
      d.fmt = FMT_B_C;
    }
  }
  else
    d.invld_ins     = true;
}

DISASM_TASK (group9_task)
{
  if (d.isa_opts.density_option > 1)
  {
    REG_B ((BITSEL(d.inst,26)<<3) | UNSIGNED_BITS(d.inst,26,24));
    if (BITSEL(d.inst,19)) {
      add_r01_u6_task (d);
    } else {
      REG_C ((BITSEL(d.inst,23)<<3) | UNSIGNED_BITS(d.inst,23,21));
      if (BITSEL(d.inst,20)) {
        REG_A ((BITSEL(d.inst,18)<<3) | UNSIGNED_BITS(d.inst,18,16));
        sub_s_task (d);
      } else {
        ld_as_16_task (d);
      }
    }
  }
  else
    d.invld_ins     = true;
}

DISASM_TASK (group10_task)
{
  if (d.isa_opts.density_option > 1)
  {
    if (BITSEL(d.inst,19)) {
      ldi_s_task (d);
    } else {
      mem_r01_gp_s9_task (d);
    }
  }
  else
    d.invld_ins     = true;
}

DISASM_TASK (finalise_task) {
  flag_enable_task (d);
}

DISASM_TASK (ext_operands_task) {
  // Firstly, decode the operand format in the same
  // way as GRP_BASECASE_32, but without the exceptional
  // formats that appear in the basecase group
  //
  switch ( UNSIGNED_BITS(d.inst,23,22) ) {
    case REG_REG_FMT: // REG_REG format for major opcode 0x04
      switch ( UNSIGNED_BITS(d.inst,21,16) ) {
          // first specify all the exceptions
        case SOP_FMT:
          regs_sop_bc_32_task (d);
          d.fmt = FMT_B_C;
          break;
          // secondly, the default operands are extracted
        default:
          regs_abc_32_task (d);
          d.fmt = FMT_A_B_C;
          break;
      }
      break;
      
    case REG_U6IMM_FMT: // REG_U6IMM format for major opcode 0x04
      switch ( UNSIGNED_BITS(d.inst,21,16) ) {
          // first specify all the exceptions
        case SOP_FMT:
          regs_sop_bu6_32_task (d);
          d.fmt = FMT_B_IMM;
          break;
          // secondly, the default operands are extracted
        default:
          regs_abu6_32_task (d);
          d.fmt = FMT_A_B_IMM;
          break;
      }
      break;
      
    case REG_S12IMM_FMT:  // REG_S12IMM format for major opcode 0x04
      switch ( UNSIGNED_BITS(d.inst,21,16) ) {
          // first specify all the exception
          // (none so far)
          // secondly, the default operands are extracted
        default:
          regs_bbs12_32_task (d);
          d.fmt = FMT_B_B_IMM;
          break;
      }
      break;
      
    case REG_COND_FMT:  // REG_COND format for major opcode 0x04
      switch ( BITSEL(d.inst,5) ) {
        case false: regs_c_32_task (d);  d.fmt = FMT_B_B_C;  break;
        case true:  regs_u6_32_task (d); d.fmt = FMT_B_B_IMM; break;
      }
      switch ( UNSIGNED_BITS(d.inst,21,16) ) {
          // first list all the exceptions
          // (none so far)
          // secondly, the default operands are extracted
        default:    regs_bbq_32_task (d);  break; // Generic reg-cond
          // take care of Section 4.9.9.1
      }
      break;
  }
}

// =====================================================================
//
// DISASSEMBLY METHOD IMPLEMENTATIONS
//
// =====================================================================

const char*
Disasm::dis_reg(uint32 reg_num)
{
  ASSERT(reg_num <= GPR_BASE_REGS && "Invalid register number for disassembly.");
  return reg_names[reg_num];
}
      
// Constructor
//
Disasm::Disasm (const IsaOptions&                            _opts,
                const arcsim::sys::cpu::EiaExtensionManager& _eia_mgr,
                uint32 _instr,
                uint32 _limm_val)
: isa_opts(_opts),
  eia_mgr(_eia_mgr),
  inst(_instr),
  len(0),
  fmt(0),
  opcode(null_str),
  cc_test(null_str),
  dslot(null_str),
  reg_a(null_str),
  reg_b(null_str),
  reg_c(null_str),
  f_bit(null_str),
  abs_val(0),
  int_val(0),
  cc_op(false),
  invld_ins(false),
  has_limm(false),
  is_16bit(false),
  size_suffix(null_str),
  extend_mode(null_str),
  write_back_mode(null_str),
  cache_byp_mode(null_str)
{
  // Initialise disassembly buffer
  //
  buf[0]    = '\0';

  // Initiliase flags
  //
  flags[0]  = flags[1] = flags[2] = flags[3] = '-';
  
  // Perform actual disassembly
  //
  disasm (_limm_val);
}

      
void
Disasm::to_string ()
{
  // Bail out out early upon an invalid instruction
  //
  if (invld_ins) {
    len = snprintf (buf, sizeof(buf), "???");
    return;
  }
  
  // IN case of a valid instruction we should do some actual work
  //
  static const uint32 kOpBufSize = 128;
  char opbuf[kOpBufSize];                 // temporary opcode buffer
  
  // Determine OPERATOR
  //
  if (fmt < FMT_L_A_B_C) {
    const char *extra = f_bit == null_str ? dslot : f_bit;
    snprintf (opbuf, kOpBufSize, opcode,
                    cc_test, extra);
  } else {
    snprintf (opbuf, kOpBufSize, opcode,
                    size_suffix, extend_mode, cache_byp_mode, write_back_mode);
  }

  // Determine OPERAND(s)
  //
  switch (fmt)
  {
    case FMT_A_B_C:     len = snprintf (buf, sizeof(buf), "%-15s%s,%s,%s",     opbuf, DST(reg_a), SRC(reg_b), SRC(reg_c)); break;
    case FMT_A_B_IMM:   len = snprintf (buf, sizeof(buf), "%-15s%s,%s,0x%x",   opbuf, DST(reg_a), SRC(reg_b), abs_val   ); break;
    case FMT_B_0_OFF:   len = snprintf (buf, sizeof(buf), "%-15s%s,0,0x%x",    opbuf,             reg_b,      int_val   ); break;
    case FMT_B_B_B:     len = snprintf (buf, sizeof(buf), "%-15s%s,%s,%s",     opbuf, reg_b,      reg_b,      reg_b     ); break;
    case FMT_B_B_C:     len = snprintf (buf, sizeof(buf), "%-15s%s,%s,%s",     opbuf, DST(reg_b), SRC(reg_b), SRC(reg_c)); break;
    case FMT_B_C_IND:   len = snprintf (buf, sizeof(buf), "%-15s%s,[%s]",      opbuf, DST(reg_b), SRC(reg_c)            ); break;
    case FMT_B_C_OFF:   len = snprintf (buf, sizeof(buf), "%-15s%s,%s,0x%x",   opbuf, SRC(reg_b), SRC(reg_c),   int_val ); break;
    case FMT_B_C:       len = snprintf (buf, sizeof(buf), "%-15s%s,%s",        opbuf, DST(reg_b), SRC(reg_c)            ); break;
    case FMT_B_C_SRC:   len = snprintf (buf, sizeof(buf), "%-15s%s,%s",        opbuf,             SRC(reg_b), SRC(reg_c)); break;
    case FMT_B_B_IMM:   len = snprintf (buf, sizeof(buf), "%-15s%s,%s,0x%x",   opbuf, DST(reg_b), SRC(reg_b), abs_val   ); break;
    case FMT_B_IMM_IND: len = snprintf (buf, sizeof(buf), "%-15s%s,[0x%x]",    opbuf, DST(reg_b), abs_val               ); break;
    case FMT_B_IMM_OFF: len = snprintf (buf, sizeof(buf), "%-15s%s,0x%x,0x%x", opbuf, SRC(reg_b), abs_val,    int_val   ); break;
    case FMT_B_IMM:     len = snprintf (buf, sizeof(buf), "%-15s%s,0x%x",      opbuf, DST(reg_b), abs_val               ); break;
    case FMT_B_IMM_SRC: len = snprintf (buf, sizeof(buf), "%-15s%s,0x%x",      opbuf,             SRC(reg_b), abs_val   ); break;
    case FMT_B_IND:     len = snprintf (buf, sizeof(buf), "%-15s[%s]",         opbuf,             SRC(reg_b)            ); break;
    case FMT_B:         len = snprintf (buf, sizeof(buf), "%-15s%s",           opbuf,             SRC(reg_b)            ); break;
    case FMT_C:         len = snprintf (buf, sizeof(buf), "%-15s%s",           opbuf,             SRC(reg_c)            ); break;
    case FMT_C_B:       len = snprintf (buf, sizeof(buf), "%-15s%s,%s",        opbuf, DST(reg_c), SRC(reg_b)            ); break;
    case FMT_C_B_SRC:   len = snprintf (buf, sizeof(buf), "%-15s%s,%s",        opbuf,             SRC(reg_c), SRC(reg_b)); break;
    case FMT_C_B_IMM:   len = snprintf (buf, sizeof(buf), "%-15s%s,%s,0x%x",   opbuf, DST(reg_c), SRC(reg_b),   abs_val ); break;
    case FMT_C_IND:     len = snprintf (buf, sizeof(buf), "%-15s[%s]",         opbuf,             SRC(reg_c)            ); break;
    case FMT_MACRO:     len = snprintf (buf, sizeof(buf), "%s",                opbuf                                    ); break;
    case FMT_IMM:       len = snprintf (buf, sizeof(buf), "%-15s0x%x",         opbuf,             abs_val               ); break;
    case FMT_OFF:       len = snprintf (buf, sizeof(buf), "%-15s0x%x",         opbuf,             int_val               ); break;
    case FMT_ZOP:       len = snprintf (buf, sizeof(buf), "%s",                opbuf                                    ); break;
    case FMT_L_A_B_C:   len = snprintf (buf, sizeof(buf), "%-15s%s,[%s,%s]",   opbuf, DST(reg_a), SRC(reg_b), SRC(reg_c)); break;
    case FMT_L_A_B_IMM: len = snprintf (buf, sizeof(buf), "%-15s%s,[%s,0x%x]", opbuf, DST(reg_a), SRC(reg_b), int_val   ); break;
    case FMT_L_A_B:     len = snprintf (buf, sizeof(buf), "%-15s%s,[%s]",      opbuf, DST(reg_a), SRC(reg_b)            ); break;
    case FMT_L_B_C_IMM: len = snprintf (buf, sizeof(buf), "%-15s%s,[%s,0x%x]", opbuf, DST(reg_b), SRC(reg_c),   int_val ); break;
    case FMT_L_C_B_IMM: len = snprintf (buf, sizeof(buf), "%-15s%s,[%s,0x%x]", opbuf, DST(reg_c), SRC(reg_b),   abs_val ); break;
    case FMT_L_C_B:     len = snprintf (buf, sizeof(buf), "%-15s%s,[%s]",      opbuf, DST(reg_c), SRC(reg_b)            ); break;
    case FMT_S_C_B_IMM: len = snprintf (buf, sizeof(buf), "%-15s%s,[%s,0x%x]", opbuf, SRC(reg_c), SRC(reg_b),   int_val ); break;
    case FMT_S_C_B:     len = snprintf (buf, sizeof(buf), "%-15s%s,[%s]",      opbuf, SRC(reg_c), SRC(reg_b)            ); break;
    case FMT_S_S6_B:    len = snprintf (buf, sizeof(buf), "%-15s%d,[%s,0x%x]", opbuf, abs_val,    SRC(reg_b),   int_val ); break;
    case FMT_S_B_C_IMM: len = snprintf (buf, sizeof(buf), "%-15s%s,[%s,0x%x]", opbuf, SRC(reg_b), SRC(reg_c),   int_val ); break;
    case FMT_H_H_S3:    len = snprintf (buf, sizeof(buf), "%-15s%s,%s,%d",     opbuf, SRC(reg_c), SRC(reg_c),   int_val ); break;
    case FMT_HS_S3:     len = snprintf (buf, sizeof(buf), "%-15s%s,%d",        opbuf, SRC(reg_c),               int_val ); break;
    case FMT_HD_S3:     len = snprintf (buf, sizeof(buf), "%-15s%s,%d",        opbuf, DST(reg_c),               int_val ); break;
  }
}

void
Disasm::disasm (uint32 l)
{
  Disasm& d = *this;

  snprintf (limm_str, sizeof(limm_str), "%08x", l);

  init_local_regs_task (d);
  
  // ---------------------------------------------------------------------------
  // THE BIG DISASM SWITCH for the 32 major opcode groups
  //
  switch ( UNSIGNED_BITS(inst,31,27) )
  {
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 0
    //
    case GRP_BRANCH_32: {
      switch ( BITSEL(inst,16) ) {
        case false: br_cond_task (d);   break;
        case true:  br_ucond_task(d);  break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 1
    //
    case GRP_BL_BRCC_32: {
      switch ( BITSEL(inst,16) ) {
      case false:
        switch ( BITSEL(inst,17) ) {
          case false: bl_cond_task (d);  break;
          case true:  bl_ucond_task (d); break;
        }
        break;
      case true:
        switch ( BITSEL(inst,4) ) {
          case false: regs_bc_32_task (d); fmt = FMT_B_C_OFF;   break;
          case true: regs_bu6_32_task (d); fmt = FMT_B_IMM_OFF; break;
        }
        switch ( UNSIGNED_BITS(inst,2,0) ) {
          case BREQ_OP:  brcc_bbit_task (d, false, BREQ_COND);  break;
          case BRNE_OP:  brcc_bbit_task (d, false, BRNE_COND);  break;
          case BRLT_OP:  brcc_bbit_task (d, false, BRLT_COND);  break;
          case BRGE_OP:  brcc_bbit_task (d, false, BRGE_COND);  break;
          case BRLO_OP:  brcc_bbit_task (d, false, BRLO_COND);  break;
          case BRHS_OP:  brcc_bbit_task (d, false, BRHS_COND);  break;
          case BBIT0_OP: brcc_bbit_task (d, true,  BBIT0_COND); break;
          case BBIT1_OP: brcc_bbit_task (d, true,  BBIT1_COND); break;
        }
        break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 2
    //
    case GRP_LOAD_32: {
      load_32_task (d);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 3
    //
    case GRP_STORE_32: {
      store_32_task (d);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 4
    //
    case GRP_BASECASE_32: {
      if (UNSIGNED_BITS(inst,21,19) == LD_RR_FMT) {
        load_rr_32_task (d);
      } else {
        // Firstly, decode the operand format
        //
        switch ( UNSIGNED_BITS(inst,23,22) ) {
        case REG_REG_FMT: // REG_REG format for major opcode 0x04
        {
          switch ( UNSIGNED_BITS(inst,21,16) ) {
          // firstly all exceptions are listed
          case MOV_OP:
              regs_mov_bc_task (d);
              fmt = FMT_B_C;
              break;
          case LR_OP:
              regs_bc_32_task (d);   // mov or lr reg-reg
              fmt = FMT_B_C_IND;
              break;
          case AEX_OP:
          case SR_OP:
          case TST_OP:
          case BTST_OP:
          case CMP_OP:
          case RCMP_OP:
              regs_bc_32_task (d);     // source regs only
              fmt = FMT_B_C_SRC;
              break;
          case FLAG_OP:
          case JCC_OP:
          case JCC_D_OP:
          case JLCC_OP:
          case JLCC_D_OP:
              regs_c_32_task (d);      // J/JL [c]
              fmt = FMT_C_IND;
              break;
          case SOP_FMT:
              regs_sop_bc_32_task (d); // SOP b,[c|limm]
              fmt = FMT_B_C;
              break;
          case LPCC_OP:
              inst_error_task (d);     // LPcc disallowed
              break;
          // secondly, the default operands are extracted
          default:
              regs_abc_32_task (d);   // Generic reg-reg
              fmt = FMT_A_B_C;
              break;
          } break;
        }
        case REG_U6IMM_FMT: // REG_U6IMM format for major opcode 0x04
        {
          switch ( UNSIGNED_BITS(inst,21,16) ) {
          // firstly all exceptions are listed
          case MOV_OP:
              regs_mov_bu6_task (d);    // mov reg,u6
              fmt = FMT_B_IMM;
              break;
          case LR_OP:
              regs_mov_bu6_task (d);    // lr reg,[u6]
              fmt = FMT_B_IMM_IND;
              break;
          case SR_OP:
          case AEX_OP:
          case TST_OP:
          case BTST_OP:
          case CMP_OP:
          case RCMP_OP:
              regs_bu6_32_task (d);     // source b,u6 only
              fmt = FMT_B_IMM_SRC;
              break;
          case JCC_OP:
          case JCC_D_OP:
          case JLCC_OP:
          case JLCC_D_OP:
              regs_u6_32_task (d);      // J/JL u6
              fmt = FMT_IMM;
              break;
          case FLAG_OP:
              regs_u6_32_task (d);
              fmt = FMT_IMM;
              break;
          case SOP_FMT:
              regs_sop_bu6_32_task (d); // SOP b,u6
              fmt = FMT_B_IMM;
              break;
          case LPCC_OP:
              regs_lpcc_u6_task (d);     // lpcc u7
              break;
          // secondly, the default operands are extracted
          default:  regs_abu6_32_task (d);    // Generic reg-u6
              fmt = FMT_A_B_IMM;
              break;
          } break;
        }
        case REG_S12IMM_FMT:  // REG_S12IMM format for major opcode 0x04
        {
          switch ( UNSIGNED_BITS(inst,21,16) ) {
          // firstly all exceptions are listed
          case MOV_OP:
              regs_mov_bs12_task (d);   // mov
              fmt = FMT_B_IMM;
              break;
          case LR_OP:
              regs_mov_bs12_task (d);   // mov or lr reg-s12
              fmt = FMT_B_IMM_IND;
              break;
          case SR_OP:
          case AEX_OP:
          case TST_OP:
          case BTST_OP:
          case CMP_OP:
          case RCMP_OP:
              regs_bs12_32_task (d);     // source b,s12 only
              fmt = FMT_B_IMM_SRC;
              break;
          case FLAG_OP:
          case JCC_OP:
          case JCC_D_OP:
          case JLCC_OP:
          case JLCC_D_OP:
              regs_s12_32_task (d);    // J/JL s12
              fmt = FMT_IMM;
              break;
          case LPCC_OP:
              regs_lpcc_s12_task (d);  // ucond LP s13
              break;
          // secondly, the default operands are extracted
          default:
              regs_bbs12_32_task (d);  // Generic reg-s12
              fmt = FMT_B_B_IMM;
              break;
          } break;
        }
        case REG_COND_FMT:  // REG_COND format for major opcode 0x04
        {
          switch ( UNSIGNED_BITS(inst,21,16) ) {
          // firstly all exceptions are listed
          case MOV_OP: 
            {
              regs_q_32_task (d);
              regs_b_32_task (d);
              switch (BITSEL(inst,5)) {
              case false: regs_c_32_task (d);  fmt = FMT_B_C;   break;
              case true:  regs_u6_32_task (d); fmt = FMT_B_IMM; break ;
              }
            } break;
          case LR_OP:                              // LR disallowed
          case SR_OP:     inst_error_task (d);     // SR disallowed
               break;
          case AEX_OP:
          case TST_OP:
          case BTST_OP:
          case CMP_OP:
          case RCMP_OP:
            {
              regs_q_32_task (d);
              regs_b_32_task (d);
              switch (BITSEL(inst,5)) {
              case false: regs_c_32_task (d);  fmt = FMT_B_C_SRC;   break;
              case true:  regs_u6_32_task (d); fmt = FMT_B_IMM_SRC; break;
              }
            } break;

          case FLAG_OP:
          case JCC_OP:
          case JCC_D_OP:
          case JLCC_OP:
          case JLCC_D_OP:
            {
              regs_q_32_task (d);
              switch (BITSEL(inst,5)) {
              case false: regs_c_32_task (d);  fmt = FMT_C_IND; break;
              case true:  regs_u6_32_task (d); fmt = FMT_IMM;   break;
              }
            } break;

          case LPCC_OP:   regs_lpcc_u6q_task (d);
              break;
          // secondly, the default operands are extracted
          default:  {
              regs_bbq_32_task (d);    // Generic reg-cond
              switch (BITSEL(inst,5)) {
              case false: regs_c_32_task (d); fmt = FMT_B_B_C;   break;
              case true: regs_u6_32_task (d); fmt = FMT_B_B_IMM; break;
              }
              } break;
          // take care of Section 4.9.9.1
          }
          } break;
        }

        // Secondly, decode the operator
        switch ( UNSIGNED_BITS(inst,21,16) ) {
        case ADD_OP:   add_task (d);  break;
        case ADC_OP:   adc_task (d);  break;
        case SUB_OP:   sub_task (d);  break;
        case SBC_OP:   sbc_task (d);  break;
        case AND_OP:   and_task (d);  break;
        case OR_OP:    or_task (d);   break;
        case BIC_OP:   bic_task (d);  break;
        case XOR_OP:   xor_task (d);  break;
        case MAX_OP:   max_task (d);  break;
        case MIN_OP:   min_task (d);  break;
        case MOV_OP:   mov_task (d);  break;
        case TST_OP:   tst_task (d);  break;
        case CMP_OP:   cmp_task (d);  break;
        case RCMP_OP:  rcmp_task (d); break;
        case RSUB_OP:  rsub_task (d); break;
        case BSET_OP:  bset_task (d); break;
        case BCLR_OP:  bclr_task (d); break;
        case BTST_OP:  btst_task (d); break;
        case BXOR_OP:  bxor_task (d); break;
        case BMSK_OP:  bmsk_task (d); break;
        case BMSKN_OP: bmskn_task (d);break;
        case ADD1_OP:  add1_task (d); break;
        case ADD2_OP:  add2_task (d); break;
        case ADD3_OP:  add3_task (d); break;
        case SUB1_OP:  sub1_task (d); break;
        case SUB2_OP:  sub2_task (d); break;
        case SUB3_OP:  sub3_task (d); break;
        case MPY_OP:   mpylo_task (d);  break;
        case MPYH_OP:  mpyhi_task (d);  break;
        case MPYU_OP:  mpylou_task (d); break;
        case MPYHU_OP: mpyhiu_task (d); break;
        case MPYW_OP:  mpyw_task (d);   break;
        case MPYWU_OP: mpywu_task (d);  break;
        case JCC_D_OP:
        case JCC_OP:   jcc_task (d);  break;
        case JLCC_D_OP:
        case JLCC_OP:  jlcc_task (d); break;
        case BI_OP:
        case BIH_OP:   bi_task (d);   break;
        case LDI_OP:   ldi_task (d);  break;
        case LPCC_OP:  lpcc_task (d); break;
        case FLAG_OP:  flag_task (d); break;
        case LR_OP:    lr_task (d);   break;
        case SR_OP:    sr_task (d);   break;
        case AEX_OP:   aex_task(d);   break;

        case SETEQ_OP: setcc_task (d, SETEQ_STR); break;
        case SETNE_OP: setcc_task (d, SETNE_STR); break;
        case SETLT_OP: setcc_task (d, SETLT_STR); break;
        case SETGE_OP: setcc_task (d, SETGE_STR); break;
        case SETLO_OP: setcc_task (d, SETLO_STR); break;
        case SETHS_OP: setcc_task (d, SETHS_STR); break;
        case SETLE_OP: setcc_task (d, SETLE_STR); break;
        case SETGT_OP: setcc_task (d, SETGT_STR); break;

        case SOP_FMT: // All Single or Zero operand 32-bit insns
          {
          switch ( UNSIGNED_BITS(inst,5,0) )
          {
            case ASL_OP:  asl_task (d);  break;
            case ASR_OP:  asr_task (d);  break;
            case LSR_OP:  lsr_task (d);  break;
            case ROR_OP:  ror_task (d);  break;
            case RRC_OP:  rrc_task (d);  break;
            case SEXB_OP: sexb_task (d); break;
            case SEXW_OP: sexw_task (d); break;
            case EXTB_OP: extb_task (d); break;
            case EXTW_OP: extw_task (d); break;
            case ABS_OP:  abs_task (d);  break;
            case NOT_OP:  not_task (d);  break;
            case RLC_OP:  rlc_task (d);  break;
            case LLOCK_OP:llock_task (d);  break;  
            case SCOND_OP:scond_task (d);  break;  
            case EX_OP:   ex_task (d);   break;
            case ROL_OP:  rol_task (d);  break;
            case ZOP_FMT: // All zero-operand 32-bit insns
            {
              zero_operand_task (d);
              switch ( UNSIGNED_BITS(inst,26,24) )
              {
                case SLEEP_OP: sleep_task (d);         break;
                case TRAP0_OP: {
                  if (d.isa_opts.is_isa_a6k() || d.isa_opts.is_isa_a600())  {
                    swi_task(d);
                  } else {
                    trap0_task(d);
                  }
                  break;
                }
                case SYNC_OP:  sync_s_task (d);        break;
                case RTIE_OP:  rtie_task (d);          break;
                case BRK_OP:   brk_s_task (d);         break;
                case SETI_OP:  seti_task(d);           break;
                case CLRI_OP:  clri_task(d);           break;
                default:       inst_error_task (d);    break;
              }
            }
            break;
            default: inst_error_task (d); break;
          }
          } break;
        default: inst_error_task (d); break;
        }
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 5
    //
    case GRP_ARC_EXT0_32: {
      // Firstly, decode the operand format in the same
      // way as GRP_BASECASE_32, but without the exceptional
      // formats that appear in the basecase group
      //
      switch ( UNSIGNED_BITS(inst,23,22) ) {
      case REG_REG_FMT: // REG_REG format for major opcode 0x04
        switch ( UNSIGNED_BITS(inst,21,16) ) {
        // first specify all the exceptions
        case SOP_FMT:
          regs_sop_bc_32_task (d);
          fmt = FMT_B_C;
          break;
        // secondly, the default operands are extracted
        default:
          regs_abc_32_task (d);
          fmt = FMT_A_B_C;
          break;
        }
        break;

      case REG_U6IMM_FMT: // REG_U6IMM format for major opcode 0x04
        switch ( UNSIGNED_BITS(inst,21,16) ) {
        // first specify all the exceptions
        case SOP_FMT:
          regs_sop_bu6_32_task (d);
          fmt = FMT_B_IMM;
          break;
        // secondly, the default operands are extracted
        default:
          regs_abu6_32_task (d);
          fmt = FMT_A_B_IMM;
          break;
        }
        break;

      case REG_S12IMM_FMT:  // REG_S12IMM format for major opcode 0x04
        switch ( UNSIGNED_BITS(inst,21,16) ) {
        // first specify all the exception
        // (none so far)
        // secondly, the default operands are extracted
        default:
          regs_bbs12_32_task (d);
          fmt = FMT_B_B_IMM;
          break;
        }
        break;

      case REG_COND_FMT:  // REG_COND format for major opcode 0x04
        switch ( BITSEL(inst,5) ) {
        case false: regs_c_32_task (d);  fmt = FMT_B_B_C;  break;
        case true:  regs_u6_32_task (d); fmt = FMT_B_B_IMM; break;
        }
        switch ( UNSIGNED_BITS(inst,21,16) ) {
        // first list all the exceptions
        // (none so far)
        // secondly, the default operands are extracted
        default:    regs_bbq_32_task (d);  break; // Generic reg-cond
        // take care of Section 4.9.9.1
        }
        break;
      }

      // Secondly, decode the operator
      //
      switch ( UNSIGNED_BITS(inst,21,16) ) {
      case ASLM_OP:   asl_task (d); break;
      case LSRM_OP:   lsr_task (d); break;
      case ASRM_OP:   asr_task (d); break;
      case RORM_OP:   ror_task (d); break;

      case ADDS_OP:   adds_task (d);   break;
      case SUBS_OP:   subs_task (d);   break;
      case ADDSDW_OP: addsdw_task (d); break;
      case SUBSDW_OP: subsdw_task (d); break;
      case ASLS_OP:   asls_task (d);   break;
      case ASRS_OP:   asrs_task (d);   break;

      // Some of the Divide/Remainder opcodes encode different operations
      // depending on the selected ISA.
      // To support the ARC 600 multiplier, define the mul64_task and mulu64_tast
      // tasks in dcode.h (not currently implememted).
      //
      case DIV_OP:  {
        if      (d.isa_opts.is_isa_a6k())  { div_task   (d); }
        else if (d.isa_opts.is_isa_a600()) { mul64_task (d); }
        break;
      }
      case DIVU_OP: {
        if      (d.isa_opts.is_isa_a6k())  { divu_task   (d); }
        else if (d.isa_opts.is_isa_a600()) { mulu64_task (d); }
        break;
      }
      case REM_OP:  {
        if    (d.isa_opts.is_isa_a6k()) { rem_task  (d); } 
        else                            { divaw_task(d); }
        break;
      }
      case REMU_OP:   remu_task (d);                                           break;

      case SOP_FMT: // All Single or Zero operand 32-bit insns
        switch ( UNSIGNED_BITS(inst,5,0) ) {
        case SWAP_OP:  gen_sop_task (d, SWAP_STR);  break;
        case SWAPE_OP: gen_sop_task (d, SWAPE_STR); break;
        case LSL16_OP: gen_sop_task (d, LSL16_STR); break;
        case LSR16_OP: gen_sop_task (d, LSR16_STR); break;
        
        case ASR16_OP: gen_sop_task (d, ASR16_STR); break;
        case ASR8_OP:  gen_sop_task (d, ASR8_STR);  break;
        case LSR8_OP:  gen_sop_task (d, LSR8_STR);  break;
        case LSL8_OP:  gen_sop_task (d, LSL8_STR);  break;
        case ROL8_OP:  gen_sop_task (d, ROL8_STR);  break;
        case ROR8_OP:  gen_sop_task (d, ROR8_STR);  break;

        case NORM_OP:  gen_sop_task (d, NORM_STR);  break;
        case NORMW_OP: gen_sop_task (d, NORMW_STR); break;
        
        case FFS_OP:   gen_sop_task (d, FFS_STR);   break;
        case FLS_OP:   gen_sop_task (d, FLS_STR);   break;

        case ABSSW_OP: gen_sop_task (d, ABSSW_STR); break;
        case ABSS_OP:  gen_sop_task (d, ABSS_STR);  break;
        case NEGS_OP:  gen_sop_task (d, NEGS_STR);  break;
        case NEGSW_OP: gen_sop_task (d, NEGSW_STR); break;

        case SAT16_OP: gen_sop_task (d, SAT16_STR); break;
        case RND16_OP: gen_sop_task (d, RND16_STR); break;

        case ZOP_FMT: // All zero-operand 32-bit EXT0 fmts
          zero_operand_task (d);
          switch ( UNSIGNED_BITS(inst,26,24) ) {
          // No instructions defined in this fmt
          default: inst_error_task (d); break;
          }
        default: inst_error_task (d); break;
        }
        break;
      default: inst_error_task (d); break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 6
    //      
    case GRP_ARC_EXT1_32: {
      ext_operands_task(d);
      unsigned minor_op_bits = UNSIGNED_BITS(inst,21,16);
      switch (minor_op_bits)
      {
      case FMUL_OP:
      case FADD_OP:
      case FSUB_OP:
      case DMULH11_OP:
      case DMULH12_OP:
      case DMULH21_OP:
      case DMULH22_OP:
      case DADDH11_OP:
      case DADDH12_OP:
      case DADDH21_OP:
      case DADDH22_OP:
      case DSUBH11_OP:
      case DSUBH12_OP:
      case DSUBH21_OP:
      case DSUBH22_OP:
      case DRSUBH11_OP:
      case DRSUBH12_OP:
      case DRSUBH21_OP:
      case DRSUBH22_OP:
      case DEXCL1_OP:
      case DEXCL2_OP:
        fpx_task (d, minor_op_bits);
        break;

      case SOP_FMT: // All Single or Zero operand 32-bit insns
        switch ( UNSIGNED_BITS(inst,5,0) ) {
        case ZOP_FMT: // All zero-operand 32-bit EXT0 fmts
          zero_operand_task (d);
          switch ( ((UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24)) ) {
          // Disassemble any ZOP-format instructions in minor group 6
          // No instructions currently defined in this fmt
          default: inst_error_task (d); break;
          }
        default: inst_error_task (d); break;
        }
        break;

      default:
        inst_error_task (d);
        break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 7
    //  FIXME: EIA
    case GRP_USR_EXT2_32: {
      // If an EIA instruction is present and decoded, this boolean variable will become false
      //
      bool   is_instr_error  = true;

      // Fast check if there are any EIA instructions enabled for this MAJOR opcode
      //
      if (eia_mgr.are_eia_instructions_defined
          && eia_mgr.eia_major_opcode_enabled_bitset[GRP_USR_EXT2_32])
      {
        // 1. DISASM OPERAND ---------------------------------------------------
        // Firstly, disasm the operand format in the same way as GRP_BASECASE_32,
        // but without the exceptional formats that appear in the basecase group
        //
        switch ( UNSIGNED_BITS(inst,23,22) ) {
          case REG_REG_FMT: // REG_REG format for major opcode 0x04
            switch ( UNSIGNED_BITS(inst,21,16) ) {
                // first specify all the exceptions
              case SOP_FMT:
                regs_sop_bc_32_task (d);
                fmt = FMT_B_C;
                break;
                // secondly, the default operands are extracted
              default:
                regs_abc_32_task (d);
                fmt = FMT_A_B_C;
                break;
            }
            break;
            
          case REG_U6IMM_FMT: // REG_U6IMM format for major opcode 0x04
            switch ( UNSIGNED_BITS(inst,21,16) ) {
                // first specify all the exceptions
              case SOP_FMT:
                regs_sop_bu6_32_task (d);
                fmt = FMT_B_IMM;
                break;
                // secondly, the default operands are extracted
              default:
                regs_abu6_32_task (d);
                fmt = FMT_A_B_IMM;
                break;
            }
            break;
            
          case REG_S12IMM_FMT:  // REG_S12IMM format for major opcode 0x04
            switch ( UNSIGNED_BITS(inst,21,16) ) {
                // first specify all the exception
                // (none so far)
                // secondly, the default operands are extracted
              default:
                regs_bbs12_32_task (d);
                fmt = FMT_B_B_IMM;
                break;
            }
            break;
            
          case REG_COND_FMT:  // REG_COND format for major opcode 0x04
            switch ( BITSEL(inst,5) ) {
              case false: regs_c_32_task (d);  fmt = FMT_B_B_C;  break;
              case true:  regs_u6_32_task (d); fmt = FMT_B_B_IMM; break;
            }
            switch ( UNSIGNED_BITS(inst,21,16) ) {
                // first list all the exceptions
                // (none so far)
                // secondly, the default operands are extracted
              default:    regs_bbq_32_task (d);  break; // Generic reg-cond
                // take care of Section 4.9.9.1
            }
            break;
        }
        
        // 2. DISASM OPERATOR  ------------------------------------------------------
        // Secondly, disasm the operator
        //        
        uint32 opcode_major    = GRP_USR_EXT2_32;
        uint32 dop_opcode      = UNSIGNED_BITS(inst,21,16);
        uint32 key             = ( ( (opcode_major & 0x1F) << 6) | (dop_opcode & 0x3F) );

        // Define iterators for convenient EIA instruction lookup
        //
        std::map<uint32,ise::eia::EiaInstructionInterface*>::const_iterator   I;
        std::map<uint32,ise::eia::EiaInstructionInterface*>::const_iterator   E = eia_mgr.opcode_eia_instruction_map.end();

        // Is this a DOP instruction?
        //
        if (dop_opcode != SOP_FMT
            && ((I = eia_mgr.opcode_eia_instruction_map.find(key)) != E))
        { // Found DOP EIA instruction
          //
          is_instr_error = false;
          // Get EIA instruction name
          //
          d.opcode = I->second->get_name();
        } 
        // Is this a SOP/ZOP instruction?
        //
        else if (dop_opcode == SOP_FMT)
        {     
          uint32 sop_opcode   = UNSIGNED_BITS(inst,21,16);
          key                 = ( ( (opcode_major & 0x1F) << 6) | (sop_opcode & 0x3F) );
          
          // Is this a SOP instruction?
          //
          if (sop_opcode != ZOP_FMT
              && ((I = eia_mgr.opcode_eia_instruction_map.find(key)) != E))
          { // FOUND SOP EIA instruction
            //
            is_instr_error = false;
            // Get EIA instruction name
            //
            d.opcode = I->second->get_name();
          }
          // Is this a ZOP instruction?
          //
          else if (sop_opcode == ZOP_FMT) {
            uint32 zop_opcode   = ((UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24));
            key                 = ( ( (opcode_major & 0x1F) << 6) | (zop_opcode & 0x3F) );
            
            if ((I = eia_mgr.opcode_eia_instruction_map.find(key)) != E)
            { // FOUND ZOP EIA instruction
              //
              is_instr_error = false;
              // Get EIA instruction name
              //
              d.opcode = I->second->get_name();
            }            
          }          
        }         
      }
      if (is_instr_error) {          
        inst_error_task (d);
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 8
    //
    case GRP_ARC_EXT0_16: {
      is_16bit = true;
      group8_task (d);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 9
    //    
    case GRP_ARC_EXT1_16: {
      is_16bit = true;
      group9_task (d);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 10
    //  
    case GRP_USR_EXT0_16: {
      is_16bit = true;
      group10_task (d);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 11
    // FIXME: EIA  
    case GRP_USR_EXT1_16: {
      is_16bit = true;
      if (BITSEL(inst,26) == 0)
        jli_s_task (d);
      else
        ei_s_task (d);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 12
    //  
    case GRP_LD_ADD_RR_16: {
      is_16bit = true;
      regs_abc_16_task (d);
      switch ( UNSIGNED_BITS(inst,20,19) ) {
      case 0: ld_s_rr_task (d, null_str); d.fmt = FMT_L_A_B_C; break;
      case 1: ld_s_rr_task (d, byte_str); d.fmt = FMT_L_A_B_C; break;
      case 2: ld_s_rr_task (d, half_str); d.fmt = FMT_L_A_B_C; break;
      case 3: add_s_task (d);             d.fmt = FMT_A_B_C;   break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 13
    //
    case GRP_ADD_SUB_SH_16: {
      is_16bit = true;
      regs_cbu3_16_task (d);
      fmt = FMT_C_B_IMM;
      switch ( UNSIGNED_BITS(inst,20,19) ) {
      case 0: add_s_task (d); break;
      case 1: sub_s_task (d); break;
      case 2: asl_s_task (d); break;
      case 3: asr_s_task (d); break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 14
    //    
    case GRP_MV_CMP_ADD_16: {
      is_16bit = true;
      int subop = (d.isa_opts.is_isa_a6k() && d.isa_opts.new_fmt_14)
                ? ((BITSEL(inst,18) << 2) | UNSIGNED_BITS(inst,20,19))
                : UNSIGNED_BITS(inst,20,19);

      switch (subop) {
      case 0: regs_bbh_16_task (d);  add_s_task (d);    fmt = FMT_B_B_C;   break;
      case 1: regs_hb_16_task (d);   mov_s_task (d);    fmt = FMT_B_C;     break;
      case 2: regs_hb_16_task (d);   cmp_s_task (d);    fmt = FMT_B_C_SRC; break;
      case 3: regs_hb_16_task (d);   mov_s_task (d);    fmt = FMT_C_B;     break;
      case 4: regs_hs3_16_task (d);  add_s_task (d);    fmt = FMT_H_H_S3;  break;
      case 5: regs_hs3_16_task (d);  mov_s_task (d);    fmt = FMT_HD_S3;   break;
      case 6: regs_hs3_16_task (d);  cmp_s_task (d);    fmt = FMT_HS_S3;   break;
      case 7: regs_hb_16_task (d);   mov_s_ne_task (d); fmt = FMT_B_C;     break;
      }
    break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 15
    //
    case GRP_GEN_OPS_16: {
      is_16bit = true;
      // First decode the operands
      switch ( UNSIGNED_BITS(inst,20,16) )
      {
        case 0x0: // SOPs or ZOPs
        {
          switch ( UNSIGNED_BITS(inst,23,21) )
          {
          case 7: // ZOPs
             zero_operand_task (d);
             fmt = FMT_B_IND;
             break;
          default:// SOPs
            regs_b_16_task (d);
            fmt = FMT_B_IND;
            break;
          }
          break;
        default:
          regs_bbc_16_task (d); // not SOPs or ZOPs
          // This should disassemble into:
          // fmt = FMT_B_B_C
          
          // But SNPS want it to look as follows
          //
          fmt = FMT_B_C;
          break;
        }
      }

      // Second decode the operators
      switch ( UNSIGNED_BITS(inst,20,16) ) {
      case 0x0: {// SOPs
        switch ( UNSIGNED_BITS(inst,23,21) ) {
        case 0:
        case 1: j_s_task (d);   break;      // j_s or j_s.d
        case 2:
        case 3: jl_s_task (d);  break;      // jl_s or jl_s.d
        case 6: sub_s_ne_task (d); break;   // sub_s.ne sets its own format
        case 7: // ZOPs
          switch ( UNSIGNED_BITS(inst,26,24) ) {
          case 0:  nop_s_task (d); break;    // nop_s
          case 1:  unimp_s_task (d); break;  // unimp_s
          case 2:  swi_s_task (d); break;    // swi_s
          case 4:  jeq_s_task (d); break;    // jeq_s [blink]
          case 5:  jne_s_task (d); break;    // jne_s [blink]
          case 6:
          case 7:  j_blink_task (d);   break; // j_s [blink]
          default: inst_error_task (d);break;
          }
          break;
        default: inst_error_task (d); break;
        }
        break;
      }
      case 0x02: sub_s_task (d);    break;
      case 0x04: and_s_task (d);    break;
      case 0x05: or_s_task (d);     break;
      case 0x06: bic_s_task (d);    break;
      case 0x07: xor_s_task (d);    break;
      case 0x09: mpyw_s_task(d);    break;
      case 0x0a: mpyuw_s_task(d);   break;
      case 0x0b: tst_s_task (d);    break;          
      case 0x0c: if (d.isa_opts.is_isa_a6k()) mpy_s_task(d); else mul64_s_task(d); break;
      case 0x0d: sexb_s_task (d);   break;
      case 0x0e: sexw_s_task (d);   break;
      case 0x0f: extb_s_task (d);   break;
      case 0x10: extw_s_task (d);   break;
      case 0x11: abs_s_task (d);    break;
      case 0x12: not_s_task (d);    break;
      case 0x13: neg_s_task (d);    break;
      case 0x14: add1_s_task (d);   break;
      case 0x15: add2_s_task (d);   break;
      case 0x16: add3_s_task (d);   break;
      case 0x18: asl_s_task (d);    break;
      case 0x19: lsr_s_task (d);    break;
      case 0x1a: asr_s_task (d);    break;
      case 0x1b: asl_s_task (d);    break;
      case 0x1c: asr_s_task (d);    break;
      case 0x1d: lsr_s_task (d);    break;
      case 0x1e: trap_s_task (d);   break;
      case 0x1f: brk_s_task (d);    break;
      default: inst_error_task (d); break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 16
    //  
    case GRP_LD_WORD_16: {
      is_16bit = true;
      fmt = FMT_L_C_B_IMM;
      opcode = op_name[LD_S_STR];
      regs_b_16_task (d);
      regs_c_16_task (d);
      abs_val = UNSIGNED_BITS(inst,20,16) << 2;
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 17
    //
    case GRP_LD_BYTE_16: {
      is_16bit = true;
      fmt = FMT_L_C_B_IMM;
      opcode = op_name[LD_S_STR];
      regs_b_16_task (d);
      regs_c_16_task (d);
      size_suffix = byte_str;
      abs_val = UNSIGNED_BITS(inst,20,16);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 18
    //  
    case GRP_LD_HALF_16: {
      is_16bit = true;
      fmt = FMT_L_C_B_IMM;
      opcode = op_name[LD_S_STR];
      regs_b_16_task (d);
      regs_c_16_task (d);
      size_suffix = half_str;
      abs_val = UNSIGNED_BITS(inst,20,16) << 1;
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 19
    //  
    case GRP_LD_HALFX_16: {
      is_16bit = true;
      fmt = FMT_L_C_B_IMM;
      opcode = op_name[LD_S_STR];
      regs_b_16_task (d);
      regs_c_16_task (d);
      size_suffix = half_str;
      extend_mode = ext_str;
      abs_val = UNSIGNED_BITS(inst,20,16) << 1;
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 20
    //
    case GRP_ST_WORD_16: {
      is_16bit = true;
      store_16_task (d, null_str, 2);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 21
    //
    case GRP_ST_BYTE_16: {
      is_16bit = true;
      store_16_task (d, byte_str, 0);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 22
    //
    case GRP_ST_HALF_16: {
      is_16bit = true;
      store_16_task (d, half_str, 1);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 23
    //
    case GRP_SH_SUB_BIT_16: {
      is_16bit = true;
      regs_bbu5_16_task (d);
      // Format was
      // fmt = FMT_B_B_IMM
      
      // SNPS want it to be as follows
      //
      fmt = FMT_B_IMM;
      switch ( UNSIGNED_BITS(inst,23,21) ) {
      case 0: asl_s_task (d); break; // asl multiple
      case 1: lsr_s_task (d); break;  // lsr multiple
      case 2: asr_s_task (d); break;  // asr multiple
      case 3: sub_s_task (d);  break;  // subtract
      case 4: bset_s_task (d); break;  // bit-set
      case 5: bclr_s_task (d); break;  // bit-cler
      case 6: bmsk_s_task (d); break;  // bit-mask
      case 7: btst_s_task (d); break;  // bit-test
      default:inst_error_task (d); break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 24
    //
    case GRP_SP_MEM_16: {
      is_16bit = true;
      switch ( UNSIGNED_BITS(inst,23,21) ) {
      case 0: mem_sp_16_task (d, LD_S_STR, null_str, FMT_L_B_C_IMM); break; // ld_s  b,[sp,u7]
      case 1: mem_sp_16_task (d, LD_S_STR, byte_str, FMT_L_B_C_IMM); break; // ldb_s b,[sp,u7]
      case 2: mem_sp_16_task (d, ST_S_STR, null_str, FMT_S_B_C_IMM); break; // st_s  b,[sp,u7]
      case 3: mem_sp_16_task (d, ST_S_STR, byte_str, FMT_S_B_C_IMM); break; // stb_s b,[sp,u7]
      case 4: add_sp_16_task (d);  break; // add_s b,sp,u7
      case 5: if (BITSEL(inst,24) == 0)
          arith_sp_sp_task (d, ADD_S_STR); // add_s sp, sp, u7
        else
          arith_sp_sp_task (d, SUB_S_STR); // sub_s sp, sp, u7
        break;
      case 6:
        if ( BITSEL(inst, 16) == 0)
          leave_s_task (d);              // leave_s[.j][.l][.f] u4
        else if (UNSIGNED_BITS(inst,20,16) == 1)
          stack_b_task (d, POP_S_STR);     // pop_s b
        else if (UNSIGNED_BITS(inst,20,16) == 17)
          stack_blink_task (d, POP_S_STR); // pop_s blink
        else
          inst_error_task (d);
        break;
      case 7:
        if ( BITSEL(inst, 16) == 0)
          enter_s_task (d);               // enter_s[.j][.l][.f] u4
        else if (UNSIGNED_BITS(inst,20,16) == 1)
          stack_b_task (d, PUSH_S_STR);     // push_s b
        else if (UNSIGNED_BITS(inst,20,16) == 17)
          stack_blink_task (d, PUSH_S_STR); // push_s blink
        else
          inst_error_task (d);
        break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 25
    //
    case GRP_GP_MEM_16: {
      is_16bit = true;
      switch ( UNSIGNED_BITS(inst,26,25) ) {
      case 0: r0_gp_16_task (d, LD_S_STR, null_str, FMT_L_A_B_IMM, 2); break; // ld_s  r0,[gp,s11]
      case 1: r0_gp_16_task (d, LD_S_STR, byte_str, FMT_L_A_B_IMM, 0); break; // ldb_s r0,[gp,s9]
      case 2: r0_gp_16_task (d, LD_S_STR, half_str, FMT_L_A_B_IMM, 1); break; // ldw_s r0,[gp,s10]
      case 3: r0_gp_16_task (d, ADD_S_STR, null_str, FMT_A_B_IMM,  2); break; // add_s r0,gp,s11
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 26
    //
    case GRP_LD_PCL_16: {
      is_16bit = true;
      load_pcl_16_task (d);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 27
    //
    case GRP_MV_IMM_16: {
      is_16bit = true;
      fmt = FMT_B_IMM;
      regs_mov_bu8_16_task (d);
      mov_s_task (d);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 28
    //
    case GRP_ADD_IMM_16: {
      is_16bit = true;
      switch ( BITSEL(inst,23) ) {
      case false: // add
        regs_bbu7_16_task (d);
        add_s_task (d);
        fmt = FMT_B_B_IMM;
        break;
      case true:  // cmp
        regs_bu7_16_task (d);
        cmp_s_task (d);
        fmt = FMT_B_IMM_SRC;
        break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 29
    //
    case GRP_BRCC_S_16: {
      is_16bit = true;
      regs_b_16_task (d);
      fmt = FMT_B_0_OFF;
      switch ( BITSEL(inst,23) ) {
      case false: brcc_s_task (d, 1); break;
      case true:  brcc_s_task (d, 2); break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 30
    //
    case GRP_BCC_S_16: {
      is_16bit = true;
      switch ( UNSIGNED_BITS(inst,26,25) ) {
      case 0: bcc_s_task (d, SIGNED_BITS(inst,24,16), 0); break; // b_s
      case 1: bcc_s_task (d, SIGNED_BITS(inst,24,16), 1); break; // beq_s
      case 2: bcc_s_task (d, SIGNED_BITS(inst,24,16), 2); break; // bne_s
      case 3: switch ( UNSIGNED_BITS(inst,24,22) ) {
        case 0: bcc_s_task (d, SIGNED_BITS(inst,21,16), 9 ); break; // bgt_s
        case 1: bcc_s_task (d, SIGNED_BITS(inst,21,16), 10); break; // bge_s
        case 2: bcc_s_task (d, SIGNED_BITS(inst,21,16), 11); break; // blt_s
        case 3: bcc_s_task (d, SIGNED_BITS(inst,21,16), 12); break; // ble_s
        case 4: bcc_s_task (d, SIGNED_BITS(inst,21,16), 13); break; // bhi_s
        case 5: bcc_s_task (d, SIGNED_BITS(inst,21,16), 6 ); break; // bhs_s
        case 6: bcc_s_task (d, SIGNED_BITS(inst,21,16), 5 ); break; // blo_s
        case 7: bcc_s_task (d, SIGNED_BITS(inst,21,16), 14); break; // bls_s
        }
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 31
    //      
    case GRP_BL_S_16: {
      is_16bit = true;
      bl_s_ucond_task (d);
      break;
    }
  }

  // Now convert the instruction components into a single string
  to_string ();
}

      
} } }  // namespace arcsim::isa::arc
