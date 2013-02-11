//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Scratchpad factory is responsible for instantiating the right Scratchpad
// implementation.
//
// =====================================================================

#ifndef INC_UARCH_MEMORY_SCRATCHPADFACTORY_H_
#define INC_UARCH_MEMORY_SCRATCHPADFACTORY_H_

#include "arch/SpadArch.h"

class CCMModel;

// 'Static' factory class creating Scratchpad model instances
//
class ScratchpadFactory {
private:
  // private constructor to prevent anyone from creating instances
  //
  ScratchpadFactory() { /* EMPTY */ };
  
  void operator=(const ScratchpadFactory &);           // DO NOT ASSIGN
  ScratchpadFactory(const ScratchpadFactory &);   // DO NOT COPY
  
public:
  // Factory method creating instances of Scratchpad models
  //
  static CCMModel* create_scratchpad(SpadArch&          arch,
                                     SpadArch::SpadKind kind);
};

#endif  // INC_UARCH_MEMORY_SCRATCHPADFACTORY_H_
