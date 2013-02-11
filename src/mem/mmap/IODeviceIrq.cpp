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

#include <utility>

#include <cstring>
#include <cstdlib>   // for atoi()
#include <time.h>    // for RTC of host system

#include "api/api_funs.h"

#include "api/ioc/api_ioc.h"
#include "api/irq/api_irq.h"

#include "mem/MemoryDeviceInterface.h"
#include "mem/mmap/IODevice.h"
#include "mem/mmap/IODeviceIrq.h"

#define CPU_ID_OPTION   "-cpuid"
#define CPU_NUM_OPTION  "-cpunum"


namespace arcsim {
  namespace mem {
    namespace mmap {

      const uint32 IODeviceIrq::kPageBaseAddrIoIrqDevice = 0xFF000000;
      const uint32 IODeviceIrq::kIoIrqDeviceMemorySize   = 0x00002000;

      // External IRQ number
      //
      const uint32 IODeviceIrq::kExtIrqLineIoIrqDevice   = 9;

      // 'Devices' mapped at various locations
      //
      enum IODeviceIrqAddrFields {
        kAddrCpuId      = 0x4,   // CPU number register
        kAddrIrq        = 0x0,   // IRQ register
        kAddrCpuNum     = 0x8,   // NUM CPU register
        kAddrRtcLow     = 0x10,  // real time clock low 32 bits
        kAddrRtcHigh    = 0x14  // real time clock high 32
      };

      
      // ---------------------------------------------------------------------
      // Constructor
      //
      IODeviceIrq::IODeviceIrq() :
        id_(std::string("irq")),
        vector(0),
        cpuid(0),
        cpunum(1),
        cpu_ctx(0),
        irq_number(kExtIrqLineIoIrqDevice),
        state(IRQ_NOT_READY)
      { /* EMPTY */ }
      
      IODeviceIrq::~IODeviceIrq()
      { /* EMPTY */ }

      
      std::string&
      IODeviceIrq::id()
      {
        return id_;
      }
      
      int
      IODeviceIrq::configure(simContext sim, IocContext sys_ctx, uint32 addr)
      {
        sim_        = sim;
        base_addr_  = addr;
        sys_ctx_    = sys_ctx;
        
        IocContext mod_ctx = iocGetContext(sys_ctx, 0);
        cpu_ctx = iocGetContext(mod_ctx, 0);
        
        if (simPluginOptionIsSet(sim, CPU_ID_OPTION)) {
          cpuid = atoi(simPluginOptionGetValue(sim, CPU_ID_OPTION));
        }
        
        if (simPluginOptionIsSet(sim, CPU_NUM_OPTION)) {
          cpunum = atoi(simPluginOptionGetValue(sim, CPU_NUM_OPTION));
        }

        return IO_API_OK;
      }
      
      void IODeviceIrq::set_irq(uint32 irq)
      {
        irq_number = irq;
      }
          
      // ---------------------------------------------------------------------
      // Query range for which this MemoryDevice is responsible
      //
      uint32
      IODeviceIrq::get_range_begin()      const
      {
        return base_addr_;
      }
      
      uint32
      IODeviceIrq::get_range_end  ()      const
      {
        return (base_addr_ + kIoIrqDeviceMemorySize);
      }
      
      int IODeviceIrq::dev_start() { return IO_API_OK; }
      int IODeviceIrq::dev_stop()  { return IO_API_OK; }

      // -----------------------------------------------------------------------
      // Init IODeviceIrq
      //
      int
      IODeviceIrq::mem_dev_init(uint32 val)
      {
        state = IRQ_READY;
        return IO_API_OK;
      }

      int
      IODeviceIrq::mem_dev_clear(uint32 val)
      {
        return IO_API_OK;
      }
      
      // -----------------------------------------------------------------------
      // Read from IODeviceIrq
      //
      int
      IODeviceIrq::mem_dev_read(uint32 addr, unsigned char *dest, int size)
      {
        uint32 saddr = addr - base_addr_;
                
        switch (saddr) {     
            // Someone is interested in the CPUID
            //
          case kAddrCpuId: {
            memcpy(dest, &cpuid, size);
            break;
          }
            // Some wants to read the IRQ vector
            //
          case kAddrIrq: {
            if (state == IRQ_READY) {
              // MUTEX START ---------------------------------------------------
              vector_mutex.acquire(); 
              
              memcpy(dest, &vector, size);
              
              vector_mutex.release();
              // MUTEX END   ---------------------------------------------------
            }
            break;
          }
            // Someone is interested in the CPU number
            //
          case kAddrCpuNum: {
            memcpy(dest, &cpunum, size);
            break;
          }
            // Memory mapped real-time clock
            //
          case kAddrRtcLow:
          case kAddrRtcHigh:
          {
            time_local = static_cast<uint64>(clock());
            
            // Adjust for changes in resolution - we need to map it to usecond
            // resolution if the host clock is not usec resolution
            //
            time_local *= (1000000 / CLOCKS_PER_SEC);
            
            if (saddr == kAddrRtcLow) {
              // Use bottom 32 bits
              //
              time_part  = static_cast<uint32>(time_local);
            } else {
              // Use high 32 bits
              //
              time_part  = static_cast<uint32>(time_local >> 32);
            }
            
            memcpy(dest, &time_part, size);
            
            break;
          }
          default: break;
        }
        
        return IO_API_OK;
      }
      
      // -----------------------------------------------------------------------
      // Write to IODeviceIrq - nothing happens because one can not write to this
      // register
      //
      int
      IODeviceIrq::mem_dev_write(uint32 addr, const unsigned char *data, int size)
      {
        return IO_API_OK;
      }
      
      // -----------------------------------------------------------------------
      // Debug read/write implementations
      //
      int
      IODeviceIrq::mem_dev_read  (uint32 addr, unsigned char* dest, int size, int agent_id)
      {
        return mem_dev_read(addr, dest, size);
      }
      
      int
      IODeviceIrq::mem_dev_write (uint32 addr, const unsigned char *data, int size, int agent_id)
      {
        return mem_dev_write(addr, data, size);
      }

      
      void
      IODeviceIrq::assert_ext_irq(const uint8 _id)
      {
        // MUTEX START ---------------------------------------------------------
        vector_mutex.acquire();
        
        // Update IRQ vector register
        //
        vector = ( vector | ( 1 << _id ) );
        
        vector_mutex.release();
        // MUTEX END   ---------------------------------------------------------

        // Assert interrupt - always do that on CPU 0, the hardware picks a random
        // CPU.
        //
        irqAssertInterrupt(cpu_ctx, irq_number);
      }

      void
      IODeviceIrq::rescind_ext_irq(const uint8 _id)
      {
        // MUTEX START ---------------------------------------------------------
        vector_mutex.acquire();
        
        // Update IRQ vector register
        //
        vector = ( vector & ( ~(uint32)(1 << _id) ) );
        
        vector_mutex.release();
        // MUTEX END   ---------------------------------------------------------
        
        
        // Rescind external interrupt if no other device is still waiting to be
        // dealt with.
        //
        if (vector == 0) {
          irqRescindInterrupt(cpu_ctx, irq_number);
        }
      }
      
} } } /* namespace arcsim::mem::mmap */


