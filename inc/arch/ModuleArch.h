//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class providing details about a Module architecture configuration defining
// blocks of cores with shared resources (e.g. L2 cache).
//
// =====================================================================

#ifndef INC_ARCH_MODULEARCH_H_
#define INC_ARCH_MODULEARCH_H_

#include "api/types.h"

#include "arch/CacheArch.h"

// -----------------------------------------------------------------------------
// Forward Declerations
//
class CoreArch;

// -----------------------------------------------------------------------------
// ModuleArch Class
//
class ModuleArch
{
public:
  static const int kModuleArchMaxNameSize = 256;
  
  static const int kModuleArchCoreTypesSize = 20;
  
  bool        is_configured;
  char        name[kModuleArchMaxNameSize];
  uint32      number_core_types;          // No of Core types
  CoreArch*   core_type[kModuleArchCoreTypesSize];      // core type[X]
  uint32      cores_of_type[kModuleArchCoreTypesSize];  // no of cores of type[X]
  
  // Caches at Module level
  //
  uint32      cache_types;                // None/Inst/Data/Unified
  CacheArch   icache;
  CacheArch   dcache;
  
  ModuleArch() : is_configured(false)
  { /* EMPTY */ }
  
  ~ModuleArch()
  { /* EMPTY */ }
};


#endif  // INC_ARCH_MODULEARCH_H_
