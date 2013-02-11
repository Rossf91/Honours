//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class providing details about a System architecture configuration defining
// blocks of modules with shared off-chip resources (e.g. L3 cache/main memory).
//
// =====================================================================

#ifndef INC_ARCH_SYSTEMARCH_H_
#define INC_ARCH_SYSTEMARCH_H_

#include "api/types.h"

#include "arch/CacheArch.h"
#include "arch/IsaOptions.h"
#include "arch/SimOptions.h"

// -----------------------------------------------------------------------------
// Forward declarations
//
class ModuleArch;

// -----------------------------------------------------------------------------
// SystemArch Class
//
class SystemArch
{
public:
  static const int kSystemArchMaxNameSize     = 256;
  
  static const int kSystemArchModuleTypesSize = 20;
  
  bool        is_configured;
  char        name[kSystemArchMaxNameSize];
  uint32      master_clock_freq;            // master clock freq (Hz)
  uint32      mem_start_addr;
  uint32      mem_end_addr;
  uint32      mem_dbw;                      // memory bus width (bits)
  uint32      mem_latency;                  // memory latency
  uint32      mem_bus_clk_div;              // memory bus clock divisor
  uint32      number_module_types;          // No of Module types
  ModuleArch* module_type[kSystemArchModuleTypesSize];    // ptr to module type[X]
  uint32      modules_of_type[kSystemArchModuleTypesSize];// no of modules of type[X]
  
  // Caches at System level
  //
  uint32      cache_types;                  // None/Inst/Data/Unified
  CacheArch   icache;
  CacheArch   dcache;
  
  IsaOptions  isa_opts;                     // ISA options
  SimOptions  sim_opts;                     // Simulation options
  
  SystemArch()
  : is_configured(false),
    master_clock_freq(0),
    mem_start_addr(0),
    mem_end_addr(0),
    mem_dbw(0),
    mem_latency(0),
    mem_bus_clk_div(0),
    number_module_types(0),
    cache_types(CacheArch::kNoCache)
  {
    name[0] = '\0';
  }
  
  ~SystemArch()
  { /* EMPTY */ }
  
};


#endif  //  INC_ARCH_SYSTEMARCH_H_
