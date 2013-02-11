//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Various allocator classes that should be used to allocate and free memory.
//
// =============================================================================

#ifndef INC_UTIL_ALLOCATE_H_
#define INC_UTIL_ALLOCATE_H_

#include <cstddef>

namespace arcsim {
  namespace util {
    
    // Classes that are managed with 'new' and 'delete' can use this class
    // as a super class. We should also never use std::malloc in our code,
    // but rather the static New, NewAligned, and Delete methods.
    //
    class Malloced
    {
    public:
      void* operator new(size_t size) { return New(size); }
      void  operator delete(void* p)  { Delete(p);        }
      
      static void*  NewAligned    (size_t size);
      static void*  New           (size_t size);
      static void   Delete        (void* p);
      static void   DeleteAligned (void* p);
    };

    
} } // arcsim::util

#endif  // INC_UTIL_ALLOCATE_H_
