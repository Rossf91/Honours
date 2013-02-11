//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  Memory mapped screen device impl.
//
// =====================================================================

#include <vector>
#include <map>
#include <utility>
#include <cstdlib>
#include <cstring>

#if (defined(__APPLE__) && defined(__MACH__))
# include <OpenGL/gl.h>
# include <OpenGL/glu.h>
#else
# include <GL/gl.h>
# include <GL/glu.h>
#endif
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkgl.h>

#include "Assertion.h"

#include "api/api_funs.h"
#include "api/mem/api_mem.h"

#include "mem/MemoryDeviceInterface.h"
#include "mem/mmap/IODevice.h"
#include "mem/mmap/IODeviceScreen.h"
#include "mem/mmap/IODeviceKeyboard.h"

#include "concurrent/Mutex.h"
#include "concurrent/ScopedLock.h"

#include "util/Allocate.h"
#include "util/Log.h"

#define SCREEN_OPT_FLIP_CLR_CHR "-screen-flip-clr-chr"
#define SCREEN_OPT_CHAR_SIZE    "-screen-char-size"
#define SCREEN_OPT_TITLE        "-screen-title"

#define WINDOW_DEFAULT_XSIZE     1024
#define WINDOW_DEFAULT_YSIZE     768

#define CLR_IDX 0
#define CHR_IDX 1

namespace arcsim {
  namespace mem {
    namespace mmap {
      
      const uint32 IODeviceScreen::kPageBaseAddrIoScreenDevice = 0xFF010000;
      
      
      // Framebuffer control register locations relative to start address
      //
      static const uint32 kPalettePointerAddr    = 0x10000 - 24;
      static const uint32 kFramebufferWidthAddr  = 0x10000 - 16;
      static const uint32 kFramebufferHeightAddr = 0x10000 - 12;
      static const uint32 kFramebufferModeAddr   = 0x10000 - 8;
      static const uint32 kFramebufferPointerAddr= 0x10000 - 4;

      
      // Include font data into arcsim::mem::mmap namespace
      //
      #include "util/screen/8x8font.h"
      
      // CGA lookup table
      //
      const static float cga_lookup[16][3] = 
        {
          {0.0f,0.0f,0.0f},
          {0.0f,0.0f,170.0f/255.0f},
          {0.0f,170.0f/255.0f,0.0f},
          {0.0f,170.0f/255.0f,170.0f/255.0f},
          {170.0f/255.0f,0.0f,0.0f},
          {170.0f/255.0f,0.0f,170.0f/255.0f},
          {170.0f/255.0f,85.0f/255.0f,0.0f},
          {170.0f/255.0f,170.0f/255.0f,170.0f/255.0f},
          {85.0f/255.0f,85.0f/255.0f,85.0f/255.0f},
          
          {85.0f/255.0f,85.0f/255.0f,1.0f},
          {85.0f/255.0f,1.0f,85.0f/255.0f},
          {85.0f/255.0f,1.0f,1.0f},
          {1.0f,85.0f/255.0f,85.0f/255.0f},
          {1.0f,85.0f/255.0f,1.0f},
          {1.0f,1.0f,85.0f/255.0f},
          {1.0f,1.0f,1.0f}
        };
      
      std::string&
      IODeviceScreen::id()
      {
        return id_;
      }
      
      // Constructor
      //
      IODeviceScreen::IODeviceScreen()
      : id_(std::string("screen")),
        buffer(0),
        framebuffer(0),
        fb_mode(CHAR_BUFFER_ONLY),
        fb_width(0),
        fb_height(0),
        fb_addr(0),
        fb_old_size(0),
        mem_base_ptr(0),
        state(SCREEN_DISABLED),
        window(0),
        gl_window(0),
        gl_context(0),
        gl_drawable(0),
        keyboard_dev(0),
        texture(0),
        flip_clr_chr(0)
      {
        // Allocate screen buffer
        //
        buffer = (screen_buf_t*)arcsim::util::Malloced::New(SCREEN_YDIM * SCREEN_XDIM * 2);
        // Check if allocation was successful
        //
        ASSERT(buffer != 0 && "Failed to allocate screen buffer.");
        // Clear out screen buffer
        //
        bzero(buffer,SCREEN_YDIM*SCREEN_XDIM*2 );

      }

      // Destructor
      //
      IODeviceScreen::~IODeviceScreen()
      {
        if (gl_context != 0) { gdk_gl_context_destroy(gl_context);  }
        if (gl_window  != 0) { gdk_gl_window_destroy(gl_window);    }
        if (window     != 0) { gdk_window_destroy(window);          }
        
        if (buffer     != 0) { arcsim::util::Malloced::Delete(buffer);      }
        if (framebuffer!= 0) { arcsim::util::Malloced::Delete(framebuffer); }
      }

      // -----------------------------------------------------------------------
      // IODeviceScreen configure
      //
      int
      IODeviceScreen::configure(simContext sim, IocContext sys_ctx, uint32 addr)
      {
        sim_       = sim;
        base_addr_ = addr;
        
        // How should we interprete values that are written to memory locations?
        ////
        if (simPluginOptionIsSet(sim, SCREEN_OPT_FLIP_CLR_CHR)) {
          flip_clr_chr = 1;
        }  

        // Initialise GDK 
        //
        gdk_init    (NULL, NULL);
        gdk_gl_init (NULL, NULL);
        
        return IO_API_OK;
      }
      
      // ---------------------------------------------------------------------
      // Query range for which this MemoryDevice is responsible
      //
      uint32
      IODeviceScreen::get_range_begin()      const
      {
        return base_addr_;
      }
      
      uint32
      IODeviceScreen::get_range_end  ()      const
      {
        return (base_addr_ + (SCREEN_XDIM*SCREEN_YDIM*2));
      }

      
      // -----------------------------------------------------------------------
      // IODeviceScreen start|stop
      //
      int
      IODeviceScreen::dev_start()
      {
        // Start screen thread
        // FIXME: remember state and do NOT start again
        //
        ASSERT(state == SCREEN_DISABLED);
        state = SCREEN_ENABLE;

        // Start screen device
        //
        start();

        return IO_API_OK;
      }
      
      int
      IODeviceScreen::dev_stop()
      {
        // Signal busy loop to stop
        //
        ASSERT(state == SCREEN_ENABLED);
        state = SCREEN_STOP;
        
        // Wait until thread finishes
        //
        join();

        return IO_API_OK;
      }
      

      void
      IODeviceScreen::init_window()
      {
        char          buf[128];
        unsigned int  char_size = kFontHeight;
        GdkGLConfig*  gl_cfg    = NULL;
        GdkWindowAttr attr; 
        
        if (simPluginOptionIsSet(sim_, SCREEN_OPT_CHAR_SIZE)) {
          char_size = atoi(simPluginOptionGetValue(sim_, SCREEN_OPT_CHAR_SIZE));
        }
        
        // set window title
        if (simPluginOptionIsSet(sim_, SCREEN_OPT_TITLE)) {
          snprintf(buf, sizeof(buf), "%s", simPluginOptionGetValue(sim_, SCREEN_OPT_TITLE));
        } else {
          snprintf(buf, sizeof(buf), "Virtual Screen");
        }
        

        attr.window_type = GDK_WINDOW_TOPLEVEL; 
        attr.wclass      = GDK_INPUT_OUTPUT; 
        attr.event_mask  = (GDK_KEY_PRESS_MASK      | GDK_BUTTON_PRESS_MASK | 
                            GDK_POINTER_MOTION_MASK | GDK_STRUCTURE_MASK    |
                            GDK_EXPOSURE_MASK); 
        attr.width  = (WINDOW_DEFAULT_XSIZE * char_size) / kFontHeight; 
        attr.height = (WINDOW_DEFAULT_YSIZE * char_size) / kFontHeight; 
        
        window = gdk_window_new(NULL, &attr, 0);
        gdk_window_show(window);
        gdk_window_set_title(window, buf);
        
        // Configure OpenGL
        //        
        gl_cfg = gdk_gl_config_new_by_mode((GdkGLConfigMode)(GDK_GL_MODE_RGBA  |
                                                             GDK_GL_MODE_DEPTH |
                                                             GDK_GL_MODE_DOUBLE));
        
        gl_window   = gdk_window_set_gl_capability(window, gl_cfg, NULL); 
        gl_drawable = GDK_GL_DRAWABLE(gl_window);
        gl_context  = gdk_gl_context_new(gl_drawable, NULL, true, GDK_GL_RGBA_TYPE); 
 
      }

      void
      IODeviceScreen::init_texture()
      {
        
        gdk_gl_drawable_gl_begin(gl_drawable, gl_context); // ------------------
        
        gluOrtho2D(0.0f, SCREEN_XDIM, SCREEN_YDIM, 0.0f);
        
        // Texture initialisation
        //
        glGenTextures( 1, &texture );
        // Select our current texture
        //
        glBindTexture( GL_TEXTURE_2D, texture );
        // Select modulate to mix texture with color for shading
        //
        glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
        // When texture area is small, bilinear filter the closest mipmap
        //
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NONE);
        // When texture area is large, bilinear filter the original
        //
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NONE);
        
        
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA,
                     kFontWidth, kFontHeight, 0,
                     GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, kFontData);
        gluBuild2DMipmaps(GL_TEXTURE_2D,  GL_LUMINANCE_ALPHA,
                          kFontWidth, kFontHeight,
                          GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, kFontData);

        gdk_gl_drawable_gl_end (gl_drawable); // -------------------------------
      }

      // -----------------------------------------------------------------------
      // IODeviceScreen run
      //
      void
      IODeviceScreen::run()
      {
        ASSERT( state == SCREEN_ENABLE );
        state = SCREEN_ENABLED;
        
        // Initialise window, textures, and mode control register
        //
        init_window();
        init_texture();
        simWriteWord(sim_, base_addr_ + kFramebufferModeAddr, 0);

        // ---------------------------------------------------------------------
        // FIXME: This is event loop is very busy - what if the screen has nothing
        //        todo? It should not waste resources in that case!
        //
        for ( ; /* ever */; ) { 
          
          // Handle pending GDK events
          //
          while(gdk_events_pending()) {
            GdkEvent* pending_event = gdk_event_get(); 
            if(pending_event != NULL) {
              
              switch(pending_event->type) { 
                case GDK_EXPOSE:    {
                  draw();
                  break;
                }
                case GDK_CONFIGURE: {
                  on_configure_event(&(pending_event->configure));
                  break;
                }
                  
                case GDK_KEY_PRESS:
                case GDK_KEY_RELEASE: {
                  on_keyboard_event(pending_event);
                  break;
                }
                  
                case GDK_DELETE:    { on_delete_event();  break; }
                default:  { break; }
              }

              gdk_event_free(pending_event); 
            } 
          }

          draw(); 
          
          // Break out of our event loop
          //
          if (state == SCREEN_STOP) break;
        }
        // ---------------------------------------------------------------------
        
        // Clean-up allocated textures
        //
        if (texture != 0) { glDeleteTextures (1, &(texture)); }
        
        return;
      }
      
      // -----------------------------------------------------------------------
      // IODeviceScreen mmap_read|write|init code
      //
      int
      IODeviceScreen::mem_dev_init(uint32 val)
      {
        return IO_API_OK;
      }
      
      int
      IODeviceScreen::mem_dev_clear(uint32 val)
      {
        return IO_API_OK;
      }
      
      // -----------------------------------------------------------------------
      // Debug read/write implementations
      //
      int
      IODeviceScreen::mem_dev_read  (uint32 addr, unsigned char* dest, int size, int agent_id)
      {
        return mem_dev_read(addr, dest, size);
      }
      
      int
      IODeviceScreen::mem_dev_write (uint32 addr, const unsigned char *data, int size, int agent_id)
      {
        return mem_dev_write(addr, data, size);
      }

      // -----------------------------------------------------------------------
      // Read/write implementations
      //
      int
      IODeviceScreen::mem_dev_read(uint32 addr, unsigned char *dest, int size)
      {
        return IO_API_OK;
      }
      
      int
      IODeviceScreen::mem_dev_write(uint32 _addr, const unsigned char *data, int size)
      {
        uint32 addr       = _addr - base_addr_;
        int    x;              // x-coordinate
        int    y;              // y-coordinate
        
        // This is how x and y coordinates are encoded as addresses:
        //
        // adr = kPageBaseAddrIoScreenDevice + (x << 0x1) + (y * (SCREEN_XDIM << 0x1));
        
        switch (size) {

          case 4: {
            // First chunk
            //
            y = (addr) / (SCREEN_XDIM << 0x1);
            x = (addr - (y * (SCREEN_XDIM << 0x1))) / 2 ;
            buffer[y][x][CLR_IDX] = *(data);    // colour value
            buffer[y][x][CHR_IDX] = *(data+1);  // character value
            
            // Second chunk
            //            
            y = (addr+2) / (SCREEN_XDIM << 0x1);
            x = ((addr+2) - (y * (SCREEN_XDIM << 0x1))) / 2 ;
            buffer[y][x][CLR_IDX] = *(data+2);  // colour value
            buffer[y][x][CHR_IDX] = *(data+3);  // character value
            break;
          }

          case 2: {
            y = (addr) / (SCREEN_XDIM << 0x1);
            x = (addr - (y * (SCREEN_XDIM << 0x1))) / 2 ;
            buffer[y][x][CLR_IDX] = *(data);    // colour value
            buffer[y][x][CHR_IDX] = *(data+1);  // character value
            break;
          }

          case 1: {
            if (addr & 1) {
              y = (addr - 1) / (SCREEN_XDIM << 0x1);
              x = ((addr - 1) - (y * (SCREEN_XDIM << 0x1))) / 2 ;
              buffer[y][x][CHR_IDX] = *(data);  // character value
            } else {
              y = (addr) / (SCREEN_XDIM << 0x1);
              x = (addr - (y * (SCREEN_XDIM << 0x1))) / 2 ;
              buffer[y][x][CLR_IDX] = *(data);  // colour value
            }
            break;
          }

          default: {
            LOG(LOG_WARNING) << "[IODeviceScreen] mem_dev_write size unsupported.";
            break;
          }
        }
                
        return IO_API_OK;
      }
 
      // -----------------------------------------------------------------------
      
      bool
      IODeviceScreen::on_configure_event(GdkEventConfigure* _event)
      {
        // Delimits the begining of the OpenGL execution
        //
        gdk_gl_drawable_gl_begin(gl_drawable, gl_context);

        glViewport(0, 0, _event->width, _event->height);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
        glAlphaFunc(GL_GEQUAL, 0.4f);
        
        gdk_gl_drawable_gl_end (gl_drawable);
        return true;
      }

      bool
      IODeviceScreen::on_keyboard_event(GdkEvent* event)
      {
        // If keyboad is not enabled we bail out right away
        //
        if (!keyboard_dev) { return true; }
        
        switch(event->type) {    
          case GDK_KEY_PRESS: {
            keyboard_dev->keyboard_add_key_to_scanbuffer(((GdkEventKey*)event)->keyval, 1);
            break;
          }
          case GDK_KEY_RELEASE: {
            keyboard_dev->keyboard_add_key_to_scanbuffer(((GdkEventKey*)event)->keyval, 0);
            break;
          }
          default: { break; }
        }
        return true;
      }                                                  
      
      bool
      IODeviceScreen::draw()
      {        
        // Delimits the begining of the OpenGL execution
        //
        gdk_gl_drawable_gl_begin(gl_drawable, gl_context);
        {
          glClear(GL_COLOR_BUFFER_BIT);
          
          frame_buffer_parse();
          
          frame_buffer_draw();
          
          char_buffer_draw();
                    
          if (gdk_gl_drawable_is_double_buffered(gl_drawable)) {
            gdk_gl_drawable_swap_buffers(gl_drawable);
          } else {
            glFlush();
          }
        }
        gdk_gl_drawable_gl_end (gl_drawable);
        
        return true;                
      }
      
      bool IODeviceScreen::on_delete_event()
      {
        simHalt(sim_);
        return true;
      }
      
      void
      IODeviceScreen::frame_buffer_parse()
      { // Read framebuffer mode control register
        //
        simReadWord(sim_, base_addr_ + kFramebufferModeAddr, &fb_mode);
        
        // Check if we are in framebuffer mode
        //
        if (fb_mode != CHAR_BUFFER_ONLY)
        { // Read width/height/addr control registers
          //
          simReadWord(sim_, base_addr_ + kFramebufferWidthAddr,  &fb_width);
          simReadWord(sim_, base_addr_ + kFramebufferHeightAddr, &fb_height);
          simReadWord(sim_, base_addr_ + kFramebufferPointerAddr,&fb_addr);
                      
          // Resize buffer if necessary
          //
          const uint32 fb_size = fb_width * fb_height * 4;

          if ( fb_old_size != fb_size ) {
            fb_old_size = fb_size;

            if(framebuffer != 0) {
              // Free-up old buffer
              //
              arcsim::util::Malloced::Delete(framebuffer);
            }
            
            // Allocate new buffer
            //
            framebuffer = (uint8*)arcsim::util::Malloced::New(fb_size);

            if (framebuffer == 0) {
              LOG(LOG_ERROR) << "[IODeviceScreen] Framebuffer allocation failed.";
            }
          }
          
          // Depending on the framebuffer mode we interpret data in different ways
          //
          switch (fb_mode)
          {
            case CHAR_FRAME_BUFFER_YUYV:
            case FRAME_BUFFER_ONLY_YUYV:
            {
              const uint32  fb_area = fb_width * fb_height;
              
              // Allocate enough space to copy framebuffer
              //
              uint8 * buffer = (uint8*)arcsim::util::Malloced::New(fb_area * 2);
              
              if (buffer == 0) {
                LOG(LOG_ERROR) << "[IODeviceScreen] Temporary buffer allocation failed.";
              }
              
              // Perform actual copy of framebuffer into fb_copy
              //
              simReadBlock(sim_,  fb_addr, fb_area * 2, buffer);
              
              // Conversion from YUYV packing into RGBRGB
              //
              uint8   r,g,b;
              int     c,d,e;
              for (uint32 i = 0; i < fb_area / 2; ++i) {
                c = buffer[i*4+0] - 16;   // Y1
                d = buffer[i*4+1] - 128;  // U
                e = buffer[i*4+3] - 128;  // V
                
                r = std::max(0,std::min(255,(298*c + 409*e + 128)>>8));
                g = std::max(0,std::min(255,(298*c - 100*d - 208*e + 128)>>8));
                b = std::max(0,std::min(255,(298*c + 516*d + 128)>>8));
                
                framebuffer[i*8+0] = r;
                framebuffer[i*8+1] = g;
                framebuffer[i*8+2] = b;
                framebuffer[i*8+3] = 255;
                
                c = buffer [i*4+2] - 16;  // Y2
                
                r = std::max(0,std::min(255,(298*c + 409*e + 128)>>8));
                g = std::max(0,std::min(255,(298*c - 100*d - 208*e + 128)>>8));
                b = std::max(0,std::min(255,(298*c + 516*d + 128)>>8));
                
                framebuffer[i*8+4] = r;
                framebuffer[i*8+5] = g;
                framebuffer[i*8+6] = b;
                framebuffer[i*8+7] = 255;
              }
              
              // Free temporary frame buffer
              //
              if (buffer) { arcsim::util::Malloced::Delete(buffer); }
              
              break;
            }
            case FRAME_BUFFER_ONLY_RGB:
            {
              simReadBlock(sim_,  fb_addr, fb_width * fb_height * 4, framebuffer);
              break;
            }
            case FRAME_BUFFER_ONLY_GREY:
            {
              const uint32  fb_area = fb_width * fb_height;
              // Allocate enough space to copy framebuffer
              //
              uint8 * buffer = (uint8*)arcsim::util::Malloced::New(fb_area);
              
              if (buffer == 0) {
                LOG(LOG_ERROR) << "[IODeviceScreen] Temporary buffer allocation failed.";
              }
              // Perform actual copy of framebuffer into fb_copy
              //
              simReadBlock(sim_,  fb_addr, fb_area, buffer);
                            
              for (uint32 i = 0; i < fb_area; ++i) {
                framebuffer[i*4]   = buffer[i];
                framebuffer[i*4+1] = buffer[i];
                framebuffer[i*4+2] = buffer[i];
                framebuffer[i*4+3] = buffer[i];
              }
              
              // Free temporary frame buffer
              //
              if (buffer) { arcsim::util::Malloced::Delete(buffer); }

              break;
            }
            case CHAR_FRAME_BUFFER_PALLET:
            {
              const uint32 palette_buffer_size = 256 * 3; 
              const uint32 fb_area = fb_width * fb_height;
              
              // Allocate enough space to copy framebuffer
              //
              uint8 * buffer = (uint8*)arcsim::util::Malloced::New(fb_area);
              if (buffer == 0) {
                LOG(LOG_ERROR) << "[IODeviceScreen] Temporary buffer allocation failed.";
              }

              // Perform actual copy of framebuffer into fb_copy
              //
              simReadBlock(sim_,  fb_addr, fb_area, buffer);

              uint8 * palette_buffer = (uint8*)arcsim::util::Malloced::New(palette_buffer_size);

              if (palette_buffer == 0) {
                LOG(LOG_ERROR) << "[IODeviceScreen] Temporary palette buffer allocation failed.";
              }

              // Read palette pointer address
              //
              uint32 palette_start_addr = 0;
              simReadWord(sim_, base_addr_ + kPalettePointerAddr, &palette_start_addr);

              // Read palette into buffer
              //
              simReadBlock(sim_, palette_start_addr, palette_buffer_size, palette_buffer);
              
              for (uint32 i = 0; i < fb_area; ++i) {
                framebuffer[i*4]   = palette_buffer[buffer[i]*3];
                framebuffer[i*4+1] = palette_buffer[buffer[i]*3+1];
                framebuffer[i*4+2] = palette_buffer[buffer[i]*3+2];
                framebuffer[i*4+3] = buffer[i];
              }

              // Free temporary buffers
              //
              if (buffer)         { arcsim::util::Malloced::Delete(buffer);         }
              if (palette_buffer) { arcsim::util::Malloced::Delete(palette_buffer); }
              
              break;
            }
            case FRAME_BUFFER_ONLY_R5G6B5:
            {
              const uint32 fb_area = fb_width * fb_height;
              // Allocate enough space to copy framebuffer
              //
              uint16 * buffer = (uint16*)arcsim::util::Malloced::New(fb_area * 2);
              if (buffer == 0) {
                LOG(LOG_ERROR) << "[IODeviceScreen] Temporary buffer allocation failed.";
              }

              // Perform actual copy of framebuffer into fb_copy
              //
              simReadBlock(sim_,  fb_addr, fb_area * 2, (uint8*)buffer);

              for (uint32 i = 0; i < fb_area; ++i) {
                framebuffer[i*4]   = (buffer[i]&0xF800)>>8;
                framebuffer[i*4+1] = (buffer[i]&0x7E0)>>3;
                framebuffer[i*4+2] = (buffer[i]&0x1F)<<3;
                framebuffer[i*4+3] = 255;
              }              
              // Free temporary buffer
              //
              if (buffer)         { arcsim::util::Malloced::Delete(buffer);         }
              break;
            }
            default:
            {break;}
          } /* switch (fb_mode) */
        } /* if (fb_mode != CHAR_BUFFER_ONLY) */
        return;
      } /* parse_framebuffer() */
      
      void
      IODeviceScreen::frame_buffer_draw()
      {
        if (fb_mode != CHAR_BUFFER_ONLY)
        {
          // Framebuffer texture
          //
          GLuint          frame_buffer_texture;
          
          glEnable(GL_TEXTURE_2D);
          glDisable(GL_ALPHA_TEST);
          glGenTextures( 1, &frame_buffer_texture );
          
          // Select our current texture
          //
          glBindTexture( GL_TEXTURE_2D, frame_buffer_texture );
          
          // Select modulate to mix texture with colour for shading, we don't
          // actually care and would be fine with GL_REPLACE except it doesn't seem to work...
          //
          glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
          
          // We don't want any texture filtering, it' will probably fall back
          // to linear anyway which is fine
          //
          glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NONE);
          glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NONE);
          
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                       fb_width, fb_height, 0,
                       GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
          gluBuild2DMipmaps(GL_TEXTURE_2D,  GL_RGBA,
                            fb_width, fb_height,
                            GL_RGBA, GL_UNSIGNED_BYTE, framebuffer);
          
          
          glBegin(GL_QUADS);
          {
            glColor4f(1.0f,1.0f,1.0f,1.0f);
            glTexCoord2d(0.0f,1.0f);
            glVertex2i(0,SCREEN_YDIM);
            
            glTexCoord2d(0.0f,0.0f);
            glVertex2i(0,0);
            
            glTexCoord2d(1.0f,0.0f);
            glVertex2i(SCREEN_XDIM,0);
            
            glTexCoord2d(1.0f,1.0f);
            glVertex2i(SCREEN_XDIM,SCREEN_YDIM);
          }
          glEnd();
          glDeleteTextures (1, &(frame_buffer_texture));
        }
        return;
      }
      
      
      void
      IODeviceScreen::char_buffer_draw()
      {
        // mode switch for drawing the character display
        //
        if (   fb_mode == CHAR_FRAME_BUFFER_YUYV
            || fb_mode == CHAR_BUFFER_ONLY
            /* || fb_mode == CHAR_FRAME_BUFFER_PALLET */)
        {
          const uint32 palette_buffer_size = 256 * 3; 
                    
          uint8 * palette_buffer = (uint8*)arcsim::util::Malloced::New(palette_buffer_size);
          
          if (palette_buffer == 0) {
            LOG(LOG_ERROR) << "[IODeviceScreen] Temporary palette buffer allocation failed.";
          }
                    
          // Read palette into buffer
          //
          simReadBlock(sim_, base_addr_ + (SCREEN_XDIM * SCREEN_YDIM * 2),
                       palette_buffer_size, palette_buffer);
          
          glBindTexture(GL_TEXTURE_2D, texture);
          glEnable(GL_ALPHA_TEST);
          glEnable(GL_TEXTURE_2D);
          
          // ---------------------------------------------------------------------
          // Delimit the vertices of a primitive or a group of like primitives
          //
          glBegin(GL_QUADS);
          {
            for (int y = 0 ; y < SCREEN_YDIM; ++y) {
              for (int x = 0 ; x < SCREEN_XDIM; ++x) {
                unsigned char  chr = buffer[y][x][flip_clr_chr];
                
                // When character is '0' we do not draw over the frame buffer.
                // When character is '255', we draw a pixel from a palette.
                // Anything between '1' and '254' means draw a character over
                // the frame buffer.
                //
                if ( chr > 0) {
                  unsigned char clr = buffer[y][x][1-flip_clr_chr];
                  if (chr != 255) {
                    // Background Colour
                    //
                    glColor4f(cga_lookup[15 & (clr >> 4)][0],
                              cga_lookup[15 & (clr >> 4)][1],
                              cga_lookup[15 & (clr >> 4)][2],
                              1.0f);
                  } else {
                    // Rectangle Colour
                    //
                    glColor4f(((float)palette_buffer[(clr << 3) + 0])/255.0f,
                              ((float)palette_buffer[(clr << 3) + 1])/255.0f,
                              ((float)palette_buffer[(clr << 3) + 2])/255.0f,
                              1.0f);
                  }
                  
                  glTexCoord2d(0.0f,1.0f);
                  glVertex2i(x,y+1);
                  
                  glTexCoord2d(0.0f,0.0f);
                  glVertex2i(x,y);
                  
                  glTexCoord2d((1.0f)/128.0f,+0.0f);
                  glVertex2i(x+1,y);
                  
                  glTexCoord2d((1.0f)/128.0f,1.0f);
                  glVertex2i(x+1,y+1);
                  
                  if( chr != 255 ) {
                    // Draw character bitmap
                    //
                    float chrx  = (chr % 128);
                    // Foreground Color
                    //
                    glColor4f(cga_lookup[15 & clr][0],
                              cga_lookup[15 & clr][1],
                              cga_lookup[15 & clr][2],
                              1.0f);
                    
                    glTexCoord2d(chrx/128.0f,1.0f);
                    glVertex2i(x,y+1);
                    
                    glTexCoord2d(chrx/128.0f,0.0f);
                    glVertex2i(x,y);
                    
                    glTexCoord2d((chrx+1.0f)/128.0f,+0.0f);
                    glVertex2i(x+1,y);
                    
                    glTexCoord2d((chrx+1.0f)/128.0f,1.0f);
                    glVertex2i(x+1,y+1);
                  }
                }
              }
            }
          }
          glEnd();
          
          // Free temporary buffer
          //
          if (palette_buffer) { arcsim::util::Malloced::Delete(palette_buffer); }
          
        } // end if (   fb_mode == CHAR_FRAME_BUFFER_YUYV 
          //         || fb_mode == CHAR_BUFFER_ONLY
          //         || fb_mode == CHAR_FRAME_BUFFER_PALLET)
        return;
      }

            
} } } /* namespace arcsim::mem::mmap */
