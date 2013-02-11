//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================

#include "util/Util.h"

namespace arcsim {
  namespace util {
    
    uint8 index_of_lowest_set_bit(uint32 word)
    {
      // From: http://graphics.stanford.edu/~seander/bithacks.html
      
      uint32 v = word;  // 32-bit word input to count zero bits on right
      uint32 c = 32;    // c will be the number of zero bits on the right
      v &= -signed(v);
      if (v) c--;
      if (v & 0x0000FFFF) c -= 16;
      if (v & 0x00FF00FF) c -= 8;
      if (v & 0x0F0F0F0F) c -= 4;
      if (v & 0x33333333) c -= 2;
      if (v & 0x55555555) c -= 1;
      return c;
    }
    
    uint8 index_of_highest_set_bit(uint32 word)
    {
      // From: http://graphics.stanford.edu/~seander/bithacks.html
      
      uint32 v = word;  // 32-bit value to find the log2 of 
      const uint32 b[] = {0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000};
      const uint32 S[] = {1, 2, 4, 8, 16};

      uint32 r = 0;                   // result of log2(v) will go here
      for (int i = 4; i >= 0; i--) {  // unroll for speed...
        if (v & b[i]) {
          v >>= S[i];
          r |= S[i];
        } 
      }
      return r;
    }
    
  }
}
