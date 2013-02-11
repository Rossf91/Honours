//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================


#ifndef INC_MEM_MEMORYDEVICEINTERFACE_H_
#define INC_MEM_MEMORYDEVICEINTERFACE_H_

#include "api/types.h"

#include "mem/MemoryRangeDeviceInterface.h"

/**
 * Return values from memory device read/write methods.
 */  
#define IO_API_OK     0
#define IO_API_ERROR -1

/**
 * Pre-defined agent identifiers for callers of memory device read write methods.
 */  
#define IO_API_DEBUGGER_AGENT_ID       1
#define IO_API_BINARY_LOADER_AGENT_ID  2

/**
 * MemoryDeviceInterfacePtr type declaration acts as an opaque wrapper for
 * @see arcsim::mem::MemoryDeviceInterface pointer types.
 */  
typedef void* MemoryDeviceInterfacePtr;

/**
 * The simLoadMemoryDevice() function MUST be implemented by the shared library
 * that contains one or more MemoryDevices. It is called by an external agent
 * (e.g. a simulator) at the appropriate time in order to enable the shared library
 * that implements one or more MemoryDevices to register the devices via the
 * @see simRegisterMemoryDevice() function.
 *
 * @param simContext the simulation context
 *
 */
extern "C" DLLEXPORT void simLoadMemoryDevice (simContext sim_ctx);

/**
 * The simRegisterMemoryDevice() function is implemented by an external agent
 * (i.e. simulator) and should be called by the shared library implementing one
 * or more MemoryDevices in order to register a MemoryDevice with a simContext.
 *
 * Note that the simContext the MemoryDevice has been registered with takes
 * responsibility for destructing the MemoryDevice.
 * 
 * @param simContext the simulation context
 * @param pointer to fully instantiated @see MemoryDeviceInterface
 * @return IO_API_OK/IO_API_ERROR
 *
 */
extern "C" DLLEXPORT int simRegisterMemoryDevice (simContext               sim_ctx,
                                                  MemoryDeviceInterfacePtr mem_dev);

// -----------------------------------------------------------------------------

namespace arcsim {
  namespace mem {
    /**
     * MemoryDeviceInterface.
     * The MemoryDeviceInterface is an interface defining the methods/behaviour
     * of MemoryDevices. Each MemoryDevice implementing this interface that is
     * registered with the memory subsytem will automatically be called for
     * reads and writes to memory locations for the memory range reported by the
     * device.
     */
    class MemoryDeviceInterface : public MemoryRangeDeviceInterface
    {
    protected:
      // Constructor MUST be protected and empty!
      //
      MemoryDeviceInterface()
      { /* EMPTY */ }
      
    public:
      /**
       * Destructor of interface must be declared AND virtual so all implementations
       * of this interface can be destroyed correctly.
       */
      virtual ~MemoryDeviceInterface()
      { /* EMPTY */ }
            
      /**
       * This method is called when the device has been registered with the
       * memory subsystem. The value used to initialise main memory is passed
       * as an argument (NOTE: instances of this method may ignore to actualy
       * initialise the contents of memory to this value)
       *
       * @param value that memory contents can be initialised with.
       * @return IO_API_OK/IO_API_ERROR.
       */
      virtual int mem_dev_init  (uint32 value)                = 0;

      /**
       * This method is called when a instantiated simulation context is re-used
       * for another simulation run, indicating to the memory device that it should
       * clear its internal state and content. The value used to initialise main
       * memory is passed as an argument (NOTE: instances of this method may ignore
       * to actualy initialise the contents of memory to this value)
       *
       * @param value that memory contents can be initialised with.
       * @return IO_API_OK/IO_API_ERROR.
       */
      virtual int mem_dev_clear (uint32 value)                = 0;
      
      /**
       * This method is called for each read transaction triggered by simulation
       * that this memory device is registered for.
       *
       * @param physical memory address.
       * @param pointer to destination where the result of the read transaction will be stored.
       * @param read size in bytes (this is either 1,2, or 4).
       * @return IO_API_OK/IO_API_ERROR.
       */
      virtual int mem_dev_read  (uint32         addr,
                                 unsigned char* dest,
                                 int            size)         = 0;
      
      /**
       * This method is called for each write transaction triggered by simulation
       * that this memory device is registered for.
       *
       * @param physical memory address.
       * @param pointer to data that should be written.
       * @param write size in bytes (this is either 1,2, or 4).
       * @return IO_API_OK/IO_API_ERROR.
       */
      virtual int mem_dev_write (uint32               addr,
                                 const unsigned char *data,
                                 int                  size)   = 0; 
      
      // -----------------------------------------------------------------------
      
      /**
       * This method is called for each read transaction triggered by an external
       * agent.
       *
       * @param physical memory address.
       * @param pointer to destination where the result of the read transaction will be stored.
       * @param read size in bytes.
       * @param agent identifier.
       * @return IO_API_OK/IO_API_ERROR.
       */
      virtual int mem_dev_read  (uint32         addr,
                                 unsigned char* dest,
                                 int            size,
                                 int            agent_id)      = 0;
      
      /**
       * This method is called for each write transaction triggered by an external
       * agent.
       *
       * @param physical memory address.
       * @param pointer to data that should be written.
       * @param write size in bytes.
       * @param agent identifier.
       * @return IO_API_OK/IO_API_ERROR.
       */
      virtual int mem_dev_write (uint32               addr,
                                 const unsigned char* data,
                                 int                  size,
                                 int                  agent_id)      = 0;
      
    };
    
  } } /* namespace arcsim::mem */


#endif /* INC_MEM_MEMORYDEVICEINTERFACE_H_ */
