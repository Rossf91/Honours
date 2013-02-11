//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2004 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  This file defines basic non-class typedefs that are needed by the
//  simulator.
//
// =====================================================================

#ifndef INC_API_APITYPES_H_
#define INC_API_APITYPES_H_

#ifdef __cplusplus
extern "C" {
#endif
  
  // -----------------------------------------------------------------------------
  // STRUCTURES/ENUMS
  //
  
  // Baseline register file contains two 32-bit write ports
  //
# define NUM_RF_WRITE_PORTS   2
  
  typedef struct {
    uint32    a;
    uint32    w;
  } WritePort;
  
  // In the update packet of one instruction, each write to RF/Mem/Aux
  // is validated by one bit in a write mask in the order set out below.
  //
  typedef enum {
    UPKT_WMASK_AUX        = 0x001U,
    UPKT_WMASK_ST         = 0x002U,
    UPKT_WMASK_RF0        = 0x004U,
    UPKT_WMASK_RF1        = 0x008U,
    UPKT_WMASK_RF_BASE    = 0x004U,
    UPKT_WMASK_RF_OFFSET  = 2,
    UPKT_WMASK_RF_WIDTH   = 2,
    UPKT_WMASK_RF_FULL    = UPKT_WMASK_RF0 | UPKT_WMASK_RF1
  } UpdPktConstType;
  
  
  // The update info of any write to memory is kept in a store packet structure
  //
  typedef struct {
    uint32    pc;     // PC of the store instruction
    uint32    addr;   // Memory address for the write
    uint32    mask;   // 32-bit write mask for the written byte (bit) position
    uint32    data[4];// Data word(s) to be written
  } StorePacket;
  
  typedef struct UpdatePacket UpdatePacket;
  struct UpdatePacket {
    uint32      pc;                     // PC of the instruction
    uint32      inst;                   // Instruction word
    uint32      limm;                   // Instruction limm
    uint32      status32;               // The new STATUS32 content
    WritePort   rf[NUM_RF_WRITE_PORTS]; // Write content for each regfile port
    WritePort   aux;                    // Auxiliary space write
    uint32      wmask;                  // Write mask to validate each write
    StorePacket store;                  // Store content if a ST/PUSH_S
    UpdatePacket* next_uop;             // Next micro-op instruction's u-packet
#ifdef __cplusplus
    UpdatePacket()
    : pc(0), inst(0), limm(0), status32(0), wmask(0), next_uop(0)
    { /* EMPTY */ }
    UpdatePacket(uint32 _pc, uint32 _inst, uint32 _limm, uint32 _status32)
    : pc(_pc), inst(_inst), limm(_limm), status32(_status32), wmask(0), next_uop(0)
    { /* EMPTY */ }
    ~UpdatePacket()
    {
      if (next_uop) { delete next_uop; }
    }
#endif
  };

#ifdef __cplusplus
}
#endif

#endif /*  INC_API_APITYPES_H_ */

