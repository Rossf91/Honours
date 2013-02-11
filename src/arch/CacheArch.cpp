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


#include "arch/CacheArch.h"

// -----------------------------------------------------------------------------
// CacheKind strings
//
static const char* cache_string[CacheArch::kCacheKindCount] =
{ 
  "0",
  "I",
  "D",
  "U",
};

// Return CacheKind as string
//
const char *
CacheArch::kind_tostring(CacheArch::CacheKind kind)
{
  switch (kind) {
    case CacheArch::kNoCache:       return cache_string[0];
    case CacheArch::kInstCache:     return cache_string[1];
    case CacheArch::kDataCache:     return cache_string[2];
    case CacheArch::kUnifiedCache:  return cache_string[3];
  }
}

// -----------------------------------------------------------------------------
// Constructor/Destructor
//
CacheArch::CacheArch()
  : is_configured(false),
    size(0),
    block_bits(0),
    ways(0),
    repl(0),
    bus_width(0),
    latency(0),
    bus_clk_div(0)
{ /* EMPTY */ }

CacheArch::~CacheArch()
{ /* EMPTY */ }


