//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class encapsulating various ISA options.
//
// =====================================================================

#include "define.h"

#include "arch/IsaOptions.h"

const uint32 IsaOptions::kMultipleIccmCount = 4;

// -----------------------------------------------------------------------------
// IsaOptions class
//
IsaOptions::IsaOptions()
: isa_(kIsaA700),

  mpy16_option(DEFAULT_MPY16_OPTION),
  mpy32_option(DEFAULT_MPY32_OPTION),
  mul64_option(DEFAULT_MUL64_OPTION),
  atomic_option(DEFAULT_ATOMIC_OPTION),
  density_option(DEFAULT_DENSITY_OPTION),
  shas_option(DEFAULT_SHAS_OPTION),
  fpx_option(DEFAULT_FPX_OPTION),
  sat_option(DEFAULT_SAT_OPTION),
  swap_option(DEFAULT_SWAP_OPTION),
  norm_option(DEFAULT_NORM_OPTION),
  shift_option(DEFAULT_SHIFT_OPTION),

  ffs_option(DEFAULT_FFS_OPTION),
  new_fmt_14(DEFAULT_FMT_14),
  has_eia(DEFAULT_HAS_EIA),
  div_rem_option(DEFAULT_DIV_REM_OPTION),
  only_16_regs(DEFAULT_ONLY_16_REGS),
  lpc_size(DEFAULT_LP_SIZE),
  pc_size(DEFAULT_PC_SIZE),

  mpy_lat_option(DEFAULT_MPY_LAT_OPTION),
  addr_size(DEFAULT_ADDR_SIZE),
  ic_feature(DEFAULT_IC_FEATURE),
  dc_feature(DEFAULT_DC_FEATURE),
  dc_unc_region(DEFAULT_DC_UNC_REGION),
  has_dmp_memory(DEFAULT_HAS_DMP_MEMORY),
  dc_uncached_region(DEFAULT_DC_UNCACHED_REGION),
  num_actionpoints(DEFAULT_NUM_ACTIONPOINTS),
  aps_full(DEFAULT_APS_FULL),
  has_timer0(DEFAULT_HAS_TIMER0),
  has_timer1(DEFAULT_HAS_TIMER1),
  use_host_timer(DEFAULT_USE_HOST_TIMER),
  rf_4port(DEFAULT_RF_4PORT),
  mpy_fast(DEFAULT_MPY_FAST),
  has_dmp_peripheral(DEFAULT_HAS_DMP_PER),

  turbo_boost(DEFAULT_TURBO_BOOST),
  smart_stack_size(DEFAULT_SMART_STACK_SIZE),
  intvbase_preset(DEFAULT_INTVBASE_PRESET),
  num_interrupts(DEFAULT_NUM_INTERRUPTS),
  ic_disable_on_reset(DEFAULT_IC_DISABLE_ON_RESET),
  timer_0_int_level(DEFAULT_TIMER_0_INT_LEVEL),
  timer_1_int_level(DEFAULT_TIMER_1_INT_LEVEL),
  ifq_size(DEFAULT_IFQ_SIZE),
  is_ccm_debug_enabled(DEFAULT_CCM_DEBUG_ENABLED),

  // Verification Options
  //
  ignore_brk_sleep(DEFAULT_IGNORE_BRK_SLEEP),
  disable_stack_setup(DEFAULT_DISABLE_STACK_SETUP),

  //A6KV2.1 Options

  //Code Protection flag bitfield for the 16 memory regions
  code_protect_bits(0),

  multiple_iccms(DEFAULT_ENABLE_MULTIPLE_ICCMS),

  //Enable stack checking in ARCv2.1 (ARC700 TODO)
  stack_checking(DEFAULT_STACK_CHECKING),

  //TODO: Move these numbers into define.h
  number_of_levels(15),

  new_interrupts(0),

  num_banked_regs(0),
  num_reg_banks(1)

{  
  setup_exception_vectors(); // configure default EVs 
}

IsaOptions::~IsaOptions()
{ /* EMPTY */ }

void IsaOptions::setup_exception_vectors()
{
  if(new_interrupts)
  {
    EV_Reset = 0;
    EV_MemoryError = 1;
    EV_InstructionError = 2;
    EV_MachineCheck = 3;
    EV_ITLBMiss = 4;
    EV_DTLBMiss = 5;
    EV_ProtV = 6;
    EV_PrivilegeV = 7;
    EV_SWI = 8;
    EV_Trap = 9;
    EV_Extension = 10;
    EV_DivZero = 11;
    EV_DCError = 12;
    EV_Maligned = 13;
    
  }
  else
  {
    EV_Reset = 0;
    EV_MemoryError = 1;
    EV_InstructionError = 2;
    EV_MachineCheck = 32;
    EV_ITLBMiss = 33;
    EV_DTLBMiss = 34;
    EV_ProtV = 35;
    EV_PrivilegeV = 36;
    if(is_isa_a6k())
    {
      EV_SWI = 0x25;
      EV_Trap = 0x26;
      EV_Extension = 0x27;
      
      EV_DivZero = 40;
      EV_DCError = 41;
      EV_Maligned = 42;
    }
    else
    {
      EV_Trap = 0x25;
      EV_Extension = 0x26;
    }
  }
  
  if(is_isa_a6kv2()){
    PV_CodeProtect = 1;
    PV_StackCheck  = 2;
    PV_Mpu         = 4;
    PV_Mmu         = 8;
  }else{
    PV_CodeProtect = 0;
    PV_StackCheck  = 0;
    PV_Mpu         = 0;
    PV_Mmu         = 0;
  }
  
}
