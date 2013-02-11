//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#include <iomanip>
#include <cstring>

#include "Assertion.h"

#include "mem/MemoryDeviceInterface.h"
#include "mem/dma/DMACloselyCoupledMemoryDevice.h"

#include "util/Allocate.h"
#include "util/Log.h"

namespace arcsim {
  namespace mem {
    namespace dma {

      DMACloselyCoupledMemoryDevice::DMACloselyCoupledMemoryDevice(const char* name, 
                                                                   uint32      base,
                                                                   uint32      size)
      : base_(base),
        size_(size),
        mask_(size - 1),
        block_((uint8*)arcsim::util::Malloced::NewAligned(size))
      { // Initialise name
        uint32 i;
        for (i = 0; i < DMACloselyCoupledMemoryDeviceMaxNameSize - 1 && name[i]; ++i)
          name_[i] = name[i];
        name_[i] = '\0';
        
        ASSERT(block_ && "DMACloselyCoupledMemoryDevice allocation of backing store failed.");
        LOG(LOG_DEBUG)  << "[" << name_ << "-DMA] Instantiating - size: "
            << size_
            << " base: 0x" << std::hex << std::setw(8) << std::setfill('0')
            << base_
            << " mask: 0x" << std::hex << std::setw(8) << std::setfill('0')
            << mask_;
      }
      
      DMACloselyCoupledMemoryDevice::~DMACloselyCoupledMemoryDevice()
      {
        if (block_) arcsim::util::Malloced::DeleteAligned(block_);
      }

      int
      DMACloselyCoupledMemoryDevice::dma_dev_init(uint32 value)
      {
        std::memset(block_, value, size_);
        return IO_API_OK;
      }
      
      int
      DMACloselyCoupledMemoryDevice::dma_dev_clear(uint32 value)
      {
        std::memset(block_, value, size_);
        return IO_API_OK;
      }
      
      // -----------------------------------------------------------------------
      // Return a raw pointer to the location within the backing store for
      // a given address.
      //
      int
      DMACloselyCoupledMemoryDevice::dma_dev_location (uint32 addr, uint8** ptr)
      {
        LOG(LOG_DEBUG)
                << "[" << name_ << "-DMA] Retrieving raw pointer to - phys_addr: 0x"
                << std::hex << std::setw(8) << std::setfill('0')
                << addr
                << " addr: 0x" << std::hex << std::setw(8) << std::setfill('0')
                << (addr & mask_)
                << " mask: 0x" << std::hex << std::setw(8) << std::setfill('0')
                << mask_;

        // Set ptr to appropriate location within backing store
        //
        *ptr = block_ + (addr & mask_);
        
        return IO_API_OK;
      }
      
      
} } } /* namespace arcsim::mem::dma */
