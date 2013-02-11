//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
// LatencyCache to avoid re-calculation of fetch/read/write memory ops.
//
// =====================================================================


#ifndef INC_UARCH_MEMORY_LATENCYCACHE_H_
#define INC_UARCH_MEMORY_LATENCYCACHE_H_

#include "api/types.h"

// -----------------------------------------------------------------------
// Forward declare cpuState structure
//
struct cpuState;

namespace arcsim {
  namespace uarch {
    namespace memory {

      
      // -----------------------------------------------------------------------
      // LatencyCache
      //
      class LatencyCache {
      private:
        // pointer to cpu state structure
        //
        cpuState* state;
        
        LatencyCache(const LatencyCache & m);        // DO NOT COPY
        void operator=(const LatencyCache &);        // DO NOT ASSIGN
        
      public:
        
        uint32  i_block_bits;
        uint32  d_block_bits;
        
        // Constructor
        //
        LatencyCache() : state(0), i_block_bits(0), d_block_bits(0)
        { /* EMPTY */ }
        
        // Destructor
        //
        ~LatencyCache()
        { /* EMPTY */ }
        
        // Init    -------------------------------------------------------------
        //
        void init(cpuState* _state);
        
        // Flush   -------------------------------------------------------------
        //
        void flush();

        // Update  -------------------------------------------------------------
        //
        void purge_fetch_entry(uint32 addr);
        void purge_read_entry (uint32 addr);
        void purge_write_entry(uint32 addr);
        
        // Update  -------------------------------------------------------------
        //
        void update_fetch_entry(uint32 addr);
        void update_read_entry (uint32 addr);
        void update_write_entry(uint32 addr);

        // Lookup  -------------------------------------------------------------
        //
        bool is_fetch_latency_hit(uint32 addr, uint16* cycles);
        bool is_read_latency_hit (uint32 addr, uint16* cycles);
        bool is_write_latency_hit(uint32 addr, uint16* cycles);

        // Query  -------------------------------------------------------------
        //
        uint64 get_fetch_hits()   const;
        uint64 get_read_hits()    const;
        uint64 get_write_hits()   const;
        
      };
      
} } } //  arcsim::uarch::memory


#endif  // INC_UARCH_MEMORY_LATENCYCACHE_H_
