//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  Memory mapped Keyboard device impl.
//
// =====================================================================

#include <vector>
#include <utility>
#include <cstring>

#include <gtk/gtk.h> 

#include "api/api_funs.h"

#include "mem/MemoryDeviceInterface.h"
#include "mem/mmap/IODevice.h"
#include "mem/mmap/IODeviceIrq.h"
#include "mem/mmap/IODeviceKeyboard.h"

#include "util/Log.h"

#define KEYBOARD_IRQ    3

// =====================================================================

// The following tables maps GTK's 'keyval' values into PS/2 scancodes that PASTA
// software will expect. Instead of a single massive map table, it has been split
// up into three small ones.
//
// CharBlock contains only printable characters like the alphanumericals and
// punctuation marks.
// Misc block contains a few extra printables that feature on the UK keyboard
// (not US so aren't in the main block) (£, ¬)
// SymBlock contains all the non printable keys like Return, Tab, Shift Fn keys,
// as well as the KeyPad.
// A full list of GTK's keyvals can be found in <gdk/gdkkeysyms.h> 
//
// Each map table has a macro stating the (inclusive) lower and upper bound of
// the codes it stores. These macros should be updated if new entries are added
// to the table. To do the mapping, check if a keyval falls within the blocks max
// and min if it does then subtract the min value from the keyval and use it as
// the arrays index.


#define MAP_CHAR_BLOCK_MIN 0x20
#define MAP_CHAR_BLOCK_MAX 0xac
#define MAP_CHAR_BLOCK_SIZE (MAP_CHAR_BLOCK_MAX - MAP_CHAR_BLOCK_MIN + 1)

uint32 keyValCharMap[MAP_CHAR_BLOCK_SIZE] = {
  //      0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f       
  /*2*/   0x29,   0x16,   0x1e,   0x5d,   0x25,   0x2e,   0x3d,   0x52,   0x46,   0x45,   0x3e,   0x55,   0x41,   0x4e,   0x49,   0x4a,
  /*3*/   0x45,   0x16,   0x1e,   0x26,   0x25,   0x2e,   0x36,   0x3d,   0x3e,   0x46,   0x4c,   0x4c,   0x41,   0x55,   0x49,   0x4a,
  /*4*/   0x52,   0x1c,   0x32,   0x21,   0x23,   0x24,   0x2b,   0x34,   0x33,   0x43,   0x3b,   0x42,   0x4b,   0x3a,   0x31,   0x44,
  /*5*/   0x4d,   0x15,   0x2d,   0x1b,   0x2c,   0x3c,   0x2a,   0x1d,   0x22,   0x35,   0x1a,   0x54,   0x61,   0x5b,   0x36,   0x4e,
  /*6*/   0x0e,   0x1c,   0x32,   0x21,   0x23,   0x24,   0x2b,   0x34,   0x33,   0x43,   0x3b,   0x42,   0x4b,   0x3a,   0x31,   0x44,
  /*7*/   0x4d,   0x15,   0x2d,   0x1b,   0x2c,   0x3c,   0x2a,   0x1d,   0x22,   0x35,   0x1a,   0x54,   0x61,   0x5b,   0x5d,   0x00,
  /*8*/   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
  /*9*/   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
  /*a*/   0x29,   0x00,   0x00,   0x26,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x0e
};

#define MAP_MISC_BLOCK_MIN 0xff08
#define MAP_MISC_BLOCK_MAX 0xff1b
#define MAP_MISC_BLOCK_SIZE (MAP_MISC_BLOCK_MAX - MAP_MISC_BLOCK_MIN + 1)

uint32 keyValMiscMap[MAP_MISC_BLOCK_SIZE] = {
  //      0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f       
  /*0*/                                                                   0x66,   0x0d,   0x00,   0x00,   0x00,   0x5a,   0x00,   0x00,  
  /*1*/   0x00,   0x00,   0x00,   0x00,   0x7e,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x76  
};

#define MAP_SYM_BLOCK_MIN 0xff50
#define MAP_SYM_BLOCK_MAX 0xffff
#define MAP_SYM_BLOCK_SIZE (MAP_SYM_BLOCK_MAX - MAP_SYM_BLOCK_MIN + 1)

uint32 keyValSymMap[MAP_SYM_BLOCK_SIZE] = {
  //      0       1       2       3       4       5       6       7       8       9       a       b       c       d       e       f       
  /*5*/   0x6ce0, 0x6be0, 0x75e0, 0x74e0, 0x72e0, 0x7de0, 0x7ae0, 0x69e0, 0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
  /*6*/   0x00,   0x00,   0x00,   0x70e0, 0x00,   0x00,   0x00,   0x2fe0, 0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
  /*7*/   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x77,
  /*8*/   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x5ae0, 0x00,   0x00,
  /*9*/   0x00,   0x00,   0x00,   0x00,   0x00,   0x6c,   0x6b,   0x75,   0x74,   0x72,   0x7d,   0x7a,   0x69,   0x00,   0x70,   0x71,
  /*A*/   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x7c,   0x79,   0x00,   0x7b,   0x71,   0x4ae0,
  /*B*/   0x70,   0x69,   0x72,   0x7a,   0x6b,   0x73,   0x74,   0x6c,   0x75,   0x7d,   0x00,   0x00,   0x00,   0x00,   0x05,   0x06,
  /*C*/   0x04,   0x0c,   0x03,   0x0b,   0x83,   0x0a,   0x01,   0x09,   0x78,   0x07,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
  /*D*/   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,
  /*E*/   0x00,   0x12,   0x59,   0x14,   0x14e0, 0x58,   0x00,   0x00,   0x00,   0x11,   0x11e0, 0x1fe0, 0x00,   0x00,   0x00,   0x00,
  /*F*/   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x00,   0x71e0
};


namespace arcsim {
  namespace mem {
    namespace mmap {
      
      const uint32 IODeviceKeyboard::kPageBaseAddrIoKeyboardDevice = 0xFF030000;
      const uint32 IODeviceKeyboard::kIoKeyboardMemorySize         = 0x00002000;
      
      
      IODeviceKeyboard::IODeviceKeyboard(IODeviceIrq* irq_dev)
      : id_(std::string("keyboard")),
        irq_device(irq_dev)
      { /* EMPTY */ }

      IODeviceKeyboard::~IODeviceKeyboard(){}
      
      std::string&
      IODeviceKeyboard::id()
      {
        return id_;
      }

      int
      IODeviceKeyboard::dev_start() { return IO_API_OK; }
      
      int
      IODeviceKeyboard::dev_stop()  { return IO_API_OK; }
      
      int
      IODeviceKeyboard::configure(simContext sim, IocContext sys_ctx, uint32 addr)
      {
        sim_       = sim;
        base_addr_ = addr;
        
        // Initialise keyboard buffer area
        //
        
        key_buffer_mutex.acquire(); // -----------------------------------------
        
        scanBuffer.scanIn  = SCANSIZE-1;
        scanBuffer.scanOut = SCANSIZE-1;
        for (int i = 0; i< SCANSIZE; ++i){
          scanBuffer.scanBuf[i] = 0;
        }
        scanBuffer.ready = -1;
        
        key_buffer_mutex.release(); // -----------------------------------------
                
        return IO_API_OK;
        
      }
            
      // ---------------------------------------------------------------------
      // Query range for which this MemoryDevice is responsible
      //
      uint32
      IODeviceKeyboard::get_range_begin()      const
      {
        return base_addr_;
      }
      
      uint32
      IODeviceKeyboard::get_range_end  ()      const
      {
        return (base_addr_ + kIoKeyboardMemorySize);
      }

      
      // Called upon page initialisation
      ////
      int
      IODeviceKeyboard::mem_dev_init (uint32 val)
      {
        return IO_API_OK;
      }
      
      int
      IODeviceKeyboard::mem_dev_clear(uint32 val)
      {
        return IO_API_OK;
      }
      
      // Called upon page read
      ////
      int
      IODeviceKeyboard::mem_dev_read (uint32 addr, unsigned char* dest, int size)
      {
        addr -= base_addr_;
        if (addr < 4){ //only deal with first four bytes
          if (addr + size > 4) {
            // if addr isn't 0, adjust the size to be returned to keep it within our 4 bytes
            size=4-addr;
          }
          // MUTEX ACQUIRE -----------------------------------------------------
          //
          key_buffer_mutex.acquire();
          
          // Are there keystrokes to be read?
          //
          if (scanBuffer.scanIn != scanBuffer.scanOut)
          {
            scanBuffer.scanOut = (scanBuffer.scanOut + 1) % SCANSIZE;
            uint32* scancode   = &(scanBuffer.scanBuf[scanBuffer.scanOut]);
            memcpy(dest, scancode + addr, size);
          } else {
            *dest=0;
          }
          if (scanBuffer.scanIn == scanBuffer.scanOut){
            // No more keystrokes in queue, rescind IRQ
            //
            irq_device->rescind_ext_irq(KEYBOARD_IRQ);
          }
          
          key_buffer_mutex.release();
          //
          // MUTEX RELEASE -----------------------------------------------------
        }
        return 0;
      }
      
      // Called upon page write
      ////
      int
      IODeviceKeyboard::mem_dev_write (uint32 addr, const unsigned char *data, int size)
      {
        return 0;
      }
      
      // -----------------------------------------------------------------------
      // Debug read/write implementations
      //
      int
      IODeviceKeyboard::mem_dev_read  (uint32 addr, unsigned char* dest, int size, int agent_id)
      {
        return mem_dev_read(addr, dest, size);
      }
      
      int
      IODeviceKeyboard::mem_dev_write (uint32 addr, const unsigned char *data, int size, int agent_id)
      {
        return mem_dev_write(addr, data, size);
      }

      
      
      // Function allowing the GTK terminal to add a keystroke to the buffer
      int
      IODeviceKeyboard::keyboard_add_key_to_scanbuffer(guint keyval, uint8 keypress)
      {        
        uint8  retval   = 0;
        uint32 scancode = 0;
        
        // Retrieve scancode
        //
        if (keyval >= MAP_CHAR_BLOCK_MIN && keyval <= MAP_CHAR_BLOCK_MAX) {
          scancode = keyValCharMap[keyval-MAP_CHAR_BLOCK_MIN];
        } else if (keyval >= MAP_MISC_BLOCK_MIN && keyval <= MAP_MISC_BLOCK_MAX) {
          scancode = keyValMiscMap[keyval-MAP_MISC_BLOCK_MIN];
        } else if (keyval >= MAP_SYM_BLOCK_MIN && keyval <= MAP_SYM_BLOCK_MAX) {
          scancode = keyValSymMap[keyval-MAP_SYM_BLOCK_MIN];
        }
        
        if (scancode != 0)
        {
          if (keypress != 0) {
            // Key released
            //
            if ((scancode & 0xFF) == 0xE0)
            {
              // Extended scancodes have F0 between the E0 tag and the keycode
              //
              scancode = ((scancode << 8) & 0xFFFF0000) | 0xF0E0;
            } else {
              // Normal scancodes just have F0 on the end 
              //
              scancode = scancode << 8 | 0xF0;
            }
          }
          
          // MUTEX ACQUIRE -----------------------------------------------------
          //
          key_buffer_mutex.acquire();
          
          if (scanBuffer.ready){
            // If adding another keystroke will overrun, don't add it
            //
            if (((scanBuffer.scanIn+1) % SCANSIZE) != scanBuffer.scanOut)
            {  
              scanBuffer.scanIn = (scanBuffer.scanIn + 1) % SCANSIZE;
              scanBuffer.scanBuf[scanBuffer.scanIn] = scancode;

              // Trigger keyboard IRQ
              //
              irq_device->assert_ext_irq(KEYBOARD_IRQ);
              retval = 1;
            }
          }

          key_buffer_mutex.release();
          //
          // MUTEX RELEASE ----------------------------------------------------- 
        }
        return retval;
      }
      
      
} } } /* namespace arcsim::mem::mmap */

