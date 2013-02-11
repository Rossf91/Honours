//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Main memory model class
//
// =====================================================================

#include "uarch/memory/MainMemoryModel.h"

#include "arch/Configuration.h"

#include "util/OutputStream.h"

// Main memory
//
MainMemoryModel::MainMemoryModel (SystemArch& sys_arch)
  : reads(0),
    writes(0)
{
  LatencyUtil::compute_read_latencies(read_lat, sys_arch.mem_dbw,
                                      sys_arch.mem_bus_clk_div,
                                      sys_arch.mem_latency, -1);
  LatencyUtil::compute_write_latencies(write_lat, sys_arch.mem_dbw,
                                       sys_arch.mem_bus_clk_div,
                                       sys_arch.mem_latency, -1);
}

MainMemoryModel::~MainMemoryModel()
{ /* EMPTY */ }

void
MainMemoryModel::clear()
{
  reads = 0;
  writes= 0;
}

void
MainMemoryModel::print_stats ()
{
  PRINTF() << "MAIN Memory Statistics"
    << "\n-----------------------------------------------------"
    << "\n Reads          " << reads
    << "\n Writes         " << writes
    << "\n\n";  
}  
