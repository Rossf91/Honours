//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class providing details about a Core architecture configuration defining
// a single core type with assiciated private resources (e.g. caches)
//
// =====================================================================

#ifndef INC_ARCH_COREARCH_H_
#define INC_ARCH_COREARCH_H_

#include "api/types.h"
#include "sim_types.h"

#include "arch/CacheArch.h"
#include "arch/SpadArch.h"
#include "arch/WpuArch.h"
#include "arch/BpuArch.h"
#include "arch/MmuArch.h"
#include "arch/IfqArch.h"

// -----------------------------------------------------------------------------
// Forward declarations
//
class PageArch;

// -----------------------------------------------------------------------------
// CoreArch Class
//
class CoreArch
{
public:
  static const int kCoreArchMaxNameSize = 256;
  
  bool        is_configured;
  char        name[kCoreArchMaxNameSize];
  uint32      cpu_clock_divisor;            // cpu clock divisor
  uint32      cpu_data_bus_width;           // cpu data bus width (bits)
  uint32      cpu_warmup_cycles;            // cpu warmup time in cycles (must be calibrated)
  
  ProcessorPipelineVariant pipeline_variant; // cpu pipeline variant to use (EC5, EC7, SKIPJACK)
  
  bool        isa_cyc;                // cpu isa exec cycles file
  int         isa[256];               // cpu isa array
  
  uint32      cache_types;            // Inst/Data/I&D/Unified
  CacheArch   icache;
  CacheArch   dcache;
  
  uint32      spad_types;             // ICCM/DCCM/I&D/UCCM
  SpadArch    iccm;
  SpadArch    iccms[4];
  SpadArch    dccm;
  
  uint32      cpu_bo;                 // "block offset" calc from dbw
  BpuArch     bpu;                    // Branch Predictor Unit
  
  uint32      wpu_types;              // Cache Way Predictor Unit/s
  WpuArch     iwpu;
  WpuArch     dwpu;
  
  IfqArch     ifq_arch;               // Instruction fetch queue architecture
  MmuArch     mmu_arch;               // Memory management unit architecture
  
  PageArch&   page_arch;              // Page architecture reference parameter
  
  explicit CoreArch(PageArch* _page_arch)
  : is_configured(false),
    page_arch(*_page_arch)
  { /* EMPTY */ }
  
  ~CoreArch()
  { /* EMPTY */ }
  
};


#endif  // INC_ARCH_COREARCH_H_
