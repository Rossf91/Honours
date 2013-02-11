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


#include "uarch/memory/LatencyUtil.h"

void
LatencyUtil::compute_read_latencies(uint16 *lat,
                                    int     bus_width,
                                    int     bus_clk_div,
                                    int     latency,
                                    int     level)
{
  int blk_size, trans;
  
  for (int i=0; i <= MAX_BLK_BITS; ++i) {
    blk_size = 1 << i;
    if (blk_size % (bus_width/8)) {
      trans = (blk_size / (bus_width/8)) + 1;
    } else {
      trans = blk_size / (bus_width/8);
    }
    if (trans < 1) trans = 1;
    if (level == 1) { 
      lat[i] =  trans * latency;  // CPU L1 or CCM
    } else  {
      lat[i] = bus_clk_div + ((bus_clk_div > latency) ? bus_clk_div : latency) + (trans * bus_clk_div);
    }
  }

}


void
LatencyUtil::compute_write_latencies(uint16 *lat,
                                     int     bus_width,
                                     int     bus_clk_div,
                                     int     latency,
                                     int     level)
{
  int blk_size, trans;
  
  for (int i=0; i <= MAX_BLK_BITS; ++i) {
    blk_size = 1 << i;
    if (blk_size % (bus_width/8)) {
      trans = (blk_size / (bus_width/8));
    } else {
      trans = (blk_size / (bus_width/8)) - 1;
    }
    if (trans < 1) trans = 1;
    if (level == 1) {
      lat[i] = trans * latency;  // CPU L1 or CCM
    } else {
      lat[i] = bus_clk_div + (trans * bus_clk_div);
    }
  }
}

