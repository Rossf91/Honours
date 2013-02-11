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

#ifndef INC_UTIL_CODEBUFFER_H_
#define INC_UTIL_CODEBUFFER_H_

#include "api/types.h"

namespace arcsim {
  namespace util {
    
    class CodeBuffer {
    public:
      
      enum State { kFull  = 0x0, kValid = 0x1 };
      
      // Default buffer size is 512K (i.e. 0x00080000)
      //
      static const uint32 kBufferSize = 0x00080000;
      
      explicit CodeBuffer(uint32 size = kBufferSize);
      ~CodeBuffer();
      
      // Safely append to code buffer
      //
      CodeBuffer& append (const char * fmt, ...);

      // Retrieve buffer size
      //
      inline uint32 get_size()               const { return size_;  }
      
      // Retrieve buffer state
      //
      inline State get_state()               const { return state_; }
      
      // Check if buffer is valid or full
      //
      inline bool is_valid()                 const { return state_ == kValid; }
      inline bool is_full()                  const { return state_ == kFull; }
      
      // Get non modifiable constant pointer to buffer
      //
      inline const char * const get_buffer() const { return buf_;   }
      
      // Clear out buffer
      //
      inline void clear()
      {
        state_  = kValid;
        buf_[0] = '\0';
        pos_    = buf_;
      }
      
    private:
      // NOTE: Declaration order matters!
      //
      const uint32 size_;    // size of buffer
      char * const buf_;     // pointer to beginning of buffer
      char * const end_;     // pointer to end of buffer
      char *       pos_;     // current position in buffer
      
      State        state_;   // state of buffer
      
    };
    
} } // arcsim::util


#endif  // INC_UTIL_CODEBUFFER_H_
