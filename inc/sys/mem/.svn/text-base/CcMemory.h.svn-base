//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  CCM - Closely Coupled Memory class.
//
// =====================================================================

#ifndef INC_MEMORY_CCMEMORY_H_
#define INC_MEMORY_CCMEMORY_H_

// =====================================================================
// HEADERS
// =====================================================================

#include "api/types.h"

#include "sys/mem/RaMemory.h"

// =====================================================================
// CLASS
// =====================================================================

class CCM : public Ram {
public:
  // Constructors
  //
	explicit CCM (uint32 base, uint32 size, bool init_with_0 = false);
  
	// Fetch instruction word/s
	//
	inline bool fetch32 (uint32 addr, uint32 &data)
  {
		uint32 tmp_data = 0;
		bool ok1 = read16 (addr, data);
		bool ok2 = read16 (addr+2, tmp_data);
		data =  (data << 16) | (tmp_data & 0x0000ffff);
		return (ok1 && ok2);
	}

};


#endif  // INC_MEMORY_CCMEMORY_H_
