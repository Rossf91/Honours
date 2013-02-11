//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class providing details about a Scratchpad architecture configuration.
//
// =====================================================================

#ifndef INC_ARCH_SPADARCH_H_
#define INC_ARCH_SPADARCH_H_

#include "api/types.h"

// -----------------------------------------------------------------------------
// SpadArch Class
//
class SpadArch
{
public:
  static const int kSpadArchMaxNameSize = 256;
  
  // Scratchpad configuration kinds
  //
  enum SpadKind {
    kNoSpad         = 0x0,
    kSpadInstCcm    = 0x1,
    kSpadDataCcm    = 0x2,
    kSpadUnifiedCcm = 0x4,
  };
  static const int kSpadKindCount = 4;
  
  bool      is_configured;
  char      name[kSpadArchMaxNameSize];
  uint32    start_addr;
  uint32    size;         // Bytes
  uint32    bus_width;    // bits
  uint32    latency;      // scratchpad latency
  
  SpadArch();  
  ~SpadArch();
  
  // Return SpadKind as string
  //
  static const char * kind_tostring(SpadKind kind);

};

#endif  // INC_ARCH_SPADARCH_H_
