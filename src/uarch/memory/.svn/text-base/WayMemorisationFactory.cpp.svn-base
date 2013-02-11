//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Factory class used for WayMemorisation instantiation.
//
// =====================================================================

#include "arch/Configuration.h"

#include "uarch/memory/WayMemorisationFactory.h"
#include "uarch/memory/WayMemorisation.h"

WayMemo*
WayMemorisationFactory::create_way_memo(WpuArch& wpu_arch,
                                        CacheArch::CacheKind kind)
{
  WayMemo* way_pred = 0;
  
  if (wpu_arch.is_configured) {
    way_pred = new WayMemo(kind,
                           wpu_arch.entries,
                           wpu_arch.indices,
                           wpu_arch.ways,
                           wpu_arch.size,
                           wpu_arch.block_size,
                           wpu_arch.phased);
  }
  
  return way_pred;
}
