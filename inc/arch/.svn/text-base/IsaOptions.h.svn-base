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

#ifndef INC_ARCH_ISAOPTIONS_H_
#define INC_ARCH_ISAOPTIONS_H_

#include "api/types.h"

// -----------------------------------------------------------------------------
// IsaOptions Class
//
class IsaOptions
{
public:
  // Supported ISAs
  //
  enum IsaKind {
    kIsaUnsupported = 0x0,
    kIsaA600        = 0x1,   // A600
    kIsaA700        = 0x2,   // A700 - DEFAULT
    kIsaA6K         = 0x4,   // ARCompact V2 (Skipjack)
    kIsaA6KV2       = 0x8,   // ARCompact V2.1
  };
  
private:
  // The current instruction kind
  //
  IsaKind isa_;  
  
  // TIMER(0|1) IRQ numbers used for various ARCompact systems. Use respective
  // get_timer0_irq_num()/get_timer1_irq_num() methods to retrieve the right
  // IRQ number based on the specified ISA.
  //
  static const uint32 kTimer0IrqNumberA600    = 3;
  static const uint32 kTimer0IrqNumberA700    = 3;
  static const uint32 kTimer0IrqNumberA6k     = 3;
  static const uint32 kTimer0IrqNumberDefault = 3; // default used if ISA unknown
  
  static const uint32 kTimer1IrqNumberA600    = 7;
  static const uint32 kTimer1IrqNumberA700    = 4;
  static const uint32 kTimer1IrqNumberA6k     = 4;
  static const uint32 kTimer1IrqNumberDefault = 4; // default used if ISA unknown
  
  // Memory error IRQ numbers used for various ARCompact systems. Use respective
  // get_memory_error_irq_num() method to retrieve the right IRQ number based on
  // specified ISA.
  //
  static const uint32 kMemoryErrorIrqNumberA600    = 1;
  static const uint32 kMemoryErrorIrqNumberA700    = 1;
  static const uint32 kMemoryErrorIrqNumberA6k     = 1;
  static const uint32 kMemoryErrorIrqNumberDefault = 1; // default used if ISA unknown
  
public:

  // Setup Exception Vectors based on configured ISA
  // This needs to be public since we can enable the new interrupt system 
  // independently of selecting our ISA, so we need it in IsaOptions. Once 
  // testing is complete we can hide it again (since we would never really
  // configure an A6kv2.1 processor with the old interrupt mechanism.
  void setup_exception_vectors();

  // Multiply options
  //
  bool    mpy16_option;
  bool    mpy32_option;
  bool    mul64_option;
  int     mpy_lat_option;
  bool    mpy_fast;
  
  int     atomic_option;
  uint32  density_option;
  
  bool    shas_option;
  bool    fpx_option;
  bool    sat_option;
  bool    swap_option;
  bool    norm_option;
  bool    shift_option;
  bool    ffs_option;
  bool    new_fmt_14;
  
//A6kv2.1 supports fast divider, so we now have multiple divider options
  uint8    div_rem_option;
  
  bool    only_16_regs;

  bool    has_eia;

  bool    stack_checking;

  uint32  lpc_size;
  uint32  pc_size;
  uint32  addr_size;
  uint32  ifq_size;

  
  // Cache features
  //
  uint32  ic_feature;
  uint32  dc_feature;
  uint32  dc_unc_region;
  bool    dc_uncached_region;
  bool    ic_disable_on_reset;

  bool    has_dmp_peripheral;
  bool    has_dmp_memory;

  // Actionpoints
  //
  uint32  num_actionpoints;
  bool    aps_full;
  
  // Timer options
  //
  bool    has_timer0;
  bool    has_timer1;
  bool    use_host_timer;
  uint32  timer_0_int_level;
  uint32  timer_1_int_level;
  
  bool    rf_4port;
  uint32  intvbase_preset;

  bool    turbo_boost;
  uint32  smart_stack_size;
  uint32  num_interrupts;

  bool    is_ccm_debug_enabled;

  // Verification options
  //
  bool    ignore_brk_sleep;
  bool    disable_stack_setup;

  //A6KV2.1 Options

  uint16 code_protect_bits; // bits encoding code protection on memory segments. 16 segments total

  uint8 num_reg_banks;
  uint8 num_banked_regs;
  bool fast_irq;
  uint8 number_of_levels;
  bool rtc_option;
  bool overload_vectors;
  bool new_interrupts;

  //Exception vectors
  uint8 EV_Reset;
  uint8 EV_MemoryError;
  uint8 EV_InstructionError;
  uint8 EV_MachineCheck;
  uint8 EV_ITLBMiss;
  uint8 EV_DTLBMiss;
  uint8 EV_ProtV;
  uint8 EV_PrivilegeV;
  uint8 EV_SWI;
  uint8 EV_Trap;
  uint8 EV_Extension;
  uint8 EV_DivZero;
  uint8 EV_DCError;
  uint8 EV_Maligned;
  
  
  uint8    PV_CodeProtect;
  uint8    PV_StackCheck;
  uint8    PV_Mpu;
  uint8    PV_Mmu;
  
  
  bool    multiple_iccms;
  static const uint32  kMultipleIccmCount;

  IsaOptions();
  ~IsaOptions();
  
  // ISA getters/setters
  //
  inline IsaKind get_isa() const        { return isa_; }
  inline void    set_isa(IsaKind isa)   {
    isa_ = isa;
    setup_exception_vectors();
    if(is_isa_a6k()) {
      new_fmt_14 = true;
    } else {
      new_fmt_14 = false;
    }
  }

  // Efficient query ISA kind query methods
  //
  inline bool is_isa_a6k()  const { return (isa_ & kIsaA6K) | (isa_ & kIsaA6KV2);  }
  inline bool is_isa_a700() const { return (isa_ & kIsaA700); }
  inline bool is_isa_a600() const { return (isa_ & kIsaA600); }
  
  inline bool is_isa_a6kv1() const{ return (isa_ & kIsaA6K);}
  inline bool is_isa_a6kv2() const{ return (isa_ & kIsaA6KV2);}
  
  inline uint32 get_timer0_irq_num() const {
    switch (isa_) {
      case kIsaA6K:  return kTimer0IrqNumberA6k;
      case kIsaA600: return kTimer0IrqNumberA600;
      case kIsaA700: return kTimer0IrqNumberA700;
      default: return kTimer0IrqNumberDefault;
    }
  }
                                          
  inline uint32 get_timer1_irq_num() const {
    switch (isa_) {
      case kIsaA6K:  return kTimer1IrqNumberA6k;
      case kIsaA600: return kTimer1IrqNumberA600;
      case kIsaA700: return kTimer1IrqNumberA700;
      default: return kTimer1IrqNumberDefault;
    }
  }
  
  inline uint32 get_memory_error_irq_num() const {
    switch (isa_) {
      case kIsaA6K:  return kMemoryErrorIrqNumberA6k;
      case kIsaA600: return kMemoryErrorIrqNumberA600;
      case kIsaA700: return kMemoryErrorIrqNumberA700;
      default: return kMemoryErrorIrqNumberDefault;
    }
  }

};

#endif  // INC_ARCH_ISAOPTIONS_H_
