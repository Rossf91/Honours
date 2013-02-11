//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Class wrapping OS specific implementations.
//
// =============================================================================

#ifndef INC_OS_H_
#define INC_OS_H_

#include "api/types.h"


namespace arcsim {
  namespace util {
    
    class Os {
    public:
      typedef struct Date {
        int     year;
        int     month;    // [1..12]
        int     day;      // [1..31]
        int     hour;     // [0..23]
        int     minute;   // [0..59]
        int     second;   // [0..60]
        uint64  usecond;
      } Date;
      
      // Retrieve current date wrapped up in portable 'Date' struct
      //
      static bool get_current_date(Date* result);
      
      // -----------------------------------------------------------------------
      // Retrieve current milliseconds and microseconds
      //
      static uint64 get_current_time_millis();
      static uint64 get_current_time_micros();
      
      // -----------------------------------------------------------------------
      // Sleep methods
      //
      static void sleep_nanos (uint32 msecs);
      static void sleep_micros(uint32 usecs);
    };
  }
}

#endif // INC_OS_H_
