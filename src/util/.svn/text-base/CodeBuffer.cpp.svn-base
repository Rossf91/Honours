//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// CodeBuffer implementation that should be used as a replacement sprintf(buf,...)
// and snprintf(buf,...). It provides all the luxury of snprintf(buf,...) and
// snprintf(buf,...) but is much easier to use and SAFER.
//
// NOTE: This buffer implemenation is NOT thread-safe, so do not try to append
//       to a CodeBuffer instance from multiple threads!
//
// =============================================================================

#include <cstdarg>
#include <cstdio>

#include "util/CodeBuffer.h"

namespace arcsim {
  namespace util {

    // -------------------------------------------------------------------------
    // Constructor
    //
    CodeBuffer::CodeBuffer(uint32 size)
    // NOTE: Declaration and initialisation order matters!
    //
    : size_(size),            // store size of buffer
      buf_(new char[size]),   // HEAP allocate buffer
      end_(&buf_[size-1]),    // store end_ of buffer
      pos_(buf_),             // point current pos_ to beginning
      state_(kValid)
    {
      buf_[0] = '\0';         // properly initialise buffer
    }
    
    // -------------------------------------------------------------------------
    // Destructor
    //
    CodeBuffer::~CodeBuffer()
    {
      delete [] buf_;         // de-allocate HEAP space for buffer
    }
    
    // -------------------------------------------------------------------------
    // Buffer append method
    //
    CodeBuffer&
    CodeBuffer::append(const char *fmt, ...)
    {
      va_list ap;
      int     size;
      
      va_start(ap, fmt);
      
      // Is there enough room left?
      //
      if (state_ && (size = end_ - pos_) > 0) {
        
        // Write to internal buffer
        //
        int ret = vsnprintf(pos_, size, fmt, ap);
        
        // If write was successful we modify internal buffer pointer
        // NOTE: 'ret' may be zero in which case nothing was written
        //
        if (ret >= 0 && ret < size) {
          pos_ += ret;
        } else {
          state_ = kFull;
        }
      }
      
      va_end(ap);
      
      return *this;
    }
    
} } // arcsim::util

