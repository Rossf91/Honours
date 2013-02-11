//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  UART impl. 
//
// =====================================================================


#include <sys/types.h>
#include <poll.h>
#include <errno.h>
#include <termios.h>

#include <cstdio>
#include <iomanip>
#include <string>

#include "Assertion.h"

#include "api/api_funs.h"

#include "api/ioc/api_ioc.h"
#include "api/irq/api_irq.h"

#include "mem/MemoryDeviceInterface.h"
#include "mem/mmap/IODevice.h"
#include "mem/mmap/IODeviceUart.h"

#include "concurrent/Mutex.h"
#include "concurrent/ScopedLock.h"

#include "util/Allocate.h"
#include "util/OutputStream.h"
#include "util/Log.h"

#define ARC_UART_REVISION 1
#define UART_BYTES 256

#define TX_EMPTY_BIT  7
#define TX_ENB_BIT    6
#define RX_EMPTY_BIT  5
#define RX_FULL_1_BIT 4
#define RX_FULL_BIT   3
#define RX_ENB_BIT    2
#define RX_OFLO_BIT   1
#define RX_FRAM_BIT   0

#define TX_EMPTY  (1UL << TX_EMPTY_BIT)
#define TX_ENB    (1UL << TX_ENB_BIT)
#define RX_EMPTY  (1UL << RX_EMPTY_BIT)
#define RX_FULL_1 (1UL << RX_FULL_1_BIT)
#define RX_FULL   (1UL << RX_FULL_BIT)
#define RX_ENB    (1UL << RX_ENB_BIT)
#define RX_OFLO   (1UL << RX_OFLO_BIT)
#define RX_FRAM   (1UL << RX_FRAM_BIT)

#define PAGE_FIELD(_addr_)     ((_addr_) >> 18)
#define PERIPH_FIELD(_addr_)   (((_addr_) >> 12) & 0x3fUL)
#define INSTANCE_FIELD(_addr_) (((_addr_) >> 8) & 15UL)
#define REG_NUM_FIELD(_addr_)  (((_addr_) >> 2) & 0x3fUL)

#define ESCAPE_CODE 0x1d                                                          

namespace arcsim {
  namespace mem {
    namespace mmap {
      
      const uint32 IODeviceUart::kPageBaseAddrIoUartDevice = 0xc0fc1000;
      const uint32 IODeviceUart::kIoUartMemorySize         = 0x00002000;
      
      const uint32 IODeviceUart::kExtIrqLineIoUartDevice = 5;
      

      IODeviceUart::IODeviceUart()
      : id_(std::string("uart0"))
      {
        term_state = reinterpret_cast<struct termios*>(arcsim::util::Malloced::New(sizeof(term_state[0])));
      }
      
      IODeviceUart::~IODeviceUart()
      {
        if (term_state)
          arcsim::util::Malloced::Delete(term_state);
      }

      std::string&
      IODeviceUart::id()
      {
        return id_;
      }
      
      int
      IODeviceUart::configure(simContext sim, IocContext sys_ctx, uint32 addr)
      {
        sim_        = sim;
        base_addr_  = addr;
        
        IocContext mod_ctx = iocGetContext(sys_ctx, 0);
        cpu_ctx = iocGetContext(mod_ctx, 0);

        buf.ib_read   = buf.ib_write = buf.ib_chars = 0;
        
        return IO_API_OK;
      }
      
      // ---------------------------------------------------------------------
      // Query range for which this MemoryDevice is responsible
      //
      uint32
      IODeviceUart::get_range_begin()      const
      {
        return base_addr_;
      }
      
      uint32
      IODeviceUart::get_range_end  ()      const
      {
        return (base_addr_ + kIoUartMemorySize);
      }
      
      
      int IODeviceUart::dev_start()
      {
        state = UART_STATE_RUN;
        hand_over_stdin ();
        start();
        return IO_API_OK;
      }
      
      int IODeviceUart::dev_stop()
      {
        state = UART_STATE_STOP;
        join();
        hand_back_stdin ();
        return IO_API_OK;
      }
      
      void
      IODeviceUart::hand_over_stdin ()
      {
        struct termios new_settings;
        
        PRINTF() << "Console now belongs to UART, hit CRTL-] to return to simulator.\n";
        tcgetattr (0,term_state);
        new_settings = *term_state;
        
        cfmakeraw (&new_settings);      
        new_settings.c_cc[VTIME] = 0;
        new_settings.c_cc[VMIN] = 1;
        
        tcsetattr (0, TCSANOW, &new_settings);
        return;
      }
      
      void
      IODeviceUart::hand_back_stdin ()
      {
        tcsetattr (0,TCSANOW, term_state);
        PRINTF() << "Console now belongs to simulator.\n";
        return;
      }
      
      // ---------------------------------------------------------------------
      // IODeviceUart run loop
      //
      void IODeviceUart::run()
      {
        ASSERT(state == UART_STATE_RUN);
        state = UART_STATE_RUNNING;
        
        // ---------------------------------------------------------------------
        for (; /* ever busy loop */ ;) {
          // Check if we should terminate ourselves
          //
          if (state == UART_STATE_STOP) {
            break;
          }
          // Check if there is input
          //
          poll_for_input();
          
        }
        // ---------------------------------------------------------------------
        
        ASSERT(state == UART_STATE_STOP);
        state = UART_STATE_STOPPED;
      }
      

      // -----------------------------------------------------------------------
      // Callback registered for IO init
      //
      int
      IODeviceUart::mem_dev_init (uint32 val)
      {
        for (int i = 0; i < 8; i++) {
          reg[i].id0     = i;
          reg[i].id1     = (~i) & 0xffUL;
          reg[i].id2     = ARC_UART_REVISION;
          reg[i].id3     = 0;
          reg[i].rx_data = reg[i].tx_data = 0;
          reg[i].status  = TX_EMPTY | RX_EMPTY;
          reg[i].baudl   = reg[i].baudh = 0;
        }
        return IO_API_OK;
      }
      
      int
      IODeviceUart::mem_dev_clear (uint32 val)
      {
        return IO_API_OK;
      }
      
      // -----------------------------------------------------------------------
      // Callback registered for IO memory write
      //
      int
      IODeviceUart::mem_dev_write (uint32 addr, const unsigned char *data, int size)
      {
        int inst        = INSTANCE_FIELD(addr);
        int reg_num     = REG_NUM_FIELD(addr);
        
        if ((addr & 3UL) != 0) 
        {
          LOG(LOG_ERROR) <<  "[UART-WRITE] Invalid Parameters - inst: "
                         << inst
                         << ", size: "
                         << size
                         << ", reg: "
                         << reg_num 
                         << ", data: " << std::hex << std::setfill('0') << std::setw(8)
                         << *data;
          return 1;
        }
        
        switch (reg_num) {
          case 4:
            put_tx_char (inst, *data);
            set_or_clear_interrupts(inst);
            break;
          case 5:
            reg[inst].status = (reg[inst].status & 0xbbUL) | (*data & 0x44UL) ;
            set_or_clear_interrupts(inst);
            break;
          case 6: reg[inst].baudl = *data;                                        break;
          case 7: reg[inst].baudh = *data;                                        break;
          default:
            LOG(LOG_ERROR) <<  "[UART-WRITE] Invalid Parameters - inst: "
                           << inst
                           << ", size: "
                           << size
                           << ", reg: "
                           << reg_num 
                           << ", data: " << std::hex << std::setfill('0') << std::setw(8)
                           << *data;
            return 1;
        }
        
        return 0;
      }

      // -----------------------------------------------------------------------
      // Callback registered for IO memory reads
      //
      int
      IODeviceUart::mem_dev_read (uint32 addr, unsigned char *data, int size)
      {
        int inst        = INSTANCE_FIELD(addr);
        int reg_num     = REG_NUM_FIELD(addr);
        
        if ((addr & 3UL) != 0) 
        {
          LOG(LOG_ERROR) <<  "[UART-READ] Invalid Parameters - inst: "
                         << inst
                         << ", size: "
                         << size
                         << ", reg: "
                         << reg_num;
          return 1;
        }
        
        uint32 val;
        
        switch (reg_num) {
          case 0: val = reg[inst].id0;          break;
          case 1: val = reg[inst].id1;          break;
          case 2: val = reg[inst].id2;          break;
          case 3: val = reg[inst].id3;          break;
          case 4:
            val = get_rx_char (inst);
            set_or_clear_interrupts(inst);
            break;
          case 5: val = reg[inst].status;       break;
          case 6: val = reg[inst].baudl;        break;
          case 7: val = reg[inst].baudh;        break;
          default:
            LOG(LOG_ERROR) <<  "[UART-READ] Invalid Parameters - inst: "
                           << inst
                           << ", size: "
                           << size
                           << ", reg: "
                           << reg_num;
            return 1;
        }
        *((uint32*)data) = val;
        return 0;
      }
      
      void
      IODeviceUart::set_or_clear_interrupts (int inst)
      {
        uint32 st = reg[inst].status;
        uint32 tx_rdy = TX_EMPTY | TX_ENB;
        uint32 rx_rdy = RX_EMPTY | RX_ENB;
        
        bool tx_int = ((st & tx_rdy) == tx_rdy); // tx-buffer-empty and tx-int-enabled
        bool rx_int = ((st & rx_rdy) == RX_ENB); // rx-buffer-not-empty and rx-int-enabled
        
        if (tx_int || rx_int) {
          irqAssertInterrupt(cpu_ctx, kExtIrqLineIoUartDevice);
        } else {
          irqRescindInterrupt(cpu_ctx, kExtIrqLineIoUartDevice);
        }
      }
      
      // -----------------------------------------------------------------------
      // Debug read/write implementations
      //
      int
      IODeviceUart::mem_dev_read  (uint32 addr, unsigned char* dest, int size, int agent_id)
      {
        return mem_dev_read(addr, dest, size);
      }
      
      int
      IODeviceUart::mem_dev_write (uint32 addr, const unsigned char *data, int size, int agent_id)
      {
        return mem_dev_write(addr, data, size);
      }
      
      // -----------------------------------------------------------------------
      // Read the next character in the receive buffer, moving the
      // buffer on to the next character if something was there.
      // This will always return the last character in the buf.
      //
      uint32
      IODeviceUart::get_rx_char (int inst)
      { 
        // Lock UART buffer before accessing and modifying it -----------------
        //
        arcsim::concurrent::ScopedLock lock(buf_mutex);

        char ch = buf.ib_inbuf[buf.ib_read];
        
        if (buf.ib_chars != 0) {
          // 'Read' next character
          //
          ++buf.ib_read;
          if (buf.ib_read == UART_INBUF_SIZE) {
            buf.ib_read = 0;
          }
          
          // 'Consume' char
          //
          --buf.ib_chars;
          if (buf.ib_chars == 0) {
            reg[0].status |= (RX_EMPTY);
          }
        }        
        return (uint32)ch;
      }
      
      // Put the next character in the transmit FIFO, setting
      // transmit flags as appopriate.
      // Then raise an interrupt if the TX_ENB flag is set
      //
      void
      IODeviceUart::put_tx_char (int inst, uint32 c)
      {
        reg[inst].tx_data = c & 0xffUL;
        putchar ((unsigned char)c);
        reg[inst].status |= TX_EMPTY;
        fflush (stdout);
      }
      
      // Check to see if there is space for new input in the buffer,
      // and if there are any characters ready to read from stdin.
      //  
      void
      IODeviceUart::poll_for_input ()
      { 
        // If buffer is full OR no data is ready to be read return immediately
        //
        if ((buf.ib_chars >= UART_INBUF_SIZE)
            || (poll_fd(0) < 0)) {
          return;
        }
        
        // Lock UART buffer before accessing and modifying it -----------------
        //
        arcsim::concurrent::ScopedLock lock(buf_mutex);
        
        // Read one character into buffer
        //
        int chrs = read (0, buf.ib_inbuf + buf.ib_write, 1);
        
        // If someone hit ESCAPE_CODE return to interactive console
        //
        if (buf.ib_inbuf[buf.ib_write] == ESCAPE_CODE) {
          // Hand back stdin and turn on interactive simulation mode
          //
          hand_back_stdin ();
          simInteractiveOn(sim_);
        } else {
          buf.ib_chars += chrs;
          buf.ib_write += chrs;
          if (buf.ib_write >= UART_INBUF_SIZE) {
            buf.ib_write -= UART_INBUF_SIZE;
          }
        }
        
        // Set the RX_EMPTY flag according to the buffer status,
        // and set/clear the interrupt line appropriately.
        //
        reg[0].status &= ~(RX_EMPTY);
        set_or_clear_interrupts (0);
      }
      
      /* Poll for readability on the given file descriptor, which
       * is assumed to be an input file (e.g. stdin).
       * Return values:
       *
       * -2 indicates invalid polling event or error
       * -1 indicates nothing ready to read
       * Any other value is the fd of a ready fd
       */
      int
      IODeviceUart::poll_fd (int fd)
      {
        struct pollfd poll_list[1];
        int retval;
        
        poll_list[0].fd = fd;                     // filedes we are interested in
        poll_list[0].events = POLLIN|POLLPRI;     // events we are interested in
        
        // Examine filedes synchronously. If nothing happens wait some millis for
        // filedes to become available before returning to the simulation loop.
        //
        retval = poll (poll_list, (nfds_t)1, 5000);
        
        // If an error occured or none of the events occured on filedes return -1
        //
        if (retval <= 0) return -1;
        
        if (   ((poll_list[0].revents & POLLHUP)  == POLLHUP)
            || ((poll_list[0].revents & POLLERR)  == POLLERR)
            || ((poll_list[0].revents & POLLNVAL) == POLLNVAL))
          return -1;
        
        if (   ((poll_list[0].revents & POLLIN)  == POLLIN)
            || ((poll_list[0].revents & POLLPRI) == POLLPRI))
          return poll_list[0].fd;
        
        return -1;
      }

      
} } } /* namespace arcsim::mem::mmap */

