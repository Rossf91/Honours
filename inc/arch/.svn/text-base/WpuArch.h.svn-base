//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class providing details about a Way predictor architecture configuration.
//
// =====================================================================

#ifndef INC_ARCH_WPUARCH_H_
#define INC_ARCH_WPUARCH_H_

#include "api/types.h"

class WpuArch
{
public:
  static const int kWpuArchMaxNameSize = 256;
  
  bool      is_configured;
  char      name[kWpuArchMaxNameSize];
  uint32    entries;
  uint32    indices;
  uint32    ways;
  uint32    size;
  uint32    block_size;
  bool      phased;
  
  WpuArch()
  : is_configured(false),
    entries(0),
    indices(0),
    ways(0),
    size(0),
    block_size(0),
    phased(false)
  { /* EMPTY */ }
  
  ~WpuArch() { /* EMPTY */ }
};


#endif  // INC_ARCH_WPUARCH_H_
