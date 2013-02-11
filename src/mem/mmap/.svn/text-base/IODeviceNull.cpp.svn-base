//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  Memory mapped null device returning '0' for each memory location
//  and ignoring ALL writes.
//
// =====================================================================

#include <iomanip>

#include "mem/mmap/IODeviceNull.h"

#include "util/Log.h"

namespace arcsim {
  namespace mem  {
    namespace mmap {

      IODeviceNull::IODeviceNull()
      : id_(std::string("null"))
      { /* EMPTY */ }

      IODeviceNull::~IODeviceNull()
      { /* EMPTY */ }
      
      int
      IODeviceNull::configure(simContext sim, IocContext sys_ctx, uint32 start_addr)
      {
        return IO_API_OK;
      }
      
      int
      IODeviceNull::dev_start()
      {
        return IO_API_OK;
      }
      
      int
      IODeviceNull::dev_stop()
      {
        return IO_API_OK;
      }

      std::string&
      IODeviceNull::id()
      {
        return id_;
      }
      
      // ---------------------------------------------------------------------
      // Query range for which this MemoryDevice is responsible
      //
      uint32
      IODeviceNull::get_range_begin()      const
      {
        return 0x00000000;
      }
      uint32
      IODeviceNull::get_range_end  ()      const
      {
        return 0xFFFFFFFF;
      }
      
      // ---------------------------------------------------------------------
      // Implement methods mandated by MemoryDeviceInterface
      //
      int
      IODeviceNull::mem_dev_init  (uint32 val)
      {
        return IO_API_OK;
      }
      
      int
      IODeviceNull::mem_dev_clear (uint32 val)
      {
        return IO_API_OK;
      }
      
      int
      IODeviceNull::mem_dev_read  (uint32 addr, unsigned char* dest, int size)
      {
        bool success = true;
        // Perform read operation
        //
        switch (size) {
          case 1: { *((uint32*)(dest))  = (uint8)0; break; }
          case 2: { *((uint32*)(dest)) = (uint16)0; break; }
          case 4: { *((uint32*)(dest)) = (uint32)0; break; }
          default: {
            LOG(LOG_ERROR) << "[NULLDEV] mem_dev_read: illegal read size '"
                           << size
                           << "'";
            success = false;
            break;
          }
        }
        LOG(LOG_DEBUG2) << "[NULLDEV] mem_dev_read: addr '0x"
                        << std::hex << std::setw(8) << std::setfill('0') << addr;
        return (success) ? IO_API_OK : IO_API_ERROR;
      }
      
      int
      IODeviceNull::mem_dev_write (uint32 addr, const unsigned char *data, int size)
      {
        LOG(LOG_DEBUG2) << "[NULLDEV] mem_dev_write: addr '0x"
                        << std::hex << std::setw(8) << std::setfill('0') << addr;
        return IO_API_OK;
      }

      int
      IODeviceNull::mem_dev_read  (uint32 addr, unsigned char* dest, int size, int agent_id)
      {
        return mem_dev_read(addr, dest, size);
      }
      
      int
      IODeviceNull::mem_dev_write (uint32 addr, const unsigned char *data, int size, int agent_id)
      {
        return mem_dev_write(addr, data, size);
      }

      
} } } // arcsim::mem::mmap
