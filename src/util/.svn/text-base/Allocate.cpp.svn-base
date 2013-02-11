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

#include <cstdlib>

#include "util/Allocate.h"

#include "util/Log.h"

namespace arcsim {
  namespace util {
        
    void*
    Malloced::NewAligned (size_t size)
    {
      void* result = valloc(size);
      if (result == 0) {
        LOG(LOG_ERROR) << "Malloced::NewAligned failed";
      }
      return result;
    }
    
    void*
    Malloced::New( size_t size)
    {
      void* result = std::malloc(size);
      if (result == 0) {
        LOG(LOG_ERROR) << "Malloced::New failed";
      }
      return result;      
    }
    
    void
    Malloced::Delete (void* p)
    {
      std::free(p);
    }

    void
    Malloced::DeleteAligned (void* p)
    {
      std::free(p);
    }
    
} } // arcsim::util
