//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  uart0 device necessary for linux kernel simulation. 
//
// =====================================================================


#ifndef _IODeviceUart_h_
#define _IODeviceUart_h_

#include "api/ioc_types.h"

#include "mem/mmap/IODevice.h"

#include "concurrent/Thread.h"
#include "concurrent/Mutex.h"

#define UART_INBUF_SIZE                 1024

// Forward declare struct termios
//
struct termios;

namespace arcsim {
  namespace mem {
    namespace mmap {

      typedef enum {
        UART_STATE_RUN,
        UART_STATE_RUNNING,
        UART_STATE_STOP,
        UART_STATE_STOPPED
      } UartRunState;
      
      // UART buffer data-type
      //
      typedef struct {
        char  ib_inbuf[UART_INBUF_SIZE];
        int   ib_write;         /* Write pointer             */
        int   ib_read;          /* Read pointer              */
        int   ib_chars;         /* Number of chars in buffer */
      } UartBuffer;

      // UART control register data-type
      //
      typedef struct {
        uint32 id0;
        uint32 id1;
        uint32 id2;
        uint32 id3;
        uint32 baudl;
        uint32 baudh;
        uint32 status;
        uint32 rx_data;
        uint32 tx_data;
      } UartRegister;
      
      
      class IODeviceUart : public arcsim::mem::mmap::IODevice,
                           public arcsim::concurrent::Thread
      {
      private:
        // Device identifier
        //
        std::string id_;
        
        // Local UART registers, one of each type for each of the 8 UARTS
        //
        UartRegister reg[8];
        
        // Input buffer for UART 0, the only one actually connected to a real channel
        //        
        UartBuffer   buf;
        
        // UART device state
        //
        UartRunState  state;
        
        // Terminal settings 
        //
        struct termios* term_state;

        // Mutex for uart buffer synchronisation
        //
        arcsim::concurrent::Mutex buf_mutex;
        
        IocContext   cpu_ctx;
        
        uint32  get_rx_char (int inst);
        void    put_tx_char (int inst, uint32 c);
        int     poll_fd (int fd);
        void    set_or_clear_interrupts (int);
        
        void run();
        
      public:
        static const uint32 kPageBaseAddrIoUartDevice;
        static const uint32 kExtIrqLineIoUartDevice;

        // Memory size this device is responsible for
        //
        static const uint32 kIoUartMemorySize;

        
        // Constructor
        //
        IODeviceUart();
        
        // Destructor
        //
        ~IODeviceUart();
        
        std::string& id();
        
        void hand_over_stdin();
        void hand_back_stdin();
        void poll_for_input();

        int configure(simContext sim, IocContext sys_ctx, uint32 addr);
        
        int dev_start();
        int dev_stop();
        
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

#endif /* _IODeviceUart_h_ */
