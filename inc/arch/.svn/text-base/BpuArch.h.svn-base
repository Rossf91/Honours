//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class providing details about a Branch predictor architecture configuration.
//
// =====================================================================

#ifndef INC_ARCH_BPUARCH_H_
#define INC_ARCH_BPUARCH_H_

#include "api/types.h"

// -----------------------------------------------------------------------------
// BpuArch Class
//
class BpuArch
{
public:
  static const int kBpuArchMaxNameSize = 256;
  
  bool      is_configured;
  char      name[kBpuArchMaxNameSize];
  char      bp_type;
  uint32    bht_entries;
  uint32    ways;
  uint32    sets;
  uint32    ras_entries;
  uint32    miss_penalty;
  
  BpuArch()
  : is_configured(false),
    bht_entries(0),
    ways(0),
    sets(0),
    ras_entries(0),
    miss_penalty(0)
  { /* EMPTY */ }
  
  ~BpuArch() { /* EMPTY */ }
};

#endif  // INC_ARCH_BPUARCH_H_
