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


#include "arch/SpadArch.h"

// -----------------------------------------------------------------------------
// SpadKind strings
//
static const char* spad_string[SpadArch::kSpadKindCount] =
{ 
  "0",
  "ICCM",
  "DCCM",
  "UCCM",
};

// Return SpadKind as string
//
const char *
SpadArch::kind_tostring(SpadArch::SpadKind kind)
{
  switch (kind) {
    case SpadArch::kNoSpad:         return spad_string[0];
    case SpadArch::kSpadInstCcm:    return spad_string[1];
    case SpadArch::kSpadDataCcm:    return spad_string[2];
    case SpadArch::kSpadUnifiedCcm: return spad_string[3];
  }
}

// -----------------------------------------------------------------------------
// Constructor/Destructor
//
SpadArch::SpadArch()
  : is_configured(false),
    start_addr(0),
    size(0),
    bus_width(0),
    latency(0)
{ /* EMPTY */ }

SpadArch::~SpadArch()
{ /* EMPTY */ }


