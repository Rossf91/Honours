//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description: ROM - Read Only Memory class.
//
// =====================================================================

#ifndef INC_MEMORY_ROMEMORY_H_
#define INC_MEMORY_ROMEMORY_H_

#include "api/types.h"

class Rom
{
public:
  uint32 *data32;
	uint16 *data16;
	uint8  *data8;
	const uint32 mem_base;
	const uint32 mem_words;
	const uint32 mem_limit8;
	const uint32 mem_limit16;
	const uint32 mem_limit32;

  // Constructor/Destructor
  //
	explicit Rom (uint32 base, uint32 size, bool init_with_0 = false);
	virtual ~Rom ();

  // Read block of data
  //
  bool read_block (uint32 addr, uint32 size, uint8 * buf);
  
	// ---------------------------------------------------------------------------
  // Inlined access methods  
  //
	inline bool read8 (uint32 addr, uint32 &data)
  {
		if ((addr >= mem_base) && (addr <= mem_limit8)) {
			data = data8[addr - mem_base];
			return true;
		} else {
			return false;
    }
	}
	
	inline bool read16 (uint32 addr, uint32 &data)
  {
		if (!(addr & 1UL) && (addr >= mem_base) && (addr <= mem_limit16)) {
      data = data16[(addr - mem_base) >> 1];
      return true;
		} else {
			return false;
    }
	}
  
	inline bool read32 (uint32 addr, uint32 &data)
  {
		if (!(addr & 3UL) && (addr >= mem_base) && (addr <= mem_limit32)) {
      data = data32[(addr - mem_base) >> 2];
      return true;
		} else {
			return false;
    }
	}
  
	inline bool load32 (uint32 addr, uint32 data)
  {  
		if (!(addr & 3UL) && (addr >= mem_base) && (addr <= mem_limit32)) {
			data32[(addr - mem_base) >> 2] = data;
			return true;
		} else {
			return false;
    }
	}
};

#endif  // INC_MEMORY_ROMEMORY_H_

