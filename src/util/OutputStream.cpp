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

#include "util/OutputStream.h"

#include <cstdio>
#include <string>
#include <sstream> // NOLINT(readability/streams)

namespace arcsim {
  namespace util {
    
    OutputStream::OutputStream(FILE* _fd) :
      fd(_fd)
    { /* EMPTY */ }

    OutputStream::~OutputStream()
    {
      fputs(os.str().c_str(), fd);
      fflush(fd);
    }
} } // namespace arcsim::util
