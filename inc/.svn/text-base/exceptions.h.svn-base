//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2004 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
//
//  This file defines the constants required to describe exceptions in
//  the ARC700 exception model.
//
// =====================================================================

#ifndef _exceptions_h_
#define _exceptions_h_

// Interrupt vector location
//
#define IRQ_VEC(_n_)      (0x18+(((_n_)-3)<<3))

// Exception Cause Register Values
//
#define ECR(_vec, _cause, _param) ((_vec)<<16 | (_cause)<<8 | (_param))
#define ECR_VECTOR(_ecr) (((_ecr)>>16) & 0xff)
#define ECR_CAUSE(_ecr) (((_ecr)>>8) & 0xff)
#define ECR_PARAM(_ecr) ((_ecr)& 0xff)
#define ECR_OFFSET(_ecr) (ECR_VECTOR(_ecr) << 3)

// Vector names/numbers have been moved to IsaOption member variables

// Exception cause codes
//
#define OverlappingTLBs       0x01
#define FatalTLBerror         0x02
#define FatalCacheError       0x03
#define KernelDataMemError    0x04
#define DCflushMemError       0x05
#define IfetchDoubleFault     0x00
#define IfetchMemoryError     0x06
#define IfetchTLBMiss         0x00
#define IfetchProtV           0x00
#define IllegalInstruction    0x00
#define IllegalSequence       0x01
#define PrivilegeViolation    0x00
#define DisabledExtension     0x01
#define MisalignedAccess      0x04
#define LoadTLBfault          0x01
#define StoreTLBfault         0x02
#define ExTLBfault            0x03
#define Trap                  0x00
#define ActionPointHit        0x02

// Create 'status32' representation that can be stored in AUX_STATUS32 register
// FIXME: @igor - replace this MACRO with a function call
#define BUILD_STATUS32(_state_) \
 (((_state_).ES << 15) | \
  ((_state_).SC << 14) | \
  ((_state_).DZ << 13) | \
  ((_state_).L  << 12) | \
  ((_state_).Z  << 11) | \
  ((_state_).N  << 10) | \
  ((_state_).C  <<  9) | \
  ((_state_).V  <<  8) | \
  ((_state_).U  <<  7) | \
  ((_state_).D  <<  6) | \
  ((_state_).AE <<  5) | \
  ((_state_).A2 <<  4) | \
  ((_state_).A1 <<  3) | \
  ((_state_).E2 <<  2) | \
  ((_state_).E1 <<  1) | \
   (_state_).H)

#define BUILD_STATUS32_A6KV21(_state_)\
 (((_state_).IE << 31) | \
  ((_state_).RB << 16) | \
  ((_state_).ES << 15) | \
  ((_state_).SC << 14) | \
  ((_state_).DZ << 13) | \
  ((_state_).L  << 12) | \
  ((_state_).Z  << 11) | \
  ((_state_).N  << 10) | \
  ((_state_).C  <<  9) | \
  ((_state_).V  <<  8) | \
  ((_state_).U  <<  7) | \
  ((_state_).D  <<  6) | \
  ((_state_).AE <<  5) | \
  ((_state_).E  <<  1) | \
   (_state_).H)

// Assign AUX_STATUS32 register contents to processor state
//
#define EXPLODE_STATUS32(_state_,_status32_) \
  do { \
    (_state_).ES = ((_status32_) >> 15) & 1UL; \
    (_state_).SC = ((_status32_) >> 14) & 1UL; \
    (_state_).DZ = ((_status32_) >> 13) & 1UL; \
    (_state_).L  = ((_status32_) >> 12) & 1UL; \
    (_state_).Z  = ((_status32_) >> 11) & 1UL; \
    (_state_).N  = ((_status32_) >> 10) & 1UL; \
    (_state_).C  = ((_status32_) >>  9) & 1UL; \
    (_state_).V  = ((_status32_) >>  8) & 1UL; \
    (_state_).U  = ((_status32_) >>  7) & 1UL; \
    (_state_).D  = ((_status32_) >>  6) & 1UL; \
    (_state_).AE = ((_status32_) >>  5) & 1UL; \
    (_state_).A2 = ((_status32_) >>  4) & 1UL; \
    (_state_).A1 = ((_status32_) >>  3) & 1UL; \
    (_state_).E2 = ((_status32_) >>  2) & 1UL; \
    (_state_).E1 = ((_status32_) >>  1) & 1UL; \
    (_state_).H  = ((_status32_) >>  0) & 1UL; \
  } while (0)

#define EXPLODE_STATUS32_A6KV21(_state_,_aux_status32_) \
  do { \
    (_state_).IE = ((_aux_status32_) >> 31) & 1UL; \
    (_state_).RB = ((_aux_status32_) >> 16) & 7UL; \
    (_state_).ES = ((_aux_status32_) >> 15) & 1UL; \
    (_state_).SC = ((_aux_status32_) >> 14) & 1UL; \
    (_state_).DZ = ((_aux_status32_) >> 13) & 1UL; \
    (_state_).L  = ((_aux_status32_) >> 12) & 1UL; \
    (_state_).Z  = ((_aux_status32_) >> 11) & 1UL; \
    (_state_).N  = ((_aux_status32_) >> 10) & 1UL; \
    (_state_).C  = ((_aux_status32_) >>  9) & 1UL; \
    (_state_).V  = ((_aux_status32_) >>  8) & 1UL; \
    (_state_).U  = ((_aux_status32_) >>  7) & 1UL; \
    (_state_).D  = ((_aux_status32_) >>  6) & 1UL; \
    (_state_).AE = ((_aux_status32_) >>  5) & 1UL; \
    (_state_).E  = ((_aux_status32_) >>  1) &15UL; \
    (_state_).H  = ((_aux_status32_) >>  0) & 1UL; \
  } while (0)


#define BUILD_XFLAGS(_state_) \
 (((_state_).X3 << 3) | \
  ((_state_).X2 << 2) | \
  ((_state_).X1 << 1) | \
   (_state_).X0)

#define EXPLODE_XFLAGS(_state_) \
  do { \
    (_state_).X3 = ((_state_).auxs[AUX_XFLAGS] >> 3) & 1UL; \
    (_state_).X2 = ((_state_).auxs[AUX_XFLAGS] >> 2) & 1UL; \
    (_state_).X1 = ((_state_).auxs[AUX_XFLAGS] >> 1) & 1UL; \
    (_state_).X0 = ((_state_).auxs[AUX_XFLAGS] >> 0) & 1UL; \
  } while (0)


#endif
