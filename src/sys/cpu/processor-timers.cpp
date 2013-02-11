//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//            Copyright (C) 2006 The University of Edinburgh
//                        All Rights Reserved
//
//                                             
// =====================================================================
//
// Description:
//
// This file implements the timer logic defined by the ARCompact ISA.
// The state information relating to each timer is contained within
// the Processor class, so all timer functions are members of Processor
// class.
//
// The ARCompact timers are implemented using the arcsim::util::system::Timer
// class and implementing the arcsim::util::system::TimerCallbackInterface.
// 
// The CONTROLn.NH bit (not halted) is used as timer enable when
// the simulation is halted (but the simulator is still running).
// If the bit is set, the host timer will be stopped when the
// processor halts. If the bit is clear, the host timer will
// run continuously. Hence, if CONTROL0.NH or CONTROL1.NH are
// clear, the timer runs continously.
// 
// If the CONTROLn.IE is set when timer n expires, it will 
// set the appropriate interrupt line to the processor and also
// set CONTROLn.IP to 1.
// 
// If the CONTROLm.W bit is set when timer n expires, it will
// raise a RESET exception.
//
// TODO: @igor - implement proper time abstraction class that transparently
//       handles ALL the timer modes we support. That means we should remove
//       all timer-related instance variables in the processor class as they
//       should be members of the Timer class implementation.
//
// =====================================================================

#include <iomanip>

#include "sys/cpu/aux-registers.h"
#include "sys/cpu/processor.h"

#include "util/Log.h"
#include "util/Counter.h"

// The frequency at which the CPU is supposedly running, in MHz
//
#define DEFAULT_MHZ 20

// The number of microseconds in each host timer period. This determines
// the rate of host timer callback interrupts to normal simulation.
// If the host timer cannot support this value, it will be set to the
// minimum supported host timer interval.
//
#define MIN_TICK_USEC 1

// Macros to test if TIMER0, TIMER1, and 'host timer' are enabled
//
#define IS_HOST_TIMER_ENABLED() (sys_arch.isa_opts.use_host_timer)
#define IS_T0_ENABLED()   (sys_arch.isa_opts.has_timer0)
#define IS_T1_ENABLED()   (sys_arch.isa_opts.has_timer1)
#define IS_T0_AND_T1_DISABLED() (!IS_T0_ENABLED() && !IS_T1_ENABLED())

// Convenience MACRO to output HEX formatted number to a stream
//
#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_

namespace arcsim {
  namespace sys {
    namespace cpu {

      // -----------------------------------------------------------------------
      // Callback method called upon Timer expiry
      //
      void
      Processor::on_timer(arcsim::util::system::Timer* _timer)
      {
        LOG(LOG_DEBUG3) << "[TIMER] CALLBACK: on_timer()";
        
        if (running0)
          vcount0 += count_increment;
          
        if (running1)
          vcount1 += count_increment;
          
        detect_timer_expiry ();
      }

      // -----------------------------------------------------------------------
      // Method to check whether any virtual counter value is equal to,
      // or greater than, the matching LIMIT register. Interrupts may be
      // raised in this case, if enabled.
      //
      void
      Processor::detect_timer_expiry ()
      {
#if defined(CYCLE_ACC_SIM)
        uint64 now  = (sim_opts.cycle_sim)  ? cnt_ctx.cycle_count.get_value() : instructions();
#else
        uint64 now  = instructions();
#endif
        if (vcount0 >= (sint64)(state.auxs[AUX_LIMIT0])) {
          LOG(LOG_DEBUG3) << "[TIMER] T0 expired at " << now;

          vcount0                = vcount0 - state.auxs[AUX_LIMIT0];
          state.auxs[AUX_COUNT0] = (vcount0 < 0) ? 0 : vcount0;
          
          // If CONTROL0.IE == 1, then set CONTROL0.IP and AUX_IRQ_PENDING[T0_IRQ_NUMBER]
          if (state.auxs[AUX_CONTROL0] & 1UL) {
            LOG(LOG_DEBUG3) << "[TIMER] Raising IRQ "
                            << get_timer0_irq_num()
                            << " at cycle " << now
                            << ", CONTROL0 = 0x" << HEX(state.auxs[AUX_CONTROL0]);
            state.auxs[AUX_CONTROL0] |= 0x8UL;   // set CONTROL0.IP bit
            assert_interrupt_line(get_timer0_irq_num());
          } else {
            LOG(LOG_DEBUG3) << "[TIMER] T0 interrupt is disabled, CONTROL0 = 0x"
                            << HEX(state.auxs[AUX_CONTROL0]);
          }
          // If CONTROL0.W, then reset the processor immediately.
          if (state.auxs[AUX_CONTROL0] & 0x4UL) {
            LOG(LOG_DEBUG3) << "[TIMER] Asserting T0 watchdog reset";
            system_reset ();
          }
          
          // If using non-host timers, then compute the time to the next expiry
          if (inst_timer_enabled) {
            timer_advance_cycles();
            time_to_expiry();
          }
        }
        
        if (vcount1 >= state.auxs[AUX_LIMIT1]) {
          LOG(LOG_DEBUG3) << "[TIMER] T1 expired at " << now;

          vcount1                = vcount1 - state.auxs[AUX_LIMIT1];
          state.auxs[AUX_COUNT1] = (vcount1 < 0) ? 0 : vcount1;

          // If CONTROL1.IE == 1, then set CONTROL1.IP and AUX_IRQ_PENDING[T1_IRQ_NUMBER]
          if (state.auxs[AUX_CONTROL1] & 1UL) {
            LOG(LOG_DEBUG3) << "[TIMER] Raising IRQ "
                            << get_timer1_irq_num()
                            << " at cycle " << now
                            << ", CONTROL1 = 0x" << HEX(state.auxs[AUX_CONTROL1]);
            state.auxs[AUX_CONTROL1] |= 8UL;    // set CONTROL0.IP bit            
            assert_interrupt_line(get_timer1_irq_num());
          } else {
            LOG(LOG_DEBUG3) << "[TIMER] T0 interrupt is disabled, CONTROL1 = 0x"
                            << HEX(state.auxs[AUX_CONTROL1]);
          }
          // If CONTROL1.W, then reset the processor immediately.
          if (state.auxs[AUX_CONTROL1] & 0x4UL) {
            LOG(LOG_DEBUG3) << "[TIMER] Asserting T1 watchdog reset";
            system_reset ();
          }
          
          // If using non-host timers, then compute the time to the next expiry.
          if (inst_timer_enabled) {
            timer_advance_cycles();
            time_to_expiry();
          }
        }
      }
      
      // -----------------------------------------------------------------------
      // Initialise timers according to TIMER_BUILD BCR (Build Configuration Register)
      // Sets up the handle_timer handler, and possibly starts the host interval timer.
      //
      void
      Processor::init_timers ()
      {
        LOG(LOG_DEBUG1) << "[TIMER] INIT";
        state.auxs[AUX_COUNT0] = 0;
        state.auxs[AUX_COUNT1] = 0;
        state.auxs[AUX_LIMIT0] = 0x00ffffff;
        state.auxs[AUX_LIMIT1] = 0x00ffffff;
        state.auxs[AUX_CONTROL0] = 0;
        state.auxs[AUX_CONTROL1] = 0;
        running0 = running1 = false;
        vcount0  = vcount1  = 0;
        count_increment = 0;
        timer_sync_time = 0;

        // Get the user option for using host timers or instruction-count
        // approximation to cpu clock cycles. If host timer is enabled,
        // then it must be set up and enabled.
        //
        use_host_timer     = IS_HOST_TIMER_ENABLED();
        inst_timer_enabled = !IS_HOST_TIMER_ENABLED() && ( IS_T0_ENABLED() || IS_T1_ENABLED() );
        
        if (use_host_timer) {
          // set interval time value
          uint32 interval = MIN_TICK_USEC;  // attempt to set mimimum tick interval
          
          // set initial interval
          timer->set_current_interval(interval);
          
          // Read back the interval actually implemented by the timer and set
          // also as the next interval
          //
          interval = timer->get_current_interval();
          timer->set_next_interval(interval);
          
          // Compute the resulting COUNT0 and COUNT1 increment that occurs
          // on each host timer tick.
          //
          count_increment = DEFAULT_MHZ * 1000 * interval;
          
          LOG(LOG_DEBUG1) << "[TIMER] INIT: host usec interval = "
                          << interval << " increment = "  << count_increment;
          start_timers();
        }
      }

      // -----------------------------------------------------------------------
      // Starts the timers operating. This is called when the processor is started
      // after being in a halted or suspended state. In addition to restarting the
      // timer it also reinstates the local signal handler for the timer.
      //
      // There must be a one-to-one correspondence between calls to start_timers
      // and stop_timers.
      //
      void
      Processor::start_timers ()
      {
        if (IS_T0_AND_T1_DISABLED())
          return;
        
        LOG(LOG_DEBUG4) << "[TIMER] START TIMERS";
        
        running0 = IS_T0_ENABLED();
        running1 = IS_T1_ENABLED();

        if (use_host_timer)
          timer->start();
      }

      // -----------------------------------------------------------------------
      // Stop all timers. This is called when the processor halts during simulation.
      // The value in the host timers is captured and saved so that it can be reinstated
      // when the processor is re-started. The state of the target timers is not
      // altered by this call, so a subsequent call to timer_restart() will restart
      // any running timers.
      //
      void
      Processor::stop_timers ()
      {        
        LOG(LOG_DEBUG3) << "[TIMER] STOP TIMERS";
        timer_sync ();
        running0 = running1 = false;
      }

      
      // -----------------------------------------------------------------------
      // Set the COUNT register for 'timer' to 'value'.
      //
      void
      Processor::timer_set_count (int timer, uint32 value)
      {
        LOG(LOG_DEBUG3) << "[TIMER] SET: AUX_COUNT" << timer << ":= 0x" << HEX(value);
                
        // Update the selected COUNT register and its vcount value
        if (timer == 0) {
          state.auxs[AUX_COUNT0] = vcount0 = value;
        } else {
          state.auxs[AUX_COUNT1] = vcount1 = value;
        }
        
        timer_sync(); // bring COUNT0 and COUNT1 up to date with the current time
        
        if (inst_timer_enabled) { // for instruction timer mode timer_sync() checks for time expiry
          if (!sim_opts.cycle_sim) { // instruction count mode
            state.iterations = 1; // invalidate pre-computed iterations
          }
          time_to_expiry(); // compute state.timer_expiry
          return;
        }
        
        // If any COUNT and LIMIT pair are now exactly equal, then
        // call detect_timer_expiry() to trigger an interrupt if enabled.
        //
        if (   (state.auxs[AUX_COUNT0] == state.auxs[AUX_LIMIT0])
            || (state.auxs[AUX_COUNT1] == state.auxs[AUX_LIMIT1])) {
          detect_timer_expiry ();
        }
      }
 
      // -----------------------------------------------------------------------
      //  Synchronize the timer with the COUNT registers to determine the 
      //  expected COUNT values at the current simulation time.
      //  Return the predicted current value of the selected COUNT register.
      //
      uint32
      Processor::timer_get_count (int timer)
      {
        timer_sync(); // bring COUNT0 and COUNT1 up to date with the current time
        
        const uint32 aux_count = (timer == 0) ? state.auxs[AUX_COUNT0] : state.auxs[AUX_COUNT1];
        
        LOG(LOG_DEBUG3) << "[TIMER] GET: AUX_COUNT" << timer << " -> 0x" << HEX(aux_count);
                        
        return aux_count;
      }
      
      // -----------------------------------------------------------------------
      //  Set the LIMIT register for the selected timer. If the selected timer
      //  is active and is the next timer to expire, determine the current COUNT
      //  value (via get_timer_count) and compute the new timer interval. Restart
      //  the host interval timer with this new interval (which may mean that the
      //  next timer to expire is now the other timer).
      //
      void
      Processor::timer_set_limit (int timer, uint32 value)
      {
        LOG(LOG_DEBUG3) << "[TIMER] SET: AUX_LIMIT" << timer << " := 0x"   << HEX(value);
        
        // Update selected LIMIT register
        if (timer == 0) {
          state.auxs[AUX_LIMIT0] = value;
        } else {
          state.auxs[AUX_LIMIT1] = value;
        }

        timer_sync(); // bring COUNT0 and COUNT1 up to date with the current time
        
        if (inst_timer_enabled) { // for instruction timer mode timer_sync() checks for time expiry
          if (!sim_opts.cycle_sim) { // instruction count mode
            state.iterations = 1; // invalidate pre-computed iterations
          }
          time_to_expiry(); // compute state.timer_expiry
          return;
        }
        
        // If any COUNT and LIMIT pair are now exactly equal, then
        // call detect_timer_expiry() to trigger an interrupt if enabled.
        // Otherwise, if using non-host timers, compute the time to the
        // expiry of the next timer.
        //
        if (   (state.auxs[AUX_COUNT0] == state.auxs[AUX_LIMIT0])
            || (state.auxs[AUX_COUNT1] == state.auxs[AUX_LIMIT1])) {
          detect_timer_expiry();
        }
      }

      // -----------------------------------------------------------------------
      // The control settings for the host timer is recomputed, based on the new
      // status of both timers.
      //
      void
      Processor::timer_set_control (int timer, uint32 value)
      {
        LOG(LOG_DEBUG3) << "[TIMER] SET: AUX_CONTROL" << timer << " := 0x" << HEX(value);
        
        // Update selected control register
        if (timer == 0) {
          state.auxs[AUX_CONTROL0] = value;
        } else {
          state.auxs[AUX_CONTROL1] = value;	
        }
        
        timer_sync(); // bring COUNT0 and COUNT1 up to date with the current time
        
        if (inst_timer_enabled) { // for instruction timer mode timer_sync() checks for time expiry
          if (!sim_opts.cycle_sim) { // instruction count mode
            state.iterations = 1; // invalidate pre-computed iterations
          }
          time_to_expiry(); // compute state.timer_expiry
          return;
        }
        
        // If any COUNT and LIMIT pair are now exactly equal, then call
        // detect_timer_expiry() to trigger an interrupt if enabled.
        // Otherwise, if using non-host timers, compute the time to the
        // expiry of the next timer.
        //
        if (   (state.auxs[AUX_COUNT0] == state.auxs[AUX_LIMIT0])
            || (state.auxs[AUX_COUNT1] == state.auxs[AUX_LIMIT1])) {
          detect_timer_expiry();
        }
      }

      // -----------------------------------------------------------------------
      // Synchronize the host or virtual timer to determine the current expected
      // values of the COUNT0 and COUNT1 aux registers. Set the COUNT0 and COUNT1
      // aux register to the values they should have at this point in time.
      //  
      void
      Processor::timer_sync ()
      {
        if (IS_T0_AND_T1_DISABLED()) // do nothing if TIMERS are disabled
          return;
        
        LOG(LOG_DEBUG3) << "[TIMER] SYNC: COUNT0 was " << state.auxs[AUX_COUNT0]
                        << ", COUNT1 was "  << state.auxs[AUX_COUNT1];
        
        if (inst_timer_enabled) {
          // If modeling timers using virtual CPU time, then increment the
          // COUNT0 and COUNT1 values according to the number of virtual
          // cycles that have elapsed since their last update, provided
          // each counter is running.
          //
          timer_advance_cycles();
          
          // Having brought COUNT0 and COUNT1 up to date with the current
          // cycle count, we must then check for the expiry of a timer.
          //
          detect_timer_expiry();
          
        } else {
          // If modeling timers using host timer, then timer_sync must
          // set the current COUNT0/COUNT1 value from the virtual counters
          // (which are allowed to go negative).
          //
          state.auxs[AUX_COUNT0] = (vcount0 < 0) ? 0 : vcount0;
          state.auxs[AUX_COUNT1] = (vcount1 < 0) ? 0 : vcount1;
        }
        
        LOG(LOG_DEBUG3) << "[TIMER] SYNC: new COUNT0 = " << state.auxs[AUX_COUNT0]
                        << ", new COUNT1 =  "  << state.auxs[AUX_COUNT1];
      }

      // -----------------------------------------------------------------------
      // This is called by Processor::detect_interrupts() when a timer interrupt
      // is taken. The nominated timer must clear its pending interrupt bit in
      // response to this acknowledgement.
      //
      void
      Processor::timer_int_ack (int timer_id)
      {	
        switch (timer_id) {
          case 0: { // TIMER0
            state.auxs[AUX_CONTROL0] &= 0x07;  // clear CONTROL0.IP
            rescind_interrupt_line (get_timer0_irq_num());
            break;
          }
          case 1: { // TIMER1
            state.auxs[AUX_CONTROL1] &= 0x07;  // clear CONTROL1.IP
            rescind_interrupt_line (get_timer1_irq_num());
            break;
          }
          default: break; // do nothing if timer not 0 or 1
        }
      }

      // -----------------------------------------------------------------------
      // This is called in order to determine how many cycles can be simulated
      // up to the point where the next timer will expire. If cycle-accurate
      // simulation is built into the simulator and is enabled, then the
      // state.timer_expiry variable will be set to the current cycle count
      // plus the time to expiry.
      //
      // If COUNTi == LIMITi, then 2^32 cycles will elapse before the counter
      // wraps around and expires. Otherwise, it will be LIMITi - COUNTi.
      //
      uint32 
      Processor::time_to_expiry()
      {
        ASSERT(inst_timer_enabled);  // ONLY ENTER IF inst_timer_enabled
        
        // compute 'current' time/cycles
#if defined(CYCLE_ACC_SIM)
        const uint64 now  = (sim_opts.cycle_sim) ? cnt_ctx.cycle_count.get_value() : instructions();
#else
        const uint64 now  = instructions();
#endif

        const uint32 limit0 = state.auxs[AUX_LIMIT0];
        const uint32 count0 = state.auxs[AUX_COUNT0];
        const uint32 limit1 = state.auxs[AUX_LIMIT1];
        const uint32 count1 = state.auxs[AUX_COUNT1];
        
        // compute expiry times for each timer
        const uint32 e0 = (limit0 == count0) ? 0xffffffffUL : (limit0 - count0);
        const uint32 e1 = (limit1 == count1) ? 0xffffffffUL : (limit1 - count1);
        // determine which timer expires next
        const uint32 et = ((e1 < e0) & (state.auxs[AUX_CONTROL1] & 1UL)) ? e1 : e0;
        // compute NEW expiry time        
        state.timer_expiry = now + et;
        LOG(LOG_DEBUG4) << "[TIMER] EXPIRY TIME = " << state.timer_expiry
                        << " NOW = " << now << " DELTA = " << et;
        return et;
      }
      
      // -----------------------------------------------------------------------
      // This method advances the instruction count based timers to the 
      // current number of instructions executed, returning the number of
      // instructions elapsed since the last call to this method.
      // This method checks whether any timer has expired, and sets the
      // appropriate interrupt line as appropriate. A side-effect of this
      // call may be to modify state.pending_actions.
      //
      uint32
      Processor::timer_advance_cycles()
      { 
        ASSERT(inst_timer_enabled); // ONLY ENTER IF inst_timer_enabled
        
        if (IS_T0_AND_T1_DISABLED()) // do nothing if TIMERS are disabled
          return 0;

        // compute 'current' time/cycles
#if defined(CYCLE_ACC_SIM)
        const uint64 now  = (sim_opts.cycle_sim) ? cnt_ctx.cycle_count.get_value() : instructions();
#else
        const uint64 now  = instructions();
#endif
        ASSERT(now >= timer_sync_time);
        const uint32 inc = now - timer_sync_time;
        timer_sync_time  = now;
                
        if (running0) {
          vcount0 += inc;
          state.auxs[AUX_COUNT0] = vcount0;
        }
        if (running1) {
          vcount1 += inc;
          state.auxs[AUX_COUNT1] = vcount1;
        }
        LOG(LOG_DEBUG4) << "[TIMER] ADVANCE TIME BY " << inc << " TO " 
                        << timer_sync_time;
        return inc;
      }

// A6KV2.1 RTC
      
      uint32 Processor::get_timer0_irq_num()
      {
          if(!sys_arch.isa_opts.new_interrupts) return sys_arch.isa_opts.get_timer0_irq_num();
          if(sys_arch.isa_opts.overload_vectors)
          {
              //if we have no mmu, the timer IRQ replaces ITLBMiss Exception
              //FIXME: This relies on MPU (not yet merged)
              //if((!core_arch.mmu_arch.is_configured || core_arch.mmu_arch.kind == MmuArch::kMpu)) return 4;
              //if we have no code protection ,stack checking or mpu, replace ProtectionV
              //FIXME: This relies on MPU (not yet merged)
              //if(!(sys_arch.isa_opts.code_protect_bits || sys_arch.isa_opts.stack_checking || (core_arch.mmu_arch.is_configured && core_arch.mmu_arch.kind == MmuArch::kMpu))) return 6;
              //if we have no extension instructions, replace Extension exception
              if(!eia_mgr.any_eia_extensions_defined) return 10;
              //if we have no divider, replace Div0 exception
              if(sys_arch.isa_opts.div_rem_option == 0) return 11;
              //assume DCError is enabled (no conditions for it being disabled specified)
              if(false) return 12;
              return 14; //Unused exception
          }
          //default timer0 IRQ
          return 16;
      }
      
      uint32 Processor::get_timer1_irq_num()
      {
          if(!sys_arch.isa_opts.new_interrupts) return sys_arch.isa_opts.get_timer1_irq_num();
          if(sys_arch.isa_opts.overload_vectors)
          {
              //Hacky solution to stopping the timers having the same IRQ: is there a better way to do this?

              //if we have no mmu, the timer IRQ replaces ITLBMiss Exception
              //FIXME: This relies on MPU (not yet merged)
              //if((!core_arch.mmu_arch.is_configured || core_arch.mmu_arch.kind == MmuArch::kMpu) && sys_arch.isa_opts.has_timer0 && get_timer0_irq_num() != 4) return 4;
              //if we have no code protection ,stack checking or mpu, replace ProtectionV
              //FIXME: This relies on MPU (not yet merged)
              //if(!(sys_arch.isa_opts.code_protect_bits || sys_arch.isa_opts.stack_checking || (core_arch.mmu_arch.is_configured && core_arch.mmu_arch.kind == MmuArch::kMpu)) && sys_arch.isa_opts.has_timer0 && get_timer0_irq_num() != 6) return 6;
              //if we have no extension instructions, replace Extension exception
              if(!eia_mgr.any_eia_extensions_defined && sys_arch.isa_opts.has_timer0 && get_timer0_irq_num() != 10) return 10;
              //if we have no divider, replace Div0 exception
              if(sys_arch.isa_opts.div_rem_option == 0 && sys_arch.isa_opts.has_timer0 && get_timer0_irq_num() != 11) return 11;
              //assume DCError is enabled (no conditions for it being disabled specified)
              if(false && get_timer0_irq_num() != 12 && sys_arch.isa_opts.has_timer0) return 12;
              if(sys_arch.isa_opts.has_timer0 && get_timer0_irq_num() == 14) return 15;
          }
          //default timer0 IRQ
          if(sys_arch.isa_opts.has_timer0 && get_timer0_irq_num() == 16) return 17;
          return 16;
      }
      
      uint32 Processor::get_rtc_low()
      {
        // TODO: This is kind of a hack: if the timer hasn't been enabled since it was cleared, 
        // return 0 for its value
        if(rtc_disabled_ticks == 0) { return 0; }

        uint64 time;
        if(sim_opts.cycle_sim)
          time = cnt_ctx.cycle_count.get_value();
        else
          time = instructions();

        uint64_t timeval = time;

        LOG(LOG_DEBUG2) << "[RTC] Reading RTC_LOW";

        // If the clock is currently disabled, take the time since we disabled it into account
        if((state.auxs[AUX_RTC_CTRL] & 0x3) == 0)
        {
          LOG(LOG_DEBUG2) << "[RTC] Offsetting RTC by disabled ticks (" << (time - last_rtc_disable) << " since last read/disable)";
          rtc_disabled_ticks += time - last_rtc_disable;
          LOG(LOG_DEBUG3) << "[RTC] Total disabled ticks: " << rtc_disabled_ticks;
          last_rtc_disable = time;
        }

        timeval -= rtc_disabled_ticks;
        state.auxs[AUX_RTC_HIGH] = timeval >> 32;

        //maintain RTC state machine
        state.auxs[AUX_RTC_CTRL] &= 0x3fffffff;
        state.auxs[AUX_RTC_CTRL] |= 0x40000000;
        return timeval & 0xFFFFFFFF;
      }
      
      uint32 Processor::get_rtc_high()
      {
        // TODO: This is kind of a hack: if the timer hasn't been enabled since it was cleared, 
        // return 0 for its value
        if(rtc_disabled_ticks == 0) { return 0; }

        uint64 time;
        if(sim_opts.cycle_sim)
          time = cnt_ctx.cycle_count.get_value();
        else
          time = instructions();

        uint64_t timeval = time;

        // If the clock is currently disabled, take the time since we disabled it into account
        if((state.auxs[AUX_RTC_CTRL] & 0x3) == 0)
        {
          LOG(LOG_DEBUG2) << "[RTC] Offsetting RTC by disabled ticks (" << (time - last_rtc_disable) << " since last read/disable)";
          rtc_disabled_ticks += timeval - last_rtc_disable;
          LOG(LOG_DEBUG3) << "[RTC] Total disabled ticks: " << rtc_disabled_ticks;
          last_rtc_disable = time;
        }

        timeval -= rtc_disabled_ticks;

        //maintain RTC state machine: if the current top 32 bits of the time value have not changed
        //since the 'low' read, the timer read is 'atomic'
        if((timeval >> 32) == state.auxs[AUX_RTC_HIGH])
            state.auxs[AUX_RTC_CTRL] |= 0x80000000;
        else
          state.auxs[AUX_RTC_CTRL] &= 0x3fffffff;
        return (timeval >> 32) & 0xFFFFFFFF;
      }
      
      void Processor::set_rtc_ctrl(uint32 wdata)
      {
        uint64 time;

        if(sim_opts.cycle_sim)
          time = cnt_ctx.cycle_count.get_value();
        else
          time = instructions();

        if(wdata & 0x2) //reset the RTC
        {
          last_rtc_disable = time;
          rtc_disabled_ticks = time;

          state.auxs[AUX_RTC_HIGH] = 0;
        }
        if(!(wdata & 0x1)) //Disable the RTC
        {
          if(state.auxs[AUX_RTC_CTRL] & 0x1)
            last_rtc_disable = time;
          state.auxs[AUX_RTC_CTRL] = 0;
        }
        else 
        {
          if(!(state.auxs[AUX_RTC_CTRL] & 0x1)) //if the rtc was previously disabled
          {
            //add on the time that has elapsed since the clock was disabled
            rtc_disabled_ticks += (time - last_rtc_disable);
          }
          state.auxs[AUX_RTC_CTRL] = 1;
        }
        //No matter what, writing to the ctrl register clears A0 and A1 (bits 31,30)
        state.auxs[AUX_RTC_CTRL] &= 0x3fffffff;
      }
      
} } } //  arcsim::sys::cpu
