//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Portable Timer class.
//
// =====================================================================


#ifndef INC_UTIL_SYSTEM_TIMER_H_
#define INC_UTIL_SYSTEM_TIMER_H_

#include "concurrent/Thread.h"

#include "api/types.h"

namespace arcsim {
  namespace util {
    namespace system {

      class Timer;
      
      // TimerCallbackInterface
      //
      class TimerCallbackInterface {
      protected:
        // Constructor MUST be protected and empty!
        //
        TimerCallbackInterface()
        { /* EMPTY */ }

      public:
        // Destructor MUST be declared AND virtual so all implementations of
        // this interface can be destroyed correctly.
        //
        virtual ~TimerCallbackInterface()
        { /* EMPTY */ }
        
        // Interface methods ---------------------------------------------------
        //
        
        // Method that is executed when timer expires
        //
        virtual void on_timer(Timer* timer) = 0;
      };
      
      // Timer class
      //
      class Timer : public arcsim::concurrent::Thread {
      public:
        // Default TIME SLICE (typically 10ms)
        //
        static const uint32 kDefaultTimeSlice;
        
        // Default PAUSE time
        //
        static const uint32 kDefaultPauseTime;

        // Timer states
        //
        enum TimerState { STOP, RUN, PAUSE, INITIAL, DESTROY };

        // Constructor/Destructor
        // Default is periodic timer using 10ms as a period
        //
        Timer(TimerCallbackInterface* callback, uint32 interval = 10, bool repeat = true, uint32 id = 0);
        ~Timer();
        
        // ------------------------------------------------------------------------
        // Timer management methods
        //
        void start();
        void stop();

        void pause();
        void resume();
        
        // Stops and destroys timer, it can not be used afterwards
        //
        void destroy();
        
        // Get/Set timer interval.
        //
        void    set_next_interval (uint32 interval) {
          if (interval == 0) { interval_nxt_ = 0; return; }
          if (interval < kDefaultTimeSlice) { interval_nxt_ = kDefaultTimeSlice; return; }
          interval_nxt_ = interval;
        }
        void    set_current_interval (uint32 interval) {
          if (interval == 0) { interval_cur_ = 0; return; }
          if (interval < kDefaultTimeSlice) { interval_cur_ = kDefaultTimeSlice; return; }
          interval_cur_ = interval;
        }

        uint32  get_current_interval ()          const { return interval_cur_;     }
        
        // ------------------------------------------------------------------------
        // Query timer state
        //
        bool        is_repeat()    const { return repeat_;	 }
        TimerState  get_state()    const { return state_;    }
        
      private:
        Timer(const Timer &);           // DO NOT COPY
        void operator=(const Timer &);  // DO NOT ASSIGN
        
        uint32                    id_;
        TimerCallbackInterface*   callback_;
        bool                      repeat_;
        uint32                    interval_cur_;
        uint32                    interval_nxt_;
        
        // Timer state variable
        //
        TimerState                state_;

        // ---------------------------------------------------------------------
        // Implementation of run() method 
        //
        void run();

      };

      
} } } // arcsim::util::system

#endif  // INC_UTIL_SYSTEM_TIMER_H_
