//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description: Portable time recording class.
//
// CounterTimer t("timer");
//
//  t.start();
//  while(condition) {
//    ... // do some operations
//  }
//  t.stop();
//
//  ... // do some operation that does not account to timing
//
//  t.start();
//  while(condition) {
//    ... // do some OTHER operations
//  }
//  t.stop();
//
//  printf("Elapsed time: %0.2f s", t.get_elapsed_seconds());
//
// =====================================================================

#ifndef INC_UTIL_COUNTERTIMER_H_
#define INC_UTIL_COUNTERTIMER_H_

#include "api/types.h"

#include "ioc/ContextItemInterface.h"

namespace arcsim {
  namespace util {
    
    class CounterTimer : public arcsim::ioc::ContextItemInterface
    {
    public:
      // Maximum name lenght
      //
      static const int kCounterTimerMaxNameSize = 256;

      explicit CounterTimer(const char* name);
      
      const uint8*   get_name() const { return name_;       }
      const Type     get_type() const { return arcsim::ioc::ContextItemInterface::kTCounterTimer; }

      // Start counting time
      //
      void start();
      
      // Stop counting time
      //
      void stop();
      
      // Reset CounterTimer to default values
      //
      void reset();
      
      // Retrieve elapsed time in seconds
      //
      double         get_elapsed_seconds()      const;
      
      // Retrieve elapsed time microseconds
      //
      inline uint64  get_elapsed_microseconds() const { return elapsed_; }
      
      
    private:
      uint8  name_[kCounterTimerMaxNameSize];      
      uint64 start_;
      uint64 stop_;
      uint64 elapsed_;
    };

} } // arcsim::util

#endif  // INC_UTIL_COUNTERTIMER_H_
