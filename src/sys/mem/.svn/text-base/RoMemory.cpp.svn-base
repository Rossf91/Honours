//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:  ROM - Read Only Memory class.
//
// =====================================================================

#include <cstring>

#include "Assertion.h"

#include "sys/mem/RoMemory.h"


Rom::Rom (uint32 base, uint32 size, bool init_with_0)
: mem_words(size >> 2),            // number 32-bit long words
  mem_base(base),                  // long word address
  mem_limit8(base + size - 1),     // byte address (data8)
  mem_limit16(base + size - 2),    // word address (data16)
  mem_limit32(base + size - 4)     // long word address (data32)
{
  data32 = new uint32 [mem_words];
  ASSERT(data32 != 0 && "[ROM] Memory allocation failed!");
  data16 = (uint16 *)data32;
  data8  = (uint8  *)data32;
  // initialise with zero if requested
  if (init_with_0) {
    std::memset(data32, 0, sizeof(data32[0]) * mem_words);
  }
}

Rom::~Rom ()
{
  delete [] data32;
}

bool
Rom::read_block (uint32 addr, uint32 size, uint8 * buf)
{ // check that address and size is within bounds
  if (   (addr >= mem_base  )
      && (addr <= mem_limit8)
      && (size < (mem_limit8 - mem_base)))
  {
    std::memcpy(buf, &data8[addr - mem_base], size);
    return true;
  } else {
    return false;
  }
}
