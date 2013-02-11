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

#ifndef INC_MEM_MMAP_IODEVICENULL_H_
#define INC_MEM_MMAP_IODEVICENULL_H_

#include <string>

#include "mem/mmap/IODevice.h"

namespace arcsim {
  namespace mem  {
    namespace mmap {
      
      class IODeviceNull : public arcsim::mem::mmap::IODevice
      {
      private:
        // Device identifier
        //
        std::string id_;

      public:
        
        IODeviceNull();
        ~IODeviceNull();
        
        std::string& id();
        
        int configure(simContext sim, IocContext sys_ctx, uint32 start_addr);
        
        int dev_start();
        int dev_stop();
        
        // ---------------------------------------------------------------------
        // Query range for which this MemoryDevice is responsible
        //
        uint32 get_range_begin()      const;
        uint32 get_range_end  ()      const;
        
        // ---------------------------------------------------------------------
        // Implement methods mandated by MemoryDeviceInterface
        //
        int mem_dev_init  (uint32 val);
        int mem_dev_clear (uint32 val);
        
        int mem_dev_read  (uint32 addr, unsigned char* dest, int size);
        int mem_dev_write (uint32 addr, const unsigned char *data, int size);

        int mem_dev_read  (uint32 addr, unsigned char* dest, int size, int agent_id);
        int mem_dev_write (uint32 addr, const unsigned char *data, int size, int agent_id);

      };

} } } // arcsim::mem::mmap

#endif  // INC_MEM_MMAP_IODEVICENULL_H_
