//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2004 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  This file defines all the constants used to perform instruction
//  decode for the ARCompact instruction set.
//
// =====================================================================


#ifndef INC_ISA_ARC_DCODECONST_H_
#define INC_ISA_ARC_DCODECONST_H_

#include "api/types.h"

// -----------------------------------------------------------------------------
// MAJOR OPCODE GROUPS
// There are 32 major opcode groups, each of which is identified by a
// 5-bit field. The following definitions give names to each of these.
//

const uint32 GRP_BRANCH_32     = 0;
const uint32 GRP_BL_BRCC_32    = 1;
const uint32 GRP_LOAD_32       = 2;
const uint32 GRP_STORE_32      = 3;

const uint32 GRP_BASECASE_32   = 4;
const uint32 GRP_ARC_EXT0_32   = 5;
const uint32 GRP_ARC_EXT1_32   = 6;
const uint32 GRP_USR_EXT2_32   = 7;

const uint32 GRP_ARC_EXT0_16   = 8;
const uint32 GRP_ARC_EXT1_16   = 9;
const uint32 GRP_USR_EXT0_16   = 10;
const uint32 GRP_USR_EXT1_16   = 11;

const uint32 GRP_LD_ADD_RR_16  = 12;
const uint32 GRP_ADD_SUB_SH_16 = 13;
const uint32 GRP_MV_CMP_ADD_16 = 14;
const uint32 GRP_GEN_OPS_16    = 15;

const uint32 GRP_LD_WORD_16    = 16;
const uint32 GRP_LD_BYTE_16    = 17;
const uint32 GRP_LD_HALF_16    = 18;
const uint32 GRP_LD_HALFX_16   = 19;

const uint32 GRP_ST_WORD_16    = 20;
const uint32 GRP_ST_BYTE_16    = 21;
const uint32 GRP_ST_HALF_16    = 22;
const uint32 GRP_SH_SUB_BIT_16 = 23;

const uint32 GRP_SP_MEM_16     = 24;
const uint32 GRP_GP_MEM_16     = 25;
const uint32 GRP_LD_PCL_16     = 26;
const uint32 GRP_MV_IMM_16     = 27;

const uint32 GRP_ADD_IMM_16    = 28;
const uint32 GRP_BRCC_S_16     = 29;
const uint32 GRP_BCC_S_16      = 30;
const uint32 GRP_BL_S_16       = 31;

// -----------------------------------------------------------------------------
// MAIN OPERAND FORMATS
// There are four main operand formats in the 0x04
// group of instructions. These are listed below.
//
const uint32 REG_REG_FMT       = 0;
const uint32 REG_U6IMM_FMT     = 1;
const uint32 REG_S12IMM_FMT    = 2;
const uint32 REG_COND_FMT      = 3;

// -----------------------------------------------------------------------------

// The 0x04 group of instruction contains the following
// dual operand instructions (mov is an exception!)
//
const uint32 ADD_OP            = 0x00;
const uint32 ADC_OP            = 0x01;
const uint32 SUB_OP            = 0x02;
const uint32 SBC_OP            = 0x03;
const uint32 AND_OP            = 0x04;
const uint32 OR_OP             = 0x05;
const uint32 BIC_OP            = 0x06;
const uint32 XOR_OP            = 0x07;
const uint32 MAX_OP            = 0x08;
const uint32 MIN_OP            = 0x09;
const uint32 MOV_OP            = 0x0a;
const uint32 TST_OP            = 0x0b;
const uint32 CMP_OP            = 0x0c;
const uint32 RCMP_OP           = 0x0d;
const uint32 RSUB_OP           = 0x0e;
const uint32 BSET_OP           = 0x0f;
const uint32 BCLR_OP           = 0x10;
const uint32 BTST_OP           = 0x11;
const uint32 BXOR_OP           = 0x12;
const uint32 BMSK_OP           = 0x13;
const uint32 ADD1_OP           = 0x14;
const uint32 ADD2_OP           = 0x15;
const uint32 ADD3_OP           = 0x16;
const uint32 SUB1_OP           = 0x17;
const uint32 SUB2_OP           = 0x18;
const uint32 SUB3_OP           = 0x19;
const uint32 JCC_OP            = 0x20;
const uint32 JCC_D_OP          = 0x21;
const uint32 JLCC_OP           = 0x22;
const uint32 JLCC_D_OP         = 0x23;
const uint32 BI_OP             = 0x24;
const uint32 BIH_OP            = 0x25;
const uint32 LDI_OP            = 0x26;
const uint32 AEX_OP            = 0x27;
const uint32 LPCC_OP           = 0x28;
const uint32 FLAG_OP           = 0x29;
const uint32 LR_OP             = 0x2a;
const uint32 SR_OP             = 0x2b;
const uint32 BMSKN_OP          = 0x2c;
const uint32 SOP_FMT           = 0x2f;
const uint32 LD_RR_FMT         = 0x06;

// SETcc opcodes are encoded in the GRP_BASECASE_32 and are
// all dual operand extension instructions
//
const uint32 SETEQ_OP          = 0x38;
const uint32 SETNE_OP          = 0x39;
const uint32 SETLT_OP          = 0x3a;
const uint32 SETGE_OP          = 0x3b;
const uint32 SETLO_OP          = 0x3c;
const uint32 SETHS_OP          = 0x3d;
const uint32 SETLE_OP          = 0x3e;
const uint32 SETGT_OP          = 0x3f;

// The compare-branch instructions are listed
// below, together with their opcodes.
//
const uint32 BREQ_OP           = 0x0;
const uint32 BRNE_OP           = 0x1;
const uint32 BRLT_OP           = 0x2;
const uint32 BRGE_OP           = 0x3;
const uint32 BRLO_OP           = 0x4;
const uint32 BRHS_OP           = 0x5;
const uint32 BBIT0_OP          = 0x6;
const uint32 BBIT1_OP          = 0x7;

// The settings of the four condition bits for 
// each of the compare-branch instructions are
// given below.
//
const uint32 BREQ_COND         = 0x1;
const uint32 BRNE_COND         = 0x2;
const uint32 BRLT_COND         = 0xb;
const uint32 BRGE_COND         = 0xa;
const uint32 BRLO_COND         = 0x5;
const uint32 BRHS_COND         = 0x6;
const uint32 BBIT0_COND        = 0x1;
const uint32 BBIT1_COND        = 0x2;

// All single-operand 32-bit opcodes
//
const uint32 ASL_OP            = 0x00;
const uint32 ASR_OP            = 0x01;
const uint32 LSR_OP            = 0x02;
const uint32 ROR_OP            = 0x03;
const uint32 RRC_OP            = 0x04;
const uint32 SEXB_OP           = 0x05;
const uint32 SEXW_OP           = 0x06;
const uint32 EXTB_OP           = 0x07;
const uint32 EXTW_OP           = 0x08;
const uint32 ABS_OP            = 0x09;
const uint32 NOT_OP            = 0x0a;
const uint32 RLC_OP            = 0x0b;
const uint32 EX_OP             = 0x0c;
const uint32 ROL_OP            = 0x0d;
const uint32 LLOCK_OP          = 0x10;
const uint32 SCOND_OP          = 0x11;
const uint32 ZOP_FMT           = 0x3f;

// All zero-operand instructions
//
const uint32 SLEEP_OP          = 0x01;
const uint32 TRAP0_OP          = 0x02;
const uint32 SYNC_OP           = 0x03;
const uint32 RTIE_OP           = 0x04;
const uint32 BRK_OP            = 0x05;
const uint32 SETI_OP           = 0x06;
const uint32 CLRI_OP           = 0x07;

////////////////////////////////////////////////////////////////////////////////
// Extension opcodes and instruction formats   
////////////////////////////////////////////////////////////////////////////////

// Barrel shift operations are encoded in GRP_ARC_EXT0_32
// and are dual operand instructions
//
const uint32 ASLM_OP           = 0x00;
const uint32 LSRM_OP           = 0x01;
const uint32 ASRM_OP           = 0x02;
const uint32 RORM_OP           = 0x03;

// SWAP operation is encoded in GRP_ARC_EXT0_32
// and is a single operand instruction
//
const uint32 SWAP_OP           = 0x00;
const uint32 SWAPE_OP          = 0x09;
const uint32 LSL16_OP          = 0x0a;
const uint32 LSR16_OP          = 0x0b;
const uint32 ASR16_OP          = 0x0c;
const uint32 ASR8_OP           = 0x0d;
const uint32 LSR8_OP           = 0x0e;
const uint32 LSL8_OP           = 0x0f;
const uint32 ROL8_OP           = 0x10;
const uint32 ROR8_OP           = 0x11;

// NORM operations are encoded in GRP_ARC_EXT0_32
// and are single operand instructions
//
const uint32 NORM_OP           = 0x01;
const uint32 NORMW_OP          = 0x08;
const uint32 FFS_OP            = 0x12;
const uint32 FLS_OP            = 0x13;

// Extended arithmetic operations are encoded in GRP_ARC_EXT0_32
//
// dual operand extended-arith instructions
//
const uint32 ADDS_OP           = 0x06;
const uint32 SUBS_OP           = 0x07;
const uint32 ADDSDW_OP         = 0x28;
const uint32 SUBSDW_OP         = 0x29;
const uint32 ASLS_OP           = 0x0a;
const uint32 ASRS_OP           = 0x0b;

// single operand extended-arith instructions
//
const uint32 NEGS_OP           = 0x07;
const uint32 NEGSW_OP          = 0x06;

// DSP operations are encoded in GRP_ARC_EXT0_32
//
// dual operand DSP instructions
//
const uint32 MUL64_OP          = 0x04;
const uint32 MULU64_OP         = 0x05;
const uint32 DIVAW_OP          = 0x08;

// single operand DSP instructions
//
const uint32 SAT16_OP          = 0x02;
const uint32 RND16_OP          = 0x03;
const uint32 ABSSW_OP          = 0x04;
const uint32 ABSS_OP           = 0x05;

// Multiplication operations are encoded in GRP_BASECASE_32
// and are dual operand instructions
//
const uint32 MPY_OP            = 0x1a;
const uint32 MPYH_OP           = 0x1b;
const uint32 MPYHU_OP          = 0x1c;
const uint32 MPYU_OP           = 0x1d;
const uint32 MPYW_OP           = 0x1e;
const uint32 MPYWU_OP          = 0x1f;

// DIV, DIVU, REM, REMU operations are encoded in GRP_ARC_EXT0_32
// These currently reuse the opcodes of DIVAW, MUL64 and MULU64, 
// which are not included in the ARC 6000 ISA.
//
const uint32 DIV_OP            = 0x04;
const uint32 DIVU_OP           = 0x05;
const uint32 REM_OP            = 0x08;
const uint32 REMU_OP           = 0x09;

// FPX extension instructions are encoded in GRP_ARC_EXT1_32
//
const uint32 FMUL_OP           = 0x00;
const uint32 FADD_OP           = 0x01;
const uint32 FSUB_OP           = 0x02;

const uint32 DMULH11_OP        = 0x08;
const uint32 DMULH12_OP        = 0x09;
const uint32 DMULH21_OP        = 0x0a;
const uint32 DMULH22_OP        = 0x0b;

const uint32 DADDH11_OP        = 0x0c;
const uint32 DADDH12_OP        = 0x0d;
const uint32 DADDH21_OP        = 0x0e;
const uint32 DADDH22_OP        = 0x0f;

const uint32 DSUBH11_OP        = 0x10;
const uint32 DSUBH12_OP        = 0x11;
const uint32 DSUBH21_OP        = 0x12;
const uint32 DSUBH22_OP        = 0x13;

const uint32 DRSUBH11_OP       = 0x14;
const uint32 DRSUBH12_OP       = 0x15;
const uint32 DRSUBH21_OP       = 0x16;
const uint32 DRSUBH22_OP       = 0x17;

const uint32 DEXCL1_OP         = 0x18;
const uint32 DEXCL2_OP         = 0x19;


#endif  // INC_ISA_ARC_DCODECONST_H_
