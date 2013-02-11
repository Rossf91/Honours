//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description: Base class for memory mapped IO devices.
//
// =====================================================================


#ifndef INC_MEM_MMAP_IODEVICE_H_
#define INC_MEM_MMAP_IODEVICE_H_

#include <string>

#include "api/types.h"
#include "api/ioc_types.h"

#include "mem/MemoryDeviceInterface.h"

namespace arcsim {
  namespace mem  {
    namespace mmap {
      
      class IODevice : public arcsim::mem::MemoryDeviceInterface
      {
      protected:
        simContext  sim_;
        IocContext  sys_ctx_;
        uint32      base_addr_;
        
        IODevice      (const IODevice & m);  // DO NOT COPY
        void operator=(const IODevice &);    // DO NOT ASSIGN
        
      public:
        
        IODevice();
        virtual ~IODevice()
        { /* EMPTY */ }
        
        // Return device identifier
        //
        virtual std::string& id()                           = 0;
        
        // ----------------------------------------------------------------
        // Configure device
        //
        virtual int configure(simContext sim, IocContext sys_ctx, uint32 addr)  = 0;

        // -----------------------------------------------------------------
        // Memory mapped IO devices 
        //
        virtual int dev_start()                                   = 0;
        virtual int dev_stop()                                    = 0;

      };                                                                            
      
} } } /* namespace arcsim::mem::mmap */

#endif /* INC_MEM_MMAP_IODEVICE_H_ */
