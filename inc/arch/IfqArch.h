//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class providing details about an IFQ (Instruction Fetch Queue) configuration.
//
// =====================================================================

#ifndef INC_ARCH_IFQARCH_H_
#define INC_ARCH_IFQARCH_H_

class IfqArch
{
public:
  static const int kIfqArchMaxNameSize = 256;
  
  bool      is_configured;
  char      name[kIfqArchMaxNameSize];
  
  uint32    size;
  
  IfqArch()
  : is_configured(false),
    size(0)
  { /* EMPTY */ }
  
  ~IfqArch()
  { /* EMPTY */ }
};

#endif  // INC_ARCH_IFQARCH_H_
