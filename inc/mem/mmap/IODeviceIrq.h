//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  Memory mapped IRQ device necessary for memory mapped IRQ devices.
//
// =====================================================================


#ifndef _IODeviceIrq_h_
#define _IODeviceIrq_h_

#include <string>

#include "mem/mmap/IODevice.h"

#include "api/ioc_types.h"

#include "concurrent/Mutex.h"


namespace arcsim {
  
  namespace ioc {
    class Context;
  }
  
  namespace mem {
    namespace mmap {
      
      typedef enum {
        IRQ_NOT_READY,
        IRQ_READY
      } IrqReadyState;

      
      class IODeviceIrq : public arcsim::mem::mmap::IODevice
      {
      private:        
        // Device identifier
        //
        std::string id_;
        
        // IRQ vector register
        //
        uint32   vector;
        
        // IRQ Number to assert
        //
        uint32 irq_number;
        
        // IRQ ready state
        //
        IrqReadyState state;
        
        // Processor context
        //
        IocContext    cpu_ctx;
        
        // Models memory mapped CPU ID
        //
        uint32   cpuid;
        uint32   cpunum;
        
        // Models memory mapped RTC
        //
        uint64 time_local; // need 64 bit resolution on localtime
        uint32 time_part;  // and a 32 bit part as required
                
        // Mutex for IRQ vector synchronisation
        //
        arcsim::concurrent::Mutex vector_mutex;
        
      public:
        // Page Base start address
        //
        static const uint32 kPageBaseAddrIoIrqDevice;
        
        // External IRQ number
        //
        static const uint32 kExtIrqLineIoIrqDevice;       
        
        // Memory size this device is responsible for
        //
        static const uint32 kIoIrqDeviceMemorySize;
                
        // Constructor
        //
        IODeviceIrq();
        
        // Destructor
        //
        ~IODeviceIrq();
        
        std::string& id();
        
        int configure(simContext sim, IocContext sys_ctx, uint32 addr);
        
        void set_irq(uint32 irq);
        
        void assert_ext_irq  (const uint8 irq_line);
        void rescind_ext_irq (const uint8 irq_line);
        
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
        
        int mem_dev_read  (uint32 addr, unsigned char* dest, int size);
        int mem_dev_write (uint32 addr, const unsigned char *data, int size);

        int mem_dev_read  (uint32 addr, unsigned char* dest, int size, int agent_id);
        int mem_dev_write (uint32 addr, const unsigned char *data, int size, int agent_id);

      };                                          
      
} } } /* namespace arcsim::mem::mmap */

#endif /* _IODeviceIrq_h_ */ 
