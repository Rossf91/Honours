//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================

#include "mem/MemoryDeviceInterface.h"

#include "system.h"

// -----------------------------------------------------------------------------
// Register MemoryDevices with appropriate system
//
int
simRegisterMemoryDevice(simContext               sim_ctx,
                        MemoryDeviceInterfacePtr mem_dev)
{ // Cast simulation context properly
  //
  System* sys = reinterpret_cast<System*>(sim_ctx);
  
  // Cast to memory device interface
  //
  arcsim::mem::MemoryDeviceInterface* mem_dev_if
  = reinterpret_cast<arcsim::mem::MemoryDeviceInterface*>(mem_dev);
  
  // Register device with Memory subsytem
  //
  sys->ext_mem->register_memory_device(mem_dev_if);
  
  return IO_API_OK;
}
