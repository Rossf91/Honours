//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  IOCCMemoryDevice base class impl. 
//
// =====================================================================

#include <iomanip>

#include "mem/ccm/CCMemoryDevice.h"
#include "sys/mem/CcMemory.h"

#include "util/Log.h"

namespace arcsim {
  namespace mem  {
    namespace ccm {
      
      CCMemoryDevice::CCMemoryDevice(const char* name, uint32 base, uint32 size)
        : base_(base),
          size_(size),
          mask_(size - 1),
          ccm_(new CCM(0, size, true))
      { // Initialise name
        uint32 i;
        for (i = 0; i < kCCMemoryDeviceMaxNameSize - 1 && name[i]; ++i)
          name_[i] = name[i];
        name_[i] = '\0';

        LOG(LOG_DEBUG)  << "[" << name_ << "] Instantiating - size: "
                        << size_
                        << " base: 0x" << std::hex << std::setw(8) << std::setfill('0')
                        << base_
                        << " mask: 0x" << std::hex << std::setw(8) << std::setfill('0')
                        << mask_;

      }
      
      CCMemoryDevice::~CCMemoryDevice()
      {
        if (ccm_ != 0) { delete ccm_; }
      }

      // ---------------------------------------------------------------------
      // For CCM devices the range query methods return 0 as it is the
      // responsibility of the processor/CCMManager to know for which range CCM
      // devices are registered.
      //
      uint32
      CCMemoryDevice::get_range_begin()              const
      {
        return 0;
      }
      uint32
      CCMemoryDevice::get_range_end  ()              const
      {
        return 0;
      }

      int
      CCMemoryDevice::mem_dev_init (uint32 val)
      {
        LOG(LOG_DEBUG4) << "[" << name_ << "] mem_dev_init";
        return IO_API_OK;
      }
      
      int
      CCMemoryDevice::mem_dev_clear (uint32 val)
      {
        LOG(LOG_DEBUG4) << "[" << name_ << "] mem_dev_clear";
        return IO_API_OK;
      }
      
      
      int
      CCMemoryDevice::mem_dev_read (uint32 phys_addr, unsigned char* dest, int size)
      {
        bool    success = true;
        uint32  data    = 0;
        uint32  addr;
        
        // Compute address, wrap around if necessary
        //
        addr = phys_addr & mask_;
        
        // Perform read operation
        //
        switch (size) {
          case 1: {
            success = ccm_->read8(addr, data);
            *((uint32*)(dest))  = (uint8)data;
            break;
          }
          case 2: {
            success = ccm_->read16(addr, data);
            *((uint32*)(dest)) = (uint16)data;
            break;
          }
          case 4: {
            success = ccm_->read32(addr, data);
            *((uint32*)(dest)) = (uint32)data;
            break;
          }
          default: {
            // Block wise READ
            //
            success = ccm_->read_block(addr, size, dest);
            if (!success) {
              LOG(LOG_ERROR) << "[" << name_ << "] mem_dev_read: illegal read size '"
              << size
              << "'";
            }
            break;
          }
        }
        
        LOG(LOG_DEBUG4) << "[" << name_ << "] mem_dev_read phys_addr: '0x"
                        << std::hex << std::setw(8) << std::setfill('0')
                        << phys_addr
                        << " addr: '0x"
                        << std::hex << std::setw(8) << std::setfill('0')
                        << addr
                        << "' data:'0x"
                        << std::hex << std::setw(8) << std::setfill('0')
                        << data
                        << "' size:"
                        << std::dec << size;
        
        return (success) ? IO_API_OK : IO_API_ERROR;
      }
      
      int
      CCMemoryDevice::mem_dev_write(uint32 phys_addr, const unsigned char *data, int size)
      {
        bool    success = true;
        uint32  addr;
        
        // Compute address, wrap around if necessary
        //
        addr = phys_addr & mask_;
        
        // Perform write operation
        //
        switch (size) {
          case 1: {
            success = ccm_->write8 (addr, *((uint8*)(data)));
            LOG(LOG_DEBUG4) << "[" << name_ << "] mem_dev_write phys_addr: '0x"
                            << std::hex << std::setw(8) << std::setfill('0')
                            << phys_addr
                            << " addr: '0x"
                            << std::hex << std::setw(8) << std::setfill('0')
                            << addr
                            << "' data:'0x"
                            << std::hex << std::setw(8) << std::setfill('0')
                            << (uint32)(*((uint8*)(data)))
                            << "' size:"
                            << std::dec << size;
            break;
          }
          case 2: {
            success = ccm_->write16(addr, *((uint16*)(data)));
            LOG(LOG_DEBUG4) << "[" << name_ << "] mem_dev_write phys_addr: '0x"
                            << std::hex << std::setw(8) << std::setfill('0')
                            << phys_addr
                            << " addr: '0x"
                            << std::hex << std::setw(8) << std::setfill('0')
                            << addr
                            << "' data:'0x"
                            << std::hex << std::setw(8) << std::setfill('0')
                            << (uint32)(*((uint16*)(data)))
                            << "' size:"
                            << std::dec << size;
            break;
          }
          case 4: {
            success = ccm_->write32(addr, *((uint32*)(data)));
            LOG(LOG_DEBUG4) << "[" << name_ << "] mem_dev_write phys_addr: '0x"
                            << std::hex << std::setw(8) << std::setfill('0')
                            << phys_addr
                            << " addr: '0x"
                            << std::hex << std::setw(8) << std::setfill('0')
                            << addr
                            << "' data:'0x"
                            << std::hex << std::setw(8) << std::setfill('0')
                            << (uint32)(*((uint32*)(data)))
                            << "' size:"
                            << std::dec << size;
            break;
          }
          default: {
            // Block wise WRITE
            //
            success = ccm_->write_block(addr, size, data);
            if (!success) {
              LOG(LOG_ERROR) << "[" << name_ << "] mem_dev_write: illegal write size '"
                             << size
                             << "'";
            }
            break;
          }
        }
        
        return (success) ? IO_API_OK : IO_API_ERROR;
      }
      
      // -----------------------------------------------------------------------
      // Debug read/write implementations
      //
      int
      CCMemoryDevice::mem_dev_read (uint32          addr,
                                        unsigned char*  dest,
                                        int             size,
                                        int             agent_id)
      {
        return mem_dev_read(addr, dest, size);
      }
      
      int
      CCMemoryDevice::mem_dev_write (uint32                addr,
                                         const unsigned char*  data,
                                         int                   size,
                                         int                   agent_id)
      {
        return mem_dev_write(addr, data, size);
      }

      
} } } /* namespace arcsim::mem::ccm */

