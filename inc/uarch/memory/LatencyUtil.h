//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Utility class for pre-computing read/write latencies
//
// =====================================================================

#ifndef INC_UARCH_MEMORY_LATENCYUTIL_H_
#define INC_UARCH_MEMORY_LATENCYUTIL_H_

#include "api/types.h"

#define MAX_BLK_BITS 12
#define MAX_WAYS     16


class LatencyUtil {
private:
  // Constructor is private to avoid instantiation 
  //
  LatencyUtil() { /* EMPTY */ }
  
  LatencyUtil(const LatencyUtil & m);     // DO NOT COPY
  void operator=(const LatencyUtil &);    // DO NOT ASSIGN

public:
  
  // Pre-calculate read latencies for different block sizes
  //
  static void compute_read_latencies(uint16 lat[MAX_BLK_BITS],
                                     int bus_width,
                                     int bus_clk_div,
                                     int latency,
                                     int level);
  
  // Pre-calculate write latencies for different block sizes
  //
  static void compute_write_latencies(uint16 lat[MAX_BLK_BITS],
                                      int bus_width,
                                      int bus_clk_div,
                                      int latency,
                                      int level);
  
  
};

#endif  // INC_UARCH_MEMORY_LATENCYUTIL_H_
