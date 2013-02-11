//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description: Portable time recording class. 
//
// =====================================================================

#include "Assertion.h"

#include "util/CounterTimer.h"

#include "util/Os.h"

namespace arcsim {
  namespace util {

    CounterTimer::CounterTimer(const char* name)
    : start_(0),
      stop_(0),
      elapsed_(0)
    {
      uint32 i;
      for (i = 0; i < kCounterTimerMaxNameSize - 1 && name[i]; ++i)
        name_[i] = static_cast<uint8>(name[i]);
      name_[i] = '\0';
    }

    void
    CounterTimer::reset()
    {
      start_   = 0;
      stop_    = 0;
      elapsed_ = 0;
    }
    
    void
    CounterTimer::start()
    {
      stop_  = 0;
      start_ = Os::get_current_time_micros();
    }
    
    void
    CounterTimer::stop()
    {
      stop_ = Os::get_current_time_micros();
      ASSERT(start_ <= stop_);
      // compute elapsed time
      elapsed_ += (stop_ - start_);
    }
    
    double
    CounterTimer::get_elapsed_seconds() const
    {
      return static_cast<double>(elapsed_) * 1.0e-6;
    }
    
} } // arcsim::util
