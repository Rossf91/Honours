//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class providing details about a Cache configuration.
//
// =====================================================================

#ifndef INC_ARCH_CACHEARCH_H_
#define INC_ARCH_CACHEARCH_H_

#include "api/types.h"

// -----------------------------------------------------------------------------
// CacheArch Class
//
class CacheArch
{
public:
  static const int kCacheArchMaxNameSize = 256;
  
  // Cache configuration kinds
  //
  enum CacheKind {
    kNoCache        = 0x0,
    kInstCache      = 0x1,
    kDataCache      = 0x2,
    kUnifiedCache   = 0x4,
  };
  static const int kCacheKindCount = 4;
  
  bool      is_configured;
  char      name[kCacheArchMaxNameSize];
  uint32    size;         // Bytes
  uint32    block_bits;   // Block offset (bits)
  uint32    ways;         // associativity
  uint32    repl;         // replacement policy
  uint32    bus_width;    // bus width above cache (bits)
  uint32    latency;      // cache latency
  uint32    bus_clk_div;  // bus clock divisor
  
  CacheArch();
  ~CacheArch();
  
  // Return CacheKind as string
  //
  static const char * kind_tostring(CacheKind kind);
};


#endif  // INC_ARCH_CACHEARCH_H_
