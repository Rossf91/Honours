//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description: RAM - Random Access Memory class.
//
// =====================================================================

#include <cstring>

#include "sys/mem/RaMemory.h"

Ram::Ram (uint32 base, uint32 size, bool init_with_0)
  : Rom (base, size, init_with_0)
{ /* EMPTY */ }

bool
Ram::write_block (uint32 addr, uint32 size, uint8 const * buf)
{ // check that address and size is within bounds
  if (   (addr >= mem_base  )
      && (addr <= mem_limit8)
      && (size < (mem_limit8 - mem_base)))
  {
    std::memcpy(&data8[addr - mem_base], buf, size);
    return true;
  } else {
    return false;
  }
}
