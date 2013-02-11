//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// The BlockProfile class implements an arbitrary collection of BlockEntry
// objects, each of which maintains the profile information for a basic block.
//
// =====================================================================

#include "Assertion.h"

#include "profile/BlockEntry.h"


namespace arcsim {
  namespace profile {


    BlockEntry::BlockEntry(uint32 phys_addr_, uint32 virt_addr_, OperatingMode m_)
    : phys_addr(phys_addr_),
      virt_addr(virt_addr_),
      mode(m_),
      inst_count(0),
      size_bytes(0),
      interp_count(0),
      native_(0),
      module_(0),
      tstate_(kNotTranslated)
    { /* EMPTY */ }

    void
    BlockEntry::remove_translation ()
    {
      ASSERT(native_ != 0);
      ASSERT(tstate_ == kTranslated);
      tstate_ = kNotTranslated;
      native_ = 0;
    }


} } // arcsim::profile
