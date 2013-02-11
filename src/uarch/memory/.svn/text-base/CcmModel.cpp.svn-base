//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// CCM timing class
//
// =====================================================================

#include <cstdio>

#include "uarch/memory/CcmModel.h"

CCMModel::CCMModel (SpadArch::SpadKind  _kind,
                    int                 start_addr,
                    int                 size,
                    int                 bus_width,
                    int                 latency)
  : kind(_kind),
    reads(0),
    writes(0),
    ccm_low(start_addr),
    ccm_high(start_addr + size - 1)
{
  // Compite read/write latencies
  //
  LatencyUtil::compute_read_latencies(read_lat, bus_width, 1, latency, 1);
  LatencyUtil::compute_write_latencies(write_lat, bus_width, 1, latency, 1);
}

CCMModel::~CCMModel()
{ /* EMPTY */ }


uint32 CCMModel::get_size ()
{
  return (ccm_high + 1 - ccm_low);
}

void CCMModel::print_stats () {
  fprintf (stderr, "%s Memory Statistics\n", SpadArch::kind_tostring(kind));
  fprintf (stderr, "------------------------------\n");
  fprintf (stderr, " Reads          %10lld\n", reads); 
  fprintf (stderr, " Writes         %10lld\n", writes); 
}  
