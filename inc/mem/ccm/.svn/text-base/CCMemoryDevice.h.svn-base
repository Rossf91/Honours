//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  Base class for CCM devices
//
//
// =====================================================================


#ifndef _IOCCMemoryDevice_h_
#define _IOCCMemoryDevice_h_

#include "mem/MemoryDeviceInterface.h"

// Forward declarations
//
class CCM;

namespace arcsim {
  namespace mem  {
    namespace ccm {
      
      class CCMemoryDevice : public arcsim::mem::MemoryDeviceInterface
      {
      public:
        // Maximum name lenght
        static const int kCCMemoryDeviceMaxNameSize = 256;
        
      private:
        char              name_[kCCMemoryDeviceMaxNameSize];
        const uint32      base_;
        const uint32      size_;
        const uint32      mask_; // mask for address wrap around
        CCM * const       ccm_;  // backing store
        
        CCMemoryDevice(const CCMemoryDevice & m);  // DO NOT COPY
        void operator=(const CCMemoryDevice &);    // DO NOT ASSIGN
              
      public:
        explicit CCMemoryDevice(const char* name, uint32 base, uint32 size);
        
        virtual ~CCMemoryDevice();
                
        // ---------------------------------------------------------------------
        // For CCM devices the range query methods return 0 as it is the
        // responsibility of the processor/CCMManager to know for which range CCM
        // devices are registered.
        //
        uint32 get_range_begin()              const;
        uint32 get_range_end  ()              const;
        
        int mem_dev_init (uint32 val);
        int mem_dev_clear(uint32 val);
        
        int mem_dev_read (uint32 addr, unsigned char* dest, int size);
        int mem_dev_write(uint32 addr, const unsigned char *data, int size); 
        
        int mem_dev_read (uint32 addr, unsigned char* dest, int size, int agent_id);
        int mem_dev_write(uint32 addr, const unsigned char *data, int size, int agent_id); 

        
      };                                                                            
      
} } } /* namespace arcsim::mem::ccm */

#endif /* _IOCCMemoryDevice_h_ */

