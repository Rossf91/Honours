//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Class wraping OS specific implementations.
//
// =============================================================================

#include <errno.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "util/Os.h"

#include "Assertion.h"

namespace arcsim {
  namespace util {
    bool
    Os::get_current_date(Date *date)
    {
      timeval     tv;
      struct tm   tm_result;
      struct tm*  ret;

      gettimeofday(&tv, (struct timezone *)0);
      ret = localtime_r(&tv.tv_sec, &tm_result);
      
      date->year   = tm_result.tm_year + 1900; // account for 1900 offset
      date->month  = tm_result.tm_mon  + 1;    // first month is '1' not '0'
      date->day    = tm_result.tm_mday;
      date->hour   = tm_result.tm_hour;
      date->minute = tm_result.tm_min;
      date->second = tm_result.tm_sec;
      date->usecond= tv.tv_usec;
      return ret != 0;
    }
   
    
    uint64
    Os::get_current_time_millis()
    {
      return get_current_time_micros() / 1000;
    }
    
    uint64
    Os::get_current_time_micros()
    {
      struct timeval tv;
      if (gettimeofday(&tv, (struct timezone *)0) < 0)
        return 0;
      return (static_cast<uint64>(tv.tv_sec) * 1000000) + tv.tv_usec;
    }
    
    void
    Os::sleep_nanos(uint32 nanos)
    {
      struct timespec ts;      
      ts.tv_sec  = (nanos / 1000);
      ts.tv_nsec = (nanos - (ts.tv_sec * 1000)) * 1000000L;
      
      // Use nanosleep to wait for a given time. This may not be portable, on
      // Windows we should call Sleep() or something similar.
      //
      // @see: http://pubs.opengroup.org/onlinepubs/007908799/xsh/nanosleep.html
      //
      nanosleep(&ts, NULL);
    }
    
    void
    Os::sleep_micros(uint32 usecs)
    {
      usleep(usecs);
    }

} }
