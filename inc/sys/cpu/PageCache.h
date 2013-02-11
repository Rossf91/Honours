//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
// Class for lookup and manipulation of page cache structures.
//
// =====================================================================

#ifndef INC_SYSTEM_CPU_PAGECACHE_H_
#define INC_SYSTEM_CPU_PAGECACHE_H_

// -----------------------------------------------------------------------
// HEADERS
// 

#include "api/types.h"

// -----------------------------------------------------------------------
// Forward declare cpuState and PageArch
//
struct cpuState;
class  PageArch;

namespace arcsim {
  namespace sys {
    namespace cpu {

      // -----------------------------------------------------------------------
      // PageCache
      //
      class PageCache {
      private:
        // pointer to cpu state structure
        //
        cpuState*       state_;
        
        // pointer to page archicture
        //
        const PageArch* page_arch_;
                        
        PageCache(const PageCache & m);           // DO NOT COPY
        void operator=(const PageCache &);        // DO NOT ASSIGN
        
      public:
        
        // Read/Write/Exec cache kind
        //
        enum Kind { READ, WRITE, EXEC, ALL };
        
        // Constructor
        //
        PageCache();
        
        // Destructor
        //
        ~PageCache();
        
        // ---------------------------------------------------------------------
        // Init
        //
        void init(cpuState* state, PageArch* page_arch);
        
        // ---------------------------------------------------------------------
        // Destroy
        //
        void destroy();
        
        // ---------------------------------------------------------------------
        // Flush
        //
        void flush(Kind page_kind);
        
        // ---------------------------------------------------------------------
        // Purge
        //
        uint32 purge(Kind kind, uint32 addr);
        uint32 purge_entry(Kind kind, uint32 const * const entry);
  
};

} } } //  arcsim::sys::cpu

#endif  // INC_SYSTEM_CPU_PAGECACHE_H_
