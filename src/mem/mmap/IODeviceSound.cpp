//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//            Copyright (C) 2009 The University of Edinburgh
//                        All Rights Reserved
//
//
// =====================================================================
//
//  Description: Memory mapped sound IO implementation. Software writes to a
//  Sound Control Register and tells the sound device where a sound buffer that
//  contains PCM sound data is located and what format (channels, frequency)
//  the data is in.
//
//  The software application is trying to fill a circular buffer by producing
//  PCM data and moving the 'head' pointer forward. The sound card looks into
//  the buffer and tries to grab chunks of data starting at 'tail'. If the sound
//  card is able to grab data from the buffer it will advance the 'tail' pointer
//  and thus 'consume' the PCM data. The circular buffer is delimited by 'base'
//  and 'top' pointers.
//
//  If the sound device is in playing state and the sound buffer is running
//  low, it will signal the software to produce more data by asserting a HW
//  interrupt.  By writing to the sound control 'register' the software can
//  acknowledge the interrupt.
//
//  NOTE: While the application produces PCM data in simulator memory, the
//  sound device has its own local buffer. When that LOCAL sound buffer is
//  'full' enough it is actually played. So if you don't write enough sound
//  data for the local sound buffer to fill up to its 'minimum' level, sound
//  will never be played. Since for our demonstration purposes we were playing
//  44100 KHz stereo AAC files we did not really run into the problem of not
//  filling up the sound cards local buffer.  If you want to play short sounds
//  you would need to change the sound device logic slightly.
//
//  Sound Control Registers:
//  ----------------------------------------------------------------------------
//  address     r/w  size                description
//  ----------------------------------------------------------------------------
//  0xFF020000  rw   32bit (int)   int - PCMFMT: PCM format of each audio sample
//  0xFF020004  rw   16bit (short) int - STAT: Sound controller status
//  0xFF020006  rw   16bit (short) int - CMD: Command and play/pause/stop status
//  0xFF02000C  rw   32bit (int)   int - TAIL: pointer to circular buffer's tail
//  0xFF020010  rw   32bit (int)   int - BASE: pointer to circular buffer's base
//  0xFF020014  rw   32bit (int)   int - TOP: pointer to top of circular buffer
//  0xFF020018  rw   32bit (int)   int - LOWATER: pointer to low water mark
//  Write register to acknowledge interrupt.
//  ----------------------------------------------------------------------------
//
//  All bits higher than the lsb of address 0xFF0200020 are 0 so reading a byte,
//  half word or word from this address will still return the state of the device.
//
//  OpenAL - http://en.wikipedia.org/wiki/OpenAL
//
// =====================================================================


// -----------------------------------------------------------------------------
// Headers
// -----------------------------------------------------------------------------

#include <cstdio>
#include <utility>
#include <vector>
#include <string>
#include <iomanip>

#if (defined(__APPLE__) && defined(__MACH__))
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

#include "mem/mmap/IODeviceIrq.h"
#include "mem/mmap/IODeviceSound.h"

#include "api/api_funs.h"
#include "api/mem/api_mem.h"

#include "util/Os.h"
#include "util/Log.h"


// -----------------------------------------------------------------------------
// Macros
// -----------------------------------------------------------------------------

namespace arcsim {
  namespace mem     {
    namespace mmap {
      
      const uint32 IODeviceSound::kPageBaseAddrIoSoundDevice = 0xFF020000;      
      const uint32 IODeviceSound::kIoSoundMemorySize         = 0x00002000;

      typedef enum {
        FORMAT_LO,      FORMAT_HI,
        CHANNELS_LO,    CHANNELS_HI,
        SAMPLE_RATE_LO, SAMPLE_RATE_HI,
        STATE_LO,       STATE_HI,
        HEAD_B0,        HEAD_B1,         HEAD_B2,        HEAD_B3,
        TAIL_B0,        TAIL_B1,         TAIL_B2,        TAIL_B3,
        BASE_B0,        BASE_B1,         BASE_B2,        BASE_B3,
        TOP_B0,         TOP_B1,          TOP_B2,         TOP_B3,
        LOWER_WATER_B0, LOWER_WATER_B1,  LOWER_WATER_B2, LOWER_WATER_B3,
        UPPER_WATER_B0, UPPER_WATER_B1,  UPPER_WATER_B2, UPPER_WATER_B3,
        INTERRUPTING_B0,INTERRUPTING_B1, INTERRUPTING_B2,INTERRUPTING_B3
      } SoundAddress;
      
      typedef enum {
        DEV_STOPPED,
        DEV_PAUSED,
        DEV_PLAYING,
        DEV_QUIT
      } DeviceState;
      
      typedef enum {
        MONO  = 1,
        STEREO
      } Channels;
      
      
      IODeviceSound::IODeviceSound(IODeviceIrq* irq_dev, uint8 irq_line_)
      : id_(std::string("sound")),
        irq_device(irq_dev),
        irq_line(irq_line_)
      { /* EMPTY */ }

      IODeviceSound::~IODeviceSound()
      { /* EMPTY */ }

      
      std::string&
      IODeviceSound::id()
      {
        return id_;
      }
      
      /**
       *  Initialise memory mapped sound extension. A page has 8K and we have
       *  40K of memory mapped IO. Thus we need to register 5 pages.
       */
      int 
      IODeviceSound::configure (simContext sim, IocContext sys_ctx, uint32 addr)
      {
        sim_       = sim;
        base_addr_ = addr;
        
        // Initialise SoundControl structure
        //
        sndCtrl.context      = sim;
        sndCtrl.base         = sndCtrl.top = sndCtrl.head = sndCtrl.tail = 0;
        sndCtrl.state        = DEV_STOPPED;            // AC97_STOP
        sndCtrl.channels     = STEREO;                 // 2 Channels
        sndCtrl.sample_rate  = 44100;                  // default sample rate of 44KHz
        sndCtrl.interrupting = 0;                      // not interrupting, device is OK
        sndCtrl.format       = AL_FORMAT_STEREO16;
        
        // Initilise SoundDevice
        //
        dev.device  = NULL;
        dev.context = NULL;
        dev.format  = AL_FORMAT_STEREO16;
        
        return IO_API_OK;
      }
      
      // ---------------------------------------------------------------------
      // Query range for which this MemoryDevice is responsible
      //
      uint32
      IODeviceSound::get_range_begin()      const
      {
        return base_addr_;
      }
      
      uint32
      IODeviceSound::get_range_end  ()      const
      {
        return (base_addr_ + kIoSoundMemorySize);
      }

      
      // Clean up sound io.
      ////
      int 
      IODeviceSound::dev_stop()
      {
        done  = 1;
        snd_mutex.acquire();
        snd_cond_state.broadcast();
        snd_cond_buf.broadcast();
        snd_mutex.release();
        join();
        return IO_API_OK;
      }
      
      
      // Called upon page initialisation
      ////
      int 
      IODeviceSound::dev_start()
      {
        // Try to open sound device
        ////
        if (sound_device_open () != IO_API_OK)
          return IO_API_ERROR;
        
        LOG(LOG_DEBUG2) << "[IO-SOUND] Started Sound Device";

        // Create sound device thread
        ////
        start();
        
        return IO_API_OK;
      }
      
      // Called upon page initialisation
      ////
      int 
      IODeviceSound::mem_dev_init (uint32 val)
      {
        return IO_API_OK;
      }
      
      int
      IODeviceSound::mem_dev_clear(uint32 val)
      {
        return IO_API_OK;
      }
      
      // -----------------------------------------------------------------------
      // Debug read/write implementations
      //
      int
      IODeviceSound::mem_dev_read  (uint32 addr, unsigned char* dest, int size, int agent_id)
      {
        return mem_dev_read(addr, dest, size);
      }
      
      int
      IODeviceSound::mem_dev_write (uint32 addr, const unsigned char *data, int size, int agent_id)
      {
        return mem_dev_write(addr, data, size);
      }

      
      // Called upon page read
      ////
      int 
      IODeviceSound::mem_dev_read (uint32 addr, unsigned char* dest, int size)
      {
        // Quickly create copy of control register as we do not want to lock it for long
        //
        snd_mutex.acquire();
        SoundControl _sndCtrl = sndCtrl;
        snd_mutex.release();
        
        unsigned char* ptr = dest;
        
        while(size>0) {
          switch(addr-base_addr_) {
            case FORMAT_LO:       *ptr = (unsigned char)(_sndCtrl.format); break;
            case FORMAT_HI:       *ptr = (unsigned char)(_sndCtrl.format>>8) ; break;
            case CHANNELS_LO:     *ptr = 7 & (unsigned char)(_sndCtrl.channels); break; //hardware only stores the lower 3 bits
            case CHANNELS_HI:     *ptr = 0; break; //see 2:
            case SAMPLE_RATE_LO:  *ptr = (unsigned char)(_sndCtrl.sample_rate); break;
            case SAMPLE_RATE_HI:  *ptr = (unsigned char)(_sndCtrl.sample_rate>>8); break;
            case STATE_LO:        *ptr = 7 & *ptr; break; //hardware only stores the lower 3 bits
            case STATE_HI:        *ptr = 0; break; //see 7:
            case HEAD_B0:         *ptr = (unsigned char)(_sndCtrl.head); break;
            case HEAD_B1:         *ptr = (unsigned char)(_sndCtrl.head>>8); break;
            case HEAD_B2:         *ptr = (unsigned char)(_sndCtrl.head>>16); break;
            case HEAD_B3:         *ptr = (unsigned char)(_sndCtrl.head>>24); break;
            case TAIL_B0:         *ptr = (unsigned char)(_sndCtrl.tail); break;
            case TAIL_B1:         *ptr = (unsigned char)(_sndCtrl.tail>>8); break;
            case TAIL_B2:         *ptr = (unsigned char)(_sndCtrl.tail>>16); break;
            case TAIL_B3:         *ptr = (unsigned char)(_sndCtrl.tail>>24); break;
            case BASE_B0:         *ptr = (unsigned char)(_sndCtrl.base); break;
            case BASE_B1:         *ptr = (unsigned char)(_sndCtrl.base>>8); break;
            case BASE_B2:         *ptr = (unsigned char)(_sndCtrl.base>>16); break;
            case BASE_B3:         *ptr = (unsigned char)(_sndCtrl.base>>24); break;
            case TOP_B0:          *ptr = (unsigned char)(_sndCtrl.top); break;
            case TOP_B1:          *ptr = (unsigned char)(_sndCtrl.top>>8); break;
            case TOP_B2:          *ptr = (unsigned char)(_sndCtrl.top>>16); break;
            case TOP_B3:          *ptr = (unsigned char)(_sndCtrl.top>>24); break;
            case LOWER_WATER_B0:  *ptr = (unsigned char)(_sndCtrl.lower_water); break;
            case LOWER_WATER_B1:  *ptr = (unsigned char)(_sndCtrl.lower_water>>8); break;
            case LOWER_WATER_B2:  *ptr = (unsigned char)(_sndCtrl.lower_water>>16); break;
            case LOWER_WATER_B3:  *ptr = (unsigned char)(_sndCtrl.lower_water>>24); break;
            case UPPER_WATER_B0:  *ptr = (unsigned char)(_sndCtrl.upper_water); break;
            case UPPER_WATER_B1:  *ptr = (unsigned char)(_sndCtrl.upper_water>>8); break;
            case UPPER_WATER_B2:  *ptr = (unsigned char)(_sndCtrl.upper_water>>16); break;
            case UPPER_WATER_B3:  *ptr = (unsigned char)(_sndCtrl.upper_water>>24); break;
            case INTERRUPTING_B0: *ptr = (unsigned char)(_sndCtrl.interrupting); break;
            case INTERRUPTING_B1: *ptr = 0; break;
            case INTERRUPTING_B2: *ptr = 0; break;
            case INTERRUPTING_B3: *ptr = 0; break;
            default:
              LOG(LOG_DEBUG4) << "[IO-SOUND] READ ERROR ADDR LOCATION: 0x"
                             << std::hex << std::setw(8) << std::setfill('0') << addr;
              break;
          }
          --size;
          ++ptr;
          ++addr;
        }
        return 0;
      }
      
      // Write to page 0
      ////
      int 
      IODeviceSound::mem_dev_write (uint32 addr, const unsigned char *data, int size)		
      {
        snd_mutex.acquire();
        
        unsigned char*  ptr           = (unsigned char*)data;
        char            state_changed = ((addr-base_addr_) < 8) ? 1 : 0;
        
        while(size > 0) {
          switch(addr-base_addr_){
            case FORMAT_LO:     sndCtrl.format = (sndCtrl.format & 0xFF00) | *ptr; break;
            case FORMAT_HI:     sndCtrl.format = (sndCtrl.format & 0x00FF) | ((*ptr)<<8); break;
            case CHANNELS_LO:   sndCtrl.channels = 7 & *ptr; break; //hardware only stores the lower 3 bits
            case CHANNELS_HI:   break; //see 2:
            case SAMPLE_RATE_LO:sndCtrl.sample_rate = (sndCtrl.sample_rate & 0xFF00) | *ptr; break;
            case SAMPLE_RATE_HI:sndCtrl.sample_rate = (sndCtrl.sample_rate & 0x00FF) | ((*ptr)<<8); break;
            case STATE_LO:      sndCtrl.state = 7 & *ptr; break; //hardware only stores the lower 3 bits
            case STATE_HI:      break; //see 7:
            case HEAD_B0:
              sndCtrl.head = (sndCtrl.head & 0xFFFFFF00) | *ptr;
              // Application moves head pointer so we signal sound card that more data
              // is available
              ////
              snd_cond_buf.broadcast();
              break;
            case HEAD_B1:       sndCtrl.head = (sndCtrl.head & 0xFFFF00FF) | ((*ptr)<<8); break;
            case HEAD_B2:       sndCtrl.head = (sndCtrl.head & 0xFF00FFFF) | ((*ptr)<<16); break;
            case HEAD_B3:       sndCtrl.head = (sndCtrl.head & 0x00FFFFFF) | ((*ptr)<<24); break;
            case TAIL_B0:       sndCtrl.tail = (sndCtrl.tail & 0xFFFFFF00) | *ptr; break;
            case TAIL_B1:       sndCtrl.tail = (sndCtrl.tail & 0xFFFF00FF) | ((*ptr)<<8); break;
            case TAIL_B2:       sndCtrl.tail = (sndCtrl.tail & 0xFF00FFFF) | ((*ptr)<<16); break;
            case TAIL_B3:       sndCtrl.tail = (sndCtrl.tail & 0x00FFFFFF) | ((*ptr)<<24); break;
            case BASE_B0:       sndCtrl.base = (sndCtrl.base & 0xFFFFFF00) | *ptr; break;
            case BASE_B1:       sndCtrl.base = (sndCtrl.base & 0xFFFF00FF) | ((*ptr)<<8); break;
            case BASE_B2:       sndCtrl.base = (sndCtrl.base & 0xFF00FFFF) | ((*ptr)<<16); break;
            case BASE_B3:       sndCtrl.base = (sndCtrl.base & 0x00FFFFFF) | ((*ptr)<<24); break;
            case TOP_B0:        sndCtrl.top  = (sndCtrl.top & 0xFFFFFF00) | *ptr; break;
            case TOP_B1:        sndCtrl.top  = (sndCtrl.top & 0xFFFF00FF) | ((*ptr)<<8); break;
            case TOP_B2:        sndCtrl.top  = (sndCtrl.top & 0xFF00FFFF) | ((*ptr)<<16); break;
            case TOP_B3:        sndCtrl.top  = (sndCtrl.top & 0x00FFFFFF) | ((*ptr)<<24); break;
            case LOWER_WATER_B0:sndCtrl.lower_water = (sndCtrl.lower_water & 0xFFFFFF00) | *ptr; break;
            case LOWER_WATER_B1:sndCtrl.lower_water = (sndCtrl.lower_water & 0xFFFF00FF) | ((*ptr)<<8); break;
            case LOWER_WATER_B2:sndCtrl.lower_water = (sndCtrl.lower_water & 0xFF00FFFF) | ((*ptr)<<16); break;
            case LOWER_WATER_B3:sndCtrl.lower_water = (sndCtrl.lower_water & 0x00FFFFFF) | ((*ptr)<<24); break;
            case UPPER_WATER_B0:sndCtrl.upper_water = (sndCtrl.upper_water & 0xFFFFFF00) | *ptr; break;
            case UPPER_WATER_B1:sndCtrl.upper_water = (sndCtrl.upper_water & 0xFFFF00FF) | ((*ptr)<<8); break;
            case UPPER_WATER_B2:sndCtrl.upper_water = (sndCtrl.upper_water & 0xFF00FFFF) | ((*ptr)<<16); break;
            case UPPER_WATER_B3:sndCtrl.upper_water = (sndCtrl.upper_water & 0x00FFFFFF) | ((*ptr)<<24); break;
            case INTERRUPTING_B0: {
              if (sndCtrl.interrupting) {
                LOG(LOG_DEBUG4) << "[IO-SOUND] SW_RESCIND_EXT_IRQ(2)";
                irq_device->rescind_ext_irq(irq_line);
              }
              break;
            }
            case INTERRUPTING_B1: break;
            case INTERRUPTING_B2: break;
            case INTERRUPTING_B3: break;
              
            default:
              LOG(LOG_DEBUG4) << "[IO-SOUND] WRITE ERROR ADDR LOCATION: 0x"
                             << std::hex << std::setw(8) << std::setfill('0') << addr;
              break;
          }
          --size;
          ++addr;
          ++ptr;
        }
        // Signal sound card if something has changed
        if (state_changed) {
          switch (sndCtrl.state) {
            case DEV_STOPPED:
            case DEV_PAUSED: {
              LOG(LOG_DEBUG4) << "[IO-SOUND] - STOPPED/PAUSED";
            }
              break;
            case DEV_PLAYING: {
              LOG(LOG_DEBUG4) << "[IO-SOUND] - PLAYING";
              // Signal sound card that it can start playing
              ////
              snd_cond_state.broadcast();
              break;
            }
            default: break;
          }
        }
        
        snd_mutex.release();
        
        return 0;
      }
      
      /**
       * SOUND DEVICE THREAD: This function implements the sound device. I tried to
       * write it as simple as possible but it still is complicated. If you are the
       * poor sole who has to read/modify the following code take a pen and paper first
       * and make a picture of how the sound card works before blindly starting to
       * modify things. In order to avoid 'busy loops' we use two condition variables
       * to wait for signals from the simulator when some memory has been modified and
       * we can go ahead and try to do some work.
       *
       * Also have a look at how OpenAL works - it is well documented.
       *
       */
      void 
      IODeviceSound::run ()
      {
        int               buf_processed;    // how many buffers have been played/processed
        int               buf_queued;       // how many buffers are queued for playing
        int               buf_level;        // how much data is in 'sound buffer' and here
        ALuint            buffer;           // sound buffer
        unsigned int      buf_current = 0;  // index into local sound buffer array
        
        LOG(LOG_DEBUG) << "[IO-SOUND] Sound Device Thread Started.";

        for (; /* ever */ ;) {
          
          if(done==1)break;
          
          //  Wait until state == DEV_PLAYING
          ////
          snd_mutex.acquire();
          while (sndCtrl.state != DEV_PLAYING) {
            
            // Pause playing
            ////
            alGetSourcei(dev.source, AL_SOURCE_STATE, &dev.state);
            if (dev.state == AL_PLAYING) {
              LOG(LOG_DEBUG4) << "[IO-SOUND] - Pausing Playback";
              alSourcePause(dev.source);
            }
            if (sndCtrl.state == DEV_STOPPED) {
              ; /* TBD: Purge the device buffer */
            }
            // Wait until simulator tells us we can start playing again
            ////
            snd_cond_state.wait(snd_mutex);            
          }

          if(done==1){
            snd_mutex.release();
            break;
          }
          // Start Playing
          ////
          
          // Calculate buf_level
          ////
          if(sndCtrl.head >= sndCtrl.tail) { // in case it wrapped
            buf_level = sndCtrl.head - sndCtrl.tail;
          } else {
            buf_level = (sndCtrl.top - sndCtrl.tail) + (sndCtrl.head - sndCtrl.base);
          }
          
          // Do we need to assert/rescind HW interrupts?
          ////
          if ((unsigned)buf_level > sndCtrl.upper_water) {  // Enough data in buffer
            if (sndCtrl.interrupting) {           // Rescind HW interrupt
              LOG(LOG_DEBUG4) << "[IO-SOUND] - RESCIND_EXT_IRQ(2)";
              irq_device->rescind_ext_irq(irq_line);
              sndCtrl.interrupting = !sndCtrl.interrupting;
            }
          }
          if ((unsigned)buf_level < sndCtrl.lower_water ) { // Buffer running low on data
            if (!sndCtrl.interrupting) {          // Assert HW interrupt (i.e. tell SW to fill buffer)
              LOG(LOG_DEBUG4) << "[IO-SOUND] - ASSERT_EXT_IRQ(2)";
              irq_device->assert_ext_irq(irq_line);
              sndCtrl.interrupting = !sndCtrl.interrupting;
            }
          }
          
          // How much sound data is queued?
          ////
          alGetSourcei(dev.source, AL_BUFFERS_QUEUED, &buf_queued);
          
          // 1. CASE - Sound queue/buffer is full:
          //      - unlock mutex immediately
          //      - wait a little (meanwhile sound is playing)
          //      - after little pause remove processed sound buffers
          ////
          if (buf_queued >= (QUEUE_SIZE-1)) {            
            // RELEASE LOCK QUICKLY
            ////
            snd_mutex.release();
            
            // MORE DATA THAN WE CAN HANDLE - SIT BACK AND ENJOY SOUND!
            ////
            LOG(LOG_DEBUG4) << "[IO-SOUND] - SOUND CARD BUFFER FULL - ENJOY SOUND";

            // If sound card has enough data start playing
            ////
            arcsim::util::Os::sleep_micros(1000);
            
            // REMOVE ALREADY PROCESSED DATA
            ////
            alGetSourcei(dev.source, AL_BUFFERS_PROCESSED, &buf_processed);
            LOG(LOG_DEBUG4) << "[IO-SOUND] - REMOVED '"
                           << buf_processed << "' PROCESSED BUFFERS FROM SOUND QUEUE";

            while(buf_processed--) {
              alSourceUnqueueBuffers(dev.source, 1, &buffer);
            }
          } else if (buf_level > (BUFFER_CHUNK * BUFFER_CHUNKS)) {
            // 2. CASE - Enough data present to be transferred to sound buffer:
            //      - create temporary copy of sndCtrl 'register'
            //      - adjust sndCtrl.tail
            //      - unlock mutex
            //      - retrieve sound data from simulator memory
            //      - remove already processed/played buffers from sound card buffer
            //      - add new data to local buffer with correct settings (i.e. frequency, format)
            //      - determine next local buffer to be filled with data
            //      - if sound card has enough data start playing :-)
            ////
            LOG(LOG_DEBUG4) << "[IO-SOUND] - THERE's ENOUGH DATA";

            uint32          buf_data[BUFFER_CHUNKS * (BUFFER_CHUNK >> 2)];
            SoundControl    _sndCtrl;
            
            // Create temporary copy of control register
            ////
            _sndCtrl = sndCtrl;
            
            // Adjust sndCtrl.tail
            ////
            for (int chunk = 0; chunk < BUFFER_CHUNKS; ++chunk) {
              sndCtrl.tail += BUFFER_CHUNK;        // adjust tail so we can release lock quickly
              if (sndCtrl.tail >= (sndCtrl.top - BUFFER_CHUNK)) { // Do we need to loop around?
                sndCtrl.tail = sndCtrl.base;
              }
            }
            
            // UNLOCK MUTEX
            ////
            snd_mutex.release();
            
            for (int chunk = 0; chunk < (BUFFER_CHUNK/4)*BUFFER_CHUNKS; ++chunk) {
              // Here we are reading from simulation memory, asynchronously to main
              // simulation loop.
              //
              simReadWord (sim_, _sndCtrl.tail, &buf_data[chunk]);
              
              _sndCtrl.tail += 4;
              if (_sndCtrl.tail >= (_sndCtrl.top - 4)) { // Do we need to loop around?
                _sndCtrl.tail = _sndCtrl.base;
              }
            }
            
            // Remove already processed buffers from sound card buffer
            ////
            alGetSourcei(dev.source, AL_BUFFERS_PROCESSED, &buf_processed);
            LOG(LOG_DEBUG4) << "[IO-SOUND] - REMOVED '"
                           << buf_processed << "' PROCESSED BUFFERS FROM SOUND QUEUE";
            while(buf_processed--) {
              alSourceUnqueueBuffers(dev.source, 1, &buffer);
            }
            
            // Add new data to local buffer with correct settings (i.e. frequency, format)
            ////
            alBufferData(dev.buffer[buf_current],       // buffer name to be filled with data
                         AL_FORMAT_STEREO16,             // format type (i.e. Stereo16, Mono8 etc.)
                         &buf_data,                      // pointer to audio data
                         (BUFFER_CHUNK * BUFFER_CHUNKS), // the size of audio data in bytes
                         _sndCtrl.sample_rate);          // the frequency of audio data
            
            // Put newly created local buffer into sound card buffer queue
            ////
            alSourceQueueBuffers(dev.source, 1, &dev.buffer[buf_current]);
            
            // Determine next local buffer to be filled with data
            ////
            if (! (++buf_current < QUEUE_SIZE)) {
              buf_current = 0;
            }
            
            // If sound card has enough data start playing
            ////
            alGetSourcei(dev.source, AL_BUFFERS_QUEUED, &buf_queued);
            LOG(LOG_DEBUG4) << "[IO-SOUND] - ADDED '1' BUFFER TO SOUND QUEUE - '"
                           << buf_queued << "' BUFFERS QUEUED";

            if (buf_queued >= QUEUE_READY_TO_PLAY) {
              alGetSourcei(dev.source, AL_SOURCE_STATE, &dev.state);
              if (dev.state != AL_PLAYING) {
                LOG(LOG_DEBUG4) << "[IO-SOUND] - ENOUGH DATA IN QUEUE - STARTING PLAYBACK";
                alSourcePlay(dev.source);
              }
            }
            
          } else {
            // 2. CASE - NOT enough data present to be put into sound card queue and sound
            //      card queue is not full:
            //        - wait until we get a signal from the simulator 'cond_snd_buf_modified'
            //          when someone has written to the 'sndCtrl' register. That means that
            //          hopefully the 'sndCtrl.head' has been modified signalling that more
            //          data is ready to be played.
            ////
            
            // Wait until simulator tells us that more data has arrived, this way we
            // avoid wasting CPU cycles
            ////
            LOG(LOG_DEBUG4) << "[IO-SOUND] - WAITING FOR MORE DATA";

            snd_cond_buf.wait(snd_mutex);
            
            // UNLOCK MUTEX
            ////
            snd_mutex.release();
          }
          
          
        } /* END for (EVER) loop */
        LOG(LOG_DEBUG4) << "[IO-SOUND] Sound Device Thread Stopped.";
        return;
      }
      
      int 
      IODeviceSound::sound_device_close () {
        // FIXME: Properly shut down OpenAL and Co.
        return 0;
      }
      
      
      int 
      IODeviceSound::sound_device_open ()
      {
        int retval = IO_API_OK;
        
        // Create OpenAL device
        // Pass NULL to specify the system’s default output device
        dev.device = alcOpenDevice(NULL);
        if (dev.device) {
          // Create a new OpenAL Context
          // The new context will render to the OpenAL Device just created
          dev.context = alcCreateContext(dev.device, NULL);
          if (dev.context) {
            // Make the new context the Current OpenAL Context
            alcMakeContextCurrent(dev .context);
          } else {
            LOG(LOG_ERROR) << "[IO-SOUND] Sound Context could not be created.";
            return IO_API_ERROR;
          }
          
        } else {
          LOG(LOG_ERROR) << "[IO-SOUND] Sound Device can not be opened.";
          return IO_API_ERROR;
        }
                
        // Configure OpenAL (see http://www.devmaster.net/articles/openal-tutorials/lesson8.php)
        alGenBuffers(QUEUE_SIZE, dev.buffer);
        if ((dev.error = alGetError()) != AL_NO_ERROR)
          sound_device_display_error( "Could not create buffers");
        
        alGenSources(1, &dev.source);
        if ((dev.error = alGetError()) != AL_NO_ERROR)
          sound_device_display_error( "Could not create source");
        
        
        alSource3f(dev.source, AL_POSITION,        0.0f, 0.0f, 0.0f);
        alSource3f(dev.source, AL_VELOCITY,        0.0f, 0.0f, 0.0f);
        alSource3f(dev.source, AL_DIRECTION,       0.0f, 0.0f, 0.0f);
        alSourcef (dev.source, AL_PITCH,           1.0f);
        alSourcef (dev.source, AL_GAIN,            1.0f);
        alSourcef (dev.source, AL_ROLLOFF_FACTOR,  0.0f);
        alSourcei (dev.source, AL_SOURCE_RELATIVE, AL_TRUE);
        
        LOG(LOG_DEBUG) << "[IO-SOUND] Configured sound device.";

        return retval;
      }
      
      void 
      IODeviceSound::sound_device_display_error (const char *msg)
      {
        const char *errMsg = NULL;
        switch (dev.error)
        {
          case AL_NO_ERROR:     { errMsg = "None";            break; }
          case AL_INVALID_NAME: { errMsg = "Invalid name.";   break; }
          case AL_INVALID_ENUM: { errMsg = "Invalid enum.";   break; }
          case AL_INVALID_VALUE:{ errMsg = "Invalid value.";  break; }
            // FIXME: check further errors
          default:              { errMsg = "Unknown error.";  break; }
        }
        LOG(LOG_ERROR) << "[IO-SOUND] " << msg << " " << errMsg;
      }
      
} } }  /* namespace arcsim::mem::mmap */

