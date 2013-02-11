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

#include "uarch/memory/LatencyCache.h"

#include "sys/cpu/state.h"

namespace arcsim {
  namespace uarch {
    namespace memory {
      
      // -----------------------------------------------------------------------
      // CONSTANTS
      //
      
      static const uint32 kDefaultTag = 0xFFFFFFFF;
      static const uint16 kDefaultVal = 1;
      
      // Init    ---------------------------------------------------------------
      //
      void
      LatencyCache::init(cpuState* _state)
      {
        state = _state;
        
        // Initiliase tags and values
        //
        flush();
      }
      
      // Flush   ---------------------------------------------------------------
      //
      void
      LatencyCache::flush()
      {
        for (int i = 0; i < DEFAULT_HASH_CACHE_SIZE; ++i) {
          state->lat_cache_d_tag[i]  = kDefaultTag;
          state->lat_cache_i_tag[i]  = kDefaultTag;
        }
        state->lat_cache_fetch_val  = kDefaultVal;
        state->lat_cache_read_val   = kDefaultVal;
        state->lat_cache_write_val  = kDefaultVal;

        // Initialise/Reset counter
        //
        state->lat_cache_fetch_hit = 0;
        state->lat_cache_read_hit  = 0;
        state->lat_cache_write_hit = 0;
      }

      // Query  -------------------------------------------------------------
      //
      uint64
      LatencyCache::get_fetch_hits()   const
      {
        return state->lat_cache_fetch_hit;
      }
      
      uint64
      LatencyCache::get_read_hits() const
      {
        return  state->lat_cache_read_hit;
      }
      
      uint64
      LatencyCache::get_write_hits() const
      {
        return state->lat_cache_write_hit;
      }

      
      // Purge  ---------------------------------------------------------------
      //
      
      void
      LatencyCache::purge_fetch_entry(uint32 addr)
      {
        uint32 blk_addr = addr >> i_block_bits;
        uint32 index = blk_addr & (DEFAULT_HASH_CACHE_SIZE-1);
        state->lat_cache_i_tag[index] = kDefaultTag;
      }
      
      void
      LatencyCache::purge_read_entry (uint32 addr)
      {
        uint32 blk_addr = addr >> d_block_bits;
        uint32 index = blk_addr & (DEFAULT_HASH_CACHE_SIZE-1);
        state->lat_cache_d_tag[index] = kDefaultTag;
      }
      
      void
      LatencyCache::purge_write_entry(uint32 addr)
      {
        uint32 blk_addr = addr >> d_block_bits;
        uint32 index = blk_addr & (DEFAULT_HASH_CACHE_SIZE-1);
        state->lat_cache_d_tag[index] = kDefaultTag;
      }
      

      
      // Update  ---------------------------------------------------------------
      //

      void
      LatencyCache::update_fetch_entry(uint32 addr)
      {
        uint32 blk_addr = addr >> i_block_bits;
        uint32 index = blk_addr & (DEFAULT_HASH_CACHE_SIZE-1);
        state->lat_cache_i_tag[index] = blk_addr;
      }

      void
      LatencyCache::update_read_entry (uint32 addr)
      {
        uint32 blk_addr = addr >> d_block_bits;
        uint32 index = blk_addr & (DEFAULT_HASH_CACHE_SIZE-1);
        state->lat_cache_d_tag[index] = blk_addr;
      }
      
      void
      LatencyCache::update_write_entry(uint32 addr)
      {
        uint32 blk_addr = addr >> d_block_bits;
        uint32 index = blk_addr & (DEFAULT_HASH_CACHE_SIZE-1);
        state->lat_cache_d_tag[index] = blk_addr | 0x80000000;
      }

      
      // Lookup  ---------------------------------------------------------------
      //
      bool
      LatencyCache::is_fetch_latency_hit (uint32 addr, uint16* cycles)
      {
        uint32 blk_addr = addr >> i_block_bits;
        uint32 index = blk_addr & (DEFAULT_HASH_CACHE_SIZE-1);
        if (blk_addr == (state->lat_cache_i_tag[index] & 0x7FFFFFFF)) {
          ++state->lat_cache_fetch_hit;
          *cycles = state->lat_cache_fetch_val;
          return true;
        } else
          return false;
      }
      
      bool
      LatencyCache::is_read_latency_hit (uint32 addr, uint16* cycles)
      {
        uint32 blk_addr = addr >> d_block_bits;
        uint32 index = blk_addr & (DEFAULT_HASH_CACHE_SIZE-1);
        if (blk_addr == (state->lat_cache_d_tag[index] & 0x7FFFFFFF)) {
          ++state->lat_cache_read_hit;
          *cycles = state->lat_cache_read_val;
          return true;
        } else
          return false;
      }
      
      bool
      LatencyCache::is_write_latency_hit (uint32 addr, uint16* cycles)
      {
        uint32 blk_addr = addr >> d_block_bits;
        uint32 index = blk_addr & (DEFAULT_HASH_CACHE_SIZE-1);
        if ((blk_addr | 0x80000000) == state->lat_cache_d_tag[index]) {  // dirty hit
          ++state->lat_cache_write_hit;
          *cycles = state->lat_cache_write_val;
          return true;
        } else
          return false;
      }                                                 
      
} } } //  arcsim::uarch::memory
