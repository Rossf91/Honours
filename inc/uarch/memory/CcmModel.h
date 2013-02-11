//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// CCM timing class
//
// =====================================================================

#ifndef _INC_UARCH_MEMORY_CCMMODEL_H_
#define _INC_UARCH_MEMORY_CCMMODEL_H_

#include "api/types.h"

#include "arch/SpadArch.h"
#include "uarch/memory/LatencyUtil.h"

class CCMModel {
private:
  SpadArch::SpadKind  kind;
  uint16              read_lat[MAX_BLK_BITS+1];
  uint16              write_lat[MAX_BLK_BITS+1];
  uint32              ccm_low;
  uint32              ccm_high;   
  uint64              reads;
  uint64              writes;

public:
  CCMModel  (SpadArch::SpadKind kind,
             int                start_addr,
             int                size,
             int                bus_width,
             int                latency);  
  ~CCMModel ();
  
  uint32 get_size ();
  
  void print_stats ();
    
  // ---------------------------------------------------------------------------
  // Inline methods
  //
  inline uint16 read (uint32 addr, uint8 blk_bits)
  { 
    if ((addr >= ccm_low) && (addr <= ccm_high)) {
      ++reads;
      return read_lat[blk_bits];
    }
    return 0;
  }
  
  inline uint16 write (uint32 addr, uint8 blk_bits) 
  { 
    if ((addr >= ccm_low) && (addr <= ccm_high)) {
      ++writes;
      return write_lat[blk_bits];
    }
    return 0;
  }    
};


#endif  // _INC_UARCH_MEMORY_CCMMODEL_H_
