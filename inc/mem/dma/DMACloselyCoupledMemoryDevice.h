//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================


#ifndef INC_MEM_DMA_DMACLOSELYCOUPLEDMEMORYEVICE_H_
#define INC_MEM_DMA_DMACLOSELYCOUPLEDMEMORYEVICE_H_

#include "mem/DirectMemoryAccessDeviceInterface.h"

namespace arcsim {
  namespace mem {
    namespace dma {
      
      class DMACloselyCoupledMemoryDevice : public DirectMemoryAccessDeviceInterface
      {
      public:
        // Maximum name lenght
        static const int DMACloselyCoupledMemoryDeviceMaxNameSize = 256;

      private:
        char           name_[DMACloselyCoupledMemoryDeviceMaxNameSize];
        const uint32   base_;
        const uint32   size_;
        const uint32   mask_;  // mask for address wrap around
        uint8* const   block_; // backing store
        
      public:
        
        explicit DMACloselyCoupledMemoryDevice(const char* name, uint32 base, uint32 size);
        ~DMACloselyCoupledMemoryDevice();
        
        /**
         * This method is called to map a simulated address to a raw host memory
         * pointer.
         *
         * @param physical memory address.
         * @param pointer to pointer that should be written.
         * @return IO_API_OK/IO_API_ERROR.
         */
        int dma_dev_location (uint32  addr, uint8** ptr);
        
        /**
         * This method is called when the device has been registered with the
         * memory subsystem. The value used to initialise main memory is passed
         * as an argument (NOTE: instances of this method may ignore to actualy
         * initialise the contents of memory to this value)
         *
         * @param value that memory contents can be initialised with.
         * @return IO_API_OK/IO_API_ERROR.
         */
        int dma_dev_init  (uint32 value);
        
        /**
         * This method is called when a instantiated simulation context is re-used
         * for another simulation run, indicating to the DMA device that it should
         * clear its internal state and content. The value used to initialise main
         * memory is passed as an argument (NOTE: instances of this method may ignore
         * to actualy initialise the contents of memory to this value)
         *
         * @param value that memory contents can be initialised with.
         * @return IO_API_OK/IO_API_ERROR.
         */
        int dma_dev_clear (uint32 value);

        
        
      };
      
      
} } } /* namespace arcsim::mem::dma */


#endif

