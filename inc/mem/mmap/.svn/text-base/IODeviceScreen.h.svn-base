//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  Memory mapped screen device.
//
// =====================================================================


#ifndef INC_IO_MMAP_IODEVICESCREEN_H_
#define INC_IO_MMAP_IODEVICESCREEN_H_

#include <string>
#include <vector>
#include <map>

#if (defined(__APPLE__) && defined(__MACH__))
# include <OpenGL/gl.h>
#else
# include <GL/gl.h>
#endif

#include <gdk/gdk.h>
#include <gdk/gdkgl.h>

#include "mem/mmap/IODevice.h"
#include "concurrent/Mutex.h"
#include "concurrent/Thread.h"

// screen dimensions
#define SCREEN_XDIM 160
#define SCREEN_YDIM 128

// -----------------------------------------------------------------------------
// FORWARD DECLARATION
// -----------------------------------------------------------------------------

namespace arcsim {
  namespace mem {
    namespace mmap {

      // Forward declare keyboard device
      //
      class IODeviceKeyboard;
      
      typedef enum {
        SCREEN_DISABLED,
        SCREEN_ENABLE,
        SCREEN_ENABLED,
        SCREEN_STOP,
        SCREEN_STOPPED
      } IODeviceScreenState;

      class IODeviceScreen : public arcsim::mem::mmap::IODevice,
                             public arcsim::concurrent::Thread
      {
      private:
        std::string         id_;
        int                 flip_clr_chr;        
        
        // Determine how to interpret character display AND framebuffer
        //
        enum ScreenMode {
          CHAR_BUFFER_ONLY        = 0,
          FRAME_BUFFER_ONLY_YUYV  = 1,
          FRAME_BUFFER_ONLY_GREY  = 2,
          CHAR_FRAME_BUFFER_YUYV  = 3,
          CHAR_FRAME_BUFFER_PALLET= 4, // DOOM Mode
          FRAME_BUFFER_ONLY_RGB   = 5,
          FRAME_BUFFER_ONLY_R5G6B5= 6  // ISS VFB mode
        };
        
        // Framebuffer and its control registers
        //
        uint8 *             framebuffer;
        uint8 *             mem_base_ptr;
        uint32              fb_width;
        uint32              fb_height;
        uint32              fb_addr;
        uint32              fb_mode;
        uint32              fb_old_size;
        
        void                frame_buffer_parse();
        
        void                frame_buffer_draw();
        
        void                char_buffer_draw();

        
        // Screen state
        //
        IODeviceScreenState state;
        
        // Screen buffer
        //
        typedef unsigned char screen_buf_t[SCREEN_XDIM][2];
        screen_buf_t  *buffer;
        
        // Main GDK window widget and drawing area widget
        //
        GdkWindow*      window;
        GdkGLWindow*    gl_window; 
        GdkGLContext*   gl_context;
        GdkGLDrawable*  gl_drawable;
        
        // IODeviceKeyboard device
        //
        IODeviceKeyboard* keyboard_dev;
        
        // OpenGL texture
        //
        GLuint          texture;


        void run();

        // Hooks for GdkEvents
        //
        bool on_configure_event(GdkEventConfigure* event);
        bool on_keyboard_event(GdkEvent* event);
        bool on_delete_event();

        // Initialisation methods
        //
        void init_texture();
        void init_window();

        // Performance critical draw method
        //
        bool draw();
        
      public:
        static const uint32 kPageBaseAddrIoScreenDevice;
        
        // Constructor
        //
        IODeviceScreen();
        
        // Destructor
        //
        ~IODeviceScreen();
        
        std::string& id();
        
        void set_keyboard_device(IODeviceKeyboard* _keyboard_dev)
        {
          keyboard_dev = _keyboard_dev;
        }
        
        int configure(simContext sim, IocContext sys_ctx, uint32 addr);

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

#endif  // INC_IO_MMAP_IODEVICESCREEN_H_
