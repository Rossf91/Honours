//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================


#ifndef INC_MEM_MEMORYRANGEDEVICEINTERFACE_H_
#define INC_MEM_MEMORYRANGEDEVICEINTERFACE_H_

#include "api/types.h"

// -----------------------------------------------------------------------------

namespace arcsim {
  namespace mem {
    /**
     * MemoryRangeDeviceInterface.
     * The MemoryRangeDeviceInterface is an interface defining the methods/behaviour
     * of MemoryRangeDevices
     */
    class MemoryRangeDeviceInterface
    {
    protected:
      // Constructor MUST be protected and empty!
      //
      MemoryRangeDeviceInterface()
      { /* EMPTY */ }
      
    public:
      /**
       * Destructor of interface must be declared AND virtual so all implementations
       * of this interface can be destroyed correctly.
       */
      virtual ~MemoryRangeDeviceInterface()
      { /* EMPTY */ }
      
      /**
       * Get start address of memory range this device is responsible for. The
       * value returned by this method must not change.
       * @return start address of memory range.
       */
      virtual uint32 get_range_begin()              const       = 0;
      
      /**
       * Get end address of memory range this device is responsible for. The
       * value returned by this method must not change.
       * @return end address of memory range.
       */
      virtual uint32 get_range_end  ()              const       = 0;

    };
    
  } } /* namespace arcsim::mem */

#endif  //INC_MEM_MEMORYRANGEDEVICEINTERFACE_H_
