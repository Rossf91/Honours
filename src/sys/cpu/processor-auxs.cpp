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
// This file implements the Processor methods for reading and writing
// to auxiliary registers during simulation.
//
// Accesses to certain registers will raise exceptions in User mode.
// Writes to read-only registers will raise exceptions in any mode.
// Writes to certain registers will be reflected in other parts of the
// processor state, especially STATUS32 - which has each bit copied to
// the corresponding explicit status bit in the cpuState structure.
//
// Operations involving the MMU auxiliary registers, and other extension
// registers are routed through this API, but are implemented in other
// extension-specific functions, which are contained in other files.
//
// =====================================================================

#include <iomanip>
#include <cmath>

#include "Assertion.h"

#include "sys/cpu/aux-registers.h"
#include "sys/cpu/processor.h"
#include "exceptions.h"
#include "sys/cpu/EiaExtensionManager.h"
#include "ise/eia/EiaAuxRegisterInterface.h"

#include "util/Log.h"
#include "util/Counter.h"

// -----------------------------------------------------------------------------
// Uncomment the following to enable verbose debugging of AUX register
// writes/reads
//
// #define DEBUG_WRITE_AUX_REGISTER
// #define DEBUG_READ_AUX_REGISTER

#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_

#define UPDATE_SUCCESS(_ctrl_,_bit_) {    \
  if ((_bit_))                            \
    state.auxs[(_ctrl_)] |= 0x00000008UL; \
  else                                    \
    state.auxs[(_ctrl_)] &= 0xfffffff7UL; \
}

namespace arcsim {
  namespace sys {
    namespace cpu {

// Define information structure describing built-in auxiliary
// registers.
//
struct aux_info_s {
  uint32 address;             // 32-bit address of aux register
  uint32 reset_value;         // initial value on reset
  uint32 valid_mask;          // 32-bit vector; 1=>implemented, 0=>reserved
  unsigned char permissions;  // SR/LR access permissions
};

// Table of information about each built-in auxiliary register.
//
struct aux_info_s aux_reg_info [NUM_BUILTIN_AUX_REGS] = 
{
  { AUX_STATUS,           0x00000000,  0xfeffffff,  AUX_K_READ }, 
  { AUX_SEMA,             0x00000000,  0x0000000f,  AUX_K_RW   }, 
  { AUX_LP_START,         0x00000000,  0xffffffff,  AUX_ANY_RW }, 
  { AUX_LP_END,           0x00000000,  0xffffffff,  AUX_ANY_RW },  
  { AUX_IDENTITY,         0x00000031,  0xffffffff,  AUX_ANY_R  }, 
  { AUX_DEBUG,            0x00000000,  0xf0800803,  AUX_K_READ }, 
  { AUX_PC,               0x00000000,  0xfffffffe,  AUX_ANY_R  }, 
  { AUX_STATUS32,         0x00000000,  0x00003fff,  AUX_ANY_R  }, 
  
  //This register is replaced by AUX_STATUS32_P1 in ARC6KV2.1
  { AUX_STATUS32_L1,      0x00000000,  0x00003ffe,  AUX_K_RW   }, 
  
  { AUX_STATUS32_L2,      0x00000000,  0x00003ffe,  AUX_K_RW   }, // 10
  
  // New Interrupt System Registers
  { AUX_IRQ_CTRL,         0x00000000,  0x00001e1f,  AUX_K_RW   },
  { AUX_IRQ_STATUS,       0x00000000,  0x8000003f,  AUX_K_READ },
  { AUX_USER_SP,          0x00000000,  0xffffffff,  AUX_K_RW   },

#if PASTA_CPU_ID_AUX_REG
  { AUX_CPU_ID,           0x00000000,  0xffffffff,  AUX_ANY_R  },
#endif

  { AUX_COUNT0,           0x00000000,  0xffffffff,  AUX_K_RW   }, 
  { AUX_CONTROL0,         0x00000000,  0x0000000f,  AUX_K_RW   },
  { AUX_LIMIT0,           0x00ffffff,  0xffffffff,  AUX_K_RW   },
  { AUX_INT_VECTOR_BASE,  0x00000000,  0xfffffc00,  AUX_K_RW   },
  { AUX_JLI_BASE,         0x00000000,  0xfffffffc,  AUX_ANY_RW },
  { AUX_LDI_BASE,         0x00000000,  0xfffffffc,  AUX_ANY_RW },
  { AUX_EI_BASE,          0x00000000,  0xfffffffc,  AUX_ANY_RW }, //10
  { AUX_MACMODE,          0x00000000,  0x00000212,  AUX_K_RW   },
  
  //This register is replaced by AUX_IRQ_ACT when the new interrupt system is enabled
  { AUX_IRQ_LV12,         0x00000000,  0x00000003,  AUX_K_RW   }, 
  
  //
  //  Build Configuration Registers (read only)
  //
  { AUX_BCR_VER,          0x00000000,  0xffffffff,  AUX_K_READ },
  { AUX_BTA_LINK_BUILD,   0x00000000,  0x00000001,  AUX_K_READ },
  { AUX_EA_BUILD,         0x00000000,  0x000000ff,  AUX_K_READ },
  { AUX_VECBASE_AC_BUILD, 0x00000000,  0xffffffff,  AUX_K_READ },
  { AUX_MPU_BUILD,        0x00000000,  0x0000ffff,  AUX_K_READ },
  { AUX_RF_BUILD,         0x00000000,  0x000007ff,  AUX_K_READ },
  { AUX_FP_BUILD,         0x00000102,  0x000001ff,  AUX_K_READ },
  { AUX_DPFP_BUILD,       0x00000102,  0x000001ff,  AUX_K_READ },
  { AUX_TIMER_BUILD,      0x00000000,  0x000003ff,  AUX_K_READ },
  { AUX_AP_BUILD,         0x00000000,  0x00000fff,  AUX_K_READ },
  { AUX_MULTIPLY_BUILD,   0x00000000,  0x00ff0fff,  AUX_K_READ },
  { AUX_SWAP_BUILD,       0x00000000,  0x000000ff,  AUX_K_READ },
  { AUX_NORM_BUILD,       0x00000000,  0x000000ff,  AUX_K_READ },
  { AUX_MINMAX_BUILD,     0x00000000,  0x000000ff,  AUX_K_READ },
  { AUX_BARREL_BUILD,     0x00000000,  0x000003ff,  AUX_K_READ },
  { AUX_ISA_CONFIG,       0x00000000,  0xffffffff,  AUX_K_READ },
  { AUX_STACK_REGION_BUILD,0x00000000, 0x000000ff,  AUX_K_READ },
  { AUX_SMART_BUILD,      0x00000000,  0xfffffcff,  AUX_K_READ }, // 16
  //
  // ARCmedia BCRs - will need updating for VRaptor...
  //
  { AUX_DMA_CONFIG,       0x00000000,  0xffffffff,  AUX_K_READ },
  { AUX_SIMD_CONIFG,      0x00000000,  0xffffffff,  AUX_K_READ },
  { AUX_SIMD_BUILD,       0x00000000,  0xffffffff,  AUX_K_READ },
  { AUX_SIMD_DMA_BUILD,   0x00000000,  0xffffffff,  AUX_K_READ }, // 4
  //
  // Aux registers above the base set
  //
  { AUX_COUNT1,           0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_CONTROL1,         0x00000000,  0x0000000f,  AUX_K_RW   },
  { AUX_LIMIT1,           0x00ffffff,  0xffffffff,  AUX_K_RW   },
  
  //RTC Aux registers
  
  { AUX_RTC_CTRL,         0x00000000,  0xb0000003,  AUX_U_R_K_RW},
  { AUX_RTC_LOW,          0x00000000,  0xffffffff,  AUX_ANY_R }, 
  { AUX_RTC_HIGH,         0x00000000,  0xffffffff,  AUX_ANY_R }, 
  
  //This register is replaced by AUX_LEVEL_PENDING in ARC6KV2.1
  { AUX_IRQ_LEV,          0xc0000002,  0xfffffff8,  AUX_K_RW   },
  { AUX_IRQ_HINT,         0x00000000,  0x000000ff,  AUX_K_RW   }, //10
  { AUX_ALIGN_CTRL,       0x00000000,  0x80000001,  AUX_K_RW   },
  { AUX_ALIGN_ADDR,       0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_ALIGN_SIZE,       0x00000000,  0x00000003,  AUX_K_RW   },
  { AUX_IRQ_PRIORITY,     0x00000001,  0x0000000F,  AUX_K_RW   },
  { AUX_IRQ_LEVEL,        0x00000000,  0x0000000F,  AUX_K_RW   },
  { AUX_ERET,             0x00000000,  0xfffffffe,  AUX_K_RW   },
  { AUX_ERBTA,            0x00000000,  0xfffffffe,  AUX_K_RW   },
  { AUX_ERSTATUS,         0x00000000,  0x00003ffe,  AUX_K_RW   },
  { AUX_ECR,              0x00000000,  0x00ffffff,  AUX_K_RW   },
  { AUX_EFA,              0x00000000,  0xffffffff,  AUX_K_RW   }, //10
  
  //This register is replaced by AUX_ICAUSE in ARC6KV2.1
  { AUX_ICAUSE1,          0x00000000,  0x0000001f,  AUX_K_RW   },
  
  //This register is replaced by AUX_IRQ_INTERRUPT in ARC6KV2.1
  { AUX_ICAUSE2,          0x00000000,  0x0000001f,  AUX_K_RW   },
  
  //This register is replaced by AUX_IRQ_ENABLE in ARC6KV2.1
  { AUX_IENABLE,          0xffffffff,  0xffffffff,  AUX_K_RW   },
  
  //This register is replaced by AUX_IRQ_TRIGGER in ARC6KV2.1
  { AUX_ITRIGGER,         0x00000000,  0xfffffff8,  AUX_K_RW   },
  { AUX_XPU,              0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_BTA,              0x00000000,  0xfffffffe,  AUX_K_RW   },
  { AUX_BTA_L1,           0x00000000,  0xfffffffe,  AUX_K_RW   },
  { AUX_BTA_L2,           0x00000000,  0xfffffffe,  AUX_K_RW   },
  { AUX_IRQ_PULSE_CANCEL, 0x00000000,  0xfffffffa,  AUX_K_WRITE},
  { AUX_IRQ_PENDING,      0x00000000,  0xfffffff8,  AUX_K_READ },
  { AUX_XFLAGS,           0x00000000,  0x0000000f,  AUX_ANY_RW }, 
  { AUX_AP_WP_PC,         0x00000000,  0xffffffff,  AUX_K_READ }, // 25

  { AUX_STACK_TOP,        0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_STACK_BASE,       0x00000000,  0xffffffff,  AUX_K_RW   },

  { AUX_KSTACK_TOP,       0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_KSTACK_BASE,      0x00000000,  0xffffffff,  AUX_K_RW   },
  //
  // Optional extension auxiliary registers
  //
  { AUX_MULHI,            0x00000000,  0xffffffff,  AUX_K_WRITE}, // 1
  //
  // MMU Build Configuration Registers (BCRs)
  //
  { AUX_MMU_BUILD,        0x00000000,  0xffffffff,  AUX_ANY_R  },
  { AUX_DATA_UNCACHED,    0xc0000601,  0xffffffff,  AUX_ANY_R  }, // 2
  //
  // MMU Maintenance and Control Registers
  //
  { AUX_TLB_PD0,          0x00000000,  0x7fffe5ff,  AUX_K_RW   }, // NOTE: dafault mask for CompatPD0 mode
  { AUX_TLB_PD1,          0x00000000,  0xffffe1fc,  AUX_K_RW   }, // NOTE: dafault mask for CompatPD1 mode
  { AUX_TLB_Index,        0x00000000,  0x800007ff,  AUX_K_RW   },
  { AUX_TLB_Command,      0x00000000,  0xffffffff,  AUX_K_WRITE},
  { AUX_PID,              0x00000000,  0xA00000ff,  AUX_K_RW   },
  { AUX_SASID,            0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_SCRATCH_DATA0,    0x00000000,  0xffffffff,  AUX_K_RW   }, // 7
  //
  // Floating-point extension registers
  //
  { AUX_FP_STATUS,        0x00000000,  0x0000000f,  AUX_ANY_R  },
  { AUX_DPFP1L,           0x00000000,  0xffffffff,  AUX_ANY_RW },
  { AUX_DPFP1H,           0x00000000,  0xffffffff,  AUX_ANY_RW },
  { AUX_DPFP2L,           0x00000000,  0xffffffff,  AUX_ANY_RW },
  { AUX_DPFP2H,           0x00000000,  0xffffffff,  AUX_ANY_RW },
  { AUX_DPFP_STATUS,      0x00000000,  0x0000000f,  AUX_ANY_R  }, // 6
  //
  // Memory Subsystem Configuration register
  //
  { AUX_MEMSUBSYS,        0x00000001,  0x0000000d,  AUX_ANY_R  }, // 1
  //
  // ICCM/DCCM auxiliary registers
  //
  //
  { AUX_ICCM,             0x00000000,  0xffffffff,  AUX_K_RW   },  
  { AUX_DCCM,             0x80000000,  0xffffffff,  AUX_K_RW   },  
  { AUX_ICCM_BUILD,       0x00000000,  0xffffffff,  AUX_K_READ },  
  { AUX_DCCM_BUILD,       0x00000000,  0x00000fff,  AUX_K_READ },
  { AUX_DCCM_BASE_BUILD,  0x00100000,  0xffffffff,  AUX_K_READ }, // 5
  //
  // I-cache and D-cache auxiliary registers
  //
  { AUX_IC_IVIC,          0x00000000,  0x00000000,  AUX_K_WRITE},
  { AUX_IC_CTRL,          0x00000000,  0x00000039,  AUX_K_RW   },
  { AUX_IC_LIL,           0x00000000,  0xffffffff,  AUX_K_WRITE},
  { AUX_IC_IVIL,          0x00000000,  0xffffffff,  AUX_K_WRITE},
  { AUX_I_CACHE_BUILD,    0x00000000,  0x003fffff,  AUX_K_READ },
  { AUX_IC_RAM_ADDRESS,   0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_IC_TAG,           0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_IC_DATA,          0x00000000,  0xffffffff,  AUX_K_RW   }, 
  { AUX_IC_PTAG,          0x00000000,  0xffffffff,  AUX_K_WRITE},// 9
  //
  { AUX_DC_IVDC,          0x00000000,  0x00000001,  AUX_K_WRITE},
  { AUX_DC_CTRL,          0x000000c2,  0x000001fd,  AUX_K_RW   },
  { AUX_DC_LDL,           0x00000000,  0xffffffff,  AUX_K_WRITE},
  { AUX_DC_IVDL,          0x00000000,  0xffffffff,  AUX_K_WRITE},
  { AUX_DC_FLSH,          0x00000000,  0x00000001,  AUX_K_WRITE},
  { AUX_DC_FLDL,          0x00000000,  0xffffffff,  AUX_K_WRITE},
  { AUX_D_CACHE_BUILD,    0x00000000,  0x003fffff,  AUX_K_READ },
  { AUX_DC_RAM_ADDRESS,   0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_DC_TAG,           0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_DC_DATA,          0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_DC_PTAG,          0x00000000,  0xffffffff,  AUX_K_WRITE},

  
  //
  // IRQ Build Register
  //
  { AUX_IRQ_BUILD,        0x00000000,  0xffffffff,  AUX_K_READ },
  
  //
  // Instruction Fetch Queue build register
  //
  { AUX_IFQUEUE_BUILD,    0x00000000,  0x00000303,  AUX_K_READ }, // 1
  //
  // Memory architecture control registers
  //
  { AUX_CACHE_LIMIT,      0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_DMP_PER,          0x00000000,  0xffffffff,  AUX_K_RW   }, // 2
  //
  // SmaRT address and data registers
  //
  { AUX_SMART_CONTROL,    0x00000000,  0xffffffff,  AUX_K_RW   },
  { AUX_SMART_DATA,       0x00000000,  0xffffffff,  AUX_K_READ }, // 2
  //
  // Simulation Control extension auxiliary register
  //
  { AUX_SIM_CONTROL,      0x00000000,  0xffffffff,  AUX_ANY_RW }, // 1
  // Simulation counter auxiliary registers
  //
  { AUX_CYCLES_LO,        0x00000000,  0xffffffff,  AUX_ANY_R  },
  { AUX_CYCLES_HI,        0x00000000,  0xffffffff,  AUX_ANY_R  },
  { AUX_INSTRS_LO,        0x00000000,  0xffffffff,  AUX_ANY_R  },
  { AUX_INSTRS_HI,        0x00000000,  0xffffffff,  AUX_ANY_R  }  // 4
};

// -----------------------------------------------------------------------------
// List of baseline aux registers defined for ARC 600
//
const uint32 baseline_aux_regs_a600[] = {
  AUX_IDENTITY,
  AUX_DEBUG,
  AUX_PC,
  AUX_STATUS32,
  AUX_STATUS32_L1,
  AUX_STATUS32_L2,
  AUX_BFU_FLUSH,
  AUX_INT_VECTOR_BASE,
  AUX_MACMODE,
  AUX_IRQ_LV12,
  AUX_BCR_VER,
  AUX_VECBASE_AC_BUILD,
  AUX_RF_BUILD,
  AUX_MINMAX_BUILD,
  AUX_IRQ_LEV,
  AUX_IRQ_HINT,
  AUX_ALIGN_CTRL,
  AUX_ALIGN_ADDR,
  AUX_ALIGN_SIZE,
  AUX_ISA_CONFIG,
  0
};

// -----------------------------------------------------------------------------
// List of baseline aux registers defined for ARC700
//
const uint32 baseline_aux_regs_a700[] = {
  AUX_IDENTITY,
  AUX_DEBUG,
  AUX_PC,
  AUX_STATUS32,
  AUX_STATUS32_L1,
  AUX_STATUS32_L2,
  AUX_BFU_FLUSH,
  AUX_INT_VECTOR_BASE,
  AUX_MACMODE,
  AUX_IRQ_LV12,
  AUX_BCR_VER,
  AUX_BTA_LINK_BUILD,
  AUX_VECBASE_AC_BUILD,
  AUX_RF_BUILD,
  AUX_MINMAX_BUILD,
  AUX_IRQ_LEV,
  AUX_IRQ_HINT,
  AUX_STACK_TOP,
  AUX_STACK_BASE,
  AUX_ERET,
  AUX_ERBTA,
  AUX_ERSTATUS,
  AUX_ECR,
  AUX_ICAUSE1,
  AUX_ICAUSE2,
  AUX_IENABLE,
  AUX_ITRIGGER,
  AUX_BTA,
  AUX_IRQ_PULSE_CANCEL,
  AUX_IRQ_PENDING,
  0
};      
      
// -----------------------------------------------------------------------------
// List of baseline aux registers defined for ARCompact V2
//
const uint32 baseline_aux_regs_av2[] = {
  AUX_IDENTITY,
  AUX_DEBUG,
  AUX_PC,
  AUX_STATUS32,
  AUX_STATUS32_L1,
  AUX_STATUS32_L2,
  AUX_INT_VECTOR_BASE,
  AUX_BCR_VER,
  AUX_BTA_LINK_BUILD,
  AUX_VECBASE_AC_BUILD,
  AUX_RF_BUILD,
  AUX_MINMAX_BUILD,
  AUX_ISA_CONFIG,
  AUX_STACK_REGION_BUILD,
  AUX_IRQ_LEV,
  AUX_IRQ_HINT,
  AUX_IRQ_LV12,
  AUX_ERET,
  AUX_ERBTA,
  AUX_ERSTATUS,
  AUX_ECR,
  AUX_ICAUSE1,
  AUX_ICAUSE2,
  AUX_IENABLE,
  AUX_ITRIGGER,
  AUX_BTA,
  AUX_IRQ_PULSE_CANCEL,
  AUX_IRQ_PENDING,
  0
};

// -----------------------------------------------------------------------------
// List of baseline aux registers defined for ARCompact V2.1
//
const uint32 baseline_aux_regs_av21[] = {
  AUX_IDENTITY,
  AUX_DEBUG,
  AUX_PC,
  AUX_STATUS32,

  AUX_USER_SP,
  AUX_IRQ_CTRL,
  AUX_IRQ_ACT,
  AUX_IRQ_LEVEL,
  AUX_ICAUSE,
  AUX_IRQ_LEVEL_PENDING,
  AUX_IRQ_INTERRUPT,
  AUX_IRQ_PRIORITY,
  AUX_IRQ_PENDING,
  AUX_IRQ_ENABLE,
  AUX_IRQ_TRIGGER,
  AUX_IRQ_PULSE_CANCEL,
  AUX_IRQ_STATUS,

  AUX_IRQ_HINT,
  
  AUX_INT_VECTOR_BASE,
  AUX_BCR_VER,
  AUX_BTA_LINK_BUILD,
  AUX_VECBASE_AC_BUILD,
  AUX_RF_BUILD,
  AUX_MINMAX_BUILD,
  AUX_ISA_CONFIG,
  AUX_IRQ_BUILD,
  AUX_ERET,
  AUX_ERBTA,
  AUX_ERSTATUS,
  AUX_ECR,
  AUX_BTA,
  
  0
};

// -----------------------------------------------------------------------------
// Read auxiliary register
//
bool
Processor::read_aux_register (uint32 aux_addr, uint32 *data, bool from_sim)
{
  unsigned char perms;
  unsigned char reqd;

#ifdef DEBUG_READ_AUX_REGISTER
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] read_aux_register: aux-addr = 0x" << HEX(aux_addr);
#endif

  // Default value of an unimplemented aux register is zero.
  //
  *data = 0;
  
  // Detect unimplemented aux register, raising exception if access is made to one
  // that does not exist.
  //
  if (aux_addr >= BUILTIN_AUX_RANGE) {
    // If not in the built-in auxiliary register range, the check if aux_addr
    // selects a defined extension aux register.
    //
    if (eia_mgr.are_eia_aux_regs_defined) {
      std::map<uint32, ise::eia::EiaAuxRegisterInterface*>::const_iterator R;
      if (( R = eia_mgr.eia_aux_reg_map.find(aux_addr) ) != eia_mgr.eia_aux_reg_map.end())
      {
        *data = R->second->get_value();
        return true;
      }
    }
    
    if (from_sim) {
      if (sys_arch.isa_opts.is_isa_a600()) { // A600 semantics
        *data = 0; // reading non-existant extension aux register returns '0'
        return true;
      } else { // A700 and ARCompact v2 semantics
        LOG(LOG_DEBUG) << "[AUX-READ] Unimplemented EXT AUX REG: 0x" << HEX(aux_addr);
        enter_exception (ECR(sys_arch.isa_opts.EV_InstructionError, IllegalInstruction, 0), state.pc, state.pc);
        return false;
      }
    }
    return false;
  }

#ifdef DEBUG_READ_AUX_REGISTER
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] \taux-reg is within built-in range";
#endif

  // Check existence of aux register in the builtin range, raising illegal instruction
  // exception if register is absent, unless register is in the BCR range, in which
  // case return 0 if we are in Kernel mode, without raising an exception.
  //
  if  (!(perms = aux_perms[aux_addr])) {
    if (    from_sim                                                // not a debug read
         && (   !(    ((aux_addr >= 0x060) && (aux_addr < 0x0080)) // or aux address is
                    || ((aux_addr >= 0x0C0) && (aux_addr < 0x0100)) // not a BCR
                    || (aux_addr == AUX_XFLAGS)                     // not XFLAGS
                  )
            )
        )
    {
      if (sys_arch.isa_opts.is_isa_a600()) { // A600 semantics
        *data = state.auxs[AUX_IDENTITY]; // reading non-existant aux register returns ID register
        return true;
      } else { // A700 and ARCompact v2 semantics
        LOG(LOG_DEBUG) << "[AUX-READ] Unimplemented AUX REG: 0x" << HEX(aux_addr);
        enter_exception (ECR(sys_arch.isa_opts.EV_InstructionError, IllegalInstruction, 0), state.pc, state.pc);
        return false;
      }
    }
    
    if(from_sim && state.U){ //If it was a user mode read to a BCR it is a privilege exception in a6kv21
        LOG(LOG_DEBUG) << "[AUX-READ] Unimplemented AUX REG: 0x" << HEX(aux_addr);
        if(sys_arch.isa_opts.is_isa_a6kv2())
          enter_exception (ECR(sys_arch.isa_opts.EV_PrivilegeV, PrivilegeViolation, 0), state.pc, state.pc);
        else
          enter_exception (ECR(sys_arch.isa_opts.EV_InstructionError, IllegalInstruction, 0), state.pc, state.pc);

    }
    
    // This aux read returns a silent failure, allowing a zero result to be
    // committed to the destination register.
    //
    return true;  
  }

#ifdef DEBUG_READ_AUX_REGISTER
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] \taux-reg exists";
#endif

  // Check whether this auxiliary register is readable, and if not raise an
  // IllegalInstruction exception regardless of operating mode.
  //
  if (!(perms & AUX_ANY_R) && from_sim) {
    if (sys_arch.isa_opts.is_isa_a600()) { // A600 semantics
      *data = state.auxs[AUX_IDENTITY]; // reading a write only aux register returns ID register
      return true;
    } else { // A700 and ARCompact v2 semantics
      LOG(LOG_DEBUG) << "[AUX-READ] READ PERMISSION DENIED AUX REG: 0x" << HEX(aux_addr);
      enter_exception (ECR(sys_arch.isa_opts.EV_InstructionError, IllegalInstruction, 0), state.pc, state.pc);
      return false;
    }
  }

  // Next, check whether the current operating mode has required privilege, and
  // if not raise a PrivilegeViolation exception.
  //  
  reqd  = state.U ? AUX_U_READ : AUX_K_READ;
    
  if (!(perms & reqd) && from_sim) {
    enter_exception (ECR(sys_arch.isa_opts.EV_PrivilegeV, PrivilegeViolation, 0), state.pc, state.pc);
    return false;
  }

#ifdef DEBUG_READ_AUX_REGISTER
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] \tread permission is granted";
#endif
  
  // If there are any LR address Actionpoints defined, check the current
  // aux_addr to see if an Actionpoint would be triggered.
  //
  if (aps.has_lr_addr_aps()) {
    aps.match_lr_addr(aux_addr);
  } else {
    aps.clear_trigger();
  }
  
  // Mask the read data according to the implemented bits
  // in the selected aux register.
  //
  uint32 rmask = aux_mask[aux_addr];
  uint32 rdata = 0;
  
  // Deal with any associated side effects for aux registers that
  // have such side effects defined on reads. For example,
  // some aux values are not obtained from the state.auxs[] array
  // but are supplied by exploded status bits (e.g. STATUS32).
  //
  switch (aux_addr)
  {
    case  AUX_STATUS:
      {
        rdata = (state.Z  << 31) | (state.N  << 30)
              | (state.C  << 29) | (state.V  << 28)
              | (state.E2 << 27) | (state.E1 << 26)
              | (state.H  << 25) | ((state.next_pc >> 2) & 0x00ffffffUL);
        break;
      }
      
    case  AUX_DEBUG:
      {
        rdata = state.auxs[AUX_DEBUG] & (aux_mask[AUX_DEBUG] ^ 0x2UL);
        break;
      }

    // Timer0 and Timer1 aux register read functions
    //  - reading CONTROLn and LIMITn requires no special semantics
    //  - reading COUNTn requires a call into the timer module
    //
    case  AUX_CONTROL0:
    case  AUX_LIMIT0:
    case  AUX_CONTROL1:
    case  AUX_LIMIT1:
      {
        rdata = state.auxs[aux_addr];
        break;
      }
    case AUX_RTC_CTRL:
    {
      rdata = state.auxs[aux_addr];
      break;
    }
      
    case  AUX_COUNT0:
      {
        if (from_sim) {
          rdata = timer_get_count (0);
        } else {
          // if read from an external agent, just read the register directly
          // as a call to timer_get_count() has side-effects.
          rdata = state.auxs[aux_addr];
        }
        break;
      }
      
    case  AUX_COUNT1:
      {
        if (from_sim) {
          rdata = timer_get_count (1);
        }  else {
          // if read from an external agent, just read the register directly
          // as a call to timer_get_count() has side-effects.
          rdata = state.auxs[aux_addr];
        }
        break;
      } 
      
    case  AUX_CYCLES_LO:
      {
#ifdef CYCLE_ACC_SIM
        rdata = state.auxs[AUX_CYCLES_LO] = (uint32)cnt_ctx.cycle_count.get_value();
        state.auxs[AUX_CYCLES_HI] = (uint32)(cnt_ctx.cycle_count.get_value() >> 32);
#endif
        break;
      }
      
    case  AUX_CYCLES_HI:
    {
#ifdef CYCLE_ACC_SIM
        state.auxs[AUX_CYCLES_LO] = (uint32)cnt_ctx.cycle_count.get_value();
        rdata = state.auxs[AUX_CYCLES_HI] = (uint32)(cnt_ctx.cycle_count.get_value() >> 32);
#endif
        break;
      }
      
    case  AUX_INSTRS_LO:
      {
        rdata = state.auxs[AUX_INSTRS_LO] = (uint32)instructions();
        state.auxs[AUX_INSTRS_HI] = (uint32)(instructions() >> 32);
        break;
      }
      
    case  AUX_INSTRS_HI:
      {
        state.auxs[AUX_INSTRS_LO] = (uint32)instructions();
        rdata = state.auxs[AUX_INSTRS_HI] = (uint32)(instructions() >> 32);
        break;
      }
    case AUX_RTC_LOW:
    {
      rdata = get_rtc_low();
      break;
    }
    case AUX_RTC_HIGH:
    {
      rdata = get_rtc_high();
      break;
    }
    
    case AUX_IRQ_STATUS:
    {
      uint16 int_num = state.auxs[AUX_IRQ_INTERRUPT];
      rdata = 0;
      if(is_interrupt_configured(int_num))
      {
        rdata |= state.irq_priority[int_num];
        rdata |= is_interrupt_enabled(int_num) << 4;
        rdata |= state.irq_trigger[int_num]     << 5;
        rdata |= (is_interrupt_pending(int_num) | (state.auxs[AUX_IRQ_HINT] == int_num)) << 31;
      }
      break;
    }
    
    case  AUX_IRQ_CTRL: //A6kv2.1 global
    case  AUX_SEMA:
    case  AUX_LP_START:
    case  AUX_LP_END:
    case  AUX_IDENTITY:
    case  AUX_STATUS32_L1: //A6kv2.1 : AUX_STATUS32_P1
    case  AUX_STATUS32_L2: 
    case  AUX_USER_SP:
    case  AUX_INT_VECTOR_BASE:
    case  AUX_JLI_BASE:
    case  AUX_LDI_BASE:
    case  AUX_EI_BASE:
    case  AUX_MACMODE:
    case  AUX_IRQ_LV12: //A6kv2.1 : AUX_IRQ_ACT
    case  AUX_BCR_VER:
    case  AUX_BTA_LINK_BUILD:
    case  AUX_EA_BUILD:
    case  AUX_VECBASE_AC_BUILD:
    case  AUX_RF_BUILD:
    case  AUX_TIMER_BUILD:
    case  AUX_AP_BUILD:
    case  AUX_MULTIPLY_BUILD:
    case  AUX_SWAP_BUILD:
    case  AUX_NORM_BUILD:
    case  AUX_BARREL_BUILD:
    case  AUX_IRQ_BUILD:
    case  AUX_IRQ_LEV: //A6kv2.1 : AUX_IRQ_LEVEL_PENDING
    case  AUX_IRQ_HINT:
    case  AUX_IRQ_LEVEL:
    case  AUX_ERET:
    case  AUX_ERBTA:
    case  AUX_ERSTATUS:
    case  AUX_ECR:
    case  AUX_EFA:
    case  AUX_ICAUSE1: //A6kv2.1 : AUX_ICAUSE
    case  AUX_ICAUSE2: //A6kv2.1 : AUX_IRQ_INTERRUPT
    case  AUX_IENABLE: //A6kv2.1 : AUX_IRQ_ENABLE
    case  AUX_ITRIGGER://A6kv2.1 : AUX_IRQ_TRIGGER
    case  AUX_XPU:
    case  AUX_BTA:
    case  AUX_BTA_L1:
    case  AUX_BTA_L2:
    case  AUX_MULHI:
    //
    // MMU Build Configuration Registers (BCRs)
    //
    case  AUX_MMU_BUILD:
    case  AUX_DATA_UNCACHED:
    //
    // MMU Maintenance and Control Registers
    // TBD - implement exceptions when MMU is not configured.
    //
    case  AUX_TLB_PD0:
    case  AUX_TLB_PD1:
    case  AUX_TLB_Index:
    case  AUX_PID:      // AUX_MPU_EN: in A6kv2.1 if MPU
    case  AUX_SASID:
    case  AUX_SCRATCH_DATA0:

    // MPU Build Configuration and Control registers
    case AUX_MPU_BUILD: //A6kv2.1
    case AUX_MPU_ECR:
    case AUX_MPU_RDB0:
    case AUX_MPU_RDP0:
    case AUX_MPU_RDB1:
    case AUX_MPU_RDP1:
    case AUX_MPU_RDB2:
    case AUX_MPU_RDP2:
    case AUX_MPU_RDB3:
    case AUX_MPU_RDP3:
    case AUX_MPU_RDB4:
    case AUX_MPU_RDP4:
    case AUX_MPU_RDB5:
    case AUX_MPU_RDP5:
    case AUX_MPU_RDB6:
    case AUX_MPU_RDP6:
    case AUX_MPU_RDB7:
    case AUX_MPU_RDP7:
    case AUX_MPU_RDB8:
    case AUX_MPU_RDP8:
    case AUX_MPU_RDB9:
    case AUX_MPU_RDP9:
    case AUX_MPU_RDB10:
    case AUX_MPU_RDP10:
    case AUX_MPU_RDB11:
    case AUX_MPU_RDP11:
    case AUX_MPU_RDB12:
    case AUX_MPU_RDP12:
    case AUX_MPU_RDB13:
    case AUX_MPU_RDP13:
    case AUX_MPU_RDB14:
    case AUX_MPU_RDP14:
    case AUX_MPU_RDB15:
    case AUX_MPU_RDP15:

    case AUX_STACK_REGION_BUILD:
    //
    // Floating point extension registers
    // TBD - implement exceptions when FPX is not configured.
    //
    case  AUX_FP_BUILD:
    case  AUX_DPFP_BUILD:
    case  AUX_FP_STATUS:
    case  AUX_DPFP_STATUS:
    case  AUX_DPFP1L:
    case  AUX_DPFP1H:
    case  AUX_DPFP2L:
    case  AUX_DPFP2H:

    //
    // ARCompact V2 BCRs
    //
    case  AUX_MINMAX_BUILD:
    case  AUX_ISA_CONFIG:

    // ARCv2 Actionpoints Watchpoint PC
    //
    case  AUX_AP_WP_PC:

    // Simulation Control extension register
    //
    case  AUX_SIM_CONTROL:
      {
        rdata = state.auxs[aux_addr];
        break;
      }
      
    // Actionpoint registers are all readable
    //
    case  AUX_AP_AMV0:
    case  AUX_AP_AMM0:
    case  AUX_AP_AC0:
    //
    case  AUX_AP_AMV1:
    case  AUX_AP_AMM1:
    case  AUX_AP_AC1:
    //
    case  AUX_AP_AMV2:
    case  AUX_AP_AMM2:
    case  AUX_AP_AC2:
    //
    case  AUX_AP_AMV3:
    case  AUX_AP_AMM3:
    case  AUX_AP_AC3:
    //
    case  AUX_AP_AMV4:
    case  AUX_AP_AMM4:
    case  AUX_AP_AC4:
    //
    case  AUX_AP_AMV5:
    case  AUX_AP_AMM5:
    case  AUX_AP_AC5:
    //
    case  AUX_AP_AMV6:
    case  AUX_AP_AMM6:
    case  AUX_AP_AC6:
    //
    case  AUX_AP_AMV7:
    case  AUX_AP_AMM7:
    case  AUX_AP_AC7:
      {
        aps.read_aux_register (aux_addr, &rdata);
        break;
      }
      
    case  AUX_SMART_BUILD:
    case  AUX_SMART_CONTROL:
    case  AUX_SMART_DATA:
      {
        smt.read_aux_register (aux_addr, &rdata);
        break;     
      }
      
    case  AUX_PC:
      {
        rdata = state.pc;
        break;
      }
      
    case  AUX_STATUS32:
      {
        if(sys_arch.isa_opts.new_interrupts && sys_arch.isa_opts.is_isa_a6kv2())
          rdata = BUILD_STATUS32_A6KV21(state);
        else
          rdata = BUILD_STATUS32(state);

        if (state.U && from_sim) {
          rdata &= 0x00000f00UL;
        }
        
        if (sys_arch.isa_opts.is_isa_a600()) // mask out unsupported fields on A600
          rdata &= kA600AuxStatus32Mask;

        break;
      }
      
    case  AUX_XFLAGS:
      {
        rdata = BUILD_XFLAGS(state);
        break;
      }
      
    case  AUX_IRQ_PENDING:
      {
        if(sys_arch.isa_opts.new_interrupts)
        {
          
          if(!is_interrupt_configured(state.auxs[AUX_IRQ_INTERRUPT]))
            rdata = 0;
          else
            rdata = is_interrupt_pending(state.auxs[AUX_IRQ_INTERRUPT]) | (state.auxs[AUX_IRQ_HINT] == state.auxs[AUX_IRQ_INTERRUPT]);
        }
        else
        {
          rdata = get_pending_ints ();
        }
        break;
      }

    case AUX_IRQ_PRIORITY:
    {
      if(is_interrupt_configured(state.auxs[AUX_IRQ_INTERRUPT])) rdata = state.irq_priority[state.auxs[AUX_IRQ_INTERRUPT]];
      else rdata = 0;
      break;
    }

    // DCCM aux register
    //
    case AUX_DCCM:
    {
      LOG(LOG_DEBUG) << "[CPU" << core_id << "] READING AUX_DCCM register '0x"
                     << std::hex << std::setw(8) << std::setfill('0')
                     << state.auxs[AUX_DCCM] << "'";
      rdata = state.auxs[AUX_DCCM];
      break;
    }
    // ICCM aux register
    //
    case AUX_ICCM:
    {
      LOG(LOG_DEBUG) << "[CPU" << core_id << "] READING AUX_ICCM register '0x"
                     << std::hex << std::setw(8) << std::setfill('0')
                     << state.auxs[AUX_ICCM] << "'";
      rdata = state.auxs[AUX_ICCM];
      break;
    }

    // ICCM and DCCM build registers
    //
    case AUX_ICCM_BUILD:
    case AUX_DCCM_BUILD:
    case AUX_DCCM_BASE_BUILD:
    //
    // I-cache aux registers
    //
    case  AUX_IC_IVIC:
    case  AUX_IC_CTRL:
    case  AUX_IC_LIL:
    case  AUX_IC_IVIL:
    case  AUX_IC_RAM_ADDRESS:
    case  AUX_I_CACHE_BUILD:
    //
    // IFQ aux registers
    //
    case  AUX_IFQUEUE_BUILD:
    //
    // D-cache aux registers
    //
    case  AUX_DC_IVDC:
    case  AUX_DC_CTRL:
    case  AUX_DC_LDL:
    case  AUX_DC_IVDL:
    case  AUX_DC_FLSH:
    case  AUX_DC_FLDL:
    case  AUX_DC_RAM_ADDRESS:
    case  AUX_D_CACHE_BUILD:
    case  AUX_CACHE_LIMIT:
    case  AUX_DMP_PER:
      {
        rdata = state.auxs[aux_addr];
        break;
      }

    case  AUX_IC_TAG:
      {
        // Reads from AUX_IC_TAG require that we first probe 
        // the cache using the AT mode bit in AUX_IC_CTRL[5], 
        // and the addres provided by AUX_IC_RAM_ADDRESS
        //
        if (mem_model && mem_model->icache_c) {
          bool success;
          
          if ((state.auxs[AUX_IC_CTRL] >> 5) & 1) {
            mem_model->icache_c->cache_addr_probe(
                                  state.auxs[AUX_IC_RAM_ADDRESS],
                                  state.auxs[AUX_IC_TAG],
                                  success);
          } else {
            mem_model->icache_c->direct_addr_probe(
                                  state.auxs[AUX_IC_RAM_ADDRESS],
                                  state.auxs[AUX_IC_TAG],
                                  success);
          }          
          rdata = state.auxs[AUX_IC_TAG];
        }
        break;
      }
      
    case  AUX_IC_DATA:
      {
        // Reads from AUX_IC_DATA require that we first probe 
        // the cache using the AT mode bit in AUX_IC_CTRL[5], 
        // and the addres provided by AUX_IC_RAM_ADDRESS
        //        
        if (mem_model && mem_model->icache_c) {
          uint32 tag;
          uint32 addr = state.auxs[AUX_IC_RAM_ADDRESS];
          bool   success = false;
          
          if ((state.auxs[AUX_IC_CTRL] >> 5) & 1) {
            mem_model->icache_c->cache_addr_probe(addr,
                                                  tag,
                                                  success);
          } else {
            addr = mem_model->icache_c->direct_addr_probe(addr,
                                                          tag,
                                                          success);
          }
                                  
          // Retrieve data from main memory at the address contained
          // in icache at the selected direct-access address, but only
          // if the address is present in cache. Otherwise, zero data
          // is returned.
          //
          if (success) {
            read32(addr & 0xfffffffc, state.auxs[AUX_IC_DATA]);
            rdata = state.auxs[AUX_IC_DATA];
          }
        }
        break;
      }

    case  AUX_DC_TAG:
      {
        // Reads from AUX_DC_TAG require that we first probe 
        // the cache using the AT mode bit in AUX_DC_CTRL[5], 
        // and the addres provided by AUX_DC_RAM_ADDRESS
        //
        if (mem_model && mem_model->dcache_c) {
          bool success;
          
          if ((state.auxs[AUX_DC_CTRL] >> 5) & 1) {
            mem_model->dcache_c->cache_addr_probe(
                                  state.auxs[AUX_DC_RAM_ADDRESS],
                                  state.auxs[AUX_DC_TAG],
                                  success);
          } else {
            mem_model->dcache_c->direct_addr_probe(
                                  state.auxs[AUX_DC_RAM_ADDRESS],
                                  state.auxs[AUX_DC_TAG],
                                  success);
          }
          rdata = state.auxs[AUX_DC_TAG];
        }
        break;
      }
      
    case  AUX_DC_DATA:
      {
        // Reads from AUX_DC_DATA require that we first probe 
        // the cache using the AT mode bit in AUX_DC_CTRL[5], 
        // and the addres provided by AUX_DC_RAM_ADDRESS
        //        
        if (mem_model && mem_model->dcache_c) {
          uint32 tag;
          uint32 addr = state.auxs[AUX_DC_RAM_ADDRESS];
          bool   success = false;
          
          if ((state.auxs[AUX_DC_CTRL] >> 5) & 1) {
            mem_model->dcache_c->cache_addr_probe(addr,
                                                  tag,
                                                  success);
          } else {
            addr = mem_model->dcache_c->direct_addr_probe(addr,
                                                          tag,
                                                          success);
          }
                                  
          // Retrieve data from main memory at the address contained
          // in icache at the selected direct-access address, but only
          // if the address is present in cache. Otherwise, zero data
          // is returned.
          //
          if (success) {
            read32(addr & 0xfffffffc, state.auxs[AUX_DC_DATA]);
            rdata = state.auxs[AUX_DC_DATA];
          }
        }
        break;
      }

    case 0x0fc: // Temporary, until ARCmedia is implemented
    case 0x0fd: // Temporary, until ARCmedia is implemented
      {
        rdata = 0;
        break;
      }
      
    case AUX_STACK_TOP: //AUX_USTACK_TOP
    {
      if (state.U == 0){
        if(sys_arch.isa_opts.stack_checking && (sys_arch.isa_opts.is_isa_a6k())){
          rdata = state.shadow_stack_top & state.addr_mask;;  //We must be in kernel mode to read, so we want the _other_ stack_top
        }else{
          if(sys_arch.isa_opts.is_isa_a700()){
            rdata = state.stack_top;        //A700 only has 1 stack, and register size is not limited by addr_size
          }else{
            rdata = 0;
          }
        }
      }else{
        rdata = 0;
      }
      break;
    }
    case AUX_STACK_BASE: //AUX_USTACK_BASE
    {
      if (sys_arch.isa_opts.stack_checking && state.U == 0){
        if(sys_arch.isa_opts.is_isa_a6kv2()){
          rdata = state.shadow_stack_base & state.addr_mask;  //We must be in kernel mode to read, so we want the _other_ stack_base
        }else{
          if(sys_arch.isa_opts.is_isa_a700()){
            rdata = state.stack_base;        //A700 only has 1 stack, and register size is not limited by addr_size
          }else{
            rdata = 0;
          }
        }
      }else{
        rdata = 0;
      }
      break;
    }

    case AUX_KSTACK_TOP:
    {
      if (sys_arch.isa_opts.stack_checking && (state.U == 0)){
        if(sys_arch.isa_opts.is_isa_a6kv2()){
          rdata = state.stack_top & state.addr_mask;;  //We must be in kernel mode to read, so we want the _other_ stack_top
        }else{
          rdata = 0;
        }
      }else{
        rdata = 0;
      }
      break;
    }

    case AUX_KSTACK_BASE:
    {
      if (sys_arch.isa_opts.stack_checking && (state.U == 0)){
        if(sys_arch.isa_opts.is_isa_a6kv2()){
          rdata = state.stack_base & state.addr_mask;  //We must be in kernel mode to read, so we want the _other_ stack_base
        }else{
          rdata = 0;
        }
      }else{
        rdata = 0;
      }
      break;
    }
  
#if PASTA_CPU_ID_AUX_REG
      // This cores cpu id and the total amount of cores
      // in the system
      //
    case AUX_CPU_ID:
    {
      // set the core id in the first half ...
      rdata = core_id << 16;
      // ... and the amount of cores in the second.
      rdata = rdata | system.total_cores;
      break;
    }      
#endif
      
    // Default case catches extension aux registers
    // and accesses to unimplemented aux registers
    //
    default:
      {
        return false;
      }
  }
  *data = (rdata & rmask);

  // If there are any LR data Actionpoints defined, check the current
  // read data value to see if an Actionpoint would be triggered.
  //
  if (aps.has_lr_data_aps()) {
    aps.match_lr_data(*data);
  }

  return true;
}

// -----------------------------------------------------------------------------
// Write auxiliary register
//
bool
Processor::write_aux_register (const uint32 aux_addr, uint32 aux_data, bool from_sim)
{
  unsigned char perms;
  unsigned char reqd;

#ifdef DEBUG_WRITE_AUX_REGISTER
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] write_aux_register: aux-addr = 0x"
                 << HEX(aux_addr) << ", value = 0x" << HEX(aux_data);
#endif

  // Detect unimplemented aux registers outside the builtin range, 
  // raising exceptions if access is made to one that does not exist,
  // unless the access is from an external agent, in which case silently
  // ignore the write.
  //
  if (aux_addr >= BUILTIN_AUX_RANGE) {
    // If not in the built-in auxiliary register range, the check if aux_addr
    // selects a defined extension aux register.
    //
    if (eia_mgr.are_eia_aux_regs_defined) {
      std::map<uint32, ise::eia::EiaAuxRegisterInterface*>::const_iterator R;
      if (( R = eia_mgr.eia_aux_reg_map.find(aux_addr) ) != eia_mgr.eia_aux_reg_map.end())
      {
        uint32* ext_reg = R->second->get_value_ptr();
        *ext_reg = aux_data;
        return true;
      }
    }
    
    if (from_sim) {
      LOG(LOG_DEBUG) << "[AUX-WRITE] Unimplemented AUX REG: 0x" << HEX(aux_addr);
      
      if (sys_arch.isa_opts.is_isa_a600()) { // A600 semantics
        return true; // ignore writes to unimplemented AUX registers
      } else { // A700 and ARCompact V2
        enter_exception (ECR(sys_arch.isa_opts.EV_InstructionError, IllegalInstruction, 0), state.pc, state.pc);
      }
    }
      
    return false;
  }

#ifdef DEBUG_WRITE_AUX_REGISTER
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] \taux-reg is within built-in range";
#endif

  // Check existence of aux registers in the builtin range, raising illegal
  // instruction exception if register is absent, unless access is from an external
  // agent, in which case ignore the write.
  //
  if  (!(perms = aux_perms[aux_addr])) {
    if (from_sim) {
      LOG(LOG_DEBUG) << "[AUX-WRITE] Unimplemented AUX REG: 0x" << HEX(aux_addr);
      
      if (sys_arch.isa_opts.is_isa_a600()) { // A600 semantics
        return true; // ignore writes to unimplemented AUX registers
      } else { // A700 and ARCompact V2
        enter_exception (ECR(sys_arch.isa_opts.EV_InstructionError, IllegalInstruction, 0), state.pc, state.pc);
      }
    } 
    return false;  
  }

#ifdef DEBUG_WRITE_AUX_REGISTER
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] \taux-reg exists";
#endif

  // Check whether this auxiliary register is writeable, and if not
  // raise an IllegalInstruction exception regardless of operating mode.
  //
  if (!(perms & AUX_ANY_W) && from_sim) {
    LOG(LOG_DEBUG) << "[AUX-WRITE] WRITE PERMISSION DENIED - AUX REG: 0x" << HEX(aux_addr);
    enter_exception (ECR(sys_arch.isa_opts.EV_InstructionError, IllegalInstruction, 0), state.pc, state.pc);
    return false;
  }
  
  // Next, check whether the current operating mode has required
  // privilege, and if not raise a PrivilegeViolation exception.
  //
  reqd  = state.U ? AUX_U_WRITE : AUX_K_WRITE;
    
  if (!(perms & reqd) && from_sim) {
    enter_exception (ECR(sys_arch.isa_opts.EV_PrivilegeV, PrivilegeViolation, 0), state.pc, state.pc);
    return false;
  }

#ifdef DEBUG_WRITE_AUX_REGISTER
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] \twrite permission is granted";
#endif

  // Mask the write data according to the implemented bits
  // in the selected aux register.
  //
  uint32 wdata = aux_data & aux_mask[aux_addr];

#ifdef DEBUG_WRITE_AUX_REGISTER
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] \tmasked write data is: 0x"
                 << std::hex << std::setw(8) << std::setfill('0')
                 << wdata;
#endif

  // Deal with any associated side effects for aux registers that
  // have such side effects defined on writes.
  //
  switch (aux_addr)
  {
    // -------------------------------------------------------------------------
    // Basecase registers that have no side effect on write
    //
    case  AUX_SEMA:
    case  AUX_USER_SP:
    case  AUX_INT_VECTOR_BASE:
    case  AUX_JLI_BASE:
    case  AUX_LDI_BASE:
    case  AUX_EI_BASE:
    case  AUX_MACMODE:
    case  AUX_ERET:
    case  AUX_ERBTA:
    case  AUX_ECR:
    case  AUX_EFA:
    case  AUX_IRQ_LEV:
    case  AUX_ITRIGGER:
    case  AUX_XPU:
    case  AUX_BTA:
    case  AUX_BTA_L1:
    case  AUX_BTA_L2: {
      state.auxs[aux_addr] = wdata;
      break;
    }
    case  AUX_IRQ_CTRL:
    {
      //saturate AIX_IRQ_CTRL.NR
      uint8 nr = wdata & 0x1f;
      //if nr is greater than 16
      if(nr > 16) 
      {
        //mask off the bits which make it greater than 16
        wdata &= 0xfffffff0;
      }
      state.auxs[aux_addr] = wdata;
      break;
    }
    // -------------------------------------------------------------------------
    // Debugger or external agent might want to write to this AUX register and
    // should be able to do so
    //
    case  AUX_STATUS32:
      {
        if (!from_sim) { // allow writes to STATUS32 register from external agents
          state.auxs[AUX_STATUS32] = wdata;
          
          if (sys_arch.isa_opts.is_isa_a600()) // mask out unsupported fields on A600
            state.auxs[AUX_STATUS32] &= kA600AuxStatus32Mask;

          // propagate changes from AUX_STATUS32 register to processor state
          if(sys_arch.isa_opts.is_isa_a6kv2()) EXPLODE_STATUS32_A6KV21(state, state.auxs[AUX_STATUS32]);
          else
              EXPLODE_STATUS32(state,state.auxs[AUX_STATUS32]);
        }
        break;
      }

    case AUX_IRQ_PRIORITY:
    {
      state.auxs[aux_addr] = wdata;
      if(is_interrupt_configured(state.auxs[AUX_IRQ_INTERRUPT])) 
      {
        LOG(LOG_DEBUG) << "Set the priority of " << state.auxs[AUX_IRQ_INTERRUPT] << " to " <<wdata;
        state.irq_priority[state.auxs[AUX_IRQ_INTERRUPT]] = wdata;
      }
      else state.auxs[aux_addr] = 0;
      break;
    }
    
    case AUX_IRQ_LEVEL:
    {
      state.auxs[aux_addr] = wdata;
      if(wdata < sys_arch.isa_opts.number_of_levels)
        state.auxs[AUX_ICAUSE] = state.irq_icause[state.auxs[aux_addr]];
      else
        state.auxs[AUX_ICAUSE] = 0;
    }

    case  AUX_ERSTATUS:
    case  AUX_STATUS32_L1:
    case  AUX_STATUS32_L2: {
      state.auxs[aux_addr] = wdata;
      if (sys_arch.isa_opts.is_isa_a600()) // mask out unsupported fields on A600
        state.auxs[aux_addr] &= kA600AuxStatus32Mask;
      break;
    }

    // -------------------------------------------------------------------------
    case  AUX_XFLAGS:
      {
        state.auxs[AUX_XFLAGS] = wdata;
        
        // Propagate changes to processor state
        //
        EXPLODE_XFLAGS(state);
        break;
      }
    
    case  AUX_ICAUSE2: //also AUX_IRQ_INTERRUPT
    {
      if(sys_arch.isa_opts.new_interrupts)
      {
        if(is_interrupt_configured(state.auxs[AUX_IRQ_INTERRUPT]))
        {
          state.auxs[AUX_IRQ_PRIORITY] = state.irq_priority[wdata];
          state.auxs[AUX_IRQ_ENABLE] = is_interrupt_enabled(wdata);
          state.auxs[AUX_IRQ_TRIGGER] = state.irq_trigger[wdata];
        }
        else
        {
          state.auxs[AUX_IRQ_PRIORITY] = 0;
          state.auxs[AUX_IRQ_ENABLE] = 0;
          state.auxs[AUX_IRQ_TRIGGER] = 0;
        }
      }
      state.auxs[aux_addr] = wdata;
      break;
    }

    case  AUX_ICAUSE1: //also AUX_IRQ_ICAUSE
    {
      if(sys_arch.isa_opts.new_interrupts)
      {
        state.irq_icause[state.auxs[AUX_IRQ_LEVEL]] = wdata;
      }
      else
        state.auxs[aux_addr] = wdata;
      break;
    }

    // -------------------------------------------------------------------------
    // MMU Interface registers that have no side effect on write
    //
    case  AUX_TLB_PD0:
    case  AUX_TLB_PD1:
    case  AUX_SASID:
    case  AUX_SCRATCH_DATA0: {
      state.auxs[aux_addr] = wdata;
      break;
    }
    case  AUX_TLB_Index: {
      // Writing has no effect on the E bit and only 11 bits are written to the index field.
      // FIXME: @igor - remove magic numbers with proper constants
      state.auxs[AUX_TLB_Index] = (state.auxs[AUX_TLB_Index] & 0x8000000UL) | (wdata & 0x7ff);
      break;
    } 
    case  AUX_TLB_Command: {
      state.auxs[AUX_TLB_Command] = wdata;        
      mmu.command (wdata); // handle MMU command
      break;
    }
    case  AUX_PID: { //AUX_MPU_EN: in A6kv2.1 with MPU
      state.auxs[AUX_PID] = wdata;
      // When ASID changes, the block address translation cache and the decode
      // cache must be flushed. When T is modified, the translation base address
      // may change also. Method is also deals with the write to MPU_EN case.
      //
      mmu.write_pid (wdata);
      break;
    }

    // -------------------------------------------------------------------------
    case  AUX_MULHI:
    {
      // Writing to the Multiply Restore Register restores the upper half multiply
      // result register MHI_REG
      //
      state.auxs[aux_addr] = wdata;
      state.gprs[MHI_REG]  = wdata;
      break;
    }

    // -------------------------------------------------------------------------
    // Floating point extension registers that are writeable.
    // TBD - implement exceptions when FPX is not configured.
    //
    case  AUX_DPFP1L:
    case  AUX_DPFP1H:
    case  AUX_DPFP2L:
    case  AUX_DPFP2H:
      {
        state.auxs[aux_addr] = wdata;
        break;
      }

    // Registers that have side effects on write
    //
    case  AUX_LP_START:
      {
        state.lp_start = state.auxs[AUX_LP_START] = (wdata & state.addr_mask);
        LOG(LOG_DEBUG4) << "Masked LP_START write from " << HEX(wdata) << " to " << HEX(state.auxs[AUX_LP_START]);
        break;
      }

    case  AUX_LP_END:
      {
        state.lp_end = state.auxs[AUX_LP_END] = (wdata & state.addr_mask);

        // Add lp_end to list if it does not exist
        //
        if (lp_end_to_lp_start_map.find(state.lp_end) == lp_end_to_lp_start_map.end()) {
          
          // Insert new lp_end into lp_end_to_lp_start_map
          //
          lp_end_to_lp_start_map[state.lp_end] = 0x1;

          // Address not on lp_end list therefore we remove dcode cache
          purge_dcode_cache();
          
          if (sim_opts.fast) {
            uint32 ecause, phys_addr;
            if ((ecause = mmu.lookup_exec(state.pc, state.U, phys_addr))) {
              LOG(LOG_DEBUG) << "[CPU" << core_id << "] write_aux_register: aux-addr = 0x"
                             << std::hex << std::setw(8) << std::setfill('0')
                             << aux_addr
                             << " - AUX_LP_END: removing ALL translations.";
              // Virtual to physical translation failed, hence we must do the safe
              // thing and remove ALL translations.
              remove_translations();
            } else {
              // Virtual to physical translation was successful, only remove translation
              // if it is present
              if (phys_profile_.is_translation_present(phys_addr)) {
                // If new LP_END points into a translation, we need to remove it as it
                // effectively adds implicit control flow that has not been compiled in.
                remove_translation(phys_addr);
              }
            }
          }
        }
        break;
      }
      
    case  AUX_COUNT0:
      {
        timer_set_count (0, wdata);
        break;
      }
      
    case  AUX_CONTROL0:
      {
        timer_set_control (0, wdata);
        break;
      }
      
    case  AUX_LIMIT0:
      {
        timer_set_limit (0, wdata);
        break;
      }
    case  AUX_COUNT1:
      {
        timer_set_count (1, wdata);
        break;
      }
    case  AUX_CONTROL1:
      {
        timer_set_control (1, wdata);
        break;
      }
    case  AUX_LIMIT1:
      {
        timer_set_limit (1, wdata);
        break;
      }

    case AUX_RTC_CTRL:
    {
      set_rtc_ctrl(wdata);
      break;
    }

    case  AUX_IRQ_LV12:
      {
        state.auxs[AUX_IRQ_LV12] = state.auxs[AUX_IRQ_LV12] & ~wdata;
        break;
      }

    case  AUX_IRQ_HINT:
      {
        write_irq_hint (wdata);
        break;
      }

    case  AUX_IENABLE: //Also AUX_IRQ_ENABLE
      {
        if(sys_arch.isa_opts.new_interrupts)
        {
          set_interrupt_enabled(state.auxs[AUX_IRQ_INTERRUPT], wdata);
          set_pending_action(kPendingAction_CPU);
        }
        else
        {
          // Bits in AUX_IENABLE above the number of interrupts are set to zero.
          // This enforces the absence of those interrupts from the system.
          //
          uint32 int_mask = 0xffffffffUL >> (32 - sys_arch.isa_opts.num_interrupts);
          state.auxs[AUX_IENABLE] = (wdata & int_mask) | 7UL;
          set_pending_action(kPendingAction_CPU);
        }
        break;
      }

    case  AUX_IRQ_PULSE_CANCEL:
      {
        if(sys_arch.isa_opts.new_interrupts)
        {
          clear_pulse_interrupts(state.auxs[AUX_IRQ_INTERRUPT]);
        }
        else
            clear_pulse_interrupts (wdata);
        break;
      }
      
    case AUX_DEBUG:
      {
        if (!from_sim) {
          state.auxs[AUX_DEBUG] = wdata;
          // DEBUG.FH (i.e. Force Halt) set, halt cpu
          //
          if (state.auxs[AUX_DEBUG] & 0x2) {
            halt_cpu(false);
          }
          break;
        } else {
          enter_exception (ECR(sys_arch.isa_opts.EV_InstructionError, IllegalInstruction, 0), state.pc, state.pc);        
        }
      }
      
    case AUX_PC:
      {
        if (!from_sim) {
          state.auxs[AUX_PC] = state.pc = (wdata & state.pc_mask);
          break;
        } else {
          enter_exception (ECR(sys_arch.isa_opts.EV_InstructionError, IllegalInstruction, 0), state.pc, state.pc);        
        }
      }

    // DCCM aux register
    //
    case AUX_DCCM:
    {
      if (   core_arch.dccm.is_configured
          && state.auxs[AUX_DCCM] != ccm_mgr_->get_memory_region_base(wdata))
      {
        LOG(LOG_DEBUG) << "[CPU" << core_id << "] MOVING DCCM to start address '0x"
                       << std::hex << std::setw(8) << std::setfill('0')
                       << wdata << "'.";
        
        state.auxs[AUX_DCCM] = ccm_mgr_->get_memory_region_base(wdata);

        // First we set the desired start address of DCCM in the core config
        //
        core_arch.dccm.start_addr = wdata;
                
        purge_page_cache(arcsim::sys::cpu::PageCache::ALL);
        purge_translation_cache();
        
        ccm_mgr_->configure(); // re-configure CCM manager
      } 
      break;
    }
      
    // ICCM aux register
    //
    case AUX_ICCM:
    {
      if (   !sys_arch.isa_opts.multiple_iccms
          && core_arch.iccm.is_configured
          && state.auxs[AUX_ICCM] != ccm_mgr_->get_memory_region_base(wdata))
      {  
        LOG(LOG_DEBUG) << "[CPU" << core_id << "] MOVING ICCM to start address '0x" << HEX(wdata) << "'.";
        
        state.auxs[AUX_ICCM] = ccm_mgr_->get_memory_region_base(wdata);

        // First we set the desired start address of ICCM in the core config
        core_arch.iccm.start_addr = wdata;
        
        purge_page_cache(arcsim::sys::cpu::PageCache::ALL);        
        purge_translation_cache();
        phys_profile_.remove_translations();
        
        ccm_mgr_->configure(); // re-configure CCM manager
      } else {
        if (   sys_arch.isa_opts.multiple_iccms
            && sys_arch.isa_opts.is_isa_a6kv2()
            && (wdata != state.auxs[AUX_ICCM]))
        {
          LOG(LOG_DEBUG) << "[CPU" << core_id << "] MOVING ICCMs";
          
          // Compute which region is covered by ICCMs
          //
          state.auxs[AUX_ICCM] = 0; // clear out AUX_ICCM
          uint32 regions       = wdata;
          
          for (int i = 0; i < IsaOptions::kMultipleIccmCount; ++i) {
            if (core_arch.iccms[i].is_configured) {
              core_arch.iccms[i].start_addr = regions & (0xF << (sys_arch.isa_opts.addr_size - 4));
            }
            regions <<= 4; // look at next region
            LOG(LOG_DEBUG) << "[ICCM" << i << "] start address:'0x" << HEX(core_arch.iccms[i].start_addr);
          }

          purge_page_cache(arcsim::sys::cpu::PageCache::ALL);
          purge_translation_cache();
          phys_profile_.remove_translations();

          ccm_mgr_->configure(); // re-configure CCM manager
        }
      }
      break;
    }
      
    // I-cache aux registers READ ONLY
    //
    case  AUX_I_CACHE_BUILD: 
    case  AUX_D_CACHE_BUILD: { 
        break;
      }
    
    case  AUX_IC_IVIC:
      {
        // TBD - add the cache operation cycles to the cycle counter
        //
        if (mem_model && mem_model->icache_c)
          mem_model->icache_c->invalidate (false);
        break;
      }

    case  AUX_IC_LIL:
      {
        // TBD - add the cache operation cycles to the cycle counter
        //
        bool success = false;
        if (mem_model && mem_model->icache_c)
          mem_model->icache_c->lock_line (wdata, false, success);
        
        UPDATE_SUCCESS(AUX_IC_CTRL,success)
        break;
      }

    case  AUX_IC_IVIL:
      {
        // TBD - add the cache operation cycles to the cycle counter
        //
        bool success = false;
        if (mem_model && mem_model->icache_c)
          mem_model->icache_c->invalidate_line (wdata, false, success);
        
        UPDATE_SUCCESS(AUX_IC_CTRL,success)
        break;
      }
      
    case  AUX_IC_CTRL:
      {
        if (mem_model && mem_model->icache_c) {
          if (wdata & 1UL)
            mem_model->disable_icache();
          else
            mem_model->enable_icache();
        }
        state.auxs[AUX_IC_CTRL] = wdata;
        break;
      }
    
    case  AUX_IC_RAM_ADDRESS:
      {
        // Writes to AUX_IC_RAM_ADDRESS require that we probe 
        // the icache using the AT mode bit in AUX_IC_CTRL[5], 
        // and the address to be written to AUX_IC_RAM_ADDRESS.
        //
        uint32 tag;
        uint32 addr = state.auxs[AUX_IC_RAM_ADDRESS] = wdata;
        
        if (mem_model && mem_model->icache_c) {          
          if (((state.auxs[AUX_IC_CTRL] >> 5) & 1) == 1) {
            bool  success;
            mem_model->icache_c->cache_addr_probe(addr,
                                                  tag,
                                                  success);
            UPDATE_SUCCESS(AUX_IC_CTRL,success)
          }
        }
        break;
      }

    case  AUX_IC_TAG:
      {
        // Writes AUX_IC_TAG are permitted only when the AT mode 
        // bit in AUX_IC_CTRL[5] indicates direct-access mode.
        //
        if (mem_model && mem_model->icache_c) {
          if (((state.auxs[AUX_IC_CTRL] >> 5) & 1) == 0) {
            mem_model->icache_c->direct_tag_write(
                                  state.auxs[AUX_IC_RAM_ADDRESS],
                                  wdata);
                                  
            state.auxs[AUX_IC_TAG] = wdata;
          }
        }
        break;
      }
      
    case  AUX_IC_DATA:
      {
        // Writes AUX_IC_DATA are permitted only when the AT mode 
        // bit in AUX_IC_CTRL[5] indicates direct-access mode.
        //
        // Writes to AUX_IC_DATA require that we first probe 
        // the cache using addres provided by AUX_IC_RAM_ADDRESS.
        // This yields the equivalent memory address, and indicates
        // if there is a valid block in cache for that address.
        // If the direct-access address is not present in cache,
        // then no memory write takes place.
        //
        if (mem_model && mem_model->icache_c) {
          if (((state.auxs[AUX_IC_CTRL] >> 5) & 1) == 0) {
            bool   success;
            uint32 tag;
            uint32 addr = state.auxs[AUX_IC_RAM_ADDRESS];
           
            addr = mem_model->icache_c->direct_addr_probe(addr,
                                                          tag,
                                                          success);
           
            // Write data to main memory at the address equivalent to
            // the direct-access address, but only if that address 
            // is valid in icache.
            //
            if (success) {
              state.auxs[AUX_IC_DATA] = wdata;
              write32(addr & 0xfffffffc, wdata);
            }
          }
        }
        break;
      }
      
      // AUX_IC_PTAG is used when MMU version > 2
      //
    case AUX_IC_PTAG: {
      break;
    }
      
    // D-cache aux registers
    //
    case  AUX_DC_IVDC:
      {
        // TBD - add the cache operation cycles to the cycle counter
        //
        bool im = ((state.auxs[AUX_DC_CTRL] >> 6) & 1) == 1;
        if (mem_model && mem_model->dcache_c)
          mem_model->dcache_c->invalidate (im);
        break;
      }

    case  AUX_DC_LDL:
      {
        // TBD - add the cache operation cycles to the cycle counter
        //
        bool lm = ((state.auxs[AUX_DC_CTRL] >> 7) & 1) == 1;
        bool success = false;
        if (mem_model && mem_model->dcache_c)
          mem_model->dcache_c->lock_line (wdata, lm, success);

        UPDATE_SUCCESS(AUX_DC_CTRL, success)
        break;
      }

    case  AUX_DC_IVDL:
      {
        // TBD - add the cache operation cycles to the cycle counter
        //
        bool im = ((state.auxs[AUX_DC_CTRL] >> 6) & 1) == 1;
        bool success = false;
        if (mem_model && mem_model->dcache_c)
          mem_model->dcache_c->invalidate_line (wdata, im, success);

        UPDATE_SUCCESS(AUX_DC_CTRL,success)
        break;
      }

    case  AUX_DC_FLSH:
      {
        // TBD - add the cache operation cycles to the cycle counter
        //
        bool lm = ((state.auxs[AUX_DC_CTRL] >> 7) & 1) == 1;
        if (mem_model && mem_model->dcache_c)
          mem_model->dcache_c->flush (lm);
        break;
      }

    case  AUX_DC_FLDL:
      {
        // TBD - add the cache operation cycles to the cycle counter
        //
        bool lm = ((state.auxs[AUX_DC_CTRL] >> 7) & 1) == 1;
        bool success = false;
        if (mem_model && mem_model->dcache_c)
          mem_model->dcache_c->flush_line (wdata, lm, success);

        UPDATE_SUCCESS(AUX_DC_CTRL,success)
        break;
      }

    case  AUX_DC_CTRL:
      {
        if (mem_model && mem_model->dcache_c) {
          if (wdata & 1UL)
            mem_model->disable_dcache();
          else
            mem_model->enable_dcache();
        }
        state.auxs[AUX_DC_CTRL] = wdata;
        break;
      }
    
    case  AUX_DC_RAM_ADDRESS:
      {
        // Writes to AUX_DC_RAM_ADDRESS require that we probe 
        // the dcache using the AT mode bit in AUX_DC_CTRL[5], 
        // and the address to be written to AUX_DC_RAM_ADDRESS.
        //
        uint32 tag;
        uint32 addr = state.auxs[AUX_DC_RAM_ADDRESS] = wdata;
        
        if (mem_model && mem_model->dcache_c) {          
          if (((state.auxs[AUX_DC_CTRL] >> 5) & 1) == 1) {
            bool  success;
            mem_model->dcache_c->cache_addr_probe(addr,
                                                  tag,
                                                  success);
            UPDATE_SUCCESS(AUX_DC_CTRL,success)
          }
        }
        break;
      }

    case  AUX_DC_TAG:
      {
        // Writes AUX_DC_TAG are permitted only when the AT mode 
        // bit in AUX_DC_CTRL[5] indicates direct-access mode.
        //
        if (mem_model && mem_model->dcache_c) {
          if (((state.auxs[AUX_DC_CTRL] >> 5) & 1) == 0) {
            
            mem_model->dcache_c->direct_tag_write(
                                  state.auxs[AUX_DC_RAM_ADDRESS],
                                  wdata);
                                  
            state.auxs[AUX_DC_TAG] = wdata;
          }
        }
        break;
      }
      
    case  AUX_DC_DATA:
      {
        // Writes AUX_DC_DATA are permitted only when the AT mode 
        // bit in AUX_DC_CTRL[5] indicates direct-access mode.
        //
        // Writes to AUX_DC_DATA require that we first probe 
        // the cache using addres provided by AUX_DC_RAM_ADDRESS.
        // This yields the equivalent memory address, and indicates
        // if there is a valid block in cache for that address.
        // If the direct-access address is not present in cache,
        // then no memory write takes place.
        //
        if (mem_model && mem_model->dcache_c) {
          if (((state.auxs[AUX_DC_CTRL] >> 5) & 1) == 0) {
            bool   success;
            uint32 tag;
            uint32 addr = state.auxs[AUX_DC_RAM_ADDRESS];
           
            addr = mem_model->dcache_c->direct_addr_probe(addr,
                                                          tag,
                                                          success);
           
            // Write data to main memory at the address equivalent to
            // the direct-access address, but only if that address 
            // is valid in dcache.
            //
            if (success) {
              state.auxs[AUX_DC_DATA] = wdata;
              write32(addr & 0xfffffffc, wdata);
            }
          }
        }
        break;
      }

      // AUX_DC_PTAG is used when MMU version > 2
      //
    case AUX_DC_PTAG: {
      break;
    }

    case  AUX_CACHE_LIMIT:
    case  AUX_DMP_PER:
      {
        state.auxs[aux_addr] = wdata;
        break;
      }
    // -------------------------------------------------------------------------
    // Actionpoint registers are all writable
    //
    case  AUX_AP_AMV0:
    case  AUX_AP_AMM0:
    case  AUX_AP_AC0:
    //
    case  AUX_AP_AMV1:
    case  AUX_AP_AMM1:
    case  AUX_AP_AC1:
    //
    case  AUX_AP_AMV2:
    case  AUX_AP_AMM2:
    case  AUX_AP_AC2:
    //
    case  AUX_AP_AMV3:
    case  AUX_AP_AMM3:
    case  AUX_AP_AC3:
    //
    case  AUX_AP_AMV4:
    case  AUX_AP_AMM4:
    case  AUX_AP_AC4:
    //
    case  AUX_AP_AMV5:
    case  AUX_AP_AMM5:
    case  AUX_AP_AC5:
    //
    case  AUX_AP_AMV6:
    case  AUX_AP_AMM6:
    case  AUX_AP_AC6:
    //
    case  AUX_AP_AMV7:
    case  AUX_AP_AMM7:
    case  AUX_AP_AC7:    
      {
        aps.write_aux_register (aux_addr, wdata);
        break;
      }
      


    case AUX_STACK_TOP: //AUX_USTACK_TOP
    {
      if (sys_arch.isa_opts.stack_checking && state.U == 0){
        if(sys_arch.isa_opts.is_isa_a6kv2()){
          state.shadow_stack_top = wdata & state.addr_mask;  //We must be in kernel mode to read, so we want the _other_ stack_base
        }else{
          if(sys_arch.isa_opts.is_isa_a700()){
            state.stack_top = wdata & state.addr_mask;       //A700 only has 1 stack, and register size is not limited by addr_size
          }else{

          }
        }
      }else{

      }
      break;
    }

    case AUX_STACK_BASE: //AUX_USTACK_BASE
    {
      if (sys_arch.isa_opts.stack_checking && state.U == 0){
        if(sys_arch.isa_opts.is_isa_a6kv2()){
          state.shadow_stack_base = wdata & state.addr_mask;  //We must be in kernel mode to read, so we want the _other_ stack_base
        }else{
          if(sys_arch.isa_opts.is_isa_a700()){
            state.stack_base = wdata & state.addr_mask;       //A700 only has 1 stack, and register size is not limited by addr_size
          }else{

          }
        }
      }else{

      }
      break;
    }

    case AUX_KSTACK_TOP:
    {
      if (sys_arch.isa_opts.stack_checking && (state.U == 0)){
        if(sys_arch.isa_opts.is_isa_a6kv2()){
          state.stack_top = wdata & state.addr_mask;;  //We must be in kernel mode to read, so we want the _other_ stack_top
        }else{

        }
      }else{

      }
      break;
    }

    case AUX_KSTACK_BASE:
    {
      if (sys_arch.isa_opts.stack_checking && (state.U == 0)){
        if(sys_arch.isa_opts.is_isa_a6kv2()){
          state.stack_base = wdata & state.addr_mask;  //We must be in kernel mode to read, so we want the _other_ stack_base
        }else{

        }
      }else{

      }
      break;
    }

    // MPU Control Registers

    case AUX_MPU_RDB0:
    case AUX_MPU_RDP0:
    case AUX_MPU_RDB1:
    case AUX_MPU_RDP1:
    case AUX_MPU_RDB2:
    case AUX_MPU_RDP2:
    case AUX_MPU_RDB3:
    case AUX_MPU_RDP3:
    case AUX_MPU_RDB4:
    case AUX_MPU_RDP4:
    case AUX_MPU_RDB5:
    case AUX_MPU_RDP5:
    case AUX_MPU_RDB6:
    case AUX_MPU_RDP6:
    case AUX_MPU_RDB7:
    case AUX_MPU_RDP7:
    case AUX_MPU_RDB8:
    case AUX_MPU_RDP8:
    case AUX_MPU_RDB9:
    case AUX_MPU_RDP9:
    case AUX_MPU_RDB10:
    case AUX_MPU_RDP10:
    case AUX_MPU_RDB11:
    case AUX_MPU_RDP11:
    case AUX_MPU_RDB12:
    case AUX_MPU_RDP12:
    case AUX_MPU_RDB13:
    case AUX_MPU_RDP13:
    case AUX_MPU_RDB14:
    case AUX_MPU_RDP14:
    case AUX_MPU_RDB15:
    case AUX_MPU_RDP15:
    {
      state.auxs[aux_addr]=wdata;
      mmu.write_pid(state.auxs[AUX_MPU_EN]);
      break;
    }


    // SmaRT control register is the the only SmaRT auxiliary
    // register with write capability
    //
    case  AUX_SMART_CONTROL:
      {
        smt.write_aux_register (aux_addr, wdata);
        break;     
      }
      
    //
    // Simulation Control extension register
    //
    case  AUX_SIM_CONTROL:
      {
        state.auxs[aux_addr] = wdata;
        break;
      }

    // Default case catches accesses to unimplemented aux registers,
    // and debugger writes to registers that are not normally 
    // writeable during program execution (in any mode).
    //
    // If we reach this point, aux_addr < BUILTIN_AUX_RANGE
    //
    default:
      {
        if (from_sim) {
          enter_exception (ECR(sys_arch.isa_opts.EV_InstructionError, IllegalInstruction, 0), state.pc, state.pc);
        } else
          state.auxs[aux_addr] = wdata;
      }
  }
  
  // If there are any SR Actionpoints defined, check the current
  // aux_addr and aux_data to see if an Actionpoint would be triggered.
  //
  if (aps.has_sr_aps()) {
    aps.match_sr(aux_addr, aux_data);
  }
  
  return true;
}


// -----------------------------------------------------------------------------
// Initialize the auxiliary register space
// 
//  
void Processor::init_aux_regs ()
{
  // ---------------------------------------------------------------------------
  // Clear all structures for the builtin aux registers before
  // initializing those registers that are present.
  //
  for (uint32 i = 0; i < BUILTIN_AUX_RANGE; ++i) {
    state.auxs[i] = 0;
    aux_perms[i]  = AUX_NONE;
    aux_mask[i]   = 0;
  }

  // ---------------------------------------------------------------------------
  // Initialise all elements of the aux_mask and aux_perms arrays.
  //
  for (uint32 i = 0; i < NUM_BUILTIN_AUX_REGS; ++i) {
    uint32 addr      = aux_reg_info[i].address;
    aux_mask[addr]   = aux_reg_info[i].valid_mask;
    aux_perms[addr]  = aux_reg_info[i].permissions;
    state.auxs[addr] = aux_reg_info[i].reset_value;    
  }

  // ---------------------------------------------------------------------------
  // Initialize the auxiliary register space according to the architecture variant
  // being simulated.
  //
  switch (sys_arch.isa_opts.get_isa())
  {
    case IsaOptions::kIsaA6K:  { init_aux_regs_a6k();  break; }
    case IsaOptions::kIsaA600: { init_aux_regs_a600(); break; }
    case IsaOptions::kIsaA700: { init_aux_regs_a700(); break; }
    case IsaOptions::kIsaA6KV2:{ init_aux_regs_a6kv21();break;}
    default: {
      LOG(LOG_WARNING) << "[CPU" << core_id << "] Unknown ISA selected.";
      break;
    }
  }
  
  // ---------------------------------------------------------------------------
  // Remove read/write permissions from all aux registers that are not now enabled.
  //
  for (uint32 i = 0; i < BUILTIN_AUX_RANGE; ++i) {
    if ((aux_perms[i] & AUX_ENABLED) == 0) { aux_perms[i] = 0; }
  } 
}

// ---------------------------------------------------------------------------
// ARCompact v2 auxiliary register initialisation
//
void
Processor::init_aux_regs_a6k ()
{
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] Initializing auxiliary registers for AV2";
  // Enable all baseline ARCompact V2 registers
  //
  for (uint32 i = 0; baseline_aux_regs_av2[i]; i++) {
    aux_perms[baseline_aux_regs_av2[i]] |= AUX_ENABLED;
  }
  
  // Set the read/write masks for registers that depend on pc_size,
  // addr_size or lpc_size
  //
  aux_mask[AUX_LP_START]        = state.pc_mask;
  aux_mask[AUX_LP_END]          = state.pc_mask;
  aux_mask[AUX_PC]              = state.pc_mask;
  aux_mask[AUX_ERET]            = state.pc_mask;
  aux_mask[AUX_ERBTA]           = state.pc_mask;
  aux_mask[AUX_BTA]             = state.pc_mask;
  aux_mask[AUX_BTA_L1]          = state.pc_mask;
  aux_mask[AUX_BTA_L2]          = state.pc_mask;
  aux_mask[AUX_JLI_BASE]        = aux_mask[AUX_JLI_BASE]        & state.pc_mask;
  aux_mask[AUX_EI_BASE]         = aux_mask[AUX_EI_BASE]         & state.pc_mask;
  aux_mask[AUX_INT_VECTOR_BASE] = aux_mask[AUX_INT_VECTOR_BASE] & state.pc_mask;
  aux_mask[AUX_EFA]             = state.addr_mask;
  aux_mask[AUX_LDI_BASE]        = aux_mask[AUX_LDI_BASE] & state.pc_mask & state.addr_mask;
  
  // Assign values to baseline BCRs and Read-only registers
  //
  state.auxs[AUX_IDENTITY]         = 0x40;
  state.auxs[AUX_BCR_VER]          = 0x2;
  state.auxs[AUX_BTA_LINK_BUILD]   = 0x0;
  state.auxs[AUX_INT_VECTOR_BASE]  = sys_arch.isa_opts.intvbase_preset & aux_mask[AUX_INT_VECTOR_BASE];
  
  { // Set the AUX_VECBASE_AC_BUILD BCR according to the number of
    // configured interrupts and the AUX_INF_VECTOR_BASE value.
    //
    uint32 p;
    switch (sys_arch.isa_opts.num_interrupts) {
      case 16: p = 0; break;
      case 8:  p = 2; break;
      case 3:  p = 3; break;
      default: p = 1;
    }
    state.auxs[AUX_VECBASE_AC_BUILD] = state.auxs[AUX_INT_VECTOR_BASE] | (0x03 << 2) | p;
  }
  
  { // Set the AUX_IRQ_LEV register bits according to the reset
    // values specified by options timer_0_int_level and timer_1_int_level
    //
    int p0 = (sys_arch.isa_opts.timer_0_int_level == 2) ? 1 : 0;
    int p1 = (sys_arch.isa_opts.timer_1_int_level == 2) ? 1 : 0;
    state.auxs[AUX_IRQ_LEV] = (p1 << 4) | (p0 << 3);
  }
  
  state.auxs[AUX_RF_BUILD]         =  ((sys_arch.isa_opts.only_16_regs ? 1 : 0) << 9)
                                    | ((sys_arch.isa_opts.rf_4port     ? 1 : 0) << 8)
                                    | 0x1;
  state.auxs[AUX_MINMAX_BUILD]     = 0x2;
  state.auxs[AUX_ISA_CONFIG]       =  ((sys_arch.isa_opts.div_rem_option ? 1 : 0)  << 28)
                                    | ((sys_arch.isa_opts.density_option & 0xf)    << 24)
                                    | ((sys_arch.isa_opts.atomic_option  & 0x1)    << 21)
#if BIG_ENDIAN_SUPPORT
                                    | ((sim_opts.big_endian ? 1 : 0)               << 20)
#endif
                                    | (((sys_arch.isa_opts.addr_size/4 - 4) & 7)   << 16)
                                    | ((sys_arch.isa_opts.lpc_size > 7
                                        ? ((sys_arch.isa_opts.lpc_size/4 - 1) & 7)
                                        : 0) 
                                       << 12)
                                    | (((sys_arch.isa_opts.pc_size/4 - 4) & 7)     <<  8)
                                    | 0x2;
  
  // Set the mask for AUX_IENABLE according to num_interrupts
  //
  aux_mask[AUX_IENABLE] = (0xffffffffUL >> (32 - sys_arch.isa_opts.num_interrupts)) | 0x7UL;
  // Set the mask for AUX_ITRIGGER according to num_interrupts
  //
  aux_mask[AUX_ITRIGGER]= (0xffffffffUL >> (32 - sys_arch.isa_opts.num_interrupts)) & 0xfffffff8UL;
  // Enable optional Zero-overhead Loop registers
  //
  if (sys_arch.isa_opts.lpc_size > 0) {
    aux_perms[AUX_LP_START] |= AUX_ENABLED;
    aux_perms[AUX_LP_END]   |= AUX_ENABLED;
  }
  // Enable optional code-density registers
  //
  if (sys_arch.isa_opts.density_option > 0) {
    aux_perms[AUX_JLI_BASE] |= AUX_ENABLED;
    
    if (sys_arch.isa_opts.density_option > 1) {
      aux_perms[AUX_LDI_BASE]   |= AUX_ENABLED;
      aux_perms[AUX_EI_BASE]    |= AUX_ENABLED;
      // Enable ES bit in status32 registers
      aux_mask[AUX_STATUS32]    |= 0x00008000;
      aux_mask[AUX_STATUS32_L1] |= 0x00008000;
      aux_mask[AUX_STATUS32_L2] |= 0x00008000;
      aux_mask[AUX_ERSTATUS]    |= 0x00008000;
    }
  }
  // Enable optional timer0 registers
  //
  if (sys_arch.isa_opts.has_timer0) {
    aux_perms[AUX_COUNT0]      |= AUX_ENABLED;
    aux_perms[AUX_LIMIT0]      |= AUX_ENABLED;
    aux_perms[AUX_CONTROL0]    |= AUX_ENABLED;
    state.auxs[AUX_LIMIT0]     = 0x00ffffff;
  }
  // Enable optional timer1 registers
  //
  if (sys_arch.isa_opts.has_timer1) {
    aux_perms[AUX_COUNT1]      |= AUX_ENABLED;
    aux_perms[AUX_LIMIT1]      |= AUX_ENABLED;
    aux_perms[AUX_CONTROL1]    |= AUX_ENABLED;
    state.auxs[AUX_LIMIT1]     = 0x00ffffff;
  }
  // Enable optional TIMER_BUILD aux register if any timers enabled
  //
  if (sys_arch.isa_opts.has_timer0 || sys_arch.isa_opts.has_timer1) {
    aux_perms[AUX_TIMER_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_TIMER_BUILD] =   ((sys_arch.isa_opts.has_timer1 ? 1 : 0) << 9)
                                  | ((sys_arch.isa_opts.has_timer0 ? 1 : 0) << 8)
                                  | 0x4;
  }
  // Enable optional ISA build configuration registers
  //
  if (sys_arch.isa_opts.mpy32_option || sys_arch.isa_opts.mpy16_option) {
    aux_perms[AUX_MULTIPLY_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_MULTIPLY_BUILD] =  (sys_arch.isa_opts.mpy32_option ? 0x6 : 0x0)
                                    | ((sys_arch.isa_opts.mpy_fast    ? 1   : 0  ) << 8)
                                    | ((sys_arch.isa_opts.mpy_lat_option-1 & 0x3 ) << 10)
                                    | (0x02 << 16);
  }
  if (sys_arch.isa_opts.swap_option) {
    aux_perms[AUX_SWAP_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_SWAP_BUILD] = 0x3;
  }
  if (sys_arch.isa_opts.norm_option) {
    aux_perms[AUX_NORM_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_NORM_BUILD] = 0x3;
  }
  if (sys_arch.isa_opts.shift_option || sys_arch.isa_opts.shas_option) {
    aux_perms[AUX_BARREL_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_BARREL_BUILD] = 0x03
    | ((sys_arch.isa_opts.shas_option  ? 1 : 0) << 8)
    | ((sys_arch.isa_opts.shift_option ? 1 : 0) << 9);
  }
  if (sys_arch.isa_opts.has_dmp_peripheral) {
    // region 15 is the dmp peripheral region (default)
    aux_perms[AUX_DMP_PER]  |= AUX_ENABLED;
    state.auxs[AUX_DMP_PER] = (0xfUL << (sys_arch.isa_opts.addr_size - 4));
    aux_mask[AUX_DMP_PER]   = state.auxs[AUX_DMP_PER];
  }
  if (sys_arch.isa_opts.dc_uncached_region) { 
    // region 15 is the uncached region (default)
    aux_perms[AUX_CACHE_LIMIT]  |= AUX_ENABLED;
    state.auxs[AUX_CACHE_LIMIT] = (0xfUL << (sys_arch.isa_opts.addr_size - 4));
    aux_mask[AUX_CACHE_LIMIT]   = state.auxs[AUX_CACHE_LIMIT];
  }
  if (core_arch.icache.is_configured) {
    aux_perms[AUX_IC_IVIC]       |= AUX_ENABLED;
    aux_perms[AUX_IC_CTRL]       |= AUX_ENABLED;
    aux_perms[AUX_I_CACHE_BUILD] |= AUX_ENABLED;
    // Compute the bsize, capacity and assoc fields
    //
    uint32 bsize    = 0;
    uint32 capacity = 0;
    uint32 assoc    = 0;
    uint32 ways     = core_arch.icache.ways;
    uint32 sets     = (core_arch.icache.size >> core_arch.icache.block_bits) / ways;
    
    bsize      = core_arch.icache.block_bits - 3;  // Block Size in bytes
    uint32 cap = ((sets * ways) << core_arch.icache.block_bits) / 512;
    while (cap >>= 1) ++capacity;
    uint32 asc = ways;
    while (asc >>= 1) ++assoc;
    
    state.auxs[AUX_I_CACHE_BUILD] =   ((sys_arch.isa_opts.ic_feature & 0x3) << 20)
                                    | ((bsize    & 0xf)                     << 16)
                                    | ((capacity & 0xf)                     << 12)
                                    | ((assoc    & 0xf)                     << 8)
                                    | 0x4;
    
    if (sys_arch.isa_opts.ic_feature > 0) {
      aux_perms[AUX_IC_LIL]  |= AUX_ENABLED;
      aux_perms[AUX_IC_IVIL] |= AUX_ENABLED;
    }
    if (sys_arch.isa_opts.ic_feature > 1) {
      aux_perms[AUX_IC_RAM_ADDRESS] |= AUX_ENABLED;
      aux_perms[AUX_IC_TAG]         |= AUX_ENABLED;
      aux_perms[AUX_IC_DATA]        |= AUX_ENABLED;
    }
    // Enable or disable the icache on reset, according to the 
    // ic_disable_on_reset option
    //
    if (sys_arch.isa_opts.ic_disable_on_reset) {
      if (mem_model) mem_model->disable_icache();
      state.auxs[AUX_IC_CTRL] |= 0x1;  // set the DC bit when disabled on reset
    }
  }
  
  if (core_arch.dcache.is_configured) {
    aux_perms[AUX_DC_IVDC]       |= AUX_ENABLED;
    aux_perms[AUX_DC_CTRL]       |= AUX_ENABLED;
    aux_perms[AUX_DC_FLSH]       |= AUX_ENABLED;
    aux_perms[AUX_D_CACHE_BUILD] |= AUX_ENABLED;
    // Compute the bsize, capacity and assoc fields
    //
    uint32 bsize    = 0;
    uint32 capacity = 0;
    uint32 assoc    = 0;
    uint32 ways     = core_arch.dcache.ways;
    uint32 sets     = (core_arch.dcache.size >> core_arch.dcache.block_bits) / ways;

    bsize      = core_arch.dcache.block_bits - 4; // Block Size in bytes
    uint32 cap = ((sets * ways) << core_arch.dcache.block_bits) / 512;
    while (cap >>= 1) ++capacity;
    uint32 asc = ways;
    while (asc >>= 1) ++assoc;
    
    state.auxs[AUX_D_CACHE_BUILD] =   ((sys_arch.isa_opts.dc_feature & 0x3) << 20)
                                    | ((bsize    & 0xf)                     << 16)
                                    | ((capacity & 0xf)                     << 12)
                                    | ((assoc    & 0xf)                     << 8)
                                    | 0x4;
    
    if (sys_arch.isa_opts.dc_feature > 0) {
      aux_perms[AUX_DC_LDL]  |= AUX_ENABLED;
      aux_perms[AUX_DC_IVDL] |= AUX_ENABLED;
      aux_perms[AUX_DC_FLDL] |= AUX_ENABLED;
    }
    if (sys_arch.isa_opts.dc_feature > 1) {
      aux_perms[AUX_DC_RAM_ADDRESS] |= AUX_ENABLED;
      aux_perms[AUX_DC_TAG]         |= AUX_ENABLED;
      aux_perms[AUX_DC_DATA]        |= AUX_ENABLED;
    }
  }
  if (sys_arch.isa_opts.ifq_size > 0) {
    aux_perms[AUX_IFQUEUE_BUILD] |= AUX_ENABLED;
    aux_perms[AUX_IC_IVIC]       |= AUX_ENABLED;
    uint32 q   = 0;
    uint32 ifq = sys_arch.isa_opts.ifq_size;
    while (ifq >>= 1) ++q;
    state.auxs[AUX_IFQUEUE_BUILD] = (q << 8) | 0x2;
  }
  
  if (core_arch.iccm.is_configured) {
    aux_perms[AUX_ICCM]       |= AUX_ENABLED;
    aux_perms[AUX_ICCM_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_ICCM]       = ccm_mgr_->get_memory_region_base(core_arch.iccm.start_addr);
    
    // get the ICCM size and start address from core_arch.iccm
    //
    uint32 iccm_size  = core_arch.iccm.size/256;
    uint32 iccm_start = ccm_mgr_->get_memory_region_base(core_arch.iccm.start_addr);

    uint32 size_field = 0;
    while (iccm_size >>= 1) ++size_field;
    state.auxs[AUX_ICCM_BUILD] = iccm_start | (size_field << 8) | 0x3;
    
    LOG(LOG_DEBUG) << "[ICCM] AUX_ICCM_BUILD register: '0x"
                   << std::hex << std::setw(8) << std::setfill('0')
                   << state.auxs[AUX_ICCM_BUILD] << "'";
  }

  if (core_arch.dccm.is_configured) {
    aux_perms[AUX_DCCM]       |= AUX_ENABLED;
    aux_perms[AUX_DCCM_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_DCCM] = ccm_mgr_->get_memory_region_base(core_arch.dccm.start_addr);

    // get the DCCM size and start address from core_arch.dccm
    //
    uint32 dccm_size  = core_arch.dccm.size/256;
    uint32 dccm_start = ccm_mgr_->get_memory_region_base(core_arch.dccm.start_addr);

    uint32 size_field = 0;
    while (dccm_size >>= 1) ++size_field;      
    state.auxs[AUX_DCCM_BUILD] = /* dccm_start | */ (size_field << 8) | 0x3;
    
    LOG(LOG_DEBUG) << "[DCCM] AUX_DCCM_BUILD register: '0x"
                   << std::hex << std::setw(8) << std::setfill('0')
                   << state.auxs[AUX_DCCM_BUILD] << "'";
  }
  // Enable Actionpoint auxiliary registers if they are configured
  //
  if (sys_arch.isa_opts.num_actionpoints > 0) {
    aux_perms[AUX_AP_BUILD]   |= AUX_ENABLED;
    aux_perms[AUX_AP_WP_PC]   |= AUX_ENABLED;
    state.auxs[AUX_AP_BUILD]  =   0x05
                                | (sys_arch.isa_opts.aps_full ? 0 : 0x400)
                                | ((sys_arch.isa_opts.num_actionpoints/4) << 8);
    
    // Create the Actionpoint auxiliary registers, in the range 0x220 - 0x237
    //
    for (uint32 i = 0; i < sys_arch.isa_opts.num_actionpoints; i++) {
      aux_perms[0x220+(3*i)+0]  = AUX_ENABLED + AUX_K_RW;    // AP_AMVi
      aux_perms[0x220+(3*i)+1]  = AUX_ENABLED + AUX_K_RW;    // AP_AMMi
      aux_perms[0x220+(3*i)+2]  = AUX_ENABLED + AUX_K_RW;    // AP_ACi
      aux_mask [0x220+(3*i)+0]  = 0xffffffff;
      aux_mask [0x220+(3*i)+1]  = 0xffffffff;
      aux_mask [0x220+(3*i)+2]  = 0x3ff;
      state.auxs[0x220+(3*i)+0] = 0;
      state.auxs[0x220+(3*i)+1] = 0;
      state.auxs[0x220+(3*i)+2] = 0;
    }
    // Extend the significant region of the AUX_DEBUG register to
    // include the AH bit (2) and ASR[7:0] field (10:3).
    //
    aux_mask[AUX_DEBUG] |= 0x7fc;
  }
  // Enable the XPU register if any extensions, of any kind, are defined.
  //
  if (eia_mgr.any_eia_extensions_defined) {
    aux_perms[AUX_XPU]    |= AUX_ENABLED;
    aux_perms[AUX_XFLAGS] |= AUX_ENABLED;
  }
  // If div_rem_option is not enabled, REMOVE the mask bit for
  // the DZ bit in AUX_STATUS32 so that it can never be written
  // or read directly.
  //
  if (!sys_arch.isa_opts.div_rem_option) {
    aux_mask[AUX_STATUS32]    = aux_mask[AUX_STATUS32]    & (~0x2000UL);
    aux_mask[AUX_ERSTATUS]    = aux_mask[AUX_ERSTATUS]    & (~0x2000UL);
    aux_mask[AUX_STATUS32_L1] = aux_mask[AUX_STATUS32_L1] & (~0x2000UL);
    aux_mask[AUX_STATUS32_L2] = aux_mask[AUX_STATUS32_L2] & (~0x2000UL);
  }
  // Enable the SmaRT auxiliary registers is SmaRT is properly configured
  //
  if (smt.is_configured() > 0) {
    LOG(LOG_DEBUG1) << "[AUX] enabling SmaRT auxiliary registers";
    aux_perms[AUX_SMART_BUILD]   |= AUX_ENABLED;
    aux_perms[AUX_SMART_CONTROL] |= AUX_ENABLED;
    aux_perms[AUX_SMART_DATA]    |= AUX_ENABLED;
  }
  
  // FP configuration
  if(sys_arch.isa_opts.fpx_option) {
    aux_perms[AUX_FP_BUILD]   |= AUX_ENABLED;
    aux_perms[AUX_DPFP_BUILD] |= AUX_ENABLED;
    
    //set the versions to version 2, but don't enable the fast variants
    state.auxs[AUX_FP_BUILD]   = 0x2;
    state.auxs[AUX_DPFP_BUILD] = 0x2;
    
    aux_perms[AUX_FP_STATUS]  |= AUX_ENABLED;
    aux_perms[AUX_DPFP_STATUS]|= AUX_ENABLED;
    
    state.auxs[AUX_FP_STATUS]  = (0x3<<2) | 0x1;
    state.auxs[AUX_DPFP_STATUS]= (0x3<<2) | 0x1;
    
    aux_perms[AUX_DPFP1L]     |= AUX_ENABLED;
    aux_perms[AUX_DPFP1H]     |= AUX_ENABLED;
    aux_perms[AUX_DPFP2L]     |= AUX_ENABLED;
    aux_perms[AUX_DPFP2H]     |= AUX_ENABLED;
    
    state.auxs[AUX_DPFP1L]     = 0;
    state.auxs[AUX_DPFP1H]     = 0;
    state.auxs[AUX_DPFP2L]     = 0;
    state.auxs[AUX_DPFP2H]     = 0; 
  }

}
      

// ---------------------------------------------------------------------------
// A600 auxiliary register initialisation
//
void Processor::init_aux_regs_a600 ()
{
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] Initializing auxiliary registers for ARC600";
  // Enable all baseline ARCompact A600 registers
  //
  for (uint32 i = 0; baseline_aux_regs_a600[i]; i++) {
    aux_perms[baseline_aux_regs_a600[i]] |= AUX_ENABLED;
  }
  // Assign values to baseline BCRs and Read-only registers
  //
  state.auxs[AUX_IDENTITY]         = 0x21;
  state.auxs[AUX_BCR_VER]          = 0x2;
  state.auxs[AUX_INT_VECTOR_BASE]  =  sys_arch.isa_opts.intvbase_preset 
                                    & aux_mask[AUX_INT_VECTOR_BASE];
  
  { // Set the AUX_VECBASE_AC_BUILD BCR according to the number of
    // configured interrupts and the AUX_INF_VECTOR_BASE value.
    //
    uint32 p;
    switch (sys_arch.isa_opts.num_interrupts) {
      case 16: p = 0; break;
      case 8:  p = 2; break;
      default: p = 1;
    }
    state.auxs[AUX_VECBASE_AC_BUILD] = state.auxs[AUX_INT_VECTOR_BASE] | (0x00 << 2) | p;
  }  
  
  { // Set the AUX_IRQ_LEV register bits according to the reset
    // values specified by options timer_0_int_level and timer_1_int_level
    //
    int p0 = (sys_arch.isa_opts.timer_0_int_level == 2) ? 1 : 0;
    int p1 = (sys_arch.isa_opts.timer_1_int_level == 2) ? 1 : 0;
    state.auxs[AUX_IRQ_LEV] = (p1 << 4) | (p0 << 3);
  }
  state.auxs[AUX_RF_BUILD]         =  ((sys_arch.isa_opts.only_16_regs ? 1 : 0) << 9)
                                    | ((sys_arch.isa_opts.rf_4port     ? 1 : 0) << 8)
                                    | 0x1;
  state.auxs[AUX_MINMAX_BUILD]     = 0x2;
  state.auxs[AUX_ISA_CONFIG]       = (((sys_arch.isa_opts.lpc_size > 7)
                                        ? ((sys_arch.isa_opts.lpc_size/4 - 1) & 7)
                                        : 0)                                      << 12)
                                    | (((sys_arch.isa_opts.pc_size/4 - 4) & 7)    <<  8)
                                    | 0x1;
  // Enable optional Zero-overhead Loop registers
  //
  if (sys_arch.isa_opts.lpc_size > 0) {
    aux_perms[AUX_LP_START] |= AUX_ENABLED;
    aux_perms[AUX_LP_END]   |= AUX_ENABLED;
  }
  // Enable optional timer0 registers
  //
  if (sys_arch.isa_opts.has_timer0) {
    aux_perms[AUX_COUNT0]      |= AUX_ENABLED;
    aux_perms[AUX_LIMIT0]      |= AUX_ENABLED;
    aux_perms[AUX_CONTROL0]    |= AUX_ENABLED;
    state.auxs[AUX_LIMIT0]     = 0x00ffffff;
  }
  // Enable optional timer1 registers
  //
  if (sys_arch.isa_opts.has_timer1) {
    aux_perms[AUX_COUNT1]      |= AUX_ENABLED;
    aux_perms[AUX_LIMIT1]      |= AUX_ENABLED;
    aux_perms[AUX_CONTROL1]    |= AUX_ENABLED;
    state.auxs[AUX_LIMIT1]     = 0x00ffffff;
  }
  // Enable optional TIMER_BUILD aux register if any timers enabled
  //
  if (sys_arch.isa_opts.has_timer0 || sys_arch.isa_opts.has_timer1) {
    aux_perms[AUX_TIMER_BUILD]  |= AUX_ENABLED;
    state.auxs[AUX_TIMER_BUILD] =   ((sys_arch.isa_opts.has_timer1 ? 1 : 0) << 9)
                                  | ((sys_arch.isa_opts.has_timer0 ? 1 : 0) << 8)
                                  | 0x3;
  }
  // MULTIPLY_BUILD configuration register 
  //
  if (   sys_arch.isa_opts.mpy32_option
      || sys_arch.isa_opts.mpy16_option
      || sys_arch.isa_opts.mul64_option)
  {
    uint32 version32x32 = 0x0;
    if (sys_arch.isa_opts.mpy32_option || sys_arch.isa_opts.mul64_option) {
      version32x32 = 0x4;
      aux_perms[AUX_MULHI] |= AUX_ENABLED;
    }
    
    aux_perms[AUX_MULTIPLY_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_MULTIPLY_BUILD] =   version32x32
                                    | ((sys_arch.isa_opts.mpy_fast     ? 0x1 : 0x0) << 8)
                                    | ((sys_arch.isa_opts.mpy_lat_option-1 & 0x3)   << 10)
                                    | ((sys_arch.isa_opts.mpy16_option ? 0x1 : 0x0) << 16);
  }
  if (sys_arch.isa_opts.swap_option) {
    aux_perms[AUX_SWAP_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_SWAP_BUILD] = 0x1;
  }
  if (sys_arch.isa_opts.norm_option) {
    aux_perms[AUX_NORM_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_NORM_BUILD] = 0x2;
  }
  if (sys_arch.isa_opts.shift_option) {
    aux_perms[AUX_BARREL_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_BARREL_BUILD] = 0x02;
  }
  // Enable the XPU register if any extensions, of any kind, are defined.
  //
  if (eia_mgr.any_eia_extensions_defined) {
    aux_perms[AUX_XFLAGS] |= AUX_ENABLED;
  }
  if (core_arch.icache.is_configured) {
    aux_perms[AUX_IC_IVIC]       |= AUX_ENABLED;
    aux_perms[AUX_IC_CTRL]       |= AUX_ENABLED;
    aux_perms[AUX_I_CACHE_BUILD] |= AUX_ENABLED;
    // Compute the bsize, capacity and assoc fields
    //
    uint32 bsize    = 0;
    uint32 capacity = 0;
    uint32 assoc    = 0;
    uint32 ways     = core_arch.icache.ways;
    uint32 sets     = (core_arch.icache.size >> core_arch.icache.block_bits) / ways;
    
    bsize      = core_arch.icache.block_bits - 3;  // Block Size in bytes
    uint32 cap = ((sets * ways) << core_arch.icache.block_bits) / 512;
    while (cap >>= 1) ++capacity;
    uint32 asc = ways;
    switch (asc) {
      case 0x2: { assoc = 0x3; break; }
      case 0x4: { assoc = 0x4; break; }
      default:  { assoc = 0x0; break; }
    }
    // FIXME: Take into account Code RAM?
    state.auxs[AUX_I_CACHE_BUILD] =   ((bsize    & 0xf) << 16)
                                    | ((capacity & 0xf) << 12)
                                    | ((assoc    & 0xf) << 8)
                                    | 0x1;    
    if (sys_arch.isa_opts.ic_feature > 0) {
      aux_perms[AUX_IC_LIL]  |= AUX_ENABLED;
      aux_perms[AUX_IC_IVIL] |= AUX_ENABLED;
    }
    if (sys_arch.isa_opts.ic_feature > 1) {
      aux_perms[AUX_IC_RAM_ADDRESS] |= AUX_ENABLED;
      aux_perms[AUX_IC_TAG]         |= AUX_ENABLED;
      aux_perms[AUX_IC_DATA]        |= AUX_ENABLED;
    }
    // Enable or disable the icache on reset, according to the 
    // ic_disable_on_reset option
    //
    if (sys_arch.isa_opts.ic_disable_on_reset) {
      if (mem_model) mem_model->disable_icache();
      state.auxs[AUX_IC_CTRL] |= 0x1;  // set the DC bit when disabled on reset
    }
  }
  
  if (core_arch.dcache.is_configured) {
    aux_perms[AUX_DC_IVDC]       |= AUX_ENABLED;
    aux_perms[AUX_DC_CTRL]       |= AUX_ENABLED;
    aux_perms[AUX_DC_FLSH]       |= AUX_ENABLED;
    aux_perms[AUX_D_CACHE_BUILD] |= AUX_ENABLED;
    // Compute the bsize, capacity and assoc fields
    //
    uint32 bsize    = 0;
    uint32 capacity = 0;
    uint32 assoc    = 0;
    uint32 ways     = core_arch.dcache.ways;
    uint32 sets     = (core_arch.dcache.size >> core_arch.dcache.block_bits) / ways;
    
    bsize      = core_arch.dcache.block_bits - 4; // Block Size in bytes;
    uint32 cap = ((sets * ways) << core_arch.dcache.block_bits) /512;
    while (cap >>= 1) ++capacity;
    uint32 asc = ways;
    while (asc >>= 1) ++assoc;  // FIXME: take into account 2/3 stage pipeline?
    
    state.auxs[AUX_D_CACHE_BUILD] =   ((bsize & 0xf)    << 16)
                                    | ((capacity & 0xf) << 12)
                                    | ((assoc & 0xf)    << 8)
                                    | 0x1;
    if (sys_arch.isa_opts.dc_feature > 0) {
      aux_perms[AUX_DC_LDL]  |= AUX_ENABLED;
      aux_perms[AUX_DC_IVDL] |= AUX_ENABLED;
      aux_perms[AUX_DC_FLDL] |= AUX_ENABLED;
    }
    if (sys_arch.isa_opts.dc_feature > 1) {
      aux_perms[AUX_DC_RAM_ADDRESS] |= AUX_ENABLED;
      aux_perms[AUX_DC_TAG]         |= AUX_ENABLED;
      aux_perms[AUX_DC_DATA]        |= AUX_ENABLED;
    }
  }
  if (core_arch.iccm.is_configured) {
    aux_perms[AUX_ICCM_BUILD] |= AUX_ENABLED;
    
    // Configure Version, Size, and Base address of ICCM
    //
    uint32 version = 0x1;
    uint32 size     = core_arch.iccm.size >> 10; // minimum is 1K, thus we shift 10 bits
    uint32 capacity = 1;
    while (size >>= 1) ++capacity;
    state.auxs[AUX_ICCM_BUILD] =    version
                                  | (capacity << 8)
                                  | (core_arch.iccm.start_addr & 0xFFFFE000UL);
  }
  if (core_arch.dccm.is_configured) {
    aux_perms[AUX_DCCM_BASE_BUILD] |= AUX_ENABLED;
    aux_perms[AUX_DCCM_BUILD]      |= AUX_ENABLED;
    
    // Configure Version and Size of ICCM
    //
    uint32 version  = 0x1;
    uint32 size     = core_arch.dccm.size >> 11; // minimum is 2K, thus we shift 11 bits
    uint32 capacity = 0;
    while (size >>= 1) ++capacity;
    
    state.auxs[AUX_DCCM_BUILD] = version | (capacity << 8);
    // Configure Base address of CCM
    //
    state.auxs[AUX_DCCM_BASE_BUILD] =   version
                                      | (core_arch.dccm.start_addr & 0xFFFFFF00UL);
  }
  
  if (sys_arch.isa_opts.sat_option) { // enable Extended Arithmetic in BCR
    state.auxs[AUX_EA_BUILD] = 0x2; // set BCR to 'current' version
    aux_perms[AUX_EA_BUILD] |= AUX_ENABLED; // enable BCR
  }
  
  // FP configuration
  if(sys_arch.isa_opts.fpx_option) {
    aux_perms[AUX_FP_BUILD]   |= AUX_ENABLED;
    aux_perms[AUX_DPFP_BUILD] |= AUX_ENABLED;
    
    //set the versions to version 2, but don't enable the fast variants
    state.auxs[AUX_FP_BUILD]   = 0x2;
    state.auxs[AUX_DPFP_BUILD] = 0x2;
    
    aux_perms[AUX_FP_STATUS]  |= AUX_ENABLED;
    aux_perms[AUX_DPFP_STATUS]|= AUX_ENABLED;
    
    state.auxs[AUX_FP_STATUS]  = (0x3<<2) | 0x1;
    state.auxs[AUX_DPFP_STATUS]= (0x3<<2) | 0x1;
    
    aux_perms[AUX_DPFP1L]     |= AUX_ENABLED;
    aux_perms[AUX_DPFP1H]     |= AUX_ENABLED;
    aux_perms[AUX_DPFP2L]     |= AUX_ENABLED;
    aux_perms[AUX_DPFP2H]     |= AUX_ENABLED;
    
    state.auxs[AUX_DPFP1L]     = 0;
    state.auxs[AUX_DPFP1H]     = 0;
    state.auxs[AUX_DPFP2L]     = 0;
    state.auxs[AUX_DPFP2H]     = 0; 
  }

  // TODO: Enable Actionpoint auxiliary registers if they are configured
  // TODO: Enable the SmaRT auxiliary registers is SmaRT is properly configured
}

// ---------------------------------------------------------------------------
// A700 auxiliary register initialisation
//
void Processor::init_aux_regs_a700 ()
{
  LOG(LOG_DEBUG) << "[CPU" << core_id << "] Initializing auxiliary registers for ARC700";
  // Enable all baseline ARCompact A700 registers
  //
  for (uint32 i = 0; baseline_aux_regs_a700[i]; i++) {
    aux_perms[baseline_aux_regs_a700[i]] |= AUX_ENABLED;
  }
  // Assign values to baseline BCRs and Read-only registers
  //
  state.auxs[AUX_IDENTITY]         = 0x32;
  state.auxs[AUX_BCR_VER]          = 0x2;
  state.auxs[AUX_BTA_LINK_BUILD]   = 0x0;
  state.auxs[AUX_INT_VECTOR_BASE]  =   sys_arch.isa_opts.intvbase_preset 
                                     & aux_mask[AUX_INT_VECTOR_BASE];
  
  { // Set the AUX_VECBASE_AC_BUILD BCR according to the number of
    // configured interrupts and the AUX_INF_VECTOR_BASE value.
    //
    uint32 p;
    if (sys_arch.isa_opts.num_interrupts == 16) { p = 0; }
    else                                        { p = 1; }
    state.auxs[AUX_VECBASE_AC_BUILD] = state.auxs[AUX_INT_VECTOR_BASE] | (0x01 << 2) | p;
  }
  
  
  { // Set the AUX_IRQ_LEV register bits according to the reset
    // values specified by options timer_0_int_level and timer_1_int_level
    //
    int p0 = (sys_arch.isa_opts.timer_0_int_level == 2) ? 1 : 0;
    int p1 = (sys_arch.isa_opts.timer_1_int_level == 2) ? 1 : 0;
    state.auxs[AUX_IRQ_LEV] = (p1 << 4) | (p0 << 3);
  }
  state.auxs[AUX_RF_BUILD]  =   ((sys_arch.isa_opts.only_16_regs ? 1 : 0) << 9)
                              | ((sys_arch.isa_opts.rf_4port     ? 1 : 0) << 8)
                              | 0x1;
  state.auxs[AUX_MINMAX_BUILD]     = 0x2;
  // Set the mask for AUX_IENABLE according to num_interrupts
  //
  aux_mask[AUX_IENABLE] = (0xffffffffUL >> (32 - sys_arch.isa_opts.num_interrupts)) | 0x7UL;
  // Set the mask for AUX_ITRIGGER according to num_interrupts
  //
  aux_mask[AUX_ITRIGGER]= (0xffffffffUL >> (32 - sys_arch.isa_opts.num_interrupts)) & 0xfffffff8UL;
  // Enable optional Zero-overhead Loop registers
  //
  if (sys_arch.isa_opts.lpc_size > 0) {
    aux_perms[AUX_LP_START] |= AUX_ENABLED;
    aux_perms[AUX_LP_END]   |= AUX_ENABLED;
  }
  // Enable optional timer0 registers
  //
  if (sys_arch.isa_opts.has_timer0) {
    aux_perms[AUX_COUNT0]      |= AUX_ENABLED;
    aux_perms[AUX_LIMIT0]      |= AUX_ENABLED;
    aux_perms[AUX_CONTROL0]    |= AUX_ENABLED;
    state.auxs[AUX_LIMIT0]     = 0x00ffffff;
  }
  // Enable optional timer1 registers
  //
  if (sys_arch.isa_opts.has_timer1) {
    aux_perms[AUX_COUNT1]      |= AUX_ENABLED;
    aux_perms[AUX_LIMIT1]      |= AUX_ENABLED;
    aux_perms[AUX_CONTROL1]    |= AUX_ENABLED;
    state.auxs[AUX_LIMIT1]     = 0x00ffffff;
  }
  // Enable optional TIMER_BUILD aux register if any timers enabled
  //
  if (sys_arch.isa_opts.has_timer0 || sys_arch.isa_opts.has_timer1) {
    aux_perms[AUX_TIMER_BUILD]  |= AUX_ENABLED;
    state.auxs[AUX_TIMER_BUILD] = ((sys_arch.isa_opts.has_timer1 ? 1 : 0) << 9)
                                  | ((sys_arch.isa_opts.has_timer0 ? 1 : 0) << 8)
                                  | 0x2;
  }  
  // Enable optional ISA build configuration registers
  //
  if (sys_arch.isa_opts.mpy32_option || sys_arch.isa_opts.mpy16_option) {
    aux_perms[AUX_MULTIPLY_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_MULTIPLY_BUILD] = (sys_arch.isa_opts.mpy32_option ? 0x2 : 0x0);
  }
  if (sys_arch.isa_opts.swap_option) {
    aux_perms[AUX_SWAP_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_SWAP_BUILD] = 0x1;
  }
  if (sys_arch.isa_opts.norm_option) {
    aux_perms[AUX_NORM_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_NORM_BUILD] = 0x2;
  }
  if (sys_arch.isa_opts.shift_option) {
    aux_perms[AUX_BARREL_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_BARREL_BUILD] = 0x02;
  }
  // Enable the XPU register if any extensions, of any kind, are defined.
  //
  if (eia_mgr.any_eia_extensions_defined) {
    aux_perms[AUX_XPU]    |= AUX_ENABLED;
    aux_perms[AUX_XFLAGS] |= AUX_ENABLED;
  }
  if (core_arch.icache.is_configured) {
    aux_perms[AUX_IC_IVIC]       |= AUX_ENABLED;
    aux_perms[AUX_IC_CTRL]       |= AUX_ENABLED;
    aux_perms[AUX_I_CACHE_BUILD] |= AUX_ENABLED;
    // Compute the bsize, capacity and assoc fields
    //
    uint32 bsize    = 0;
    uint32 capacity = 0;
    uint32 assoc    = 0;
    uint32 ways     = core_arch.icache.ways;
    uint32 sets     = (core_arch.icache.size >> core_arch.icache.block_bits) / ways;
    
    bsize      = core_arch.icache.block_bits - 3;  // Block Size in bytes
    uint32 cap = ((sets * ways) << core_arch.icache.block_bits) / 512;
    while (cap >>= 1) ++capacity;
    uint32 asc = ways;
    while (asc >>= 1) ++assoc;
    
    state.auxs[AUX_I_CACHE_BUILD] =   ((bsize    & 0xf) << 16)
                                    | ((capacity & 0xf) << 12)
                                    | ((assoc    & 0xf) << 8)
                                    | 0x1;
    if (sys_arch.isa_opts.ic_feature > 0) {
      aux_perms[AUX_IC_LIL]  |= AUX_ENABLED;
      aux_perms[AUX_IC_IVIL] |= AUX_ENABLED;
    }
    if (sys_arch.isa_opts.ic_feature > 1) {
      aux_perms[AUX_IC_RAM_ADDRESS] |= AUX_ENABLED;
      aux_perms[AUX_IC_TAG]         |= AUX_ENABLED;
      aux_perms[AUX_IC_DATA]        |= AUX_ENABLED;
    }
    // Enable or disable the icache on reset, according to the ic_disable_on_reset
    // option
    //
    if (sys_arch.isa_opts.ic_disable_on_reset) {
      if (mem_model) mem_model->disable_icache();
      state.auxs[AUX_IC_CTRL] |= 0x1;  // set the DC bit when disabled on reset
    }
  }
  if (core_arch.dcache.is_configured) {
    aux_perms[AUX_DC_IVDC]       |= AUX_ENABLED;
    aux_perms[AUX_DC_CTRL]       |= AUX_ENABLED;
    aux_perms[AUX_DC_FLSH]       |= AUX_ENABLED;
    aux_perms[AUX_D_CACHE_BUILD] |= AUX_ENABLED;
    // Compute the bsize, capacity and assoc fields
    //
    uint32 bsize    = 0;
    uint32 capacity = 0;
    uint32 assoc    = 0;
    uint32 ways     = core_arch.dcache.ways;
    uint32 sets     = (core_arch.dcache.size >> core_arch.dcache.block_bits) / ways;
    
    bsize      = core_arch.dcache.block_bits - 4; // Block Size in bytes
    uint32 cap = ((sets * ways) << core_arch.dcache.block_bits) / 512;
    while (cap >>= 1) ++capacity;
    uint32 asc = ways;
    while (asc >>= 1) ++assoc;

    state.auxs[AUX_D_CACHE_BUILD] =   ((bsize    & 0xf) << 16)
                                    | ((capacity & 0xf) << 12)
                                    | ((assoc    & 0xf) << 8)
                                    | 0x1;
    
    if (sys_arch.isa_opts.dc_feature > 0) {
      aux_perms[AUX_DC_LDL]  |= AUX_ENABLED;
      aux_perms[AUX_DC_IVDL] |= AUX_ENABLED;
      aux_perms[AUX_DC_FLDL] |= AUX_ENABLED;
    }
    if (sys_arch.isa_opts.dc_feature > 1) {
      aux_perms[AUX_DC_RAM_ADDRESS] |= AUX_ENABLED;
      aux_perms[AUX_DC_TAG]         |= AUX_ENABLED;
      aux_perms[AUX_DC_DATA]        |= AUX_ENABLED;
    }
  }
 
  if (core_arch.mmu_arch.is_configured) {
    // configure MMU BCR
    if (core_arch.mmu_arch.version == MmuArch::kMmuV3) {
      state.auxs[AUX_MMU_BUILD] =   ((core_arch.mmu_arch.version               & 0xffUL) << 24)
                                    | ((core_arch.mmu_arch.get_jtlb_ways_log2()& 0x0fUL) << 20)
                                    | ((core_arch.mmu_arch.get_jtlb_sets_log2()& 0x0fUL) << 16)
                                    | (0 << 15) /* Overlay support mode disabled for now */
                                    | ((core_arch.mmu_arch.get_page_size_bcr_encoding() & 0x0fUL) << 8)
                                    | ((core_arch.mmu_arch.u_itlb_entries       & 0x0fUL) <<  4)
                                    | ((core_arch.mmu_arch.u_dtlb_entries       & 0x0fUL));
      // we also need to modify I/D-Cache configs for MMU v3
      if (core_arch.icache.is_configured) {
        state.auxs[AUX_I_CACHE_BUILD] |= 0x3;
        aux_perms[AUX_IC_PTAG]        |= AUX_ENABLED;
      }
      if (core_arch.dcache.is_configured) {
        state.auxs[AUX_D_CACHE_BUILD] |= 0x3; 
        aux_perms[AUX_DC_PTAG]        |= AUX_ENABLED;
      }
      // change the default mask for AUX_TLB_PD0 and AUX_TLB_PD1
      aux_mask[AUX_TLB_PD0] = 0xffffffff; // FIXME: @igor - compute proper mask given config
      aux_mask[AUX_TLB_PD1] = 0xffffffff; // FIXME: @igor - compute proper mask given config
      // enable scratch auxiliary register
      aux_perms[AUX_SCRATCH_DATA0] |= AUX_ENABLED;
      // TODO(iboehm): enable SASID auxiliary register
      // aux_perms[AUX_SASID] |= AUX_ENABLED;
    } else {
      state.auxs[AUX_MMU_BUILD] =   ((core_arch.mmu_arch.version               & 0xffUL) << 24)
                                    | ((core_arch.mmu_arch.get_jtlb_ways_log2()& 0x0fUL) << 20)
                                    | ((core_arch.mmu_arch.get_jtlb_sets_log2()& 0x0fUL) << 16)
                                    | ((core_arch.mmu_arch.u_itlb_entries      & 0xffUL) <<  8)
                                    | ((core_arch.mmu_arch.u_dtlb_entries      & 0xffUL));      
    }
    
    
    // Enable MMU related AUX registers
    //
    aux_perms[AUX_MMU_BUILD]    |= AUX_ENABLED;
    aux_perms[AUX_TLB_PD0]      |= AUX_ENABLED;
    aux_perms[AUX_TLB_PD1]      |= AUX_ENABLED;
    aux_perms[AUX_TLB_Index]    |= AUX_ENABLED;
    aux_perms[AUX_TLB_Command]  |= AUX_ENABLED;
    aux_perms[AUX_PID]          |= AUX_ENABLED;
    aux_perms[AUX_SCRATCH_DATA0]|= AUX_ENABLED;
    aux_perms[AUX_EFA]          |= AUX_ENABLED;
    
    // FIXME: does BTA depend on MMU?
    //
    aux_perms[AUX_BTA_L1]       |= AUX_ENABLED;
    aux_perms[AUX_BTA_L2]       |= AUX_ENABLED;
  }
  
  if (core_arch.iccm.is_configured) {
    aux_perms[AUX_ICCM_BUILD] |= AUX_ENABLED;
    
    // Configure Version, Size, and Base address of ICCM
    //
    uint32 version = 0x1;
    uint32 size     = core_arch.iccm.size >> 13; // minimum is 8K, thus we shift 13 bits
    uint32 capacity = 1;
    while (size >>= 1) ++capacity;
    state.auxs[AUX_ICCM_BUILD] =    version
                                  | (capacity << 8)
                                  | (core_arch.iccm.start_addr & 0xFFFFE000UL);
  }
  if (core_arch.dccm.is_configured) {
    aux_perms[AUX_DCCM_BASE_BUILD] |= AUX_ENABLED;
    aux_perms[AUX_DCCM_BUILD]      |= AUX_ENABLED;
    
    // Configure Version and Size of DCCM
    //
    uint32 version  = 0x1;
    uint32 size     = core_arch.dccm.size >> 11; // minimum is 2K, thus we shift 11 bits
    uint32 capacity = 0;
    while (size >>= 1) ++capacity;
    
    state.auxs[AUX_DCCM_BUILD] = version | (capacity << 8);
    // Configure Base address of CCM
    //
    state.auxs[AUX_DCCM_BASE_BUILD] =   version
                                      | (core_arch.dccm.start_addr & 0xFFFFFF00UL);
  }

  if (sys_arch.isa_opts.sat_option) { // enable Extended Arithmetic in BCR
    state.auxs[AUX_EA_BUILD] = 0x2; // set BCR to 'current' version
    aux_perms[AUX_EA_BUILD] |= AUX_ENABLED; // enable BCR
  }

  if (sys_arch.isa_opts.stack_checking) {
    aux_perms[AUX_STACK_REGION_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_STACK_REGION_BUILD] = 0x1;
    aux_perms[AUX_STACK_TOP]          |= AUX_ENABLED;
    aux_perms[AUX_STACK_BASE]         |= AUX_ENABLED;

    aux_mask[AUX_STATUS32]            |= 0x00004000; // enable the SC bit
    aux_mask[AUX_ERSTATUS]            |= 0x00004000; // enable the SC bit
  }
  
  // FP configuration
  if (sys_arch.isa_opts.fpx_option) {
    aux_perms[AUX_FP_BUILD]   |= AUX_ENABLED;
    aux_perms[AUX_DPFP_BUILD] |= AUX_ENABLED;
    
    //set the versions to version 2, but don't enable the fast variants
    state.auxs[AUX_FP_BUILD]   = 0x2;
    state.auxs[AUX_DPFP_BUILD] = 0x2;
    
    aux_perms[AUX_FP_STATUS]  |= AUX_ENABLED;
    aux_perms[AUX_DPFP_STATUS]|= AUX_ENABLED;
    
    state.auxs[AUX_FP_STATUS]  = (0x3<<2) | 0x1;
    state.auxs[AUX_DPFP_STATUS]= (0x3<<2) | 0x1;
    
    aux_perms[AUX_DPFP1L]     |= AUX_ENABLED;
    aux_perms[AUX_DPFP1H]     |= AUX_ENABLED;
    aux_perms[AUX_DPFP2L]     |= AUX_ENABLED;
    aux_perms[AUX_DPFP2H]     |= AUX_ENABLED;
    
    state.auxs[AUX_DPFP1L]     = 0;
    state.auxs[AUX_DPFP1H]     = 0;
    state.auxs[AUX_DPFP2L]     = 0;
    state.auxs[AUX_DPFP2H]     = 0; 
  }
  
  // TODO: Enable Actionpoint auxiliary registers if they are configured
  // TODO: Enable the SmaRT auxiliary registers is SmaRT is properly configured
}

void Processor::init_aux_regs_a6kv21()
{
    LOG(LOG_DEBUG) << "[CPU" << core_id << "] Initializing auxiliary registers for AV2.1";
  // Enable all baseline ARCompact V2 registers
  //
  for (uint32 i = 0; baseline_aux_regs_av21[i]; i++) {
    aux_perms[baseline_aux_regs_av21[i]] |= AUX_ENABLED;
  }
  
  // Set the read/write masks for registers that depend on pc_size,
  // addr_size or lpc_size
  //
  aux_mask[AUX_LP_START]        = state.pc_mask;
  aux_mask[AUX_LP_END]          = state.pc_mask;
  aux_mask[AUX_PC]              = state.pc_mask;
  aux_mask[AUX_ERET]            = state.pc_mask;
  aux_mask[AUX_ERBTA]           = state.pc_mask;
  aux_mask[AUX_BTA]             = state.pc_mask;
  aux_mask[AUX_BTA_L1]          = state.pc_mask;
  aux_mask[AUX_BTA_L2]          = state.pc_mask;
  aux_mask[AUX_JLI_BASE]        = aux_mask[AUX_JLI_BASE]        & state.pc_mask;
  aux_mask[AUX_EI_BASE]         = aux_mask[AUX_EI_BASE]         & state.pc_mask;
  aux_mask[AUX_INT_VECTOR_BASE] = aux_mask[AUX_INT_VECTOR_BASE] & state.pc_mask;
  aux_mask[AUX_EFA]             = state.addr_mask;
  aux_mask[AUX_LDI_BASE]        = aux_mask[AUX_LDI_BASE] & state.pc_mask & state.addr_mask;
  
  // Assign values to baseline BCRs and Read-only registers
  //
  state.auxs[AUX_IDENTITY]         = 0x40;
  state.auxs[AUX_BCR_VER]          = 0x2;
  state.auxs[AUX_BTA_LINK_BUILD]   = 0x0;
  state.auxs[AUX_INT_VECTOR_BASE]  = sys_arch.isa_opts.intvbase_preset & aux_mask[AUX_INT_VECTOR_BASE];
  
  { // Set the AUX_VECBASE_AC_BUILD BCR according to the number of
    // configured interrupts and the AUX_INF_VECTOR_BASE value.
    //
    uint32 p;
    switch (sys_arch.isa_opts.num_interrupts) {
      case 16: p = 0; break;
      case 8:  p = 2; break;
      case 3:  p = 3; break;
      default: p = 1;
    }
    state.auxs[AUX_VECBASE_AC_BUILD] = state.auxs[AUX_INT_VECTOR_BASE] | (0x04 << 2) | p;
  }
  
  uint8 dup_reg_field;
  switch (sys_arch.isa_opts.num_banked_regs) {
    case 4:  dup_reg_field = 0;
    case 8:  dup_reg_field = 1;
    case 16: dup_reg_field = 2;
    case 32: dup_reg_field = 3;
    default: LOG(LOG_WARNING) << "Unrecognized duplicated register count.";
  }
  
  aux_mask[AUX_RF_BUILD]           = 0x0000ffff;
  state.auxs[AUX_RF_BUILD]         =  ((dup_reg_field) << 14)
                                    | ((sys_arch.isa_opts.num_reg_banks == 2) << 11)
                                    | ((0) << 10) //regs cleared on reset
                                    | ((sys_arch.isa_opts.only_16_regs ? 1 : 0) << 9)
                                    | ((sys_arch.isa_opts.rf_4port     ? 1 : 0) << 8)
                                    | 0x2;
  
  state.auxs[AUX_MINMAX_BUILD]     = 0x2;
  
  state.auxs[AUX_ISA_CONFIG]       =  ((sys_arch.isa_opts.div_rem_option ? 1 : 0)  << 28)
                                    | ((sys_arch.isa_opts.density_option & 0xf)    << 24)
                                    | ((sys_arch.isa_opts.atomic_option  & 0x1)    << 21)
#if BIG_ENDIAN_SUPPORT
                                    | ((sim_opts.big_endian ? 1 : 0)               << 20)
#endif
                                    | (((sys_arch.isa_opts.addr_size/4 - 4) & 7)   << 16)
                                    | ((sys_arch.isa_opts.lpc_size > 7
                                        ? ((sys_arch.isa_opts.lpc_size/4 - 1) & 7)
                                        : 0) 
                                       << 12)
                                    | (((sys_arch.isa_opts.pc_size/4 - 4) & 7)     <<  8)
                                    | 0x2;
  
  state.auxs[AUX_IRQ_BUILD]       =  0x01 << 0 //version
                                   | ((sys_arch.isa_opts.num_interrupts & 0xFF) << 8) //number of interrupts
                                   | ((sys_arch.isa_opts.num_interrupts & 0xFF) << 16) //number of external interrupt lines
                                   | ((sys_arch.isa_opts.number_of_levels-1) << 24) //number of interrupt levels - 1
                                   | ((sys_arch.isa_opts.fast_irq? 1 : 0) << 28)      //fast_irq
                                   | ((sys_arch.isa_opts.overload_vectors) << 29);                                     //overload_vectors
  
  //Enable AUX_USER_SP
  aux_perms[AUX_USER_SP] |= AUX_ENABLED;

  if (sys_arch.isa_opts.new_interrupts) {
        
    state.auxs[AUX_IRQ_ACT] = 0;
    aux_perms[AUX_IRQ_ACT]  = AUX_ENABLED | AUX_K_RW;
    aux_mask[AUX_IRQ_ACT]   = (uint32)(pow(2, sys_arch.isa_opts.number_of_levels + 1)) - 1;
    
    state.auxs[AUX_IRQ_CTRL] = 0;
    aux_perms[AUX_IRQ_CTRL]  = AUX_ENABLED | AUX_K_RW;  
    aux_mask[AUX_IRQ_CTRL]   = 0x00001e1f;
    
    state.auxs[AUX_IRQ_LEVEL] = 0;
    aux_perms[AUX_IRQ_LEVEL]  = AUX_ENABLED | AUX_K_RW;
    aux_mask[AUX_IRQ_LEVEL]   = (uint32)(pow(2, ceil(log2(sys_arch.isa_opts.number_of_levels))))-1;
    
    state.auxs[AUX_ICAUSE] = 0;
    aux_perms[AUX_ICAUSE]  = AUX_ENABLED | AUX_K_RW;
    aux_mask[AUX_ICAUSE]   = (uint32)(pow(2, ceil(log2(sys_arch.isa_opts.num_interrupts))) - 1);
    
    state.auxs[AUX_IRQ_LEVEL_PENDING] = 0;
    aux_perms[AUX_IRQ_LEVEL_PENDING] = AUX_ENABLED | AUX_K_READ;
    aux_mask[AUX_IRQ_LEVEL_PENDING]  = (uint32)(pow(2, sys_arch.isa_opts.num_interrupts) - 1);
    
    state.auxs[AUX_IRQ_INTERRUPT] = 0;
    aux_perms[AUX_IRQ_INTERRUPT]  = AUX_ENABLED | AUX_K_RW;
    aux_mask[AUX_IRQ_INTERRUPT]   = 0x000000ff;
    
    aux_perms[AUX_IRQ_PRIORITY]   = AUX_ENABLED | AUX_K_RW;
    aux_mask[AUX_IRQ_PRIORITY]    = (uint32)(pow(2, sys_arch.isa_opts.number_of_levels) - 1);
    
    aux_perms[AUX_IRQ_PENDING]    = AUX_ENABLED | AUX_K_READ;
    aux_mask[AUX_IRQ_PENDING]     = 0x1;
    
    aux_perms[AUX_IRQ_ENABLE]     = AUX_ENABLED | AUX_K_RW;
    aux_mask[AUX_IRQ_ENABLE]      = 0x1;
    
    aux_perms[AUX_IRQ_TRIGGER]    = AUX_ENABLED | AUX_K_RW;
    aux_mask[AUX_IRQ_TRIGGER]     = 0x1;
    
    aux_perms[AUX_IRQ_PULSE_CANCEL] = AUX_ENABLED | AUX_K_WRITE;
    aux_mask[AUX_IRQ_PULSE_CANCEL]  = 0x1;
    
    aux_perms[AUX_IRQ_STATUS]     = AUX_ENABLED | AUX_K_READ;
    aux_mask[AUX_IRQ_STATUS]      = 0x8000031f;

    //enable RB, IE bit and E bits for new interrupt model
    //
    aux_mask[AUX_STATUS32]       |= 0x8007001E;
    aux_mask[AUX_ERSTATUS]       |= 0x8007001E;  
    
    //Apply any mask changes to AUX_STATUS32_P0 as well
    state.auxs[AUX_STATUS32_P0] = 0;
    aux_perms[AUX_STATUS32_P0]  = AUX_ENABLED | AUX_K_RW; //TODO: Should be 'rG'?
    aux_mask[AUX_STATUS32_P0]   = aux_mask[AUX_STATUS32];
  }else{
    aux_perms[AUX_STATUS32_L1] |= AUX_ENABLED;
    aux_perms[AUX_STATUS32_L2] |= AUX_ENABLED;
  }
  
  if (sys_arch.isa_opts.lpc_size > 0) {
    aux_perms[AUX_LP_START] |= AUX_ENABLED;
    aux_perms[AUX_LP_END]   |= AUX_ENABLED;
  }
  // Enable optional code-density registers
  //
  if (sys_arch.isa_opts.density_option > 0) {
    aux_perms[AUX_JLI_BASE] |= AUX_ENABLED;
    

    if (sys_arch.isa_opts.density_option > 1) {
      aux_perms[AUX_LDI_BASE]   |= AUX_ENABLED;
      aux_perms[AUX_EI_BASE]    |= AUX_ENABLED;
      if(!sys_arch.isa_opts.new_interrupts){
        aux_mask[AUX_STATUS32_L1] |= 0x00008000;
        aux_mask[AUX_STATUS32_L2] |= 0x00008000;
      }
      aux_mask[AUX_STATUS32]    |= 0x00008000; // enable the ES bit
      aux_mask[AUX_ERSTATUS]    |= 0x00008000; // enable the ES bit
    }
  }

  if (sys_arch.isa_opts.stack_checking) {

    aux_perms[AUX_STACK_REGION_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_STACK_REGION_BUILD] = 0x2;
    aux_perms[AUX_USTACK_TOP]         |= AUX_ENABLED;
    aux_perms[AUX_USTACK_BASE]        |= AUX_ENABLED;
    aux_perms[AUX_KSTACK_TOP]         |= AUX_ENABLED;
    aux_perms[AUX_KSTACK_BASE]        |= AUX_ENABLED;
  
    aux_mask[AUX_STATUS32]    |= 0x00004000; // enable the SC bit
    aux_mask[AUX_ERSTATUS]    |= 0x00004000; // enable the SC bit
    if ((sys_arch.isa_opts.density_option > 1) && (!sys_arch.isa_opts.new_interrupts)) {
      aux_mask[AUX_STATUS32_L1] |= 0x00004000; // enable the SC bit
      aux_mask[AUX_STATUS32_L2] |= 0x00004000; // enable the SC bit
    }
  }

  // Enable optional timer0 registers
  //
  if (sys_arch.isa_opts.has_timer0) {
    aux_perms[AUX_COUNT0]      |= AUX_ENABLED;
    aux_perms[AUX_LIMIT0]      |= AUX_ENABLED;
    aux_perms[AUX_CONTROL0]    |= AUX_ENABLED;
    state.auxs[AUX_LIMIT0]     = 0x00ffffff;
  }
  // Enable optional timer1 registers
  //
  if (sys_arch.isa_opts.has_timer1) {
    aux_perms[AUX_COUNT1]      |= AUX_ENABLED;
    aux_perms[AUX_LIMIT1]      |= AUX_ENABLED;
    aux_perms[AUX_CONTROL1]    |= AUX_ENABLED;
    state.auxs[AUX_LIMIT1]     = 0x00ffffff;
  }
  
  // Enable RTC registers
  if(sys_arch.isa_opts.rtc_option)
  {
    aux_perms[AUX_RTC_CTRL]    |= AUX_ENABLED;
    aux_perms[AUX_RTC_LOW]     |= AUX_ENABLED;
    aux_perms[AUX_RTC_HIGH]    |= AUX_ENABLED;
  }
  
  // Enable optional TIMER_BUILD aux register if any timers enabled
  //
  if (sys_arch.isa_opts.has_timer0 || sys_arch.isa_opts.has_timer1 || sys_arch.isa_opts.rtc_option) {
    aux_perms[AUX_TIMER_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_TIMER_BUILD] =   ((sys_arch.isa_opts.rtc_option ? 1 : 0) << 10)
                                  | ((sys_arch.isa_opts.has_timer1 ? 1 : 0) << 9)
                                  | ((sys_arch.isa_opts.has_timer0 ? 1 : 0) << 8)
                                  | 0x4;
  }
  // Enable optional ISA build configuration registers
  //
  if (sys_arch.isa_opts.mpy32_option || sys_arch.isa_opts.mpy16_option) {
    aux_perms[AUX_MULTIPLY_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_MULTIPLY_BUILD] =  (sys_arch.isa_opts.mpy32_option ? 0x6 : 0x0)
                                    | ((sys_arch.isa_opts.mpy_fast    ? 1   : 0  ) << 8)
                                    | ((sys_arch.isa_opts.mpy_lat_option-1 & 0x3 ) << 10)
                                    | (0x02 << 16);
  }
  if (sys_arch.isa_opts.swap_option) {
    aux_perms[AUX_SWAP_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_SWAP_BUILD] = 0x3;
  }
  if (sys_arch.isa_opts.norm_option) {
    aux_perms[AUX_NORM_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_NORM_BUILD] = 0x3;
  }
  if (sys_arch.isa_opts.shift_option || sys_arch.isa_opts.shas_option) {
    aux_perms[AUX_BARREL_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_BARREL_BUILD] = 0x03
    | ((sys_arch.isa_opts.shas_option  ? 1 : 0) << 8)
    | ((sys_arch.isa_opts.shift_option ? 1 : 0) << 9);
  }
  if (sys_arch.isa_opts.has_dmp_peripheral) {
    // region 15 is the dmp peripheral region (default)
    aux_perms[AUX_DMP_PER]  |= AUX_ENABLED;
    state.auxs[AUX_DMP_PER] = (0xfUL << (sys_arch.isa_opts.addr_size - 4));
    aux_mask[AUX_DMP_PER]   = state.auxs[AUX_DMP_PER];
  }
  if (sys_arch.isa_opts.dc_uncached_region) { 
    // region 15 is the uncached region (default)
    aux_perms[AUX_CACHE_LIMIT]  |= AUX_ENABLED;
    state.auxs[AUX_CACHE_LIMIT] = (0xfUL << (sys_arch.isa_opts.addr_size - 4));
    aux_mask[AUX_CACHE_LIMIT]   = state.auxs[AUX_CACHE_LIMIT];
  }
  if (core_arch.icache.is_configured) {
    aux_perms[AUX_IC_IVIC]       |= AUX_ENABLED;
    aux_perms[AUX_IC_CTRL]       |= AUX_ENABLED;
    aux_perms[AUX_I_CACHE_BUILD] |= AUX_ENABLED;
    // Compute the bsize, capacity and assoc fields
    //
    uint32 bsize    = 0;
    uint32 capacity = 0;
    uint32 assoc    = 0;
    uint32 ways     = core_arch.icache.ways;
    uint32 sets     = (core_arch.icache.size >> core_arch.icache.block_bits) / ways;
    
    bsize      = core_arch.icache.block_bits - 3;  // Block Size in bytes
    uint32 cap = ((sets * ways) << core_arch.icache.block_bits) / 512;
    while (cap >>= 1) ++capacity;
    uint32 asc = ways;
    while (asc >>= 1) ++assoc;
    
    state.auxs[AUX_I_CACHE_BUILD] =   ((sys_arch.isa_opts.ic_feature & 0x3) << 20)
                                    | ((bsize    & 0xf)                     << 16)
                                    | ((capacity & 0xf)                     << 12)
                                    | ((assoc    & 0xf)                     << 8)
                                    | 0x4;
    
    if (sys_arch.isa_opts.ic_feature > 0) {
      aux_perms[AUX_IC_LIL]  |= AUX_ENABLED;
      aux_perms[AUX_IC_IVIL] |= AUX_ENABLED;
    }
    if (sys_arch.isa_opts.ic_feature > 1) {
      aux_perms[AUX_IC_RAM_ADDRESS] |= AUX_ENABLED;
      aux_perms[AUX_IC_TAG]         |= AUX_ENABLED;
      aux_perms[AUX_IC_DATA]        |= AUX_ENABLED;
    }
    // Enable or disable the icache on reset, according to the 
    // ic_disable_on_reset option
    //
    if (sys_arch.isa_opts.ic_disable_on_reset) {
      if (mem_model) mem_model->disable_icache();
      state.auxs[AUX_IC_CTRL] |= 0x1;  // set the DC bit when disabled on reset
    }
  }
  
  if (core_arch.dcache.is_configured) {
    aux_perms[AUX_DC_IVDC]       |= AUX_ENABLED;
    aux_perms[AUX_DC_CTRL]       |= AUX_ENABLED;
    aux_perms[AUX_DC_FLSH]       |= AUX_ENABLED;
    aux_perms[AUX_D_CACHE_BUILD] |= AUX_ENABLED;
    // Compute the bsize, capacity and assoc fields
    //
    uint32 bsize    = 0;
    uint32 capacity = 0;
    uint32 assoc    = 0;
    uint32 ways     = core_arch.dcache.ways;
    uint32 sets     = (core_arch.dcache.size >> core_arch.dcache.block_bits) / ways;

    bsize      = core_arch.dcache.block_bits - 4; // Block Size in bytes
    uint32 cap = ((sets * ways) << core_arch.dcache.block_bits) / 512;
    while (cap >>= 1) ++capacity;
    uint32 asc = ways;
    while (asc >>= 1) ++assoc;
    
    state.auxs[AUX_D_CACHE_BUILD] =   ((sys_arch.isa_opts.dc_feature & 0x3) << 20)
                                    | ((bsize    & 0xf)                     << 16)
                                    | ((capacity & 0xf)                     << 12)
                                    | ((assoc    & 0xf)                     << 8)
                                    | 0x4;
    
    if (sys_arch.isa_opts.dc_feature > 0) {
      aux_perms[AUX_DC_LDL]  |= AUX_ENABLED;
      aux_perms[AUX_DC_IVDL] |= AUX_ENABLED;
      aux_perms[AUX_DC_FLDL] |= AUX_ENABLED;
    }
    if (sys_arch.isa_opts.dc_feature > 1) {
      aux_perms[AUX_DC_RAM_ADDRESS] |= AUX_ENABLED;
      aux_perms[AUX_DC_TAG]         |= AUX_ENABLED;
      aux_perms[AUX_DC_DATA]        |= AUX_ENABLED;
    }
  }
  if (sys_arch.isa_opts.ifq_size > 0) {
    aux_perms[AUX_IFQUEUE_BUILD] |= AUX_ENABLED;
    aux_perms[AUX_IC_IVIC]       |= AUX_ENABLED;
    uint32 q   = 0;
    uint32 ifq = sys_arch.isa_opts.ifq_size;
    while (ifq >>= 1) ++q;
    state.auxs[AUX_IFQUEUE_BUILD] = (q << 8) | 0x2;
  }
  if (core_arch.iccm.is_configured) {
    if (sys_arch.isa_opts.multiple_iccms) {
      // Multiple ICCMs (EM 1.1)
      //
      aux_perms[AUX_ICCM]       |= AUX_ENABLED;
      aux_perms[AUX_ICCM_BUILD] |= AUX_ENABLED;
      state.auxs[AUX_ICCM]       = 0;
      state.auxs[AUX_ICCM_BUILD] = 0x3; //version

      // Incrementally compute ICCM BCR and ICCM region mappings
      //
      for (int i = 0; i < IsaOptions::kMultipleIccmCount; ++i) {
        uint32 iccm_size  = (core_arch.iccms[i].is_configured) ? (core_arch.iccms[i].size / 256) : 0;
        uint32 size_field = 0;
        while (iccm_size >>= 1) ++size_field;
        state.auxs[AUX_ICCM_BUILD] |= (size_field & 0xF) << (4 * i + 8);
        uint32 location_field       = ((core_arch.iccms[i].start_addr >> (sys_arch.isa_opts.addr_size - 4)) & 0xF);
        state.auxs[AUX_ICCM]       |= location_field << (sys_arch.isa_opts.addr_size - 4 - (4 * i));
      }
      
      LOG(LOG_DEBUG) << "[ICCM] AUX_ICCM register: 0x"   << HEX(state.auxs[AUX_ICCM]);
    } else {
      // Standard ICCM configuration
      //
      aux_perms[AUX_ICCM]       |= AUX_ENABLED;
      aux_perms[AUX_ICCM_BUILD] |= AUX_ENABLED;
      state.auxs[AUX_ICCM]       = ccm_mgr_->get_memory_region_base(core_arch.iccm.start_addr);
      
      // get the ICCM size and start address from core_arch.iccm
      //
      uint32 iccm_size  = core_arch.iccm.size/256;
      uint32 iccm_start = ccm_mgr_->get_memory_region_base(core_arch.iccm.start_addr);
      
      uint32 size_field = 0;
      while (iccm_size >>= 1) ++size_field;
      state.auxs[AUX_ICCM_BUILD] = iccm_start | (size_field << 8) | 0x2;
      
    }
    LOG(LOG_DEBUG) << "[ICCM] AUX_ICCM_BUILD register: 0x" << HEX(state.auxs[AUX_ICCM_BUILD]);
  }

  if (core_arch.dccm.is_configured) {
    aux_perms[AUX_DCCM]       |= AUX_ENABLED;
    aux_perms[AUX_DCCM_BUILD] |= AUX_ENABLED;
    state.auxs[AUX_DCCM] = ccm_mgr_->get_memory_region_base(core_arch.dccm.start_addr);

    // get the DCCM size and start address from core_arch.dccm
    //
    uint32 dccm_size  = core_arch.dccm.size/256;
    uint32 dccm_start = ccm_mgr_->get_memory_region_base(core_arch.dccm.start_addr);

    uint32 size_field = 0;
    while (dccm_size >>= 1) ++size_field;      
    state.auxs[AUX_DCCM_BUILD] = /* dccm_start | */ (size_field << 8) | 0x3;
    
    LOG(LOG_DEBUG) << "[DCCM] AUX_DCCM_BUILD register: '0x"
                   << std::hex << std::setw(8) << std::setfill('0')
                   << state.auxs[AUX_DCCM_BUILD] << "'";
  }
  // Enable Actionpoint auxiliary registers if they are configured
  //
  if (sys_arch.isa_opts.num_actionpoints > 0) {
    aux_perms[AUX_AP_BUILD]   |= AUX_ENABLED;
    aux_perms[AUX_AP_WP_PC]   |= AUX_ENABLED;
    state.auxs[AUX_AP_BUILD]  =   0x05
                                | (sys_arch.isa_opts.aps_full ? 0 : 0x400)
                                | ((sys_arch.isa_opts.num_actionpoints/4) << 8);
    
    // Create the Actionpoint auxiliary registers, in the range 0x220 - 0x237
    //
    for (uint32 i = 0; i < sys_arch.isa_opts.num_actionpoints; i++) {
      aux_perms[0x220+(3*i)+0]  = AUX_ENABLED + AUX_K_RW;    // AP_AMVi
      aux_perms[0x220+(3*i)+1]  = AUX_ENABLED + AUX_K_RW;    // AP_AMMi
      aux_perms[0x220+(3*i)+2]  = AUX_ENABLED + AUX_K_RW;    // AP_ACi
      aux_mask [0x220+(3*i)+0]  = 0xffffffff;
      aux_mask [0x220+(3*i)+1]  = 0xffffffff;
      aux_mask [0x220+(3*i)+2]  = 0x3ff;
      state.auxs[0x220+(3*i)+0] = 0;
      state.auxs[0x220+(3*i)+1] = 0;
      state.auxs[0x220+(3*i)+2] = 0;
    }
    // Extend the significant region of the AUX_DEBUG register to
    // include the AH bit (2) and ASR[7:0] field (10:3).
    //
    aux_mask[AUX_DEBUG] |= 0x7fc;
  }
  // Enable the XPU register if any extensions, of any kind, are defined.
  //
  if (eia_mgr.any_eia_extensions_defined) {
    aux_perms[AUX_XPU]    |= AUX_ENABLED;
    aux_perms[AUX_XFLAGS] |= AUX_ENABLED;
  }
  // If div_rem_option is not enabled, remove the mask bit for
  // the DZ bit in AUX_STATUS32 so that it can never be written
  // or read directly.
  //
  if (!sys_arch.isa_opts.div_rem_option) {
    aux_mask[AUX_STATUS32]    = aux_mask[AUX_STATUS32]    & (~0x2000UL);
    aux_mask[AUX_ERSTATUS]    = aux_mask[AUX_ERSTATUS]    & (~0x2000UL);
  }
  // Enable the SmaRT auxiliary registers is SmaRT is properly configured
  //
  if (smt.is_configured() > 0) {
    LOG(LOG_DEBUG1) << "[AUX] enabling SmaRT auxiliary registers";
    aux_perms[AUX_SMART_BUILD]   |= AUX_ENABLED;
    aux_perms[AUX_SMART_CONTROL] |= AUX_ENABLED;
    aux_perms[AUX_SMART_DATA]    |= AUX_ENABLED;
  }


  //MPU configuration
  if( core_arch.mmu_arch.kind == MmuArch::kMpu){
    aux_perms[AUX_EFA]           |= AUX_ENABLED;
    
    aux_perms[AUX_MPU_BUILD]     |= AUX_ENABLED | AUX_K_READ;
    aux_mask[AUX_MPU_BUILD]       = 0x0000FFFF;
    state.auxs[AUX_MPU_BUILD]     = ((core_arch.mmu_arch.mpu_num_regions & 0xFF) << 8) | (core_arch.mmu_arch.version & 0xFF);
    aux_perms[AUX_MPU_EN]        |= AUX_ENABLED | AUX_K_RW;
    aux_mask[AUX_MPU_EN]          = 0x400001F8;
    state.auxs[AUX_MPU_EN]        = 0;

    aux_perms[AUX_MPU_ECR]       |= AUX_ENABLED | AUX_K_READ;
    aux_mask[AUX_MPU_ECR]         = 0xFFFF03FF;
    state.auxs[AUX_MPU_ECR]       = 0x00230000;

    for (uint32 auxr = 0; auxr < core_arch.mmu_arch.mpu_num_regions; ++auxr){
      aux_perms[AUX_MPU_RDB0  + auxr*2] |= AUX_ENABLED | AUX_K_RW;
      aux_mask[AUX_MPU_RDB0   + auxr*2]  = 0xFFFFF801;
      state.auxs[AUX_MPU_RDB0 + auxr*2]  = 0;
      aux_perms[AUX_MPU_RDP0  + auxr*2] |= AUX_ENABLED | AUX_K_RW;
      aux_mask[AUX_MPU_RDP0   + auxr*2]  = 0x00000FFB;
      state.auxs[AUX_MPU_RDP0 + auxr*2]  = 0;
    }
  }
  
  // FP configuration
  if(sys_arch.isa_opts.fpx_option)
  {
      aux_perms[AUX_FP_BUILD]   |= AUX_ENABLED;
      aux_perms[AUX_DPFP_BUILD] |= AUX_ENABLED;
      
      //set the versions to version 2, but don't enable the fast variants
      state.auxs[AUX_FP_BUILD]   = 0x2;
      state.auxs[AUX_DPFP_BUILD] = 0x2;
      
      aux_perms[AUX_FP_STATUS]  |= AUX_ENABLED;
      aux_perms[AUX_DPFP_STATUS]|= AUX_ENABLED;
      
      state.auxs[AUX_FP_STATUS]  = (0x3<<2) | 0x1;
      state.auxs[AUX_DPFP_STATUS]= (0x3<<2) | 0x1;
      
      aux_perms[AUX_DPFP1L]     |= AUX_ENABLED;
      aux_perms[AUX_DPFP1H]     |= AUX_ENABLED;
      aux_perms[AUX_DPFP2L]     |= AUX_ENABLED;
      aux_perms[AUX_DPFP2H]     |= AUX_ENABLED;
      
      state.auxs[AUX_DPFP1L]     = 0;
      state.auxs[AUX_DPFP1H]     = 0;
      state.auxs[AUX_DPFP2L]     = 0;
      state.auxs[AUX_DPFP2H]     = 0; 
  }
}
    
} } } //  arcsim::sys::cpu

