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


#ifndef INC_MEMORY_RAMEMORY_H_
#define INC_MEMORY_RAMEMORY_H_

#include "api/types.h"
#include "sys/mem/RoMemory.h"

class Ram : public Rom
{
public:
  // Constructor
  //
	explicit Ram (uint32 base, uint32 size, bool init_with_0 = false);
	
  // Write block of data
  //
  bool write_block (uint32 addr, uint32 size, uint8 const * buf);
  
  // ---------------------------------------------------------------------------
  // Inlined access methods  
  //
	inline bool write8 (uint32 addr, uint32 data)
  {
		if ((addr >= mem_base) && (addr <= mem_limit8)) {
			data8[addr - mem_base] = (uint8)data;
			return true;
		} else {
			return false;
    }
	}
	
	inline bool write16 (uint32 addr, uint32 data)
  {
		if (!(addr & 1UL) && (addr >= mem_base) && (addr <= mem_limit16)) {
      data16[(addr - mem_base) >> 1] = (uint16)data;
      return true;
		} else {
			return false;
    }
	}
  
	inline bool write32 (uint32 addr, uint32 data)
  {
		if (!(addr & 3UL) && (addr >= mem_base) && (addr <= mem_limit32)) {
      data32[(addr - mem_base) >> 2] = data;
      return true;
		} else {
			return false;
    }
	}
  
};

#endif  // INC_MEMORY_RAMEMORY_H_
