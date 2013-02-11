//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
// 
// DESCRIPTION: Various generic utility functions and templates.
//
// =====================================================================

#ifndef INC_UTIL_UTIL_H_
#define INC_UTIL_UTIL_H_

#include <cstdio>
#include <stdint.h>

#include "api/types.h"

namespace arcsim {
  namespace util {
    
    // -------------------------------------------------------------------------
    // 'Address' conversion helper templates
    //
    
    // Conversion of 'Address' and other integral types into zero relative offset
    // types.
    //
    // NOTE: We use a template using a type parameter so it will be instantiated
    //       to the appropriate type.
    template <typename T>
    inline intptr_t offset_from(T x) {
      return x - static_cast<T>(0);
    }
    
    // Convesion of zero relative offsets into 'Address' and other integral types
    //
    template <typename T>
    inline T address_from(intptr_t x) {
      return static_cast<T>(static_cast<T>(0) + x);
    }
    
    // -------------------------------------------------------------------------
    // 'Address' alignment helper templates
    //
    
    // Compute largest multiple of m which is <= x
    //
    template <typename T>
    inline T round_down(T x, intptr_t m) {
      return address_from<T>(offset_from(x) & -m);
    }
    
    // Compute smallest multiple of m which is >= x
    //
    template <typename T>
    inline T round_up(T x, intptr_t m) {
      return round_down<T>(static_cast<T>(x + m - 1), m);
    }
    
    // -------------------------------------------------------------------------
    // Min/Max functions
    //
    
    // Templetized max implementation that returns the maximum of 'a' and 'b'.
    template <typename T>
    T max(T a, T b) {
      return a < b ? b : a;
    }
    
    // Templetized min implementation that returns the minimum of 'a' and 'b'.
    template <typename T>
    T min(T a, T b) {
      return a > b ? b : a;
    }
    
    // -------------------------------------------------------------------------
    // Bit manipulation functions
    // 
    // Note that many of these could be replaced with compiler intrinsics or single instruction 
    // inline asm blocks
    
    uint8 index_of_lowest_set_bit (uint32 word); 
    uint8 index_of_highest_set_bit(uint32 word);
    
} }

#endif  // INC_UTIL_UTIL_H_
