//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
// IODeviceManager impl. 
//
// =====================================================================


#include "mem/mmap/IODeviceManager.h"
#include "mem/mmap/IODevice.h"

#include "mem/mmap/IODeviceUart.h"
#include "mem/mmap/IODeviceNull.h"

#include "arch/SystemArch.h"

#include "api/api_funs.h"

#include "util/Log.h"

#ifdef ENABLE_IO_DEVICES
# include "mem/mmap/IODeviceIrq.h"
# include "mem/mmap/IODeviceScreen.h"
# include "mem/mmap/IODeviceSound.h"
# include "mem/mmap/IODeviceKeyboard.h"
#endif

namespace arcsim {
  namespace mem {
    namespace mmap {
      
      IODeviceManager::IODeviceManager() :
        started(false)
      { /* EMPTY */ }
      
      IODeviceManager::~IODeviceManager()
      {
        if (started) {
          stop_devices();
        }
        destroy_devices();
      }                                                        
      
      
      bool
      IODeviceManager::create_devices(simContext    sim,
                                      IocContext    sys_ctx,
                                      SystemArch&   sys_arch)
      { 
        // Uart0 device for linux simulation
        //
        IODeviceUart* uart_dev = new IODeviceUart();
        devices_available.push_back(uart_dev);

        if (sys_arch.sim_opts.builtin_mem_dev_list.find(uart_dev->id()) != sys_arch.sim_opts.builtin_mem_dev_list.end()) {
          uart_dev->configure(sim, sys_ctx, uart_dev->kPageBaseAddrIoUartDevice);
          devices_ready.push_back(uart_dev);
        }
#ifdef ENABLE_IO_DEVICES
      
        // External IRQ Device
        //
        IODeviceIrq* irq_dev = new IODeviceIrq();
        devices_available.push_back(irq_dev);

        if (sys_arch.sim_opts.builtin_mem_dev_list.find(irq_dev->id()) != sys_arch.sim_opts.builtin_mem_dev_list.end()) {
          irq_dev->configure(sim, sys_ctx, irq_dev->kPageBaseAddrIoIrqDevice);
          if(sys_arch.isa_opts.new_interrupts)irq_dev->set_irq(20); //if we're using A6Kv2.1, we need to use a different IRQ number
          devices_ready.push_back(irq_dev);
        }
        
        // External Keyboard Device
        //
        IODeviceKeyboard* keyboard_dev = new IODeviceKeyboard(irq_dev);
        devices_available.push_back(keyboard_dev);
        
        if (sys_arch.sim_opts.builtin_mem_dev_list.find(keyboard_dev->id()) != sys_arch.sim_opts.builtin_mem_dev_list.end()) {
          // Keyboard device does raise interrupts
          //
          keyboard_dev->configure(sim, sys_ctx, keyboard_dev->kPageBaseAddrIoKeyboardDevice);
          devices_ready.push_back(keyboard_dev);
        }

        // External Screen Device
        //
        IODeviceScreen* screen_dev = new IODeviceScreen();
        devices_available.push_back(screen_dev);

        if (sys_arch.sim_opts.builtin_mem_dev_list.find(screen_dev->id()) != sys_arch.sim_opts.builtin_mem_dev_list.end()) {
          
          // Screen device does not raise interrupts
          //
          screen_dev->configure(sim, sys_ctx, screen_dev->kPageBaseAddrIoScreenDevice);
          
          // If a keyboard is configured pass its pointer to IODeviceScreen 
          //
          if (sys_arch.sim_opts.builtin_mem_dev_list.find(keyboard_dev->id()) != sys_arch.sim_opts.builtin_mem_dev_list.end()) {
            screen_dev->set_keyboard_device(keyboard_dev);
          }
          devices_ready.push_back(screen_dev);
        }
        
        // External Sound Device
        //
        IODeviceSound* snd_dev = new IODeviceSound(irq_dev, 2);
        devices_available.push_back(snd_dev);

        if (sys_arch.sim_opts.builtin_mem_dev_list.find(snd_dev->id()) != sys_arch.sim_opts.builtin_mem_dev_list.end()) {
          // Screen device does not raise interrupts
          //
          snd_dev->configure(sim, sys_ctx, snd_dev->kPageBaseAddrIoSoundDevice);
          devices_ready.push_back(snd_dev);
        }

#endif
        
        // Register NULL device as LAST, note that it will, if enabled,
        // cover the whole address space that is not covered by CCMs or other
        // IODevices
        //
        IODeviceNull* null_dev = new IODeviceNull();
        devices_available.push_back(null_dev);
        
        if (sys_arch.sim_opts.builtin_mem_dev_list.find(null_dev->id()) != sys_arch.sim_opts.builtin_mem_dev_list.end()) {
          null_dev->configure(sim, sys_ctx, 0);
          devices_ready.push_back(null_dev);
        }

        
        // Register ALL devices with the memory sub-system
        //
        for (std::vector<IODevice*>::iterator
             I = devices_ready.begin(),
             E = devices_ready.end();
             I != E; ++I)
        {
          LOG(LOG_DEBUG3) << "[IO-DEVMGR] Registering device: " << (*I)->id();
          simRegisterMemoryDevice(sim, (*I));
        }

        return true;
      }
      
      bool
      IODeviceManager::start_devices()
      {
        if (started) return false;
        
        // Start devices (we only have one now but we could/will have more
        // in the future)
        //
        for (std::vector<IODevice*>::iterator
             I = devices_ready.begin(),
             E = devices_ready.end();
             I != E; ++I)
        {
          LOG(LOG_DEBUG3) << "[IO-DEVMGR] Starting device: " << (*I)->id();
          (*I)->dev_start();
        }
       
        started = true;
        
        return true;
      }
      
      bool
      IODeviceManager::stop_devices()
      {
        if (!started) return false;
        
        // Stop devices in reverse start order
        //
        for (std::vector<IODevice*>::reverse_iterator
             I = devices_ready.rbegin(),
             E = devices_ready.rend();
             I != E; ++I)
        {
          LOG(LOG_DEBUG3) << "[IO-DEVMGR] Stopping device: " << (*I)->id();
          (*I)->dev_stop();
        }
        
        started = false;
        
        return true;
      }
      
      
      bool
      IODeviceManager::destroy_devices()
      {
        // Destroy all devices in reverse order
        //
        while(!devices_available.empty()){
          delete devices_available.back();
          devices_available.pop_back();
        }
        started = false;
        return true;
      }
      
} } } /* namespace arcsim::mem::mmap */

