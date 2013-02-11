//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  Classes and types for Way Memorisation.
//
// =====================================================================


#ifndef _INC_UARCH_WAYMEMORISATION_H_
#define _INC_UARCH_WAYMEMORISATION_H_

#include "sim_types.h"

class LRUstate {
private:
  char*   lru_stack;
  uint32  depth;

public:
  LRUstate (uint32 elements)
  : depth(elements)
  {
    lru_stack = new char [depth];
    for (uint32 i = 0; i < depth; ++i) { lru_stack[i] = i; }
  }
  
  ~LRUstate ()
  {
    delete [] lru_stack;
  }
  
  void touch (uint32 el)
  {
    uint32 pos;
    for (pos = 0; pos < depth; ++pos) {
      if (lru_stack[pos] == el)
        break;
    }
    for (int i = pos; i > 0; --i) {
      lru_stack[i] = lru_stack[i-1];
    }
    
    lru_stack[0] = el;
  }
  
  uint32 lru () const { return lru_stack[depth-1]; }
  
};

class WayEntry {
private:
  uint32  tag;
  uint32  tagMask;
  uint32  blockMask;
  uint32  assocMask;
  uint32  numBlocks;
  uint32 *blocks;
  uint32  valid;
  LRUstate     *lruState;
  bool          randomReplacement;

public:
  
  WayEntry () :
    tag(0), tagMask(0), valid(0), blocks(0), numBlocks(0)
  { /* EMPTY */ }
  
  ~WayEntry ()
  {
    if (blocks) delete [] blocks;
  }

  void initWayEntries (uint32 ix, uint32 mask, uint32 bsize)
  {
    tag = 0;
    tagMask = mask;
    blockMask = ~mask & ~(bsize - 1);
    assocMask = ix - 1;
    valid = 0;
    if (blocks) delete [] blocks;
    blocks = new uint32 [ix];
    
    for (uint32 i = 0; i < ix; ++i)
      blocks[i] = 0;
    
    numBlocks = ix;
    
    lruState = new LRUstate (ix);
    randomReplacement = false;
  }

  
  bool tagMatch (uint32 addr) const { return ((addr & tagMask) == tag); }
  
  bool checkAddress (uint32 addr)
  {
    if (tagMatch(addr)) {
      uint32 bl = addr & blockMask;
      for (uint32 i = 0; i < numBlocks; ++i) {
        if ((blocks[i] == bl) && ((valid >> i) & 1)) {
          lruState->touch(i);
          return true;
        }
      }
    }
    return false;
  }
  
  void replaceIndex (uint32 addr);
  
  void setAddress (uint32 addr)
  {
    uint32 victim = 0;
    
    blocks[victim] = addr & blockMask;
    tag = addr & tagMask;
    valid = (1 << victim);
  }
};


class WayMemo {
private:
  // Way memorisation table contains numEntries entries, each of
  // which contains a tag and numIndices block index fields.
  
  uint32 numEntries;
  uint32 numIndices;
  WayEntry *rows;
  
  int wpu_type;
  uint32 cacheWays;
  uint32 cacheSize;
  uint32 blockSize;
  uint32 bytesPerWay;
  uint32 tagMask;
  uint32 entryMask;
  LRUstate    *lruState;
  bool         randomReplacement;
  bool         phasedCache;

public:
  uint32 read_accesses;
  uint32 full_reads;
  uint32 write_accesses;
  uint32 full_writes;
  uint32 read_hits;
  uint32 write_hits;

  
  
  WayMemo (int wptype, uint32 entries, uint32 indices,
           uint32 ways, uint32 size, uint32 bsize,
           bool phased)
  {
    wpu_type = wptype;
    numEntries  = entries;
    numIndices  = indices;
    cacheSize   = size;
    cacheWays   = ways;
    blockSize   = bsize;
    bytesPerWay = cacheSize / cacheWays;
    tagMask     = ~(bytesPerWay - 1);
    entryMask   = numEntries - 1;
    phasedCache = phased;
    
    rows = new WayEntry [numEntries];
    
    for (uint32 i = 0; i < entries; ++i)
      rows[i].initWayEntries (numIndices, tagMask, blockSize);
    
    clearCounters ();
    
    lruState = new LRUstate (numEntries);
    randomReplacement = false;
  }
  
  ~WayMemo ()
  {
    delete [] rows;
  }
  
  void clearCounters ()
  {
    read_accesses = 0;
    write_accesses = 0;
    full_reads = 0;
    full_writes = 0;
    read_hits = 0;
    write_hits = 0;
  }
  
  void replaceAddress (uint32 addr);
  
  void readAccess (uint32 addr)
  {
    ++read_accesses;
    for (uint32 i = 0; i < numEntries; ++i)
      if (rows[i].checkAddress (addr)) {
        ++read_hits;
        return;
      }
    replaceAddress (addr);
  }
  
  void fullReadAccess (uint32 addr)
  {
    ++full_reads;
    for (uint32 i = 0; i < numEntries; ++i)
      if (rows[i].checkAddress (addr)) {
        return;
      }
    replaceAddress (addr);
  }
  
  void writeAccess (uint32 addr)
  {
    ++write_accesses;
    for (uint32 i = 0; i < numEntries; ++i)
      if (rows[i].checkAddress (addr)) {
        ++write_hits;
        return;
      }
    replaceAddress (addr);
  }
  
  void fullWriteAccess (uint32 addr)
  {
    ++full_writes;
    for (uint32 i = 0; i < numEntries; ++i)
      if (rows[i].checkAddress (addr)) {
        return;
      }
    replaceAddress (addr);
  }
  
  void print_stats();
    
};

#endif /* _INC_UARCH_WAYMEMORISATION_H_ */

