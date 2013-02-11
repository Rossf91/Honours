//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Simple and thread safe stream implementation used for outputting strings.
//
// =====================================================================

#ifndef INC_UTIL_OUTPUTSTREAM_H_
#define INC_UTIL_OUTPUTSTREAM_H_

#include <cstdio>
#include <string>
#include <sstream> // NOLINT(readability/streams)

// This macro MUST be used for output. Here is an example:
// 
//  PRINTF() << "ArcSim: " << some_string;
//
//  FPRINTF(stderr) << "ArcSim Error: " << error_str;
//
#define FPRINTF(fd) arcsim::util::OutputStream(fd).Get()
#define PRINTF()    arcsim::util::OutputStream().Get()

namespace arcsim {
  namespace util {
    
    class OutputStream {
    public:
      explicit OutputStream(FILE* fd = stdout);

      virtual ~OutputStream();
      
      inline std::ostringstream& Get() { return os; }
      
    protected:
      std::ostringstream os;
      FILE*              fd;
    private:
      
      OutputStream(const OutputStream&);              // DO NOT COPY
      OutputStream& operator =(const OutputStream&);  // DO NOT ASSIGN
    };
    
} } // namespace arcsim::util

#endif  // INC_UTIL_OUTPUTSTREAM_H_
