//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Everything that should be managed by our minimalistic and light-weight
// Dependency Injection (DI) framework must implement the ContextItemInterface.
//
// =====================================================================

#include "ioc/ContextItemInterface.h"

#include <cstring>

namespace arcsim {
  namespace ioc {
    
    
    // -------------------------------------------------------------------------
    // ContextItemInterfaceComparator implementation comparing ContextItems based
    // on their name
    //
    bool
    ContextItemInterfaceComparator::operator() (const ContextItemInterface* lhs,
                                                const ContextItemInterface* rhs) const
    { return (std::strcmp((char const*)lhs->get_name(),
                          (char const*)rhs->get_name()) < 0); }
    
    
  } } // namespace arcsim::ioc
