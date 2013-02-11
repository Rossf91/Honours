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

#include "uarch/memory/ScratchpadFactory.h"

#include "uarch/memory/CcmModel.h"

// Factory method creating instances of Scratchpad models
//
CCMModel*
ScratchpadFactory::create_scratchpad(SpadArch&          arch,
                                     SpadArch::SpadKind kind)
{
  CCMModel* model = 0;
  if (arch.is_configured) {
    model = new CCMModel(kind, arch.start_addr, arch.size, arch.bus_width, arch.latency);
  }
  return model;
}


