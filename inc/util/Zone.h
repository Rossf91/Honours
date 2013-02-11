//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Zones support very fast allocation of small chunks of memory. Objects
// allocated within a Zone can not be deallocated individually. Instead all
// objects allocated in a Zone can be freed with one operation.
//
// One can think of a Zone as an independent heap. Objects allocated from
// the same zone should live on the same set of virtual memory pages and 
// thus if they are used near in time potentially cause less paging. 
// Likewise, if they are to be deallocated at the same time there is less
// fragmentation (and thus less paging).
//
// We mainly use Zones to hold tracing related data-structures such as BlockEntry
// and PageProfile instances.
//
// NOTES: Zones have been introduced for performance reasons, mainly to enable
// very fast allocations of objects. They are not thread-safe, hence they should
// only be modified by one and the same thread only!
//
// For more information about Zone based allocation see:
// @see: http://www.cocoadev.com/index.pl?NSZone
//
// =====================================================================

#ifndef INC_UTIL_ZONE_H_
#define INC_UTIL_ZONE_H_

#include <cstdlib>

#include "Globals.h"

class System;

namespace arcsim {  
  namespace sys {
    namespace cpu {
      class Processor;
    }
  }
  
  namespace util {
    // Forward declaration
    //
    class Chunk;
    
    // ------------------------------------------------------------------------
    // Zone - custom allocator supporting fast chunk wise allocation
    // NOTE: Zones are NOT thread-safe for performance reasons.
    //
    class Zone {
    public:
      // returns aligned pointer to memory of size
      void* New(int size);
      // zaps whole Zone
      void Delete();

    private:
      // limit users of Zone to Processor and System class for now
      friend class arcsim::sys::cpu::Processor;
      friend class System;
      
      // Only friend classes such as processor and possibly system may instantiate
      // a Zone.
      //
      Zone();

      // create new chunk at front of chain of chunks
      Chunk*  NewChunk(int size);
      Address NewExpand(int size);

      // constants
      static const int kAlignment = arcsim::kPointerSize;
      static const int kMinimumChunkSize = 8 * arcsim::KB;      
      static const int kMaximumChunkSize = 1 * arcsim::MB;

      // Range [pos_,end_) denotes free region in chunk
      Address pos_;
      Address end_;
      // Pointer to head of chunks
      Chunk* head_;
    };
    
    // ------------------------------------------------------------------------
    // ZoneObject - base class for objects that can be allocated in a zone
    //              using placement new
    //
    class ZoneObject {
    public:
      // provide operator that will allocate in a Zone
      inline void* operator new(size_t size, Zone* zone) {
        return zone->New(static_cast<int>(size));
      }
      // NOTE: objects that were zone allocated should not be deleted using 'delete'
    };

    
} } // namespace arcsim::util

#endif  // INC_UTIL_ZONE_H_
