//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//            Copyright (C) 2009 The University of Edinburgh
//                        All Rights Reserved
//
//
// =====================================================================

#ifndef INC_IO_MMAP_IODEVICESOUND_H_
#define INC_IO_MMAP_IODEVICESOUND_H_

#if (defined(__APPLE__) && defined(__MACH__))
# include <OpenAL/al.h>
# include <OpenAL/alc.h>
#else
# include <AL/al.h>
# include <AL/alc.h>
#endif

#include "api/types.h"

#include "mem/mmap/IODevice.h"
#include "mem/mmap/IODeviceIrq.h"

#include "concurrent/Thread.h"
#include "concurrent/Mutex.h"
#include "concurrent/ConditionVariable.h"

#include <string>

// Size of OpenAL sound queue
//
#define QUEUE_SIZE          64

// How many buffers should be in queue before we can start playing
//
#define QUEUE_READY_TO_PLAY 32

// 4096 minimum buffer size for OpenAL buffer - we play it safe and have a static
// buffer of size (BUFFER_CHUNKS * BUFFER_CHUNK) to avoid crackling sounds
//
#define BUFFER_CHUNKS       2048

// BUFFER_CHUNK must be multiple of 4
//
#define BUFFER_CHUNK        8


namespace arcsim {
  namespace mem     {
    namespace mmap {
      
      // -----------------------------------------------------------------------------
      // Type definitions and structures
      // -----------------------------------------------------------------------------
      
      typedef struct {
        simContext      context;
        unsigned short  format;     // PCM stream format (e.g. 0 = signed 8bit, 1 = unsigned 8bit)
        unsigned short  channels;   // Mono/Stereo
        unsigned int    sample_rate;// PCM stream frequency in Hz (e.g. 16000, 44100)
        unsigned short  state;      // sound device state (0, 1, 2 => AC97_STOP, AC97_PAUSE, AC97_PLAY)
        unsigned int    head;       // head of circular buffer in memory
        unsigned int    tail;       // tail of circular buffer in memory
        unsigned int    base;       // pointer to base of circular sound buffer
        unsigned int    top;        // pointer to top of circular sound buffer
        unsigned int    lower_water;// threshold indicating when HW interrupted should be asserted
        // (i.e. we are running low on data)
        unsigned int    upper_water;// threshold indicating when we have enough data and
        // can rescind HW interrupt
        // indicates if an interrupt has been raised and the upper water mark has not
        // been reached since. This in turn indicates the soundcard needs more data.
        // Values currently used are 0 (device is OK) and 1 (device needs more data) but
        // there is the capability in the external interrupt handler and device driver
        // to add other states if necessary (e.g. if sound recording is implemented)
        unsigned short  interrupting;
      } SoundControl;
      
      typedef struct {
        ALCdevice     *device;              // sound device
        ALCcontext    *context;             // sound context
        ALuint        buffer[QUEUE_SIZE];   // playback buffers
        ALuint        source;               // audio source
        ALenum        format;               // internal format
        ALenum        state;                // AL_SOURCE_STATE (i.e. AL_PLAYING, AL_STOPPED)
        ALenum        error;                // error code
      } SoundDevice;
      
      class IODeviceSound : public arcsim::mem::mmap::IODevice, public arcsim::concurrent::Thread {
      private:
        
        std::string id_;
        
        // Sound control 'register' - this is a shared variable
        //
        SoundControl sndCtrl;
        
        // Sound device
        //
        SoundDevice  dev;
                
        // Variables used for buffer/state synchronisation
        //
        arcsim::concurrent::Mutex             snd_mutex;
        arcsim::concurrent::ConditionVariable snd_cond_buf;
        arcsim::concurrent::ConditionVariable snd_cond_state;
        
        unsigned char done;
        
        IODeviceIrq*  irq_device;
        uint8         irq_line;
        
        void run();

        // sound device functions
        int    sound_device_open           ();
        int    sound_device_close          ();
        void   sound_device_display_error  (const char *msg);
        
      public:
        static const uint32 kPageBaseAddrIoSoundDevice;
        
        // Memory size this device is responsible for
        //
        static const uint32 kIoSoundMemorySize;
        
        explicit IODeviceSound(IODeviceIrq* irq_dev, uint8 irq_line);
        ~IODeviceSound();
        
        std::string& id();
        
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
      
} } }  /* namespace arcsim::mem::mmap */

#endif  // INC_IO_MMAP_IODEVICESOUND_H_
