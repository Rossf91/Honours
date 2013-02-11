//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  Memory mapped Keyboard device.
//
// =====================================================================


#ifndef _IODeviceKeyboard_h_
#define _IODeviceKeyboard_h_

#include <string>

#include "mem/mmap/IODevice.h"
#include "mem/mmap/IODeviceIrq.h"

#include "concurrent/Mutex.h"

#include <glib.h>

#define SCANSIZE	32

namespace arcsim {
  namespace mem {
    namespace mmap {
      
      typedef struct {
        uint8		scanIn;
        uint8		scanOut;
        uint32	scanBuf[SCANSIZE];
        uint8		ready;
      } ScancodeBuffer;     
      
      class IODeviceKeyboard : public arcsim::mem::mmap::IODevice
      {
      private:        
        // Device identifier
        //
        std::string id_;
        
        ScancodeBuffer              scanBuffer;  
        IODeviceIrq*                irq_device;
        arcsim::concurrent::Mutex   key_buffer_mutex;
        
      public:
        // Page Base start address
        //
        static const uint32 kPageBaseAddrIoKeyboardDevice;        
        // Memory size this device is responsible for
        //
        static const uint32 kIoKeyboardMemorySize;
        
        // Constructor
        //
        explicit IODeviceKeyboard(IODeviceIrq* irq_dev);
        
        // Destructor
        //
        ~IODeviceKeyboard();
        
        std::string& id();
        
        int keyboard_add_key_to_scanbuffer(guint keyval, uint8 keypress);

        int configure(simContext sim, IocContext sys_ctx, uint32 start_addr);
        
        int dev_stop();
        int dev_start();
        
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
        
        int mem_dev_read (uint32 addr, unsigned char* dest, int size);
        int mem_dev_write(uint32 addr, const unsigned char *data, int size);
        
        int mem_dev_read (uint32 addr, unsigned char* dest, int size, int agent_id);
        int mem_dev_write(uint32 addr, const unsigned char *data, int size, int agent_id);
        
      };                                          
      
} } } /* namespace arcsim::mem::mmap */

#endif /* _IODeviceKeyboard_h_ */ 
