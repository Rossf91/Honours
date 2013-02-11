//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Main memory model class
//
// =====================================================================


#ifndef _INC_UARCH_MEMORY_EXTERNALMEMORYMODEL_H_
#define _INC_UARCH_MEMORY_EXTERNALMEMORYMODEL_H_

#include "api/types.h"

#include "uarch/memory/LatencyUtil.h"


// Forward declaration
//
class SystemArch;

class MainMemoryModel
{
private:
  uint64      reads;
  uint64      writes;

  uint16     read_lat [MAX_BLK_BITS+1];
  uint16     write_lat[MAX_BLK_BITS+1];

public:
  
  // Constructor/Destructor
  //
  MainMemoryModel(SystemArch& sys_arch);  
  ~MainMemoryModel ();
  
  void clear();
  
  // ---------------------------------------------------------------------------
  // Efficient inlined memory latency methods
  //  
  inline uint16 get_read_latency(uint8 blk_bits) const
  {
    return read_lat[blk_bits];
  }

  inline uint16 get_write_latency(uint8 blk_bits) const
  {
    return write_lat[blk_bits];
  }
  
  inline uint16 read (uint8 blk_bits)
  {
    ++reads;
    return read_lat[blk_bits];
  }
  
  inline uint16 write (uint8 blk_bits) 
  { 
    ++writes;
    return write_lat[blk_bits];
  }
  
  void print_stats ();  
};



#endif  // _INC_UARCH_MEMORY_EXTERNALMEMORYMODEL_H_
