//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Cache timing class (inclusive, write-back)
//
// =====================================================================

#ifndef _INC_UARCH_MEMORY_CACHEMODEL_H_
#define _INC_UARCH_MEMORY_CACHEMODEL_H_

#include "util/Counter.h"
#include "api/types.h"

#include "arch/CacheArch.h"
#include "uarch/memory/LatencyUtil.h"

#include "util/Histogram.h"


// ---------------------------------------------------------------------------
// Macro definitions
//
#define VALID_BIT   1UL
#define LOCK_BIT    2UL
#define DIRTY_BIT   4UL
#define DROWSY_BIT 8UL
#define windowSize 200

// ---------------------------------------------------------------------------
// Forward decls.
//
class MainMemoryModel;
class MemoryModel;

// ---------------------------------------------------------------------------
// Cache timing class
//
class CacheModel
{
private:
  CacheArch::CacheKind  kind;
  uint8                 level;
  uint8                 rep_policy;
  
  uint32                sets;
  uint32                ways;
  uint32                blkmask;
  uint32                setmask;
  uint32                tagmask;
  uint32                waymask;
  uint32                index_bits;
  
  uint32**              tags;
  
  uint32                hit_way;
  uint32                hit_set;
  uint32                valid_match;
  uint32                victim_way;
  uint32                victim_rotate;
  uint32                windowsPassed;
  
  CacheModel*           next_level; // pointer to cache model further up in hierarchy
  MainMemoryModel*      ext_mem;

  uint64                read_hits;
  uint64                read_misses;
  uint64                write_hits;
  uint64                write_misses;
  uint64                dirty_write_hits;
  uint64                dirty_misses;
  
  uint16 replace_block  (uint32 addr, uint8 blk_bits, uint32 pc, bool write_op, bool &success);
  uint32 line_copy_back (uint32 way, uint32 set);
  uint32 line_reload    (uint32 way, uint32 set);
  bool   get_victim_way (uint32 vset);
  void   purge_victim   (uint32 victim_addr);
  
  static const uint32   kInvalidProgramCount;


public:
  uint32    block_bits;
  uint16    read_lat  [MAX_BLK_BITS+1];
  uint16    write_lat [MAX_BLK_BITS+1];


  MemoryModel*          memory_model;

  // Per PC miss frequency histogram
  //
  bool                      is_cache_miss_frequency_recording_enabled;
  arcsim::util::Histogram*  miss_freq_hist;
  // Per PC miss cycles histogram
  //
  bool                      is_cache_miss_cycle_recording_enabled;
  arcsim::util::Histogram*  miss_cycles_hist;
  arcsim::util::Counter64*  cycle_count;
  // ---------------------------------------------------------------------------
  // Constructor/Destructor
  //
  CacheModel (int                   level,
              CacheArch::CacheKind  cache_kind,
              CacheArch&            cache_arch,
              CacheModel*           next,
              MainMemoryModel*      main_mem);
  ~CacheModel ();
    
  // ---------------------------------------------------------------------------
  // Cache control functions
  //  
  uint32 invalidate      (bool flush_dirty_entries);
  uint32 invalidate_line (uint32 addr, bool flush_dirty_entries,  bool &success);
  
  uint32 flush           (bool flush_locked_entries);
  uint32 flush_line      (uint32 addr, bool flush_locked_entries, bool &success);
  uint32 lock_line       (uint32 addr, bool flush_on_lock,        bool &success);
  
  
  // ---------------------------------------------------------------------------
  // Clear cache and all of its counters
  //
  void   clear();

  // ---------------------------------------------------------------------------
  // Retrieve cache metrics
  //
  uint64  get_read_hits();
  uint64  get_read_misses();
  
  uint64  get_write_hits();
  uint64  get_write_misses();
  
  double  get_read_hit_ratio();
  double  get_write_hit_ratio();
  
  void print_stats ();
  
  // ---------------------------------------------------------------------------
  // Advanced cache debug methods
  //
  uint32 cache_addr_probe  (uint32 addr, uint32 &tag_result, bool &success);
  uint32 direct_addr_probe (uint32 addr, uint32 &tag_result, bool &success);
  uint32 direct_tag_read   (uint32 addr, uint32 &tag_result);
  uint32 direct_tag_write  (uint32 addr, uint32 tag_value);
  uint32 direct_access_way (uint32 addr);
  uint32 direct_access_set (uint32 addr);
    
  // ---------------------------------------------------------------------------
  // Public inline methods
  //
  inline bool is_hit (uint32 addr)
  {
    hit_set = (addr >> block_bits) & setmask;
    valid_match = (addr & tagmask) | VALID_BIT;
    for (hit_way = 0; hit_way < ways; ++hit_way)
      if ((tags[hit_way][hit_set] & (tagmask | VALID_BIT)) == valid_match)
        return true;
    return false;
  }
  
  inline bool is_dirty_hit (uint32 addr)
  {
    hit_set = (addr >> block_bits) & setmask;
    valid_match = (addr & tagmask) | VALID_BIT;
    for (hit_way = 0; hit_way < ways; ++hit_way)
      if ((tags[hit_way][hit_set] & (tagmask | VALID_BIT)) == valid_match)
        return ((tags[hit_way][hit_set] & DIRTY_BIT) == DIRTY_BIT);
    return false;
  }

  
  inline uint16 read (uint32 addr, uint8 blk_bits, uint32 pc)
  {
    uint16 latency = 0;
    if(level==2 && ((cycle_count->get_value()) -(windowsPassed*windowSize))<windowSize){
    if (!is_hit(addr)) {    /* CACHE MISS */
      ++read_misses;
      bool success;
      latency = read_lat[blk_bits] + replace_block (addr, block_bits, pc, false, success);
      // -----------------------------------------------------------------------
      // Profiling counters
      //
      if (is_cache_miss_frequency_recording_enabled   // Cache miss per PC profiling counter
          && miss_freq_hist                     //
          && pc != kInvalidProgramCount) {
        miss_freq_hist->inc(pc);
      }      
      if (is_cache_miss_cycle_recording_enabled  // Cache miss cycles per PC profiling counter
          && miss_cycles_hist                   //
          && pc != kInvalidProgramCount) {
        miss_cycles_hist->inc(pc, latency);
      }
    } else {                /* CACHE HIT */
      ++read_hits;
      latency = read_lat[blk_bits];
    }
    return latency;
  }
  else{ //Cache is drowsy!
      tags[hit_way][hit_set] |= DROWSY_BIT;
      if (!is_hit(addr)) {    // CACHE MISS 
      ++read_misses;
      bool success;
      latency = read_lat[blk_bits] + 2 + replace_block (addr, block_bits, pc, false, success); //ADD 2 FOR DROWSY WAKE
      // -----------------------------------------------------------------------
      // Profiling counters
      //
      if (is_cache_miss_frequency_recording_enabled   // Cache miss per PC profiling counter
          && miss_freq_hist                     //
          && pc != kInvalidProgramCount) {
        miss_freq_hist->inc(pc);
      }      
      if (is_cache_miss_cycle_recording_enabled  // Cache miss cycles per PC profiling counter
          && miss_cycles_hist                   //
          && pc != kInvalidProgramCount) {
        miss_cycles_hist->inc(pc, latency);
      }
    } else {                // CACHE HIT
      ++read_hits;
      latency = read_lat[blk_bits] + 2; //ADD 2 FOR DROWSY WAKE
    }
    return latency;
  }
  }
  
  inline uint16 write (uint32 addr, uint8 blk_bits, uint32 pc)
  {
    uint16 latency = 0;
    if (!is_hit(addr)) {    /* CACHE MISS */
      ++write_misses;
      bool success;
      latency = write_lat[blk_bits] + replace_block (addr, block_bits, pc, true, success);
      // -----------------------------------------------------------------------
      // Profiling counters
      //
      if (is_cache_miss_frequency_recording_enabled   // Cache miss per PC profiling counter
          && miss_freq_hist                     //
          && pc != kInvalidProgramCount) {
        miss_freq_hist->inc(pc);
      }      
      if (is_cache_miss_cycle_recording_enabled  // Cache miss cycles per PC profiling counter
          && miss_cycles_hist                   //
          && pc != kInvalidProgramCount) {
        miss_cycles_hist->inc(pc, latency);
      }
    } else {                /* CACHE HIT */
      ++write_hits;
      tags[hit_way][hit_set] |= DIRTY_BIT;
      latency = write_lat[blk_bits];
    }
    return latency;
  }  

};

#endif  // _INC_UARCH_MEMORY_CACHEMODEL_H_
