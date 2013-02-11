//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Thread-safe Counter classes responsible for maintaining and  instantiating
// all kinds of profiling counters in a generic way.
//
// There are two types of Counters, 32-bit counters (i.e. Counter), and
// 64-bit counters (i.e. Counter64).
//
// =====================================================================

#include "util/Counter.h"

#include <sstream>

namespace arcsim {
  namespace util {    
    // -------------------------------------------------------------------------
    // Counter
    //
    Counter::Counter(const char* name)
    : count_(0)
    {
      uint32 i;
      for (i = 0; i < kCounterMaxNameSize - 1 && name[i]; ++i)
        name_[i] = static_cast<uint8>(name[i]);
      name_[i] = '\0';
    }

    std::string
    Counter::to_string()
    {
      std::ostringstream buf;
      buf << name_ << ": " << count_;
      return buf.str();
    }
    // -------------------------------------------------------------------------
    // Counter64
    //
    Counter64::Counter64(const char* name)
    : count_(0)
    {
      uint32 i;
      // init name
      //
      for (i = 0; i < kCounterMaxNameSize - 1 && name[i]; ++i)
        name_[i] = static_cast<uint8>(name[i]);
      name_[i] = '\0';
    }
    
    std::string
    Counter64::to_string()
    {
      std::ostringstream buf;
      buf << name_ << ": " << count_;
      return buf.str();
    }
    
} } // namespace arcsim::util

