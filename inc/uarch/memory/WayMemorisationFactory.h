//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// WayMemo factory is responsible for instantiating the right WayMemo implementation.
//
// =====================================================================

#ifndef INC_UARCH_MEMORY_WAYMEMORISATIONFACTORY_H_
#define INC_UARCH_MEMORY_WAYMEMORISATIONFACTORY_H_

#include "arch/CacheArch.h"

class WayMemo;
class WpuArch;

// 'Static' factory class creating WayMemo model instances
//
class WayMemorisationFactory {
private:
  // private constructor to prevent anyone from creating instances
  //
  WayMemorisationFactory() { /* EMPTY */ };
  
  void operator=(const WayMemorisationFactory &);           // DO NOT ASSIGN
  WayMemorisationFactory(const WayMemorisationFactory &);   // DO NOT COPY

public:
  // Factory method creating instances of WayMemo models
  //
  static WayMemo* create_way_memo(WpuArch& core_arch, CacheArch::CacheKind kind);
};



#endif  // INC_UARCH_MEMORY_WAYMEMORISATIONFACTORY_H_
