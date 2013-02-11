//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2004 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  This file defines the names of registers in the ARCompact ISA
// 
// =====================================================================

#ifndef INC_SYS_CPU_AUX_REGISTERS_H_
#define INC_SYS_CPU_AUX_REGISTERS_H_

#include "define.h"
#include "api/types.h"

// -----------------------------------------------------------------------------
// CORE REGISTER MAP
//
#define GPR_BASE_REGS 64

#define GP_REG        26
#define FP_REG        27
#define SP_REG        28
#define ILINK1        29
#define ILINK2        30
#define BLINK         31

/* Registers r32-r59 are available for EIA extension purposes */

#define RESERVED_REG  61
#define LP_COUNT      60
#define LIMM_REG      62
#define PCL_REG       63

// -----------------------------------------------------------------------------
// MULTIPLY RESULT REGISTERS (ARC 600 only)
// These extension core registers are defined for the ARC 600 optional multiply.
// NOTE that these registers are READ ONLY. 
//
#define MLO_REG       57
#define MMID_REG      58
#define MHI_REG       59

// -----------------------------------------------------------------------------
// RTSC RESULT REGISTERS (ARC 700 only)
// These extension core registers are defined for the ARC 700 .
// NOTE that these registers are READ ONLY. 
//
#define TSCH_REG      58

// -----------------------------------------------------------------------------
// BASELINE AUXILIARY REGISTER MAP
// (merge in progress: we don't actually have this many right now but we will very soon)
#define __NUM_BUILTIN_AUX_REGS_BASELINE (130+34)

// Special build options that might add to '__NUM_BUILTIN_AUX_REGS_BASELINE'
//
#if (PASTA_CPU_ID_AUX_REG == 1)
  #define NUM_BUILTIN_AUX_REGS  (__NUM_BUILTIN_AUX_REGS_BASELINE)
#else
  #define NUM_BUILTIN_AUX_REGS  __NUM_BUILTIN_AUX_REGS_BASELINE
#endif

#define BUILTIN_AUX_RANGE     0x702

// In ARC6KV2.1, several aux register addresses are reused for different aux registers. Therefore
// some of the aux register addresses in this list are duplicated.

#define AUX_STATUS            0x000
#define AUX_SEMA              0x001
#define AUX_LP_START          0x002
#define AUX_LP_END            0x003
#define AUX_IDENTITY          0x004
#define AUX_DEBUG             0x005
#define AUX_PC                0x006
#define AUX_ADCR              0x007
#define AUX_APCR              0x008
#define AUX_ACR               0x009


// -----------------------------------------------------------------------------
// Mask for STATUS32 AUX register on A600:
//  The ARC 600 processor does not use the A1, A2, AE, DE, U, L or SE fields.
//
//  We need to mask those bits out:
//    - Fields:        A1:3,A2:4,AE:5,DE:6,U:7,L:12,SE:14
//    - Mask-Binary:   1111 1111 1111 1111 1010 1111 0000 0111
//    - Mask-Hex:      0xFFFF AF07
//
const uint32 kA600AuxStatus32Mask = 0xFFFFAF07;


#define AUX_STATUS32          0x00A
#define AUX_STATUS32_P0       0x00B
#define AUX_STATUS32_L1       0x00B
#define AUX_STATUS32_L2       0x00C
#define AUX_USER_SP           0x00D
#define AUX_IRQ_CTRL          0x00E
#define AUX_BFU_FLUSH         0x00F

// READ ONLY REGISTER CONTAINING CPU ID AND NUMBER OF CORES
//
#if PASTA_CPU_ID_AUX_REG
  #define AUX_CPU_ID            0x00D
#endif

#define AUX_IC_IVIC             0x010
#define AUX_IC_CTRL             0x011
#define AUX_IC_LIL              0x013
#define AUX_DMC_CODE_RAM        0x014
#define AUX_TAG_ADDR_MASK       0x015
#define AUX_TAG_DATA_MASK       0x016
#define AUX_LINE_LENGTH_MASK    0x017
#define AUX_DCCM                0x018
#define AUX_IC_IVIL             0x019
#define AUX_IC_RAM_ADDRESS      0x01A
#define AUX_IC_TAG              0x01B
#define AUX_IC_WP               0x01C
#define AUX_IC_DATA             0x01D
// used with MMU verion > 2
#define AUX_IC_PTAG             0x01E

#define AUX_SRAM_SEQ            0x020
#define AUX_COUNT0              0x021
#define AUX_CONTROL0            0x022
#define AUX_LIMIT0              0x023
#define AUX_PCPORT              0x024
#define AUX_INT_VECTOR_BASE     0x025
#define AUX_JLI_BASE            0x290
#define AUX_LDI_BASE            0x291
#define AUX_EI_BASE             0x292

#define AUX_MACMODE             0x041
#define AUX_IRQ_LV12            0x043
#define AUX_IRQ_ACT             0x043

#define AUX_XMAC0               0x044
#define AUX_XMAC1               0x045
#define AUX_XMAC2               0x046
#define AUX_DC_IVDC             0x047
#define AUX_DC_CTRL             0x048
#define AUX_DC_LDL              0x049
#define AUX_DC_IVDL             0x04A
#define AUX_DC_FLSH             0x04B
#define AUX_DC_FLDL             0x04C

#define AUX_HEXDATA             0x050
#define AUX_HEXCTRL             0x051
#define AUX_LED                 0x052
#define AUX_LCDINSTR            0x053
#define AUX_AUX_LCD_DATA        0x053
#define AUX_LDCDATA             0x054
#define AUX_AUX_LCD_CNTRL       0x054
#define AUX_LCDSTAT             0x055
#define AUX_US_COUNT            0x055
#define AUX_DILSTAT             0x056
#define AUX_SWSTAT              0x057
#define AUX_LCD_BUTTON_REGISTER 0x057
#define AUX_DC_RAM_ADDRESS      0x058
#define AUX_DC_TAG              0x059
#define AUX_DC_WP               0x05A
#define AUX_DC_DATA             0x05B
// used with MMU verion > 2
#define AUX_DC_PTAG             0x05C

// -----------------------------------------------------------------------------
//
// 0x060 - 0x07F = Build Configuration Registers (read only)
//
#define AUX_BCR_VER             0x060
#define AUX_DCCM_BASE_BUILD     0x061
#define AUX_BTA_LINK_BUILD      0x063
#define AUX_EA_BUILD            0x065

// -----------------------------------------------------------------------------
// MEMSUBSYS Memory Subsystem Configuration Register
//
//   31                                          4   3   2    1      0
//   +--------------------------------------------+----+----+---+----+
//   | RESERVED                                   | DW | BE | R | EM |
//   +--------------------------------------------+----+----+---+----+
//
//   EM[1:0]: External Memory System Enabled
//     + 0x0: No off chip RAM interface
//     + 0x1: Off chip RAM interface present
//
//   BE[1:0]: Big Endian System Enabled
//     + 0x0: Little Endian Configuration
//     + 0x1: Big Endian Configuration
//
//   DW[1:0]: Memory System Data Width
//     + 0x0: 32-bit data width
//     + 0x1: 64-bit data width
//
#define AUX_MEMSUBSYS           0x067

#define AUX_VECBASE_AC_BUILD    0x068
#define AUX_FP_BUILD            0x06B
#define AUX_DPFP_BUILD          0x06C
#define AUX_MPU_BUILD           0x06D
#define AUX_RF_BUILD            0x06E
#define AUX_D_CACHE_BUILD       0x072
#define AUX_DCCM_BUILD          0x074
#define AUX_TIMER_BUILD         0x075
#define AUX_AP_BUILD            0x076
#define AUX_I_CACHE_BUILD       0x077
#define AUX_ICCM_BUILD          0x078
#define AUX_MULTIPLY_BUILD      0x07B
#define AUX_SWAP_BUILD          0x07C
#define AUX_NORM_BUILD          0x07D
#define AUX_MINMAX_BUILD        0x07E
#define AUX_BARREL_BUILD        0x07F

//
// 0x080 - 0x09E = XY Memory Registers
//
#define AUX_AX0                 0x080
#define AUX_AX1                 0x081
#define AUX_AX2                 0x082
#define AUX_AX3                 0x083
#define AUX_AY0                 0x084
#define AUX_AY1                 0x085
#define AUX_AY2                 0x086
#define AUX_AY3                 0x087
#define AUX_MX00                0x088
#define AUX_MX01                0x089
#define AUX_MX10                0x08A
#define AUX_MX11                0x08B
#define AUX_MX20                0x08C
#define AUX_MX21                0x08D
#define AUX_MX30                0x08E
#define AUX_MX31                0x08F
#define AUX_MY00                0x090
#define AUX_MY01                0x091
#define AUX_MY10                0x092
#define AUX_MY11                0x093
#define AUX_MY20                0x094
#define AUX_MY21                0x095
#define AUX_MY30                0x096
#define AUX_MY31                0x097
#define AUX_XYCONFIG            0x098
#define AUX_BURSTSYS            0x099
#define AUX_BURSTXYM            0x09A
#define AUX_BURSTSZ             0x09B
#define AUX_BURSTVAL            0x09C
#define AUX_XYLSBASEX           0x09D
#define AUX_XYLSBASEY           0x09E

//
// 0x0C0 - 0x0FF = Build Configuration Registers (read only)
//
#define AUX_ISA_CONFIG          0x0C1
#define AUX_STACK_REGION_BUILD  0x0C5
#define AUX_IRQ_BUILD           0x0F3
#define AUX_DMA_CONFIG          0x0FA
#define AUX_SIMD_CONIFG         0x0FB
#define AUX_SIMD_BUILD          0x0FC
#define AUX_SIMD_DMA_BUILD      0x0FD
#define AUX_IFQUEUE_BUILD       0x0FE
#define AUX_SMART_BUILD         0x0FF

// Aux registers above the base set
//
#define AUX_COUNT1              0x100
#define AUX_CONTROL1            0x101
#define AUX_LIMIT1              0x102

//RTC AUX Registers
#define AUX_RTC_CTRL            0x103
#define AUX_RTC_LOW             0x104
#define AUX_RTC_HIGH            0x105

// Virtual cycle and instruction counters, obtained only in simulation
//
#define AUX_CYCLES_LO           0x110
#define AUX_CYCLES_HI           0x111
#define AUX_INSTRS_LO           0x112
#define AUX_INSTRS_HI           0x113

// 0x103 - 0x11F = Reserved
// 0x120 - 0x13F = Reserved
// 0x140 - 0x1FF = Reserved

#define AUX_IRQ_LEV             0x200
#define AUX_IRQ_LEVEL_PENDING   0x200
#define AUX_IRQ_HINT            0x201
#define AUX_ALIGN_CTRL          0x203
#define AUX_ALIGN_ADDR          0x204
#define AUX_ALIGN_SIZE          0x205
#define AUX_IRQ_PRIORITY        0x206
#define AUX_IRQ_LEVEL           0x207
#define AUX_ICCM                0x208
#define AUX_CACHE_LIMIT         0x209
#define AUX_DMP_PER             0x20a

// 0x202 - 0x21F = Reserved

// 0x220 - 0x237 = AP_A* (actionpoints)
//
#define AUX_AP_AMV0             0x220
#define AUX_AP_AMM0             0x221
#define AUX_AP_AC0              0x222
//
#define AUX_AP_AMV1             0x223
#define AUX_AP_AMM1             0x224
#define AUX_AP_AC1              0x225
//
#define AUX_AP_AMV2             0x226
#define AUX_AP_AMM2             0x227
#define AUX_AP_AC2              0x228
//
#define AUX_AP_AMV3             0x229
#define AUX_AP_AMM3             0x22a
#define AUX_AP_AC3              0x22b
//
#define AUX_AP_AMV4             0x22c
#define AUX_AP_AMM4             0x22d
#define AUX_AP_AC4              0x22e
//
#define AUX_AP_AMV5             0x22f
#define AUX_AP_AMM5             0x230
#define AUX_AP_AC5              0x231
//
#define AUX_AP_AMV6             0x232
#define AUX_AP_AMM6             0x233
#define AUX_AP_AC6              0x234
//
#define AUX_AP_AMV7             0x235
#define AUX_AP_AMM7             0x236
#define AUX_AP_AC7              0x237
//
#define AUX_AP_WP_PC            0x23F

// 0x238 - 0x3FF = Reserved

#define AUX_STACK_TOP           0x260
#define AUX_STACK_BASE          0x261
#define AUX_USTACK_TOP          0x260
#define AUX_USTACK_BASE         0x261
#define AUX_KSTACK_TOP          0x264
#define AUX_KSTACK_BASE         0x265

#define AUX_ERET                0x400
#define AUX_ERBTA               0x401
#define AUX_ERSTATUS            0x402
#define AUX_ECR                 0x403
#define AUX_EFA                 0x404
#define AUX_ICAUSE1             0x40A
#define AUX_ICAUSE              0x40A
#define AUX_ICAUSE2             0x40B
#define AUX_IRQ_INTERRUPT       0x40B
#define AUX_IENABLE             0x40C
#define AUX_IRQ_ENABLE          0x40C
#define AUX_ITRIGGER            0x40D
#define AUX_IRQ_TRIGGER         0x40D
#define AUX_IRQ_STATUS          0x40F
#define AUX_XPU                 0x410
#define AUX_BTA                 0x412
#define AUX_BTA_L1              0x413
#define AUX_BTA_L2              0x414
#define AUX_IRQ_PULSE_CANCEL    0x415
#define AUX_IRQ_PENDING         0x416

// -----------------------------------------------------------------------------
// MULTIPLY_BUILD configuration register
//
//   31       24 23           16 15    12 11 10 9   8 7             0
//   +----------+---------------+--------+-----+-----+---------------+
//   | RESERVED | VERSION 16x16 |RESERVED| CYC | TYP | VERSION 32x32 |
//   +----------+---------------+--------+-----+-----+---------------+
//
//   VERSION 32x32: Version of 32x32 Multiply
//      + 0x01: ARC 600 Multiply 32x32, MUL64, MULU64, with special result
//              registers. Slow (10-cycle) and Fast (3-cycle) variants are
//              supported, but are not indicated in this register for version 0x1.
//      + 0x02: ARC 700 Multiply 32x32, MPY, MPYU, with any result register.
//      + 0x04: ARC 600 Multiply 32x32, MUL64, MULU64, with special result
//              registers, new configuration fields TYPE and CYC, and result
//              registers configured to support co-existence with components
//              like XMAC.
//
//  TYP[1:0]: Type of Multiply
//      + 0x0: Slow, 10-cycle
//      + 0x1: Fast, as defined in the CYC field
//
//  CYC[2:0]: Number of cycles
//      + 0x0: 1-Cycle
//      + 0x1: 2-Cycle
//      + 0x2: 3-Cycle
//      + 0x3: 4-Cycle
//
//  VERSION 16x16: Version of 16x16 Multiply
//      + 0x01: ARC 600 Multiply 16x16, MPYW, MPYUW, with any result register
//
#define AUX_MULTIPLY_BUILD      0x07B

// -----------------------------------------------------------------------------
// AUX_MULHI extension auxiliary register
//
// MULHI is used to restore the high part of multiply result register if the
// multiply has been used, for example, by an interrupt service routine. The
// lower part of the multiply result register can be restored by multiplying
// the desired value by 1.
//
#define AUX_MULHI               0x012


// MMU Build Configuration Registers (BCRs)
//
#define AUX_MMU_BUILD           0x06F
#define AUX_DATA_UNCACHED       0x06A
//
// MMU Maintenance and Control Registers
//
#define AUX_TLB_PD0             0x405
#define AUX_TLB_PD1             0x406
#define AUX_TLB_Index           0x407
#define AUX_TLB_Command         0x408
#define AUX_PID                 0x409
#define AUX_MPU_EN              0x409
#define AUX_SASID               0x40E
#define AUX_SCRATCH_DATA0       0x418

#define AUX_MPU_ECR             0x420
#define AUX_MPU_RDB0            0x422
#define AUX_MPU_RDP0            0x423
#define AUX_MPU_RDB1            0x424
#define AUX_MPU_RDP1            0x425
#define AUX_MPU_RDB2            0x426
#define AUX_MPU_RDP2            0x427
#define AUX_MPU_RDB3            0x428
#define AUX_MPU_RDP3            0x429
#define AUX_MPU_RDB4            0x42A
#define AUX_MPU_RDP4            0x42B
#define AUX_MPU_RDB5            0x42C
#define AUX_MPU_RDP5            0x42D
#define AUX_MPU_RDB6            0x42E
#define AUX_MPU_RDP6            0x42F
#define AUX_MPU_RDB7            0x430
#define AUX_MPU_RDP7            0x431
#define AUX_MPU_RDB8            0x432
#define AUX_MPU_RDP8            0x433
#define AUX_MPU_RDB9            0x434
#define AUX_MPU_RDP9            0x435
#define AUX_MPU_RDB10           0x436
#define AUX_MPU_RDP10           0x437
#define AUX_MPU_RDB11           0x438
#define AUX_MPU_RDP11           0x439
#define AUX_MPU_RDB12           0x43A
#define AUX_MPU_RDP12           0x43B
#define AUX_MPU_RDB13           0x43C
#define AUX_MPU_RDP13           0x43D
#define AUX_MPU_RDB14           0x43E
#define AUX_MPU_RDP14           0x43F
#define AUX_MPU_RDB15           0x440
#define AUX_MPU_RDP15           0x441

// EIA aux registers
//
#define AUX_XFLAGS              0x44F

// Floating-point extension auxiliary registers
//
#define AUX_FP_STATUS   0x300
#define AUX_DPFP1L      0x301
#define AUX_DPFP1H      0x302
#define AUX_DPFP2L      0x303
#define AUX_DPFP2H      0x304
#define AUX_DPFP_STATUS 0x305

// SmaRT (Small Real-time Trace) auxiliary registers
//
#define AUX_SMART_CONTROL 0x700
#define AUX_SMART_DATA    0x701

// Simulation Control extension auxiliary register
//
#define AUX_SIM_CONTROL 0x417

// Auxiliary Register LR/SR access permission bits

#define AUX_NONE      0   /* Unimplemented aux register */
#define AUX_K_READ    1   /* Kernel read permission     */
#define AUX_K_WRITE   2   /* Kernel write permission    */
#define AUX_U_READ    4   /* User read permission       */
#define AUX_U_WRITE   8   /* User write permission      */
#define AUX_U_MASKED  16  /* User ZNCV only in STATUS32 */
#define AUX_K_RW      3
#define AUX_U_RW      12
#define AUX_U_R_K_RW  7
#define AUX_ANY_R     5
#define AUX_ANY_W     10
#define AUX_ANY_RW    15
#define AUX_MASKED    (AUX_K_READ+AUX_U_MASKED)
#define AUX_ENABLED   128

// SIMD Register definitions

#define NUM_VECTOR_REGS 32
#define SDM_WORDS       4096

#endif // INC_SYS_CPU_AUX_REGISTERS_H_
