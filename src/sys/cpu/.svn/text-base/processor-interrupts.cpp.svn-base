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
// This file implements the timers defined by the ARCompact ISA.
// The state information relating to each timer is contained within
// the Processor class, so all timer functions (with the exception of
// the alarm handlers) are members of Processor class.
//
// =====================================================================

#include <iomanip>
#include <limits.h>
#include <queue>

#include "sys/cpu/processor.h"
#include "exceptions.h"
#include "util/Util.h"

#include "util/Log.h"

#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_


namespace arcsim {
  namespace sys {
    namespace cpu {

// #define DEBUG_DETECT_INTERRUPTS
// #define DEBUG_DETECT_INTERRUPTS_VERBOSE
// #define DEBUG_INT_ENTRY

#define INTERRUPT_BIT(_b_) (1UL << ((_b_) & 0x1f))


/* Implementation notes:
 
 Processor methods for managing timers
 -------------------------------------
 
 void assert_interrupt_line (int int_num)
 
    Implements the assertion of an interrupt line, given by int_num.
    The value of int_num should be in the range [0..31], but in
    effect, the upper 27 bits of int_num will be ignored.
    
 void rescind_interrupt_line (int int_num)
 
    Implements the rescinding (de-assertion) of an interrupt line
    given by the lower 5 bits of int_num. If the interrupt is 
    configured as pulse-triggered, then the rescinding of the
    interrupt signal does not clear the corresponding interrupt
    pending bit. If the interrupt is configured as level-sensitive,
    then this function will clear the corresponding interrupt
    pending bit.
    
 void cancel_interrupt (int int_num)

    Cancel the single interrupt defined by int_num, iff it is a
    pulse-triggered interrupt.

 void write_irq_hint (uint32 int_no)

    Write the given interrupt number to AUX_IRQ_HINT according to the
    rules defined in the ARCompact PRM. 
     - if int_no is zero or > 31, clear AUX_IRQ_HINT
     - if int_no is 2, ignore it
     - otherwise, set AUX_IRQ_HINT to int_no and subsequently
       check interrupts at the earliest opportunity.

 uint32 get_pending_ints ()

    Return a 32-bit value representing a bit-vector of pending
    interrupts. This is formed from the AUX_IRQ_PENDING register
    and the decoded value present in AUX_IRQ_HINT (if legal).

 void Processor::clear_pulse_interrupts (uint32 int_bits)

    Clear the pending interrupt whose number is given by int_bits, if
    that particular interrupt is pulse-triggered. If it's level sensitive,
    then do not alter the value in AUX_IRQ_PENDING.

 void detect_interrupts ()
 
    This function is called at a point in the simulation loop
    where an interrupt is permitted to be taken.
    In theory, it ought to be called after every instruction, 
    but in practice it needs to be called only when 'pending_actions'
    is set to true.
    It finds the highest priority interrupt that is enabled
    and if higher priority than the current operating mode, it
    causes the interrupt to be raised.

    Interrupts are globally masked using AUX_IENABLE.
    The priority of interrupts is determined by computing
    separately the highest priority level-1 and level-2
    interrupts that are currently pending.
    
    When option a6k == 1, we assume that interrupts are disabled
    in a delay slot. In this case detect_interrupts() will return
    immediately.
*/

/*
 Implements the assertion of an interrupt line, given by int_num.
 The value of int_num should be in the range [0..31], but in
 effect, the upper 27 bits of int_num will be ignored.
 */
void Processor::assert_interrupt_line(int int_num)
{
#ifdef DEBUG_DETECT_INTERRUPTS_VERBOSE
  LOG(LOG_DEBUG2) << "[IRQ] Asserting interrupt " << int_num;
#endif
  
  if (!sys_arch.isa_opts.new_interrupts) {  // A600/A700/EM_1.1 behaviour ------
    state.auxs[AUX_IRQ_PENDING] |= INTERRUPT_BIT(int_num);
    set_pending_action(kPendingAction_CPU);
    return;
  }
  
  if (is_interrupt_configured(int_num)) {   // EM_1.1 behaviour ----------------
    set_interrupt_pending(int_num, true);
    set_pending_action(kPendingAction_CPU);
    return;
  }
  
  LOG(LOG_ERROR) << "[IRQ] Attempt to assert interrupt line " << int_num << ", which is not configured.";
}



/*
 Implements the rescinding (de-assertion) of an interrupt line
 given by the lower 5 bits of int_num. If the interrupt is 
 configured as pulse-triggered, then the rescinding of the
 interrupt signal does not clear the corresponding interrupt
 pending bit. If the interrupt is configured as level-sensitive,
 then this function will clear the corresponding interrupt
 pending bit.  
 */
void Processor::rescind_interrupt_line (int int_num)
{
  if (!sys_arch.isa_opts.new_interrupts) {
    uint32 int_bit = INTERRUPT_BIT(int_num);
    state.auxs[AUX_IRQ_PENDING] &= (~int_bit | state.auxs[AUX_ITRIGGER]);
    return;
  }
  
  set_interrupt_pending(int_num, false);
}


/*
 Cancel the single interrupt defined by int_num, iff it is a
 pulse-triggered interrupt.
 */
void Processor::cancel_interrupt (int int_num)
{
  uint32 int_bit = INTERRUPT_BIT(int_num);
  state.auxs[AUX_IRQ_PENDING] &= (~int_bit | ~state.auxs[AUX_ITRIGGER]);  
}


/*
 Write the given interrupt number to AUX_IRQ_HINT according to the
 rules defined in the ARCompact PRM. 
 - set AUX_IRQ_HINT to (int_no % 32) and subsequently
   check interrupts at the earliest opportunity.
 */
void Processor::write_irq_hint (uint32 int_no)
{
  uint32 aux_irq_hint_mask = 0x0000001f;

  if (sys_arch.isa_opts.new_interrupts) {
    if(int_no < 16) //exception vectors ignored
      return;
    aux_irq_hint_mask = 0x000000ff; // override irq hint mask for
  }

  state.auxs[AUX_IRQ_HINT] = int_no & aux_irq_hint_mask;
  set_pending_action(kPendingAction_CPU);

#ifdef DEBUG_DETECT_INTERRUPTS_VERBOSE
  LOG(LOG_DEBUG2) << "[IRQ] Wrote IRQ Hint " << int_no;
#endif
}

/*
 Return a 32-bit value representing a bit-vector of pending
 interrupts. This is formed from the AUX_IRQ_PENDING register
 and the decoded value present in AUX_IRQ_HINT (if legal).
 
 A6KV2: Get the number of the highest priority pending interrupt or -1 if no interrupt is pending
 */
uint32 Processor::get_pending_ints ()
{
  if(sys_arch.isa_opts.new_interrupts) {
    return get_pending_ints_a6kv2();    
  }
  
  return get_pending_ints_legacy();
}


uint32 Processor::get_pending_ints_a6kv2()
{
  if(!is_interrupt_enabled_and_pending()) {
    return kInvalidInterrupt;
  }

  uint8 highest_irq;
  uint8 highest_prio;
  bool  irq_set = false;
  const uint32 max_irq_num = max_interrupt_num();

  for (int irq = 0; irq < max_irq_num; ++irq)  {
    if (is_interrupt_configured(irq) // is IRQ configured, enabled, and pending
        && is_interrupt_enabled(irq)
        && is_interrupt_pending(irq)) {
      LOG(LOG_DEBUG3) << "[IRQ] Interrupt " << irq << " enabled and pending";
      if (!irq_set || (state.irq_priority[irq] < highest_prio)) {
        highest_irq  = irq;
        highest_prio = state.irq_priority[irq];
        irq_set      = true;
      }
    }
  }

  if (state.auxs[AUX_IRQ_HINT] != 0) {
    LOG(LOG_DEBUG3) << "[IRQ] AUX_IRQ_HINT Interrupt Pending";
    const uint32 irq = state.auxs[AUX_IRQ_HINT];
    if (   is_interrupt_enabled(irq)
        && (!irq_set || (state.irq_priority[irq] < highest_prio)))
    {
      LOG(LOG_DEBUG3) << "[IRQ] AUX_IRQ_HINT Has Priority and is enabled";
      highest_irq  = irq;
      highest_prio = state.irq_priority[irq];
      irq_set      = true;
      state.auxs[AUX_IRQ_HINT] = 0;
    }
  }

  if (irq_set) {
    LOG(LOG_DEBUG3) << "[IRQ] Interrupt "<< (uint16)(highest_irq) << " is the pending interrupt";
    return highest_irq;
  }

  LOG(LOG_DEBUG3) << "[IRQ] No interrupt is pending and enabled";
  return kInvalidInterrupt;
}

uint32 Processor::get_pending_ints_legacy()
{
  const uint32 mask = (0xffffffffUL >> (32 - sys_arch.isa_opts.num_interrupts)) & 0xfffffff8;
  const uint32 triggered = (1UL << state.auxs[AUX_IRQ_HINT]) & mask;
  
  return (state.auxs[AUX_IRQ_PENDING] | triggered); // return pending interrupts
}

/*
 Clear the pending interrupt whose number is given by int_bits, if
 that particular interrupt is pulse-triggered. If it's level sensitive,
 then do not alter the value in AUX_IRQ_PENDING.
 */
void Processor::clear_pulse_interrupts (uint32 int_bits)
{
  state.auxs[AUX_IRQ_PENDING] &= (~int_bits | ~state.auxs[AUX_ITRIGGER]);  
}

void Processor::swap_reg_banks()
{
  ASSERT(sys_arch.isa_opts.new_interrupts);
  
  // Do nothing if an additional register bank is not configured
  //
  if(sys_arch.isa_opts.num_reg_banks != 2)
    return;
  
  state.RB = !state.RB;
    
  // We don't need to take the number of actual registers into account since the
  // behaviour is the same whether we have 16 or 32
  
  switch(sys_arch.isa_opts.num_banked_regs) {
    case 32: //start with the simplest case
      swap_reg_group(0,  28);
      swap_reg_group(30, 31);
      break;
    case 16:
      swap_reg_group(0,  3);
      swap_reg_group(10, 15);
      swap_reg_group(26, 28);
      swap_reg_group(30, 31);
      break;
    case 8:
      swap_reg_group(0,  3);
      swap_reg_group(12, 15);
      break;
    case 4:
      swap_reg_group(0, 3);
      break;
  }
}

void Processor::swap_reg_group(int low, int high)
{
  ASSERT(high >= low);
  for (int i = low; i <= high; i++) {
    uint32 temp   = state.gprs[i];
    state.gprs[i] = state.irq_gprs_bank[i];
    state.irq_gprs_bank[i] = temp;
  }
}

bool Processor::jump_to_vector(uint32 PI, uint32 &ecause, uint32 &efa)
{
#ifndef V30_VECTOR_TABLE
  
  // Get the interrupt vector
  //
  uint32 int_vector;
  uint32 vector_addr = state.auxs[AUX_INT_VECTOR_BASE] + (PI << 2);
  if(ecause = fetch32(vector_addr, int_vector, efa, true))
  {
    return false;
  }
  int_vector = ((int_vector & 0xffff) << 16) | ((int_vector & 0xffff0000) >> 16);
  
  LOG(LOG_DEBUG2) << "[IRQ] Read vector " << HEX(int_vector) << " from " << HEX(vector_addr) << " for vector " << PI;
  
  // If SmaRT is enabled, then push an entry onto the SmaRT stack 
  // for the current interrupt.
  //
  if (smt.enabled()) {
    smt.push_exception(state.pc, int_vector);
  }
  
  // Set PC to the interrupt vector and adjust the operating mode
  // to complete the semantic actions associated with taking an interrupt.
  //
  state.pc      = int_vector;
  state.next_pc = int_vector;
  
  return true;
  
#else
  state.pc = state.auxs[AUX_INT_VECTOR_BASE] + (PI << 3);
  return true;
#endif
}

/*
 Enter the interrupt whose number is PI and who has priority (preemption level) W
 */
void Processor::enter_a6kv21_interrupt(uint8 PI, uint8 W)
{
  ASSERT(sys_arch.isa_opts.new_interrupts);
  
  // Back up our operating mode and stack pointer in case we take an exception
  OperatingMode entry_op_mode = MAP_OPERATING_MODE(state.U);
  
  // We are not in an exception and interrupts are enabled and the pending interrupt has a 
  // higher priority than our  current operating priority: therefore we take the interrupt

  // Reset the RTC state machine
  state.auxs[AUX_RTC_CTRL] &= 0x3fffffff;
  
  // first, check to see if we should do an early swap of the stack pointers 
  if(entry_op_mode && !(state.auxs[AUX_IRQ_CTRL] & (1 << 11))) {
    set_operating_mode(KERNEL_MODE);  
  }
    
  // Determine register save workload
  //
  std::queue<uint32> save_reg_queue;
  
  save_reg_queue.push(BUILD_STATUS32_A6KV21(state)); // push STATUS32
  save_reg_queue.push(state.pc); // push PC
  
  if((state.auxs[AUX_IRQ_CTRL] & (1 << 10))) //AUX_IRQ_CTRL.L
  {
    //Save the LP regs
    save_reg_queue.push(state.gprs[LP_COUNT]);
    save_reg_queue.push(state.auxs[AUX_LP_START]);
    save_reg_queue.push(state.auxs[AUX_LP_END]);
  }
  
  if((state.auxs[AUX_IRQ_CTRL] & (1 << 9)) && ((state.auxs[AUX_IRQ_CTRL] & 0x1F) != 16)) {
    save_reg_queue.push(state.gprs[BLINK]); // push BLINK onto the stack
    if(state.auxs[AUX_IRQ_CTRL] & (1 << 12)) // M bit is set in AUX_IRQ_CTRL for double word operations
      save_reg_queue.push(state.gprs[30]);
  } 
  const uint32 save_reg_count = (2 * state.auxs[AUX_IRQ_CTRL] & 0x1F);
  for(uint32 i = 0; i < save_reg_count; ++i) {
    save_reg_queue.push(state.gprs[i]); // push selected GPR
  }

  uint32 sp = state.gprs[SP_REG];

  while (!save_reg_queue.empty()) {
    sp -= 4; // decrement stack pointer
    
    if (   state.SC
        && ((state.stack_base <= sp) || (state.stack_top > sp))) { // stack checking
      set_operating_mode(entry_op_mode);
      enter_exception(ECR(sys_arch.isa_opts.EV_ProtV, StoreTLBfault, sys_arch.isa_opts.PV_StackCheck),
                      state.pc, state.pc);
      return;
    }
    
    LOG(LOG_DEBUG3) << "[IRQ] Interrupt entry: pushed-0x"
                    << HEX(save_reg_queue.front()) << " to 0x" << HEX(sp); 
    
    if (!write32(sp, save_reg_queue.front())) {
      return; // return for memory errors
    }
    
    save_reg_queue.pop();
  }
  
  state.gprs[SP_REG] = sp;
  
  if((state.auxs[AUX_IRQ_CTRL] & (1 << 11)) && entry_op_mode) {
    set_operating_mode (KERNEL_MODE);
  }

  //If we are the first exception to activate
  if((state.auxs[AUX_IRQ_ACT] & 0xff) == 0) {
    state.auxs[AUX_IRQ_ACT] |= entry_op_mode << 31; // record current privilege level in AUX_IRQ_ACT.U
  }

  //Set the bit corresponding to our new interrupt priority level in AUX_IRQ_ACT.Active
  state.auxs[AUX_IRQ_ACT] |= 1 << (W);

  state.Z = entry_op_mode;
  state.L = 1;

  // -------------------------------------------------------------------------
  // Acknowledge interrupts
  // FIXME: at the moment we only 'ack' timer IRQs, see comment below
  //
  acknowledge_interrupt(PI);

  //Set the appropriate ICAUSE register (not in spec but necessary)
  state.irq_icause[W] = PI;
  
  uint32 ecause;
  uint32 efa;
  if(!jump_to_vector(PI, ecause, efa))
  {
    enter_exception(ecause, efa, state.pc);
    return;
  }
    
  interrupt_stack.push((InterruptState)(INTERRUPT_P0_IRQ + W));  
}

/*
 Enter the interrupt whose number is PI and who has priority 0 (the highest).
 */
void Processor::enter_a6kv21_fast_interrupt(uint8 PI)
{
  ASSERT(sys_arch.isa_opts.new_interrupts);
  
  //Reset the RTC state machine
  state.auxs[AUX_RTC_CTRL] &= 0x3fffffff;
  
  //first, check to see if we should do an early swap of the stack pointers 
  
  LOG(LOG_DEBUG3) << "[IRQ] Fast IRQ: U=" << (uint32)state.U << ", AUX_IRQ_CTRL.U = " << (uint32)((state.auxs[AUX_IRQ_CTRL] & (1 << 11)) >> 11); 
  if(state.U && !(state.auxs[AUX_IRQ_CTRL] & (1 << 11)))
  {
    LOG(LOG_DEBUG3) << "[IRQ] Fast IRQ: Setting operating mode";
    set_operating_mode(KERNEL_MODE);
  }
  
#ifdef DEBUG_INT_ENTRY
  LOG(LOG_DEBUG) << "[IRQ] Entering fast interrupt " << (uint32)PI << " at PC "
                 << std::hex << std::setfill('0') << std::setw(8) << state.pc;
#endif
  
  state.gprs[ILINK1] = state.pc;
  state.auxs[AUX_STATUS32_P0] = BUILD_STATUS32_A6KV21(state);
  
  if((state.auxs[AUX_IRQ_ACT] & 0xff) == 0)
    state.auxs[AUX_IRQ_ACT] |= state.U << 31;
  
  state.auxs[AUX_IRQ_ACT] |= 1;
  
  state.Z = state.U;
  state.U = 0;
  state.L = 1;
  
  // Set the ICAUSE register (not in spec but necessary)
  state.irq_icause[0] = PI;
    
  //Swap the current register file with the banked file
  //
  swap_reg_banks();

  // -------------------------------------------------------------------------
  // Acknowledge interrupts
  // FIXME: at the moment we only 'ack' timer IRQs, see comment below
  //
  acknowledge_interrupt(PI);

  uint32 efa;
  uint32 ecause;
  if(!jump_to_vector(PI, ecause, efa))
  {
    enter_exception(ecause, efa, state.pc);
    return;
  }
  
  interrupt_stack.push((InterruptState)(INTERRUPT_P0_IRQ));
}

void Processor::enter_a6kv21_exception(uint32 ecr, uint32 efa, uint32 eret)
{
  ASSERT(sys_arch.isa_opts.new_interrupts);
  
  state.auxs[AUX_RTC_CTRL] &= 0x3fffffff; // reset RTC state machine
  
  if (state.AE) { // if already in an exception which is not MachineCheck, enter MachineCheck
    state.auxs[AUX_ECR] = ECR(sys_arch.isa_opts.EV_MachineCheck, 0, 0);
    ecr = state.auxs[AUX_ECR];
    mmu.write_pid (state.auxs[AUX_PID] & 0xff);
  } else {
    state.auxs[AUX_ECR] = ecr;
  }
  
  if (state.U) { // if in user mode, switch to kernel stack
    uint32 temp = state.auxs[AUX_USER_SP];
    state.auxs[AUX_USER_SP] = state.gprs[SP_REG];
    state.gprs[SP_REG] = temp;        
    
    LOG(LOG_DEBUG3) << "[IRQ] Exception entry: Swapping stacks";
  }
  
  state.auxs[AUX_ERET] = eret & 0xfffffffe;
  // Why was this masked? this can be the address of a single byte load, 
  // not just an instruction so should not have the bottom bit truncated.
  // @chris 27/3/2012 star 9000531924, see processor.cpp enter_exception also.
  //state.auxs[AUX_EFA]  = efa  & 0xfffffffe;
  state.auxs[AUX_EFA]  = efa;
  state.auxs[AUX_ECR]  = ecr;
  state.auxs[AUX_ERSTATUS] = BUILD_STATUS32_A6KV21(state);
  
  state.AE  = 1;
  state.Z   = state.U;
  //state.U = 0;
  state.L   = 1;
  state.D   = 0;
  state.IE  = 0;
    
  //if exception entry fails here, we have an unrecoverable error so arcsim should halt
  //(otherwise we will enter an infinite loop as if we can't load the EV for _an_ exception we 
  //probably can't load the EV for _any_ exception.
  //
  uint32 dummy_efa;
  uint32 dummy_ecause;
  if(!jump_to_vector(ECR_VECTOR(ecr), dummy_ecause, dummy_efa))
  {
    LOG(LOG_INFO) << "[IRQ] An exception was raised when attempting to load an exception vector. This is unrecoverable so arcsim will now halt.";
    halt_cpu(false);
    return;
  }
  
  // Signal fast mode that we are in an exception
  //
  interrupt_stack.push(INTERRUPT_EXCEPTION);
  state.raise_exception = 1;  // for current inst only
  set_operating_mode (KERNEL_MODE);
}
  
void Processor::return_from_ie_a6kv21()
{
  ASSERT(state.U == 0);
  ASSERT(sys_arch.isa_opts.new_interrupts);
  
  state.auxs[AUX_RTC_CTRL] &= 0x3fffffff; // reset the RTC state machine
  
  const uint32 irq_active = state.auxs[AUX_IRQ_ACT] & 0xffff;
  uint32 irq_prio = 16;
  if( irq_active) {
    irq_prio = util::index_of_lowest_set_bit(irq_active);
  }
  // If in exception perform 'exception return'
  //
  if(state.AE || irq_prio == 16) {
    return_from_exception_a6kv21();
    return;
  }
  
  // Get current pending interrupt (if there is one) so that we can do a fast switch
  // if possible
  //
  uint32 pending  = kInvalidInterrupt;
  uint32 priority = sys_arch.isa_opts.number_of_levels + 1;
  if(is_interrupt_enabled_and_pending())  {
    pending = get_pending_ints();
    // Technically this shouldn't be reached if get_pending_ints can return kInvalidInterrupt.
    // However, sometimes it is reached in this case, so we check here to make sure the interrupt
    // is still invalid.
    if(pending != kInvalidInterrupt) {
      priority = state.irq_priority[pending];
    }
  }
  
  // Now return from interrupt
  //
  if(irq_prio == 0 && (state.auxs[AUX_IRQ_BUILD] & (1 << 28))) {
    LOG(LOG_DEBUG2) << "[IRQ] Returning from fast interrupt";
    return_from_fast_interrupt_a6kv21(pending, priority);
    return;
  }
  
  LOG(LOG_DEBUG2) << "[IRQ] Returning from standard interrupt";
  return_from_interrupt_a6kv21(pending, priority);
}

void Processor::return_from_exception_a6kv21()
{
  ASSERT(sys_arch.isa_opts.new_interrupts);
  
  uint32 next_pc = state.auxs[AUX_ERET];
  EXPLODE_STATUS32_A6KV21(state, state.auxs[AUX_ERSTATUS]);
  
  if(state.U) {
    uint32 temp = state.auxs[AUX_USER_SP];
    state.auxs[AUX_USER_SP] = state.gprs[SP_REG];
    state.gprs[SP_REG] = temp;        
    
    LOG(LOG_DEBUG3) << "[IRQ] Exception return: Swapping stacks";
  }
  
  state.pc      = next_pc;
  state.next_pc = next_pc;
  
  if (interrupt_stack.top() == INTERRUPT_EXCEPTION) {
      interrupt_stack.pop();
  }
}

void Processor::return_from_interrupt_a6kv21(uint32 PI, uint8 W)
{
  const uint32 ret_irq_prio = util::index_of_lowest_set_bit(state.auxs[AUX_IRQ_ACT] & 0xFFFF);
  
  LOG(LOG_DEBUG3) << "[IRQ] Returning from interrupt priority "
                  << ret_irq_prio << ", pending int "
                  << PI << " (" << (uint32)W << ")";
  
  // Back up our operating mode and stack pointer in case we take an exception
  //
  OperatingMode entry_op_mode = MAP_OPERATING_MODE(state.U);
  
  // We should either not be entering a new interrupt, or the new interrupt should
  // be at the same or a higher priority than our current interrupt.
  //
  ASSERT(   ((PI == kInvalidInterrupt) || (W >= ret_irq_prio) || (W > state.E))
         && ret_irq_prio >= 0 && !state.AE);
  
#ifdef DEBUG_INT_EXIT  
  LOG(LOG_DEBUG) << "[IRQ] Interrupt return (" << state.irq_icause[returning_int_priority]
                 << ") at (" << returning_int_priority << ")";
#endif
  
  state.auxs[AUX_IRQ_ACT] &= ~(1 << ret_irq_prio);
  
  uint32 next_int = 16;
  if ((state.auxs[AUX_IRQ_ACT] & 0xFFFF) != 0) // if we are not returning from the only active interrupt
    next_int = util::index_of_lowest_set_bit(state.auxs[AUX_IRQ_ACT] & 0xFF);
  
  // pop returning interrupt off of the interrupt stack
  if (interrupt_stack.top() == INTERRUPT_P0_IRQ + ret_irq_prio) {
      interrupt_stack.pop();
  }
  
  // if there is a pending interrupt with higher priority than the active interrupt
  if ((PI != kInvalidInterrupt) && (next_int > W) && (W <= state.E)) {
#ifdef DEBUG_INT_EXIT
    LOG(LOG_DEBUG) << "[IRQ] Avoid context switch into " << PI;
#endif
    // do a context-switch-avoiding switch into the pending interrupt
    state.auxs[AUX_IRQ_ACT] |= (1 << W);
    
    interrupt_stack.push((InterruptState)(INTERRUPT_P0_IRQ + W));
    acknowledge_interrupt(PI);
    
    state.irq_icause[W] = PI;
    
    uint32 ecause;
    uint32 efa;
    if(!jump_to_vector(PI, ecause, efa))
    {
      enter_exception(ecause, efa, state.pc);
      return;
    }
  
    return;
  } 
  
  // If we are returning to the user thread and we should switch back to the user stack,
  // then swap the stacks
  if(  ((state.auxs[AUX_IRQ_ACT]  & 0xFFFF) == 0)
     && (state.auxs[AUX_IRQ_ACT]  & 0x80000000)
     && (state.auxs[AUX_IRQ_CTRL] & (1 << 11)))
  {
    set_operating_mode(USER_MODE);
    uint32 temp = state.auxs[AUX_USER_SP];
    state.auxs[AUX_USER_SP] = state.gprs[SP_REG];
    state.gprs[SP_REG] = temp;
    
    LOG(LOG_DEBUG3) << "[IRQ] Interrupt return: Swapping stacks (early)";
  }
  
  // Determine register numbers to load
  //
  std::queue<uint32> load_reg_queue;
  
  const uint32 save_reg_count = (2 * state.auxs[AUX_IRQ_CTRL] & 0x1F);
  for(uint32 i = save_reg_count; i > 0; --i) {
    load_reg_queue.push(i-1); // push selected GPR
  }
  // if we have saved BLINK and friends (AUX_ITQ_CTRL.B && AUX_IRQ_CTRL.NR != 16)
  if((state.auxs[AUX_IRQ_CTRL] & (1 << 9)) && (state.auxs[AUX_IRQ_CTRL] & 0x1F) != 16) {
    if(state.auxs[AUX_IRQ_CTRL] & (1 << 12)) //if the M bit is set in AUX_IRQ_CTRL
      load_reg_queue.push(30);
    
    load_reg_queue.push(BLINK); // restore BLINK
  }
  
  // back up SP we are using since we might reload a different stack pointer
  uint32 sp = state.gprs[SP_REG];
  
  while (!load_reg_queue.empty()) {
    
    if (state.SC && ((state.stack_base <= sp) || (state.stack_top > sp))) { // stack checking
      set_operating_mode(entry_op_mode);
      enter_exception(ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck),
                      state.pc, state.pc);
      return; // bail out in case of stack checking error
    }
        
    if (!read32(sp, state.gprs[load_reg_queue.front()])) {
      return; // return in case of memory error
    }
    LOG(LOG_DEBUG3) << "[IRQ] Interrupt exit: popped-0x" << HEX(state.gprs[load_reg_queue.front()])
                    << " to gprs[" << load_reg_queue.front() << "], SP 0x"
                    << HEX(sp);
    
    load_reg_queue.pop(); // pop off read register
    sp += 4; // increment stack pointer
  }

  if(state.auxs[AUX_IRQ_CTRL] & (1 << 10)) //AUX_IRQ_CTRL.L
  {
    // Restore LP_END
    if (state.SC && ((state.stack_base <= sp) || (state.stack_top > sp))) { // stack checking
      set_operating_mode(entry_op_mode);
      enter_exception(ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck),
                      state.pc, state.pc);
      return;
    }
    
    if (!read32(sp, state.auxs[AUX_LP_END])) {
      return; // return in case of memory error
    }
    
    LOG(LOG_DEBUG3) << "[IRQ] Interrupt exit: popped-0x" << HEX(state.auxs[AUX_LP_END])
                    << " to AUX_LP_END, SP 0x"
                    << HEX(sp);
    
    sp += 4; // increment stack pointer
    
    
    // Restore LP_START
    if (state.SC && ((state.stack_base <= sp) || (state.stack_top > sp))) { // stack checking
      set_operating_mode(entry_op_mode);
      enter_exception(ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck),
                      state.pc, state.pc);
      return;
    }
    
    if (!read32(sp, state.auxs[AUX_LP_START])) {
      return; // return in case of memory error
    }
    
    LOG(LOG_DEBUG3) << "[IRQ] Interrupt exit: popped-0x" << HEX(state.auxs[AUX_LP_START])
                    << " to AUX_LP_START, SP 0x"
                    << HEX(sp);
    
    sp += 4; // increment stack pointer
    
    // Restore LP_COUNT
    if (state.SC && ((state.stack_base <= sp) || (state.stack_top > sp))) { // stack checking
      set_operating_mode(entry_op_mode);
      enter_exception(ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck),
                      state.pc, state.pc);
      return;
    }
    if (!read32(sp, state.gprs[LP_COUNT])) {
      return; // return in case of memory error
    }
    
    LOG(LOG_DEBUG3) << "[IRQ] Interrupt exit: popped-0x" << HEX(state.gprs[LP_COUNT])
                    << " to LP_COUNT, SP 0x"
                    << HEX(sp);
    state.next_lpc = state.gprs[LP_COUNT];
    
    sp += 4; // increment stack pointer
    
  }
  
  // Restore PC ----------------------------------------------------------------
  if (state.SC && ((state.stack_base <= sp) || (state.stack_top > sp))) { // stack checking
    set_operating_mode(entry_op_mode);
    enter_exception(ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck),
                    state.pc, state.pc);
    return;
  }
  // NOTE: here we set the PC correctly
  //
  if (!read32(sp, state.pc)) {
    return; // return in case of memory error
  }
  
  state.next_pc = state.pc;
  
  LOG(LOG_DEBUG3) << "[IRQ] Interrupt exit: popped-0x" << HEX(state.pc) << ", SP 0x" << HEX(sp); 
  sp += 4; // increment stack pointer

  // Restore STATUS32 ----------------------------------------------------------
  uint32 status32;
  if (state.SC && ((state.stack_base <= sp) || (state.stack_top > sp))) { // stack checking
    set_operating_mode(entry_op_mode);
    enter_exception(ECR(sys_arch.isa_opts.EV_ProtV, LoadTLBfault, sys_arch.isa_opts.PV_StackCheck),
                    state.pc, state.pc);
    return;
  }
  
  if (!read32(sp, status32)) {
    return; // return in case of memory error
  }
  LOG(LOG_DEBUG3) << "[IRQ] Interrupt exit: popped-0x" << HEX(status32) << ", SP 0x" << HEX(sp); 
  sp += 4; // increment stack pointer
    
  uint8 state_u = state.U;
  EXPLODE_STATUS32_A6KV21(state, status32);
  uint8 req_state_u = state.U;
  
  state.gprs[SP_REG] = sp;
  
  // If we are returning to the user thread and we should switch back to the user stack,
  // then swap the stacks
  if(  ((state.auxs[AUX_IRQ_ACT]   & 0xFFFF) == 0)
     && (state.auxs[AUX_IRQ_ACT]   & 0x80000000)
     && !(state.auxs[AUX_IRQ_CTRL] & (1 << 11)))
  {
    set_operating_mode(USER_MODE);
    uint32 temp = state.auxs[AUX_USER_SP];
    state.auxs[AUX_USER_SP] = state.gprs[SP_REG];
    state.gprs[SP_REG] = temp;
    
    LOG(LOG_DEBUG3) << "[IRQ] Exception entry: Swapping stacks (late)";
  }
  state.U = state_u;
  set_operating_mode(MAP_OPERATING_MODE(req_state_u));
  
  // PC already restored so finish here.
}

void Processor::return_from_fast_interrupt_a6kv21(uint32 PI, uint8 W)
{
  ASSERT(sys_arch.isa_opts.new_interrupts);
  ASSERT(util::index_of_lowest_set_bit(state.auxs[AUX_IRQ_ACT] & 0xFFFF) == 0
         && "We are returning from a P0 interrupt");
  ASSERT(state.AE == 0 && "We are not in an exception");
  
  // FIXME: The W <= state.E check is not required here but is in the spec
  if (false  // FIXME: Fast interrupt switch currently disabled
      && (PI != kInvalidInterrupt)
      && (W == 0)
      && (W <= state.E)) 
  {
#ifdef DEBUG_INT_EXIT
    LOG(LOG_DEBUG) << "[IRQ] Another interrupt " << PI << " is waiting at priority " << (uint32)W;
#endif 
    
    acknowledge_interrupt(PI);
    
    uint32 ecause;
    uint32 efa;
    if(!jump_to_vector(PI, ecause, efa))
    {
      enter_exception(ecause, efa, state.pc);
      return;
    }
    
    state.irq_icause[0] = PI;
    return;
  }

  state.auxs[AUX_IRQ_ACT] &= 0xfffffffe; // clear the 0th bit in AUX_IRQ_ACT.ACTIVE
    
  // swap stacks if we are returning from the only interrupt
  if (((state.auxs[AUX_IRQ_ACT] & 0xFFFF) == 0) && ((state.auxs[AUX_IRQ_CTRL] & (1 << 11)) != 0)) {
    LOG(LOG_DEBUG3) << "[IRQ] Fast IRQ return: Swapping stacks";
    
    uint32 temp = state.auxs[AUX_USER_SP];
    state.auxs[AUX_USER_SP] = state.gprs[SP_REG];
    state.gprs[SP_REG] = temp;
  }
  
  // swap the current register file with banked file
  swap_reg_banks();
  
  EXPLODE_STATUS32_A6KV21(state, state.auxs[AUX_STATUS32_P0]);
        
#ifdef DEBUG_INT_EXIT
  LOG(LOG_DEBUG) << "[IRQ] Fast interrupt return (" << state.irq_icause[0]
                 << ") at (0) to " HEX(state.gprs[ILINK1]);
#endif
  
  state.pc      = state.gprs[ILINK1];
  state.next_pc = state.pc;
}

void Processor::acknowledge_interrupt(uint32 PI)
{
  if (PI == get_timer0_irq_num()) {
    timer_int_ack(0);
  } else if (PI == get_timer1_irq_num()) {
    timer_int_ack(1);
  }
}


uint32 Processor::max_interrupt_num() const
{
  ASSERT(sys_arch.isa_opts.is_isa_a6kv2() && sys_arch.isa_opts.new_interrupts);
  
  uint32 max_int = sys_arch.isa_opts.num_interrupts + 16 - 1;
  if (sys_arch.isa_opts.overload_vectors) {
    if (!core_arch.mmu_arch.is_configured || core_arch.mmu_arch.kind == MmuArch::kMpu) {
      max_int -= 2; //TLB Misses
    }
    if (!(   sys_arch.isa_opts.code_protect_bits 
          || sys_arch.isa_opts.stack_checking
          || (core_arch.mmu_arch.is_configured && core_arch.mmu_arch.kind == MmuArch::kMpu)
          )) {
      max_int -= 1; //ProtV
    }
    if (!eia_mgr.any_eia_extensions_defined)
      max_int -= 1; //Extension
    if (sys_arch.isa_opts.div_rem_option == 0)
      max_int -= 1; // Div0
    max_int -= 1;   // DCError
    max_int -= 2;   // Unused
  }
  return max_int;
}

bool Processor::is_interrupt_configured(uint32 num) const
{
  ASSERT(sys_arch.isa_opts.is_isa_a6kv2() && sys_arch.isa_opts.new_interrupts);
  
  if (num > max_interrupt_num()) return false;
  if (num >= 16)
    return true;
  
  if (sys_arch.isa_opts.overload_vectors) {
    // These are always exceptions:
    //   Reset ........... 0
    //   Memory Error .... 1
    //   Instr Error ..... 2
    //   Machine Check ... 3
    //   Priv Violation... 7
    //   SWI/TRAP ........ 8
    //   Maligned ........ 13
    if (num <= 3 || (num >= 7 && num <= 9) || num == 13)
      return false; 
    
    switch (num) {                  
      case 4: //ITLBMiss
      case 5: //DTLBMiss
        //if mmu is present
        if (    core_arch.mmu_arch.is_configured
            && (core_arch.mmu_arch.kind == MmuArch::kMmu))
          return false;
      case 6: //ProtV
        return !(   sys_arch.isa_opts.code_protect_bits
                 || sys_arch.isa_opts.stack_checking
                 || (core_arch.mmu_arch.is_configured && core_arch.mmu_arch.kind == MmuArch::kMpu));
      case 10: //Extension
        return !eia_mgr.any_eia_extensions_defined;
      case 11: //DivZero
        return sys_arch.isa_opts.div_rem_option == 0;
      case 12: //DCError
        // condition of this exception aren't specified, assume it is enabled
        return false;
      case 14:
      case 15:
        //unused exception vectors
        return true;
      default: 
        return true;
    }
  }
  
  return false;
}

void Processor::set_interrupt_enabled(uint32 irq, bool enable)
{
  const uint8 bank  = irq / 32;
  const uint8 index = irq % 32;
  const uint32 bit  = (1 << index);
  
  switch (bank) {
    case 0: {
      if (enable) state.irq_enabled_bitset_0_31  |= bit;
      else        state.irq_enabled_bitset_0_31  &= ~bit;
      break;
    }
    case 1: {
      if (enable) state.irq_enabled_bitset_32_63   |= bit;
      else        state.irq_enabled_bitset_32_63   &= ~bit;
      break;
    }
    case 2: {
      if(enable) state.irq_enabled_bitset_64_95   |= bit;
      else       state.irq_enabled_bitset_64_95   &= ~bit;
      break;
    }
    case 3: {
      if (enable) state.irq_enabled_bitset_96_127  |= bit;
      else        state.irq_enabled_bitset_96_127  &= ~bit;
      break;
    }
    case 4: {
      if (enable) state.irq_enabled_bitset_128_159 |= bit;
      else        state.irq_enabled_bitset_128_159 &= ~bit;
      break;
    }
    case 5: {
      if (enable) state.irq_enabled_bitset_160_191 |= bit;
      else        state.irq_enabled_bitset_160_191 &= ~bit;
      break;
    }
    case 6: {
      if (enable) state.irq_enabled_bitset_192_223 |= bit;
      else        state.irq_enabled_bitset_192_223 &= ~bit;
      break;
    }
    case 7: {
      if (enable) state.irq_enabled_bitset_224_255 |= bit;
      else        state.irq_enabled_bitset_224_255 &= ~bit;
      break;
    }
    default: UNREACHABLE();
  }
}

void Processor::set_interrupt_pending(uint32 irq, bool pending)
{
  const uint8 bank  = irq / 32;
  const uint8 index = irq % 32;
  const uint32 bit  = (1 << index);
  switch (bank) {
    case 0: {
      if (pending) state.irq_pending_bitset_0_31    |= bit;
      else         state.irq_pending_bitset_0_31    &= ~bit;
      break;
    }
    case 1: {
      if (pending) state.irq_pending_bitset_32_63   |= bit;
      else         state.irq_pending_bitset_32_63   &= ~bit;
      break;
    }
    case 2: {
      if (pending) state.irq_pending_bitset_64_95   |= bit;
      else         state.irq_pending_bitset_64_95   &= ~bit;
      break;
    }
    case 3: {
      if (pending) state.irq_pending_bitset_96_127  |= bit;
      else         state.irq_pending_bitset_96_127  &= ~bit;
      break;
    }
    case 4: {
      if (pending) state.irq_pending_bitset_128_159 |= bit;
      else         state.irq_pending_bitset_128_159 &= ~bit;
      break;
    }
    case 5: {
      if (pending) state.irq_pending_bitset_160_191 |= bit;
      else         state.irq_pending_bitset_160_191 &= ~bit;
      break;
    }
    case 6: {
      if (pending) state.irq_pending_bitset_192_223 |= bit;
      else         state.irq_pending_bitset_192_223 &= ~bit;
      break;
    }
    case 7: {
      if (pending) state.irq_pending_bitset_224_255 |= bit;
      else         state.irq_pending_bitset_224_255 &= ~bit;
      break;
    }
    default: UNREACHABLE();
  }
}

bool Processor::is_interrupt_enabled(uint32 interrupt) const
{
  const uint8 bank  = interrupt / 32;
  const uint8 index = interrupt % 32;
  const uint32 bit  = (1 << index);
  switch (bank) {
    case 0: return (state.irq_enabled_bitset_0_31    & bit) != 0;
    case 1: return (state.irq_enabled_bitset_32_63   & bit) != 0;
    case 2: return (state.irq_enabled_bitset_64_95   & bit) != 0;
    case 3: return (state.irq_enabled_bitset_96_127  & bit) != 0;
    case 4: return (state.irq_enabled_bitset_128_159 & bit) != 0;
    case 5: return (state.irq_enabled_bitset_160_191 & bit) != 0;
    case 6: return (state.irq_enabled_bitset_192_223 & bit) != 0;
    case 7: return (state.irq_enabled_bitset_224_255 & bit) != 0;
    default: UNREACHABLE();
  }
  return false; // we should never get here
}

bool Processor::is_interrupt_pending(uint32 interrupt) const
{
  const uint8 bank  = interrupt / 32;
  const uint8 index = interrupt % 32;
  const uint32 bit  = (1 << index);
  switch(bank) {
    case 0: return (state.irq_pending_bitset_0_31    & bit) != 0;
    case 1: return (state.irq_pending_bitset_32_63   & bit) != 0;
    case 2: return (state.irq_pending_bitset_64_95   & bit) != 0;
    case 3: return (state.irq_pending_bitset_96_127  & bit) != 0;
    case 4: return (state.irq_pending_bitset_128_159 & bit) != 0;
    case 5: return (state.irq_pending_bitset_160_191 & bit) != 0;
    case 6: return (state.irq_pending_bitset_192_223 & bit) != 0;
    case 7: return (state.irq_pending_bitset_224_255 & bit) != 0;
    default: UNREACHABLE();
  }
  return false; // we should never get here
}

bool Processor::is_interrupt_enabled_and_pending() const
{
  // An interrupt can never be enabled and pending if interrupts are disabled. Exceptions are fine
  // since they are immediately entered when encountered
  if (!state.IE)return false;
  uint32 value = 
         ((state.irq_pending_bitset_0_31    & state.irq_enabled_bitset_0_31)
        | (state.irq_pending_bitset_32_63   & state.irq_enabled_bitset_32_63)
        | (state.irq_pending_bitset_64_95   & state.irq_enabled_bitset_64_95)
        | (state.irq_pending_bitset_96_127  & state.irq_enabled_bitset_96_127)
        | (state.irq_pending_bitset_128_159 & state.irq_enabled_bitset_128_159)
        | (state.irq_pending_bitset_160_191 & state.irq_enabled_bitset_160_191)
        | (state.irq_pending_bitset_192_223 & state.irq_enabled_bitset_192_223)
        | (state.irq_pending_bitset_224_255 & state.irq_enabled_bitset_224_255));
  return ((value != 0)
          || ((   state.auxs[AUX_IRQ_INTERRUPT] != 0)
               && is_interrupt_enabled(state.auxs[AUX_IRQ_HINT])));
}

/*
 This function is called at a point in the simulation loop
 where an interrupt is permitted to be taken.
 In theory, it ought to be called after every instruction, 
 but in practice it needs to be called only when 'pending_actions'
 is set to true.
 It finds the highest priority interrupt that is enabled
 and if higher priority than the current operating mode, it
 causes the interrupt to be raised.
 
 Interrupts are globally masked using AUX_IENABLE.
 The priority of interrupts is determined by computing
 separately the highest priority level-1 and level-2
 interrupts that are currently pending.
 */
void Processor::detect_interrupts ()
{
  if ((sys_arch.isa_opts.is_isa_a6k() || sys_arch.isa_opts.is_isa_a600())
      && state.D)
    return;
  
  if(sys_arch.isa_opts.new_interrupts) {
    detect_interrupts_a6kv2();
    return;
  }
  
  detect_interrupts_legacy();
}

void Processor::detect_interrupts_a6kv2()
{
  const uint32 pending = get_pending_ints();
  
  if (pending == kInvalidInterrupt)
    return;

  uint32 priority = state.irq_priority[pending]; // priority of pending IRQ

  // If IRQ is configured with a higher priority level than we support, clip the 
  // priority level.
  //
  if (priority >= sys_arch.isa_opts.number_of_levels)
    priority = sys_arch.isa_opts.number_of_levels - 1;

  // If we are not in an interrupt, consider our current priority to be lower
  // than the lowest priority possible
  //
  uint32 curr_level = sys_arch.isa_opts.number_of_levels + 1;
  if ( (state.auxs[AUX_IRQ_ACT] & 0xffff) != 0)
    curr_level = util::index_of_lowest_set_bit(state.auxs[AUX_IRQ_ACT] & 0xffff);

  LOG(LOG_DEBUG3) << "[IRQ] Interrupt priority: " << priority <<", current priority " << curr_level;

  // Check to see if we should take this interrupt (we are not in an exception,
  // interrupts are enabled and this interrupt is above our minimum interrupt
  // priority and our current operating level

  LOG(LOG_DEBUG4) << "[IRQ] State.AE:" << (uint32)state.AE
                  << ", state.IE:"     << (uint32)state.IE
                  << ", state.E:"      << (uint32)state.E
                  << ", priority:"     << priority
                  << ", level:" << curr_level;
  
  if(!state.AE && state.IE
     && (state.E >= priority) && (priority < curr_level)) {
    
    LOG(LOG_DEBUG3) << "[IRQ] Entering interrupt";

    // If this is a P0 interrupt and we have fast IRQ enabled
    if(priority == 0 && (state.auxs[AUX_IRQ_BUILD] & (1 << 28))) {
      enter_a6kv21_fast_interrupt(pending); // take fast IRQ
      return;
    }
    
    enter_a6kv21_interrupt(pending, priority);
  }
}

void Processor::detect_interrupts_legacy()
{
  uint32 pending         = get_pending_ints ();
  uint32 enabled_pending = pending         &  state.auxs[AUX_IENABLE];
  uint32 level2_ints     = enabled_pending &  state.auxs[AUX_IRQ_LEV];
  uint32 level1_ints     = enabled_pending & ~state.auxs[AUX_IRQ_LEV];
  sint32 int_num  = 0;
  sint32 link_num = 1;
  bool next_e1;
  bool next_e2;
  bool next_a1;
  bool next_a2;

  // Look for level-2 interrupts first, and only then consider level-1 interrupts.
  // This (cleverly) uses the x86 'bsf' instruction to scan bits from least-significant
  // to most-significant end of the operand word, returning the bit position of the
  // first '1' found during the scan.
  // We only search for a level-2 interrupt if the state.AE bit is not set.
  // Similarly, we only search for a level-1 interrupt if both the state.AE and state.A2
  // bits are not set.

  //#define DEBUG_DETECT_INTERRUPTS_VERBOSE
#ifdef DEBUG_DETECT_INTERRUPTS_VERBOSE
  fprintf (stdout, "IRQ: PC=0x%08x PENDING=0x%08x ENABLED=0x%08x\n", 
          state.pc, pending, state.auxs[AUX_IENABLE]);
  fprintf (stdout, "     IRQ-LEVELS=0x%08x L2-IRQs=0x%08x L1-IRQs=0x%08x\n",
           state.auxs[AUX_IRQ_LEV], level2_ints, level1_ints);
  fprintf (stdout, "     STATE - AE:%d A2:%d E1:%d E2:%d\n",state.AE, state.A2, state.E1, state.E2);
#endif

  if (!state.AE && (level2_ints != 0) && state.E2)
  {
    // A non-masked level-2 interrupt has been detected, and the processor is 
    // ready to accept a level-2 interrupt.
    
    // Find the highest-priority non-masked level-2 interrupt
    // FIXME: This can be replaced with a compiler intrinsic (__builtin_ctz or __builtin_clz)
    __asm__ __volatile__ ("bsfl %1, %0"
                            : "=r"(int_num)     /* output - %0 */
                            : "m"(level2_ints)  /* output - %1 */
                            : "cc");            /* clobber register */
    
    // Set the link register number used by all level-2 interrupts.
    //
    link_num = 2;
    
    // Determine next A2, A1, E2 and E1 values for a level-2 interrupt.
    //
    next_a2 = true;
    next_a1 = state.A1;
    next_e2 = false;
    next_e1 = false;
    
#ifdef DEBUG_DETECT_INTERRUPTS
    fprintf (stdout, "!! taking L2 interrupt %d when pc = %08x%c%c", int_num, state.pc, 10, 13);
#endif
  } 
  else if (!state.AE && !state.A2 && (level1_ints != 0) && state.E1)
  {
    // A non-masked level-1 interrupt thas been detected, and the processor is 
    // ready to accept a level-1 interrupt.
    
    // Find the highest-priority non-masked level-2 interrupt
    // FIXME: This can be replaced with a compiler intrinsic (__builtin_ctz or __builtin_clz)
    __asm__ __volatile__ ("bsfl %1, %0"
                            : "=r"(int_num)     /* output - %0 */
                            : "m"(level1_ints)  /* output - %1 */
                            : "cc");            /* clobber register */

    // Set the link register number used by all level-1 interrupts.
    //
    link_num = 1;
    
    // Determine next A2, A1, E2 and E1 values for a level-1 interrupt.
    //
    next_a2 = state.A2;
    next_a1 = true;
    next_e2 = state.E2;
    next_e1 = false;
    
#ifdef DEBUG_DETECT_INTERRUPTS
    fprintf (stdout, "!! taking L1 interrupt %d when pc = %08x%c%c", int_num, state.pc, 10, 13);
#endif
  }

  // If int_num is non-zero, an interrupt has been taken.
  // Now we must set PC to the appropriate interrupt vector location,
  // and if the interrupt is pulse-sensitive we must also clear the
  // corresponding interrupt pending bit (by calling 'cancel_interrupt').
  // The link and status registers are saved according to the link_num
  // defined by the interrupt number.
  //
  if (int_num) {
    uint32 old_status32 = BUILD_STATUS32(state);
    state.auxs[AUX_IRQ_LV12] |= link_num;

    if (link_num == 2) {
      state.gprs[ILINK2]          = state.pc;
      state.auxs[AUX_STATUS32_L2] = old_status32;
      state.auxs[AUX_ICAUSE2]     = int_num;
      state.auxs[AUX_BTA_L2]      = state.auxs[AUX_BTA];
      interrupt_stack.push(INTERRUPT_L2_IRQ);
    } else {
      state.gprs[ILINK1]          = state.pc;
      state.auxs[AUX_STATUS32_L1] = old_status32;
      state.auxs[AUX_ICAUSE1]     = int_num;
      state.auxs[AUX_BTA_L1]      = state.auxs[AUX_BTA];
      interrupt_stack.push(INTERRUPT_L1_IRQ);
    }

    // Perform interrupt acknowledgements, firstly within the AUX_INT_PENDING
    // register, and secondly within the I/O devices themselves.
    // For now, only ARC peripherals have this feature implemented.
    //
    cancel_interrupt(int_num);

    // -------------------------------------------------------------------------
    // Acknowledge interrupts
    // FIXME: at the moment we only 'ack' timer IRQs, @see next comment.
    //
    if (int_num == sys_arch.isa_opts.get_timer0_irq_num()) {
      timer_int_ack(0);
    } else if (int_num == sys_arch.isa_opts.get_timer1_irq_num()) {
      timer_int_ack(1);
    }

// FIXME: @igor - provide better and more flexible way to acknowledge interrupts.
//        Hardcoding IRQ numbers in this switch () table does not work as some
//        ARC processors have differing IRQ numbers.
//
//    switch (int_num) {
//      // see p.73 ARCompact Programmers Reference
//      case 0: break; // nothing to acknowledge for Reset
//      case 1: break; // nothing to acknowledge for Memory Error
//      case 2: break; // nothing to acknowledge for Instruction Error
//      case 3: timer_int_ack (0); break; // Timer 0
//      case 4: timer_int_ack (1); break; // Timer 1
//      case 5: /* uart_int_ack (); */  break; // UART
//      case 6: /* emac_int_ack (); */  break; // TBD: emac_int_ack()
//      case 7: /* xymem_int_ack (); */ break; // TBD: xymem_int_ack()
//      default: {
//        // TBD: extension interrupts will each supply an acknowlegement
//        //      routine, which must be called at this point.
//        break;
//      }
//    }

    // Install the changes to STATUS32 required for interrupt entry
    //
    state.E1 = next_e1;
    state.E2 = next_e2;
    state.A1 = next_a1;
    state.A2 = next_a2;
    state.D  = 0;
    state.L  = 1;
    state.DZ = 0;
    // the following will set state.U correctly
    set_operating_mode(MAP_OPERATING_MODE(KERNEL_MODE));
    
    // Get the interrupt vector
    //
    const uint32 int_vector = state.auxs[AUX_INT_VECTOR_BASE] + (int_num << 3);
    
    // If SmaRT is enabled, then push an entry onto the SmaRT stack 
    // for the current interrupt.
    //
    if (smt.enabled())
      smt.push_exception(state.pc, int_vector);
    
    // Set PC to the interrupt vector and adjust the operating mode to complete
    // the semantic actions associated with taking an interrupt.
    //
    state.pc = int_vector;

#ifdef DEBUG_DETECT_INTERRUPTS_VERBOSE
    uint32 new_status32 = BUILD_STATUS32(state);
    fprintf (stdout, "IRQ: STATUS32 = 0x%08x, now vectoring to 0x%08x...\n", new_status32, state.pc);
#endif
  }
#ifdef DEBUG_DETECT_INTERRUPTS_VERBOSE
  else fprintf (stdout, "IRQ: none pending and enabled\n");
#endif
}


} } } //  arcsim::sys::cpu

