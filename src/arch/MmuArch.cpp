//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================

#include "api/types.h"

#include "arch/MmuArch.h"

#include "Globals.h"

#include "Assertion.h"

// -----------------------------------------------------------------------------

// Structure encoding one page size parameter entry
struct PageSizeParamEntry {
  const char* name_;
  uint32      size_bytes_;
  uint32      size_bytes_log2;
  uint32      bcr_encoding_;
};

// Enum capturing valid indices into page size parameter table
enum PageSizeParamEntryIndex {
#define DECL_ENUM(str_,...) kIndex##str_,
  MMU_PAGE_SIZE_LIST(DECL_ENUM)
#undef DECL_ENUM
  kPageSizeParamEntryIndexCount,
};

// Table holding page size parameter configurations
static PageSizeParamEntry page_size_param_tab[kPageSizeParamEntryIndexCount] = {
#define DECL_ENTRY(str_,siz_,siz_log2_,bcr_) { #str_,siz_,siz_log2_, bcr_ },
  MMU_PAGE_SIZE_LIST(DECL_ENTRY)
#undef DECL_ENTRY
};

// -----------------------------------------------------------------------------

// Enum and table capturing valid indices for JTLB 'set' configurations
enum JtlbSetSizeIndex {
#define DECL_ENUM_V(siz_) kJtlbSetIndex##siz_,  
  MMU_JTLB_SET_SIZE_LIST(DECL_ENUM_V)
#undef DECL_ENUM_V
  kJtlbSetSizeIndexCount
};
static uint32 jtlb_valid_set_param_tab[kJtlbSetSizeIndexCount] = {
#define DECL_ENTRY(siz_) siz_ ,
  MMU_JTLB_SET_SIZE_LIST(DECL_ENTRY)
#undef DECL_ENTRY
};
// Enum and table capturing valid indices for JTLB 'way' configurations
enum JtlbWaySizeIndex {
#define DECL_ENUM_V(siz_) kJtlbWaySize##siz_,  
  MMU_JTLB_WAY_SIZE_LIST(DECL_ENUM_V)
#undef DECL_ENUM_V
  kJtlbWaySizeIndexCount
};
static uint32 jtlb_valid_way_param_tab[kJtlbWaySizeIndexCount] = {
#define DECL_ENTRY(siz_) siz_ ,
  MMU_JTLB_WAY_SIZE_LIST(DECL_ENTRY)
#undef DECL_ENTRY
};

// -----------------------------------------------------------------------------

MmuArch::MmuArch()
: is_configured(false),
  kind(kMmu),
  version(kMmuV1),    // default: MMU v1
  entry_(&page_size_param_tab[kIndex8K]), // default: 8K page descriptor compatibility mode
  jtlb_sets_(128),    // default: 128 JTLB sets
  jtlb_ways_(2),      // default: 2 JTLB ways
  u_itlb_entries(4),  // default: 4 ITLB entries
  u_dtlb_entries(8)   // default: 8 DTLB entries
{ /* EMPTY */ }


MmuArch::~MmuArch()
{ /* EMPTY */ }


bool
MmuArch::set_page_size(uint32 size_bytes)
{
  for (uint32 i = 0; i < kPageSizeParamEntryIndexCount; ++i) {
    if (page_size_param_tab[i].size_bytes_ == size_bytes) {
      entry_ = &page_size_param_tab[i];
      return true;
    }
  }
  return false;
}

uint32
MmuArch::get_page_size() const
{
  return entry_->size_bytes_;
}

uint32
MmuArch::get_page_size_log2() const
{
  return entry_->size_bytes_log2;
}

uint32
MmuArch::get_page_size_bcr_encoding() const
{
  return entry_->bcr_encoding_;
}


bool
MmuArch::set_jtlb_sets(uint32 sets)
{
  for (uint32 i = 0; i < kJtlbSetSizeIndexCount; ++i) {
    if (jtlb_valid_set_param_tab[i] == sets) {
      jtlb_sets_ = sets;
      return true;
    }
  }
  return false;
}

uint32
MmuArch::get_jtlb_sets() const
{
  return jtlb_sets_;
}

uint32
MmuArch::get_jtlb_sets_log2() const
{
  uint32 sets_log2 = 0;
  uint32 sets      = jtlb_sets_;
  while (sets >>= 1) ++sets_log2;
  return sets_log2;
}

bool
MmuArch::set_jtlb_ways(uint32 ways)
{
  for (uint32 i = 0; i < kJtlbWaySizeIndexCount; ++i) {
    if (jtlb_valid_way_param_tab[i] == ways) {
      jtlb_ways_ = ways;
      return true;
    }
  }
  return false;
}

uint32
MmuArch::get_jtlb_ways() const
{
  return jtlb_ways_;
}

uint32
MmuArch::get_jtlb_ways_log2() const
{
  uint32 ways_log2 = 0;
  uint32 ways      = jtlb_ways_;
  while (ways >>= 1) ++ways_log2;
  return ways_log2;
}

uint32
MmuArch::get_jtlb_entries() const
{
  return jtlb_sets_ * jtlb_ways_;
}




