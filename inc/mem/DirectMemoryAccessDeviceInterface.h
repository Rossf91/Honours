//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================


#ifndef INC_MEM_DIRECTMEMORYACCESSDEVICEINTERFACE_H_
#define INC_MEM_DIRECTMEMORYACCESSDEVICEINTERFACE_H_

#include "api/types.h"

// -----------------------------------------------------------------------------

namespace arcsim {
  namespace mem {
    /**
     * DirectMemoryAccessDeviceInterface.
     * The DirectMemoryAccessDeviceInterface is an interface defining the methods
     * and behaviour of DirectMemoryAccessDevices. Such devices provide a method
     * to return the raw host pointer to the backing store for a given simulated
     * address.
     */
    class DirectMemoryAccessDeviceInterface
    {
    protected:
      // Constructor MUST be protected and empty!
      //
      DirectMemoryAccessDeviceInterface()
      { /* EMPTY */ }
      
    public:
      /**
       * Destructor of interface must be declared AND virtual so all implementations
       * of this interface can be destroyed correctly.
       */
      virtual ~DirectMemoryAccessDeviceInterface()
      { /* EMPTY */ }
      
      /**
       * This method is called to map a simulated address to a raw host memory
       * pointer.
       *
       * @param physical memory address.
       * @param pointer to pointer that should be written.
       * @return IO_API_OK/IO_API_ERROR.
       */
      virtual int dma_dev_location (uint32  addr, uint8** ptr) = 0;
      
      /**
       * This method is called when the device has been registered with the
       * memory subsystem. The value used to initialise main memory is passed
       * as an argument (NOTE: instances of this method may ignore to actualy
       * initialise the contents of memory to this value)
       *
       * @param value that memory contents can be initialised with.
       * @return IO_API_OK/IO_API_ERROR.
       */
      virtual int dma_dev_init  (uint32 value)                = 0;
      
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
      virtual int dma_dev_clear (uint32 value)                = 0;

      
    };
    
  } } /* namespace arcsim::mem */


#endif /* INC_MEM_DIRECTMEMORYACCESSDEVICEINTERFACE_H_ */
