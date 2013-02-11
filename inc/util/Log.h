//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Simple, light-weight and thread safe logging class. The reason why it
// is thread safe (even though we do NOT use explicit synchronisation) is
// because the POSIX Standard requires that by default the stream operations
// are atomic. Issuing two stream operations for the same stream in two threads
// at the same time will cause the operations to be executed as if they were
// issued sequentially.
//
// NOTE: Log output is line oriented! There is no need to append '\n'!!!
//
// =====================================================================


#ifndef INC_UTIL_LOG_H_
#define INC_UTIL_LOG_H_

#include <sstream> // NOLINT(readability/streams)

// This macro MUST be used for logging. Here is an example:
// 
//  LOG(LOG_INFO) << "ArcSim " << some_string;
//
#define LOG(level) \
if (level > arcsim::util::Log::ReportingLevel()) ; \
else arcsim::util::Log().Get(level)

enum TLogLevel {
  LOG_ERROR = 0,
  LOG_WARNING,
  LOG_INFO,
  LOG_DEBUG,
  LOG_DEBUG1,
  LOG_DEBUG2,
  LOG_DEBUG3,
  LOG_DEBUG4,
  NUM_LOG_LEVELS
};


namespace arcsim {
  namespace util {
    
    class Log {
    public:
      
      Log();
      virtual ~Log();
      
      std::ostringstream& Get(TLogLevel level = LOG_INFO);
      
      inline static TLogLevel& ReportingLevel() { return reportingLevel; }
      static TLogLevel reportingLevel;
      static const char* kLogLevelStr[NUM_LOG_LEVELS];
      
    protected:
      std::ostringstream os;
    private:
      Log(const Log&);              // DO NOT COPY
      Log& operator =(const Log&);  // DO NOT ASSIGN

      TLogLevel messageLevel;
    };
    
} } // namespace arcsim::util

#endif  // INC_UTIL_LOG_H_
