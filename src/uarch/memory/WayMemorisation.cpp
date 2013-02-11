//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  Way Memorisation Impl.
//
// =====================================================================

#include <cstdlib>
#include <cstdio>

#include "arch/Configuration.h"
#include "uarch/memory/WayMemorisation.h"

void
WayEntry::replaceIndex (uint32 addr)
{
  uint32 victim;
  
  if (randomReplacement) {
    victim = ((uint32)lrand48()) & assocMask;
  } else {
    victim = lruState->lru();
    lruState->touch (victim);
  }
  
  blocks[victim] = addr & blockMask;
  tag = addr & tagMask;
  valid = valid | (1 << victim);
}


void
WayMemo::replaceAddress (uint32 addr)
{
  uint32 victim;
  
  // First search for any row that has a matching tag
  
  for (victim = 0; victim < numEntries; ++victim)
    if (rows[victim].tagMatch(addr)) {
      rows[victim].replaceIndex (addr);
      lruState->touch(victim);
      return;
    }
  
  // If we get here, there is no matching tag so
  // we have to evict a complete row.
  
  if (randomReplacement)
    victim = ((uint32)lrand48()) & entryMask;
  else {
    victim = lruState->lru();
    lruState->touch(victim);
  }
  rows[victim].setAddress (addr);
}

void
WayMemo::print_stats()
{
  unsigned int total_reads = full_reads + read_accesses;
  unsigned int total_writes = full_writes + write_accesses;
  unsigned int memo_tag_reads, phased_tag_reads, full_tag_reads;
  unsigned int memo_data_reads, memo_data_writes;
  unsigned int phased_data_reads, phased_data_writes;
  unsigned int full_data_reads, full_data_writes;
  unsigned int memo_read_misses;
  unsigned int memo_write_misses;
  double       full_power;
  double       phased_power;
  double       memo_power;
  double       memo_power_saving, phased_power_saving, memo_improvement;
//  extern       const char* cache_str[CACHE_CONF_COUNT];
  
  memo_read_misses  = read_accesses  - read_hits;
  memo_write_misses = write_accesses - write_hits;
  
  memo_tag_reads    = (cacheWays * (full_reads  + memo_read_misses))   // reads
  + (cacheWays * (full_writes + memo_write_misses)); // writes
  
  memo_data_reads   = (cacheWays * (full_reads  + memo_read_misses)) + read_hits;
  memo_data_writes  = total_writes;
  
  full_tag_reads    = cacheWays * (total_reads + total_writes);
  full_data_reads   = cacheWays * total_reads;
  full_data_writes  = total_writes;
  
  phased_tag_reads  = full_tag_reads;
  phased_data_reads = total_reads;
  phased_data_writes= total_writes;
  
  full_power   = (full_tag_reads   * 1.0) + (full_data_reads   * 1.41) + (full_data_writes   * 1.48);
  phased_power = (phased_tag_reads * 1.0) + (phased_data_reads * 1.41) + (phased_data_writes * 1.48);
  memo_power   = (memo_tag_reads   * 1.0) + (memo_data_reads   * 1.41) + (memo_data_writes   * 1.48);
  
  phased_power_saving = (full_power   - phased_power) / full_power;
  memo_power_saving   = (full_power   - memo_power)   / full_power;
  memo_improvement    = (phased_power - memo_power)   / phased_power;
  
//  fprintf (stderr,"%s-cache Way Predictor Simulation Results\n", cache_str[wpu_type]);
  fprintf (stderr,"\tAssociativity (tags) %10d\n", numEntries);
  fprintf (stderr,"\tIndices per tag      %10d\n", numIndices);
  fprintf (stderr,"\tTotal cache capacity %10d\n", cacheSize);
  fprintf (stderr,"\tCache associativity  %10d\n", cacheWays);
  fprintf (stderr,"\tCache block size     %10d\n", blockSize);
  
  fprintf (stderr,"\tTotal read accesses  %10d\n", total_reads);
  
  fprintf (stderr,"\tmemo read queries    %10d\t%6.2f%%\n",
           read_accesses, 100.0*read_accesses/total_reads);
  
  fprintf (stderr,"\tmemo read hits       %10d\t%6.2f%%\n",
           read_hits, 100.0*read_hits/total_reads);
  
  fprintf (stderr,"\tnon-memo reads       %10d\t%6.2f%%\n",
           full_reads, 100.0*full_reads/total_reads);
  
  if (total_writes != 0) {
    fprintf (stderr,"\tTotal write accesses %10d\n", total_writes);
    
    fprintf (stderr,"\tmemo write queries   %10d\t%6.2f%%\n",
             write_accesses, 100.0*write_accesses/total_writes);
    
    fprintf (stderr,"\tmemo write Hits      %10d\t%6.2f%%\n",
             write_hits, 100.0*write_hits/total_writes);
    
    fprintf (stderr,"\tnon-memo writes      %10d\t%6.2f%%\n",
             full_writes, 100.0*full_writes/total_writes);
  }
  
//  fprintf (stderr,"%s-cache Way Predictor Power Saving Results\n", cache_str[wpu_type]);
  
  fprintf (stderr,"\tFull tag  RAM reads    (1.00) %10d\n",   full_tag_reads);
  fprintf (stderr,"\tFull data RAM reads    (1.41) %10d\n",   full_data_reads);
  fprintf (stderr,"\tFull data RAM writes   (1.48) %10d\n\n", full_data_writes);
  
  fprintf (stderr,"\tPhased tag  RAM reads  (1.00) %10d\n",   phased_tag_reads);
  fprintf (stderr,"\tPhased data RAM reads  (1.41) %10d\n",   phased_data_reads);
  fprintf (stderr,"\tPhased data RAM writes (1.48) %10d\n\n", phased_data_writes);
  
  fprintf (stderr,"\tMemo tag  RAM reads    (1.00) %10d\n",   memo_tag_reads);
  fprintf (stderr,"\tMemo data RAM reads    (1.41) %10d\n",   memo_data_reads);
  fprintf (stderr,"\tMemo data RAM writes   (1.48) %10d\n",   memo_data_writes);
  
  fprintf (stderr,"\tMemo/Full power saving is:    %10.2f%%\n",   memo_power_saving*100.0);
  fprintf (stderr,"\tPhased/Full power saving is:  %10.2f%%\n",   phased_power_saving*100.0);
  fprintf (stderr,"\tMemo/Phased power saving is:  %10.2f%%\n\n", memo_improvement*100.0);
}
