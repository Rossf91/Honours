//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Simple, light-weight and thread safe logging implementation.
//
// =====================================================================

#include <cstdio>

#include <string>
#include <sstream> // NOLINT(readability/streams)
#include <iostream>

#include "util/Os.h"
#include "util/Log.h"

namespace arcsim {
  namespace util {
    
    static const int kTimeStampBufferSize = 64; // size of timestamp buffer
    
    TLogLevel Log::reportingLevel = LOG_ERROR;
    
    const char* Log::kLogLevelStr[NUM_LOG_LEVELS] = {
      "ERROR: ",
      "WARNING: ",
      "INFO: ",
      "DEBUG:  ",
      "DEBUG1: ",
      "DEBUG2: ",
      "DEBUG3: ",
      "DEBUG4: ",
    };
    
    Log::Log()
    { /* EMPTY */ }

    std::ostringstream&
    Log::Get(TLogLevel level)
    {
      char     buf[kTimeStampBufferSize];
      Os::Date d;
      
      // retrieve current date
      Os::get_current_date(&d);
      // format timestamp
      snprintf(buf, sizeof(buf), "%.2d:%.2d:%.2d.%.6u ",
               d.hour, d.minute, d.second, static_cast<uint32>(d.usecond));
      
      // output log message
      os << buf << kLogLevelStr[level];
      messageLevel = level;
      return os;
    }
    
    Log::~Log()
    {
      if (messageLevel <= Log::ReportingLevel()) {
        os << std::endl;
        std::clog << os.str();
        std::clog.flush();
      }
    }

  }
}

