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

#include <cstdio>

#include "define.h"

#include "api/types.h"

#include "arch/Configuration.h"

#include "uarch/memory/CacheModel.h"
#include "uarch/memory/MainMemoryModel.h"
#include "uarch/memory/MemoryModel.h"

#include "util/Log.h"

// ---------------------------------------------------------------------------
// Macro definitions
//
#define VALID_DIRTY (VALID_BIT | DIRTY_BIT)
#define ALL_STATE   (VALID_BIT | LOCK_BIT | DIRTY_BIT)

const uint32   CacheModel::kInvalidProgramCount = 0x1;

// Cache
//
CacheModel::CacheModel (int lev,
                        CacheArch::CacheKind  _kind,
                        CacheArch&            cache_arch,
                        CacheModel*           _next,
                        MainMemoryModel*      _main_mem)
  : level(lev),
    kind(_kind),
    ways(cache_arch.ways),
    rep_policy(cache_arch.repl),
    block_bits(cache_arch.block_bits), // must be a minimum of 2 (4 bytes)
    read_hits(0),
    read_misses(0),
    write_hits(0),
    write_misses(0),
    dirty_misses(0),
    dirty_write_hits(0),
    is_cache_miss_frequency_recording_enabled(false),
    miss_freq_hist(0),
    is_cache_miss_cycle_recording_enabled(false),
    miss_cycles_hist(0),
    hit_way(0),
    hit_set(0),
    valid_match(0),
    victim_way(0),
    victim_rotate(0),
    next_level(_next),
    memory_model(0),
    ext_mem(_main_mem)
{
  // Compute read/write latencies
  //
  LatencyUtil::compute_read_latencies(read_lat, cache_arch.bus_width, cache_arch.bus_clk_div,
                                      cache_arch.latency, level);
  LatencyUtil::compute_write_latencies(write_lat, cache_arch.bus_width, cache_arch.bus_clk_div,
                                       cache_arch.latency, level);
  
  uint16 totalblocks = cache_arch.size >> block_bits;
  uint16 blocksize   = (1 << block_bits);
  
  sets               = totalblocks / ways;
  blkmask            = (blocksize - 1);
  tagmask            = ~blkmask;
  setmask            = (sets - 1);
  waymask            = (ways - 1);
  
  // Compute index bits
  //
  index_bits         = 0;
  for (uint32 s = setmask;
       s > 0;
       s = s >> 1, ++index_bits);
  
  // Create ways/sets
  //
  tags = new uint32* [ways];
  for (uint32 w = 0; w < ways; ++w) {
    tags[w] = new uint32 [sets];
    for (uint32 s = 0; s < sets; ++s) {
      tags[w][s] = 0;
    }
  }
}

CacheModel::~CacheModel ()
{
  for (uint32 w = 0; w < ways; ++w)
    delete [] tags[w];
  delete [] tags;
}

void
CacheModel::clear()
{
  // Clear out ways/sets
  //
  for (uint32 w = 0; w < ways; ++w) {
    for (uint32 s = 0; s < sets; ++s) {
      tags[w][s] = 0;
    }
  }
  read_hits   = 0;
  read_misses = 0;
  write_hits  = 0;
  write_misses= 0;
  dirty_write_hits = 0;
  dirty_misses     = 0;
  hit_way       = 0;
  hit_set       = 0;
  victim_way    = 0;
  victim_rotate = 0;
  valid_match   = 0;

}

// Compute the cost of copying-back a line to the next level in
// the memory hierarchy.
//
uint32
CacheModel::line_copy_back (uint32 way, uint32 set)
{
  uint32 victim_addr = tags[way][set] & tagmask;

  if (next_level)
    return next_level->write (victim_addr, block_bits, kInvalidProgramCount);
  else
    return ext_mem->write (block_bits);        
}

// Compute the cost of reloading a line from the next level in
// the memory hierarchy.
//
uint32
CacheModel::line_reload (uint32 way, uint32 set)
{
  uint32 victim_addr = tags[way][set] & tagmask;

  if (next_level)
    return next_level->read (victim_addr, block_bits, kInvalidProgramCount);
  else
    return ext_mem->read (block_bits);        
}

// Determine the victim way when bringing a new block into cache
//
bool
CacheModel::get_victim_way (uint32 vset)
{
  // 1. Rotate 'pseudo random' register used for victim selection.
  //    This is always rotated when replacement is invoked.
  //    It is used if no free block is found in the victim set.
  //
  victim_rotate = (victim_rotate + 1) % ways;
  
  // 2. Check if there is a free way in that can be replaced in the victim set,
  //    if so assign it to victim_way.
  //
  for (uint32 w = 0; w < this->ways ; ++w) {
    if ( !(tags[w][vset] & (VALID_BIT | LOCK_BIT)) ) {
      victim_way = w;
      return true;
    }
  }
  
  // 3. If no unused way was found, use 'pseudo random' replacement policy
  //    bearing in mind that lines may be locked (and therefore not candidates
  //    for replacement).
  //
  //    Starting from the next victim_rotate way, find the next way 'v' 
  //    that is not locked, and assign that as the victim_way.
  //
  uint32 v = victim_rotate;

  for (uint32 w = 0; w < this->ways ; ++w) {
    if (!(tags[v][vset] & LOCK_BIT)) {
      victim_way = v;
      return true;
    }
    else {
      v = (v + 1) % ways;
    }
  }
  
  // 4. If no unlocked way was found then replacement fails due to all
  //    ways in the set being locked.
  //
  return false;
}

void
CacheModel::purge_victim (uint32 victim_addr)
{
  if (level == CORE_LEVEL && memory_model) {
    // Invalidate victim hash entry
    //
    switch (kind) {
      case CacheArch::kInstCache: case CacheArch::kUnifiedCache: {        
        memory_model->latency_cache.purge_fetch_entry(victim_addr);
        break;
      }
      case CacheArch::kDataCache: {
        // FIXME: is this really necessary
        //
        memory_model->latency_cache.purge_read_entry(victim_addr);
        memory_model->latency_cache.purge_write_entry(victim_addr);
        break;
      }
      default: break;
    }
  }                                    
}
 
// Get a new block or evict old block (dirty) and get new block
//
uint16
CacheModel::replace_block (uint32 addr, uint8 blk_bits, uint32 pc, bool write_op, bool &success)
{
  // lower level caches must have the same or larger cache block size as victim_addr
  //  is really victim cache block address
  uint32  victim_addr        = 0; 
  uint16  miss_time          = 0;
  bool    dirty              = false;
  
  // Replacement policy (a.k.a. victim selection)
  //
  if (!(success = get_victim_way(hit_set))) {
    // This only happens of all ways in a set are locked
    // In this case, return the cost of reading or writing
    // 2^2 = 4 bytes directly to/from memory to model an 
    // uncached operation.
    //
    return (write_op ? ext_mem->write(2) : ext_mem->read(2));  
  }
    
  // Is victim dirty?
  //
  if (tags[victim_way][hit_set] & DIRTY_BIT) {
    dirty = true;
    ++dirty_misses;
  }
  
  // Calculate victim address
  //
  victim_addr = tags[victim_way][hit_set] & tagmask;
  
  // Go to next memory hierarchy level
  //
  if (next_level) {
    if (dirty) {
      miss_time = next_level->write (victim_addr, blk_bits, pc) + next_level->read (addr, blk_bits, pc);
    } else {
      miss_time = next_level->read (addr, blk_bits, pc);
    }
  } else {
    if (dirty) {
      miss_time = ext_mem->write (blk_bits) + ext_mem->read (blk_bits);      
    } else {
      miss_time = ext_mem->read (blk_bits);
    }
  }
    
  purge_victim (victim_addr);
  
  if (write_op) {
    tags[victim_way][hit_set] = (addr & tagmask) | (VALID_BIT | DIRTY_BIT);
  } else {
    tags[victim_way][hit_set] = (addr & tagmask) | VALID_BIT;
  }
  return miss_time;
}

// function to invalidate the entire cache
//
uint32
CacheModel::invalidate (bool flush_dirty_entries)
{
  uint32 w, s;
  uint32 cost = sets;
  
  for (w = 0; w < ways; ++w) {
    for (s = 0; s < sets; ++s) {
      if (flush_dirty_entries && ((tags[w][s] & VALID_DIRTY) == VALID_DIRTY)) {
        cost += line_copy_back (w, s);
      }
      purge_victim (tags[w][s] & tagmask);
      tags[w][s] &= tagmask; // clear VALID, DIRTY and LOCK bits
    }
  }
  
  return cost;
}

// function to flush the entire cache
//
uint32
CacheModel::flush (bool flush_locked_entries)
{
  uint32 w, s;
  uint32 cost = sets;
  
  for (w = 0; w < ways; ++w) {
    for (s = 0; s < sets; ++s) {
      if (    ((tags[w][s] & VALID_DIRTY) == VALID_DIRTY)
           && (flush_locked_entries || !(tags[w][s] & LOCK_BIT) )) { 
        cost += line_copy_back (w, s);        
        tags[w][s] ^= DIRTY_BIT; // toggle the dirty bit to clear it
      }
    }
  }
  
  return cost;
}

// function to flush one line in the cache
//
uint32
CacheModel::flush_line (uint32 addr, bool flush_locked_entries, bool &success)
{
  uint32 cost = 1; // one cycle minimum, to check tags.
  
  hit_set     = (addr >> block_bits) & setmask;
  valid_match = (addr & tagmask) | VALID_DIRTY;
  success     = false;
  
  for (hit_way = 0; hit_way < ways; ++hit_way)
    if ((tags[hit_way][hit_set] & (tagmask | VALID_DIRTY)) == valid_match) {
      // this is a match and the line is dirty
      success = true;
      if (flush_locked_entries || !(tags[hit_way][hit_set] & LOCK_BIT)) {
        cost += line_copy_back (hit_way, hit_set);
        tags[hit_way][hit_set] ^= DIRTY_BIT; // toggle the dirty bit to clear it
      }
      break;
    }
    
  return cost;
}

// function to invalidate one line in the cache
//
uint32
CacheModel::invalidate_line (uint32 addr, bool flush_dirty_entries, bool &success)
{
  uint32 cost = 1; // one cycle minimum, to check tags.
  hit_set     = (addr >> block_bits) & setmask;
  valid_match = (addr & tagmask) | VALID_BIT;
  success     = false;
  
  for (hit_way = 0; hit_way < ways; ++hit_way) {
    if ((tags[hit_way][hit_set] & (tagmask | VALID_BIT)) == valid_match) {
      // this is a match
      if (flush_dirty_entries && (tags[hit_way][hit_set] & VALID_BIT)) {
        // the line is dirty and a dirty line should be flushed
        cost += line_copy_back (hit_way, hit_set);
      }
      purge_victim (tags[hit_way][hit_set] & tagmask);
      tags[hit_way][hit_set] &= tagmask; // clear VALID, DIRTY and LOCK bits
      success = true;
      break;
    }
  }
    
  return cost;
}

// function to load and lock a line in the cache
//
uint32
CacheModel::lock_line (uint32 addr, bool flush_on_lock, bool &success)
{
  uint32 cost = 1;
  hit_set     = (addr >> block_bits) & setmask;
  valid_match = (addr & tagmask) | VALID_BIT;
  success     = false;
  
  // 1. Check to see if the given address is already present in cache
  //
  for (hit_way = 0; hit_way < ways; ++hit_way)
    if ((tags[hit_way][hit_set] & (tagmask | VALID_BIT)) == valid_match) 
    {
      // The address to be locked is aleady in cache.      
      // If the line is clean, re-read it from memory
      //
      if (!(tags[hit_way][hit_set] & DIRTY_BIT)) {
        cost += line_reload (hit_way, hit_set);
      }
      
      // If flush_on_lock, and the line is dirty, copy it back to memory
      //
      if (flush_on_lock && (tags[hit_way][hit_set] & DIRTY_BIT)) {
        cost += line_copy_back (hit_way, hit_set);
        tags[hit_way][hit_set] ^= DIRTY_BIT; // toggle the dirty bit to clear it
      }
      
      tags[hit_way][hit_set] |= LOCK_BIT; // lock the line
      success = true;
      return cost;
    }
  
  // 2. Addess was not present in cache, so use normal read-replacement
  //
  cost += replace_block (addr, block_bits, kInvalidProgramCount, false, success);
  
  if (success) {
    // A victim line was found, and was loaded with the lock address,
    // so we can now lock that line.
    //
    tags[victim_way][hit_set] |= LOCK_BIT; // lock the line
  }
  
  return cost;
}

// Probe the cache by presenting a normally-formatted address to
// the cache and getting back the tag value and hit result.
//
uint32
CacheModel::cache_addr_probe (uint32 addr, uint32 &tag_result, bool &success)
{
  // Locate 'addr' in cache, if possible, returning hit/miss 
  // result in 'success'
  //
  success = is_hit(addr);

  // Read the selected tag value.
  // N.B. if there is a cache miss we return zero, otherwise we
  // return the formatted tag field with state bits and
  // set number, as required.
  //
  if (success) {
    tag_result = (tags[hit_way][hit_set] & (tagmask | ALL_STATE))
               | (hit_set << block_bits);
  } else {
    tag_result = 0;
  }
  
  // Return the cycle cost of this operation, which is assumed
  // to be 1 cycle.
  //
  return 1;
}

// Perform a direct probe of one tag memory, given the way
// and set index contained in the direct-access formatted
// 'addr' argument. 'success' will be true if the corresponding
// tag is valid.
// This method also returns the corresponding memory address
// formed from the index and offset bits provided in 'addr'
// merged with the tag at the given way.
//
uint32
CacheModel::direct_addr_probe (uint32 addr, uint32 &tag_result, bool &success)
{
  // Select the 'way' and 'set' fields from the given 'addr'
  // argument, according to the format specified for Direct
  // Cache Access Addressing.
  //
  uint32 way = (addr >> (block_bits + index_bits)) & waymask;
  uint32 set = (addr >> block_bits) & setmask;
  
  // Probe the selected way, at the selected set index,
  // mask out any unwanted bits, and merge-in the set index
  // at the appropriate index position.
  //
  tag_result = (tags[way][set] & (tagmask | ALL_STATE))
             | (set << block_bits);
  
  success = tag_result & VALID_BIT;
  
  // Return the memory address corresponding to the location
  // contained in the selected tag, with the block offset
  // given in the lower bits of 'addr'.
  //
  return (tag_result & tagmask) | (addr & blkmask);
}

// Perform a direct read of the tag memory, given the tag address
// (way and index) specified by the addr argument.
//
uint32
CacheModel::direct_tag_read (uint32 addr, uint32 &tag_result)
{
  // Select the 'way' and 'set' fields from the given 'addr'
  // argument, according to the format specified for Direct
  // Cache Access Addressing.
  //
  uint32 way = (addr >> (block_bits + index_bits)) & waymask;
  uint32 set = (addr >> block_bits) & setmask;
  
  // Read the selected way, at the selected set index,
  // mask out any unwanted bits, and merge-in the set index
  // at the appropriate index position.
  //
  tag_result = (tags[way][set] & (tagmask | ALL_STATE))
             | (set << block_bits);
  
  // Return the cycle cost of this operation, which is assumed
  // to be 1 cycle.
  //
  return 1;
}

// Perform a direct write to the tag memory, given the tag address
// (way and index) specified by the addr argument, and given the
// tag_value argument to write to the tag.
//
uint32
CacheModel::direct_tag_write (uint32 addr, uint32 tag_value)
{
  // Select the 'way' and 'set' fields from the given 'addr'
  // argument, according to the format specified for Direct
  // Cache Access Addressing.
  //
  uint32 way = (addr >> (block_bits + index_bits)) & waymask;
  uint32 set = (addr >> block_bits) & setmask;
  
  // Mask out reserved and unused bits from the supplied tag
  // value, and write the selected way, at the selected set index.
  //
  tags[way][set] = tag_value & (tagmask | ALL_STATE);
  
  // Return the cycle cost of this operation, which is assumed
  // to be 1 cycle.
  //
  return 1;
}

// ---------------------------------------------------------------------------
// Retrieve cache metrics
//
uint64
CacheModel::get_read_hits()
{
  uint64 value = 0;
  switch (kind) {
    case CacheArch::kInstCache: {
      if (memory_model) { value = memory_model->latency_cache.get_fetch_hits(); }
      value += read_hits;
      break;
    }
    case CacheArch::kDataCache: {
      if (memory_model) { value = memory_model->latency_cache.get_read_hits();  }
      value += read_hits;
      break;
    }
    case CacheArch::kUnifiedCache: {
      if (memory_model) {
        value =   memory_model->latency_cache.get_read_hits()
                + memory_model->latency_cache.get_fetch_hits();
      }
      value += read_hits;
      break;
    }
    default: break;
  }
  return value;  
}

uint64 
CacheModel::get_read_misses()
{
  return read_misses;
}

uint64
CacheModel::get_write_hits()
{
  uint64 value = 0;
  switch (kind) {
    case CacheArch::kInstCache: {
      value = 0;
      break;
    }
    case CacheArch::kDataCache: {
      if (memory_model) {
        value = memory_model->latency_cache.get_write_hits(); 
      }
      value += write_hits;
      break;
    }
    case CacheArch::kUnifiedCache: {
      if (memory_model) {
        value =   memory_model->latency_cache.get_write_hits();
      }
      value += write_hits;
      break;
    }
    default: break;
  }
  return value;
}

uint64
CacheModel::get_write_misses()
{
  return write_misses;
}

double
CacheModel::get_read_hit_ratio()
{
  double value = 0;
  switch (kind) {
    case CacheArch::kInstCache:
    case CacheArch::kDataCache:
    case CacheArch::kUnifiedCache:
    {
      uint64 total_reads = get_read_hits() + get_read_misses();
      if (total_reads > 0) {
        value = (100.0 * (double)(get_read_hits())/(double)total_reads);
      }
      break;
    }
    default: {
      value =  0;
    }
  }
  return value;
}

double
CacheModel::get_write_hit_ratio()
{
  double value = 0;
  switch (kind) {
    case CacheArch::kInstCache:
    case CacheArch::kDataCache:
    case CacheArch::kUnifiedCache:
    {
      uint64 total_writes = get_write_hits() + get_write_misses();
      if (total_writes > 0) {
        value = (100.0 * (double)(get_write_hits())/(double)total_writes);
      }
      break;
    }
    default: {
      value =  0;
    }
  }
  return value;
}


// ---------------------------------------------------------------------------
// Print cache metrics
//
void
CacheModel::print_stats ()
{
  const char * cache_str = CacheArch::kind_tostring(kind);
  fprintf (stderr, "L%i %s-Cache Statistics\n", level, cache_str);
  switch (kind) {
    case CacheArch::kInstCache: {
      fprintf (stderr, " %s$-Read hits          %10lld\n",  cache_str, get_read_hits()); 
      fprintf (stderr, " %s$-Read misses        %10lld\n",  cache_str, get_read_misses()); 
      fprintf (stderr, " %s$-Read hit ratio      %8.2f%%\n",cache_str, get_read_hit_ratio());
    } break;
    case CacheArch::kDataCache: {
      uint64 mem_read_hits  = (memory_model) ? memory_model->latency_cache.get_read_hits() : 0;
      uint64 mem_write_hits = (memory_model) ? memory_model->latency_cache.get_write_hits() : 0;
      uint64 all_reads  = get_read_hits() + get_read_misses();
      uint64 all_writes = get_write_hits()+ get_write_misses();
      uint64 all        = all_reads + all_writes;
      fprintf (stderr, " %s$-RW misses           %10lld\n",   cache_str, write_misses + read_misses);
      fprintf (stderr, " %s$-RW hits             %10lld\n",   cache_str,   write_hits
                                                                         + read_hits
                                                                         + mem_read_hits
                                                                         + mem_write_hits);
      fprintf (stderr, " %s$-Read hits           %10lld\n",   cache_str, get_read_hits()); 
      fprintf (stderr, " %s$-Read misses         %10lld\n",   cache_str, get_read_misses()); 
      fprintf (stderr, " %s$-Read hit ratio       %8.2f%%\n", cache_str, get_read_hit_ratio());
      fprintf (stderr, " %s$-Write hits          %10lld\n",   cache_str, get_write_hits()); 
      fprintf (stderr, " %s$-Write misses        %10lld\n",   cache_str, get_write_misses());
      fprintf (stderr, " %s$-Dirty misses        %10lld\n",   cache_str, dirty_misses); 
      fprintf (stderr, " %s$-Write hit ratio      %8.2f%%\n", cache_str, get_write_hit_ratio());
      fprintf (stderr, " %s$-Combined hit ratio   %8.2f%%\n", cache_str,
               all ? (100.0*(double)(  write_hits
                                     + read_hits
                                     + mem_write_hits
                                     + mem_read_hits)/(double)all) : 0 );
      if(next_level){
          next_level->print_stats();
      }
      break;
    }
    case CacheArch::kUnifiedCache: {
      uint64 mem_read_hits  = (memory_model) ? memory_model->latency_cache.get_read_hits() : 0;
      uint64 mem_write_hits = (memory_model) ? memory_model->latency_cache.get_write_hits() : 0;
      uint64 mem_fetch_hits = (memory_model) ? memory_model->latency_cache.get_fetch_hits() : 0;
      uint64 all_reads  = get_read_hits() + get_read_misses();
      uint64 all_writes = get_write_hits()+ get_write_misses();
      uint64 all        = all_reads + all_writes;
      fprintf (stderr, " Read hits          %10lld\n",  get_read_hits()); 
      fprintf (stderr, " Read misses        %10lld\n",  get_read_misses()); 
      fprintf (stderr, " Read hit ratio      %8.2f%%\n",get_read_hit_ratio());
      fprintf (stderr, " Write hits         %10lld\n",  get_write_hits()); 
      fprintf (stderr, " Write misses       %10lld\n",  get_write_misses()); 
      fprintf (stderr, " Dirty misses       %10lld\n",  dirty_misses); 
      fprintf (stderr, " Write hit ratio     %8.2f%%\n",get_write_hit_ratio());
      fprintf (stderr, " Combined hit ratio  %8.2f%%\n",
               all ? (100.0*(double)(  write_hits
                                     + read_hits
                                     + mem_read_hits
                                     + mem_write_hits
                                     + mem_fetch_hits)/(double)all) : 0 );
      break;
    }
    default: { break; }
  }
}
