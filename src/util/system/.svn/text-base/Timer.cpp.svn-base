//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Portable Timer class impl.
//
// =====================================================================

#include "Assertion.h"

#include "util/system/Timer.h"
#include "util/Os.h"
#include "util/Log.h"


namespace arcsim {
  namespace util {
    namespace system {
      
      // Default TIME SLICE
      //
      const uint32 Timer::kDefaultTimeSlice = 1;
      
      // Default PAUSE time
      //
      const uint32 Timer::kDefaultPauseTime = 1000;
              
      // -----------------------------------------------------------------------
      // Timer constructor/destructor
      //
      Timer::Timer(TimerCallbackInterface* callback, uint32 interval,
                   bool repeat, uint32 id)
      : state_(INITIAL),
        callback_(callback),
        interval_cur_(interval),
        interval_nxt_(interval),
        repeat_(repeat),
        id_(id)
      {
        ASSERT(callback_ && "TimerCallbackInterface is NULL!");
      }
      
      Timer::~Timer()
      {
        // Set state to destroy if that has not happened yet
        //
        destroy();
        
        // Join timer thread thread
        //
        join();
      }
      

      // ----------------------------------------------------------------------- 
      // Timer run method
      //
      void
      Timer::run()
      {
        LOG(LOG_DEBUG1) << "[TIMER" << id_ << "] STARTED";
        
        // ---------------------------------------------------------------------
        // RUN LOOP START
        //
        while (state_ != DESTROY)
        {
          // Update state
          //
          TimerState state = state_;
          
          // Timer state machine
          //
          switch (state)
          {
            case RUN: {
              
              if (interval_cur_ == 0) { // STOP if current interval is 0
                stop();
                break;
              }
              
              // Let the clock tick
              //
              Os::sleep_nanos(kDefaultTimeSlice);
              
              LOG(LOG_DEBUG3) << "[TIMER" << id_ << "] TICK interval_cur_ = "
                              << interval_cur_ 
                              << " kDefaultTimeSlice = "
                              << kDefaultTimeSlice
                              << " interval_nxt_ = "
                              << interval_nxt_;
               
              if (interval_cur_ > kDefaultTimeSlice) {  // TIMER DID NOT EXPIRE YET
                interval_cur_ -= kDefaultTimeSlice;
              } else {                                  // TIMER EXPIRED - FIRE!
                interval_cur_ = interval_nxt_;  // set next inverval before callback
                                                // so it can be queried
                callback_->on_timer(this);      // execute callback method

                if (!repeat_) { // STOP if this is a one-shot timer
                  stop();
                  break;
                }
              }
              break;
            }
            case PAUSE:
            case STOP:
            {
              // FIXME: We should have a more efficient 'sleep' mechanism based on
              //        condition variables and signals.
              // Sleep for some time
              //
              Os::sleep_nanos(kDefaultPauseTime);

              // Set next interval in case someone changed state/interval
              //
              interval_cur_ = interval_nxt_;
              break;
            }
            case INITIAL:
            case DESTROY:
            {
              break;
            }
          }
        }
        //
        // RUN LOOP END
        // ---------------------------------------------------------------------

        LOG(LOG_DEBUG1) << "[TIMER" << id_ << "] DESTROYED";        
      }
      
      
      // -----------------------------------------------------------------------
      // Timer state modification methods
      //
      void
      Timer::start(void)
      {
        LOG(LOG_DEBUG3) << "[TIMER" << id_ << "] Timer::start, state = " << state_;
        
        if (state_ == DESTROY) { 
          return;
        }
                
        if (state_ == INITIAL) {
          state_ = RUN;
          LOG(LOG_DEBUG3) << "[TIMER" << id_ << "] Timer::start, state <= " << state_;
          Thread::start();
        }        
      }
      
      void
      Timer::stop(void)
      {
        if (state_ == DESTROY) { return;        }
        if (state_ != INITIAL) { state_ = STOP; }
        LOG(LOG_DEBUG3) << "[TIMER" << id_ << "] Timer::stop, state <= " << state_;
      }
      
      void
      Timer::pause(void)
      {
        if (state_ == DESTROY) { return; }
        if (state_ != INITIAL) { state_ = PAUSE; }
        LOG(LOG_DEBUG3) << "[TIMER" << id_ << "] Timer::pause, state <= " << state_;
      }
      
      void
      Timer::resume(void)
      {
        if (state_ == DESTROY) { return;          }
        if (state_ == INITIAL) { Thread::start(); }
        state_ = RUN;
        LOG(LOG_DEBUG3) << "[TIMER" << id_ << "] Timer::resume, state <= " << state_;
      }
      
      void
      Timer::destroy(void)
      {
        LOG(LOG_DEBUG3) << "[TIMER" << id_ << "] Timer::destroy";
        state_ = DESTROY;
      }
      
      
} } } // arcsim::util::system
