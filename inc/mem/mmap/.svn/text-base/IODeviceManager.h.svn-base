//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
// IODeviceManager class definition. 
//
// =====================================================================


#ifndef _IODeviceManager_h_
#define _IODeviceManager_h_

#include <vector>

#include "api/types.h"
#include "api/ioc_types.h"

class SystemArch;

namespace arcsim {
  namespace mem {
    namespace mmap {

      class IODevice; // FORWARD DECL
      
      class IODeviceManager
      {  
      private:
        // List of available devices
        std::vector<IODevice*> devices_available;
        
        // List of ready devices, ready devices can be started
        std::vector<IODevice*> devices_ready;
        
        bool started;

      public:
        IODeviceManager();
        ~IODeviceManager();

        bool create_devices(simContext  sim,
                            IocContext  sys_ctx,
                            SystemArch& sys_arch);
        bool destroy_devices();
        
        bool start_devices();
        bool stop_devices();

      };                                                   
      
} } } /* namespace arcsim::mem::mmap */

#endif /* _IODeviceManager_h_ */
