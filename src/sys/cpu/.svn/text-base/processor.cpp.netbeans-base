//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2005 The University of Edinburgh
//                        All Rights Reserved
//
//
// =====================================================================
//
// Description:
//
// This file implements the methods declared in the Processor class.
//
// =====================================================================

#include <iomanip>
#include <cstdlib>
#include <cstdio>

#include "Assertion.h"

#include "system.h"
#include "exceptions.h"

#include "sys/cpu/state.h"
#include "sys/cpu/processor.h"
#include "sys/cpu/EiaExtensionManager.h"

#include "sys/mmu/Mmu.h"

#include "isa/arc/DcodeConst.h"
#include "isa/arc/Opcode.h"
#include "isa/arc/Dcode.h"
#include "isa/arc/Disasm.h"

#include "ise/eia/EiaInstructionInterface.h"
#include "ise/eia/EiaCoreRegisterInterface.h"
#include "ise/eia/EiaConditionCodeInterface.h"

#include "uarch/processor/ProcessorPipelineInterface.h"
#include "uarch/processor/ProcessorPipelineFactory.h"

#include "uarch/bpu/BranchPredictorInterface.h"
#include "uarch/bpu/BranchPredictorFactory.h"

#include "uarch/memory/WayMemorisation.h"
#include "uarch/memory/WayMemorisationFactory.h"

#include "ioc/Context.h"
#include "ioc/ContextItemId.h"
#include "ioc/ContextItemInterface.h"

#include "ipt/IPTManager.h"

#include "util/Util.h"
#include "util/Log.h"
#include "util/OutputStream.h"

#include "util/Counter.h"
#include "util/CounterTimer.h"
#include "util/Histogram.h"
#include "util/MultiHistogram.h"

#include "ioc/Context.h"


#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_

namespace arcsim {
  namespace sys {
    namespace cpu {

      // Formats used to print out trace information about load and store instructions.
      //
      static const char * const kTraceFormatStr[NUM_T_FORMATS] = {
        " : lw [%08x] => %08x",           // T_FORMAT_LW
        " : lh [%08x] => %04x",           // T_FORMAT_LH
        " : lb [%08x] => %02x",           // T_FORMAT_LB
        " : sw [%08x] <= %08x",           // T_FORMAT_SW
        " : sh [%08x] <= %04x",           // T_FORMAT_SH
        " : sb [%08x] <= %02x",           // T_FORMAT_SB
        " : sw EXCEPTION [%08x] <= %08x", // T_FORMAT_SWX
        " : sh EXCEPTION [%08x] <= %04x", // T_FORMAT_SHX
        " : sb EXCEPTION [%08x] <= %02x", // T_FORMAT_SBX
        " : lw EXCEPTION [%08x] => %08x", // T_FORMAT_LWX
        " : lh EXCEPTION [%08x] => %04x", // T_FORMAT_LHX
        " : lb EXCEPTION [%08x] => %02x"  // T_FORMAT_LBX
      };      
            
// -----------------------------------------------------------------------------
// Processor Constructor
//
Processor::Processor (System&                   sys,
                      CoreArch&                 core_arch,
                      arcsim::ioc::Context&     ctx, 
                      arcsim::sys::mem::Memory* memory,
                      MemoryModel*              memory_model,
                      int                       core_id,
                       const char*              name)
  : system(sys),
    mem(*memory),
    sys_arch(sys.sys_conf.sys_arch),
    sim_opts(sys.sys_conf.sys_arch.sim_opts),
    cur_exec_mode(kExecModeInterpretive),
    core_arch(core_arch),
    core_id(core_id),
    mem_model(memory_model),
    bpu(BranchPredictorFactory::create_branch_predictor(core_arch.bpu)),
    iway_pred(WayMemorisationFactory::create_way_memo(core_arch.iwpu, CacheArch::kInstCache)),
    dway_pred(WayMemorisationFactory::create_way_memo(core_arch.dwpu, CacheArch::kDataCache)),
    sim_started(false),
    local_hotspot_threshold(sys_arch.sim_opts.hotspot_threshold),
    pipeline(ProcessorPipelineFactory::create_pipeline(core_arch.pipeline_variant)),
    // Create CCM manager
    // FIXME: construct this via Container
    ccm_mgr_(new CcmManager(core_arch, sys.sys_conf.sys_arch.isa_opts, sys.sys_conf.sys_arch.sim_opts)),
    // Instantiate CounterTimers
    exec_time(*(arcsim::util::CounterTimer*)ctx.create_item(arcsim::ioc::ContextItemInterface::kTCounterTimer,
                                                            arcsim::ioc::ContextItemId::kExecutionTime)),
    trace_time(*(arcsim::util::CounterTimer*)ctx.create_item(arcsim::ioc::ContextItemInterface::kTCounterTimer,
                                                             arcsim::ioc::ContextItemId::kTraceTime)),
    // Instantiate CounterContext class Processor Context
    cnt_ctx(ctx),
    // Instantiate PhysicalProfile
    phys_profile_(*(arcsim::profile::PhysicalProfile*)ctx.create_item(arcsim::ioc::ContextItemInterface::kTPhysicalProfile,
                                                                      arcsim::ioc::ContextItemId::kPhysicalProfile)),
    // Instantiate IPTManager
    ipt_mgr(*(arcsim::ipt::IPTManager*)ctx.create_item(arcsim::ioc::ContextItemInterface::kTIPTManager,
                                                       arcsim::ioc::ContextItemId::kIPTManager)),
    // Initialise timer specific members
    use_host_timer(false),
    inst_timer_enabled(false),
    count_increment(0),
    running0(false),
    running1(false),
    vcount0(0),
    vcount1(0),
    timer_sync_time(0),
    last_rtc_clear(0),
    last_rtc_disable(0)
{  
  uint32 i;
  for (i = 0; i < kProcessorMaxNameSize - 1 && name[i]; ++i)
    name_[i] = static_cast<uint8>(name[i]);
  name_[i] = '\0';      

  // Instantiate Timer
  timer = new arcsim::util::system::Timer(this, arcsim::util::system::Timer::kDefaultTimeSlice,
                                          true, core_id);
  
  // Initialise TraceStream
  //
  TS.set_buffer_size(100000);
  
  if (sim_opts.redir_inst_trace_output) {
    // Associates stream with existing file descriptor
    //
    TS.set_out_fd(fdopen(sim_opts.rinst_trace_fd, "w"));

    // Write copyright header into the trace file, including the revision number
    //
    TS.write(ARCSIM_COPYRIGHT, strlen(ARCSIM_COPYRIGHT));
  }
    
  // Configure CCM Manager
  //
  ccm_mgr_->configure();  

  // Initialise processor state structure
  //
  init_cpu_state ();
    
  // Initiliase timers used for this processor
  //
  init_timers ();
  
  // Initiliase data structure holding all information about physical pages
  //
  phys_profile_.construct(256,                                   // page lookup cache size
                          core_arch.page_arch.byte_index_shift,
                          core_arch.page_arch.page_byte_frame_mask);

  // Initialise PageCache
  //
  page_cache.init(&state, &core_arch.page_arch);

  // Put CPU in proper state and clear counters
  //
  reset ();
  clear_cpu_counters ();
  
  if (sim_opts.memory_sim) {    
    // Initialise LatencyCache
    //
    mem_model->latency_cache.init(&state);
    
    if (sys_arch.isa_opts.dc_uncached_region) {
      mem_model->cached_limit = state.auxs[AUX_CACHE_LIMIT] - 1;
    }
    
    // Initialise cache model counters
    //
    if (mem_model->icache_c) {
      mem_model->icache_c->miss_freq_hist   = &cnt_ctx.icache_miss_freq_hist;
      mem_model->icache_c->miss_cycles_hist = &cnt_ctx.icache_miss_cycles_hist;
      mem_model->icache_c->is_cache_miss_frequency_recording_enabled  = sim_opts.is_cache_miss_recording_enabled;
      mem_model->icache_c->is_cache_miss_cycle_recording_enabled      = sim_opts.is_cache_miss_cycle_recording_enabled;
    }
    if (mem_model->dcache_c) {
      mem_model->dcache_c->miss_freq_hist   = &cnt_ctx.dcache_miss_freq_hist;
      mem_model->dcache_c->miss_cycles_hist = &cnt_ctx.dcache_miss_cycles_hist;
      mem_model->dcache_c->is_cache_miss_frequency_recording_enabled  = sim_opts.is_cache_miss_recording_enabled;
      mem_model->dcache_c->is_cache_miss_cycle_recording_enabled      = sim_opts.is_cache_miss_cycle_recording_enabled;
    }
  }

  // Initialise translation cache
  //
  trans_cache.construct(sim_opts.trans_cache_size);
  
  // Initialise decode caches
  //
  for (uint32 i = 0; i < NUM_OPERATING_MODES; ++i) {
    dcode_caches[i].construct(sim_opts.dcode_cache_size);
  }
  // Currently active decode cache is kernel cache
  //
  dcode_cache = &(dcode_caches[KERNEL_MODE]);  
}

Processor::~Processor ()
{
  // Clean-up dynamically allocated members
  if (timer)    { delete timer;     timer    = 0; }
  if (bpu)      { delete bpu;       bpu      = 0; }
  if (pipeline) { delete pipeline;  pipeline = 0; }
  if (iway_pred){ delete iway_pred; iway_pred= 0; }
  if (dway_pred){ delete dway_pred; dway_pred= 0; }
  if (ccm_mgr_) { delete ccm_mgr_;                }
  
  // Destroy dynamically allocated PageCache data 
  page_cache.destroy();
  
  // Destroy Zone used for fast allocation
  zone_.Delete();
}


bool
Processor::reset_to_initial_state (bool purge_translations)
{
  bool success = true;
  
  // Initialise processor state structure
  //
  init_cpu_state ();
  
  // Reset CPU
  //
  reset ();

  // Clear all counters
  //
  clear_cpu_counters ();
  
  // Clear profiling counters 
  //
  cnt_ctx.clear();
  
  // Re-initialise timers
  //
  init_timers ();
  
  // Reset timing counters
  exec_time.reset();
  trace_time.reset();
  
  // Configure the Actionpoints sub-system
  //
  aps.configure(this, sys_arch.isa_opts.num_actionpoints, sys_arch.isa_opts.aps_full);
  
  // Configure the SmaRT sub-system
  //
  smt.configure(this, sys_arch.isa_opts.smart_stack_size);
  
  if (purge_translations) {
    // Remove ALL translations
    phys_profile_.remove_translations();
  }
  
  // Flush Page Cache
  //
  page_cache.flush(arcsim::sys::cpu::PageCache::ALL);
  
  // Purge translation cache
  //
  purge_translation_cache();
  
  // Purge decode caches
  //
  purge_dcode_cache();
  
  // Re-configure CCM Manager
  //
  ccm_mgr_->configure();
  
  // Clear memory model states
  //
  if (mem_model) { mem_model->clear(); }
  
  return success;
}


// -----------------------------------------------------------------------------
// The INTERPRETER LOOP
//

// Include Processor::step method
//
#define STEP
#include "processor-step.cpp"
#undef STEP

// Include Processor::step_fast method
//
#define STEP_FAST
#include "processor-step.cpp"
#undef STEP_FAST

// -----------------------------------------------------------------------------
// Processor state/register etc. initialisation
//

void
Processor::prepare_cpu ()
{
  // Empty and set-up interrupt state
  //
  while (!interrupt_stack.empty()) { interrupt_stack.pop(); }
  interrupt_stack.push(INTERRUPT_NONE);

  lp_end_to_lp_start_map[0] = 0x1;

  end_of_block   = false;
  prev_had_dslot = false;
}

void
Processor::clear_cpu_counters ()
{
  state.iterations         = 0;
  trace_interval           = 0;
  trace_interp_block_count = 0;
  clear_pending_actions();
}

// Initialise cpuState structure
//
void
Processor::init_cpu_state () {
  // Wire-up pointer to surrounding cpuContext on cpuState struct
  //
  state.cpu_ctx       = (cpuContext)this;
  state_trace.cpu_ctx = (cpuContext)this;
  
  state.pc           = 0;
  state.next_pc      = 0;
  state.iterations   = 0;

  // Initialise ibuff_addr to be invalid
  //
  state.ibuff_addr = 0x80000000;

#ifdef CYCLE_ACC_SIM
  // Init of counters for cycle accurate mode
  //
  for(size_t stage = 0; stage < PIPELINE_STAGES; ++stage) {
    state.pl[stage] = 0;
  }
  // warmup-time in cycles from configuration
  state.pl[FET_ST] = core_arch.cpu_warmup_cycles;
  state.flag_avail = 0;
  state.ignore = 0;
  state.t0 = 0;
  state.timer_expiry = 0xffffffffffffffffULL;
#endif /* CYCLE_ACC_SIM */

#ifdef ENABLE_BPRED
  state.delayed_call    = false;
  state.delayed_return  = false;
#endif /* ENABLE_BPRED */
  
  // Initialize the register pointers, in preparation
  // for loading EIA extensions.
  //
  for (int i = 0; i < GPR_BASE_REGS; ++i) {
     state.xregs[i] = &(state.gprs[i]);
  }
}

void
Processor::init_gprs ()
{
  for (int i = 0; i < GPR_BASE_REGS; ++i) {
    state.gprs[i]  = 0;
#ifdef CYCLE_ACC_SIM
    state.gprs_avail[i]  = 0;
#endif
#ifdef REGTRACK_SIM
    state.gprs_stats[i].read  = 0;
    state.gprs_stats[i].write = 0;
    state.gprs_stats[i].geom  = 1;
    state.gprs_stats[i].arith = 0;
    state.gprs_stats[i].last  = 0;
    state.gprs_stats[i].regno = i;
#endif
  }
}

// -----------------------------------------------------------------------------
// Register EIA extensions
//
bool
Processor::register_eia_extension(arcsim::ise::eia::EiaExtensionInterface* eia_ext)
{
  // Load EIA extension
  //
  bool loaded_eia = eia_mgr.add_eia_extension(core_id, eia_ext);
  
  if (loaded_eia && eia_mgr.are_eia_core_regs_defined) {
    // Set up the addresses of any extension core registers in state.xregs[]
    //
    for (std::map<uint32,arcsim::ise::eia::EiaCoreRegisterInterface*>::const_iterator
            I = eia_mgr.eia_core_reg_map.begin(),
            E = eia_mgr.eia_core_reg_map.end();
         I != E; ++I)
    { 
      LOG(LOG_DEBUG2) << "[CPU" << core_id << "] CPU state maps register r"
                      << I->second->get_number()
                      << " at address 0x" << std::hex << I->second->get_value_ptr();
                      
      state.xregs[I->second->get_number()] = I->second->get_value_ptr();
    }
  }
  
  return loaded_eia;
}
      
// -----------------------------------------------------------------------------
// Evaluate an EIA extension condition
//
bool
Processor::eval_extension_cc (uint8 cc) const
{
  return inst->eia_cond->eval_condition_code (cc);
}

// -----------------------------------------------------------------------------
// Processor timing
//

// Called just before simulation starts on processor
//
void
Processor::simulation_start()
{
  if (!sim_started) {
    sim_started = true;
    // reset time recording counters
    exec_time.reset();
    trace_time.reset();
  }
}

void Processor::simulation_continued()
{
  restart_from_halted ();
  timing_restart ();
}

// Called immediately after simulation ends on processor
//
void Processor::simulation_end()
{
  // FIXME: here we should probably set 'sim_started' to false
  //
}

void Processor::simulation_stopped()
{
  stop_timers ();
}

void Processor::timing_checkpoint ()
{
  // First we stop the simulated TIMER0 and TIMER1 peripherals
  //
  stop_timers ();  
}

void Processor::timing_restart ()
{
  // Start the TIMER0 and TIMER1 peripherals.
  start_timers();
}


// -----------------------------------------------------------------------------
// Processor performance metrics
//

uint64 
Processor::instructions () const
{ 
  return cnt_ctx.interp_inst_count.get_value() 
         + cnt_ctx.native_inst_count.get_value(); 
}

      
// Cycle Accurate CPI function
//
double Processor::cycle_sim_cpi () const
{
  double inst_count = instructions();
  double cycl_count = cnt_ctx.cycle_count.get_value();
  
  return (inst_count > 0) ? (cycl_count/inst_count) : 0;
}

// Cycle Accurate IPC function
//
double Processor::cycle_sim_ipc () const
{
  double inst_count = instructions();
  double cycl_count = cnt_ctx.cycle_count.get_value();
    
  return (cycl_count > 0 ) ? (inst_count/cycl_count) : 0;
}


// -----------------------------------------------------------------------------
// Instruction decode logic
//

// Fetch and Decode instruction method
//
uint32
Processor::decode_instruction (uint32                   pc,
                               arcsim::isa::arc::Dcode& d,
                               cpuState&                state,
                               uint32&                  efa,
                               bool                     in_dslot)
{
  // Fetch and Decode into new Dcode object
  //
  return fetch_and_decode(pc, d, state, efa, in_dslot, false);
}

// Fetch and Decode an instruction using the DecodeCache as an object/memory pool:
//  1. Lookup in the DecodeCache if an instruction for that PC already exists
//  1.a On a CACHE HIT assign the pointer reference to iptr
//  1.b On a CACHE MISS the memory location/memory space in the cache is used 
//      to decode the instruction to
//    
// ATTENTION: THIS METHOD IS NOT THREAD SAFE AND SHOULD ONLY BE CALLED BY THE
//            MAIN SIMULATION LOOP!
//
uint32
Processor::decode_instruction_using_cache (uint32                     pc,
                                           arcsim::isa::arc::Dcode* & dptr,
                                           uint32&                    efa,
                                           bool                       in_dslot)
{
  uint32                                ecause   = 0;
  arcsim::isa::arc::DcodeCache::HitType hit_type = arcsim::isa::arc::DcodeCache::kCacheMiss;
  
  // lookup which Dcode object we can use in decode cache and mark it as valid
  dptr = dcode_cache->lookup(pc, &hit_type);
  if (hit_type == arcsim::isa::arc::DcodeCache::kCacheMiss) {
    
    // fetch and decode instruction on decode cache miss
    ecause = fetch_and_decode(pc, *dptr, state, efa, in_dslot, true);
    
    if (ecause) { // instr decode failed
      dcode_cache->purge_entry(pc); // mark entry in decode cache as invalid
    }
  }
  return ecause;
}

// Fetch and decode an instruction
//
uint32
Processor::fetch_and_decode (uint32                   pc,
                             arcsim::isa::arc::Dcode& d,
                             cpuState&                state,
                             uint32&                  efa,
                             bool                     in_dslot,
                             bool                     have_side_effect)
{
  uint32 ecause       = 0;
  
  // Fetch next instruction from memory
  //
  if ((ecause = fetch32(pc, d.info.ir, efa, have_side_effect))) {
    return ecause;
  }                
  // Pre-decode instruction
  //
  d.decode(sys_arch.isa_opts, d.info.ir, pc, state, eia_mgr, in_dslot);
  
  // Actionpoints: if there are any instruction-based Actionpoints defined
  //  then match the current pc and ir against those defined Actionpoints.
  //
  if (    aps.has_inst_aps()
       && !state.AE 
       && aps.match_instruction (pc, d.info.ir, &(d.aps_inst_matches), &(d.shimm)))
  { // This is a complete match, so adjust the sim opcode to SIM_AP_BREAK
    //
    d.code = arcsim::isa::arc::OpCode::AP_BREAK;
  }
  
  // Memory model and cycle accurate simulation
  //
  if (sim_opts.memory_sim)
  {
    uint32 buffer_addr = pc & 0xFFFFFFFC;       // 32-bit inst buffer addr
    d.fetches          = 1;                     // instruction fetch (1st fetch)
    d.fetch_addr[0]    = pc;
    // 16-bit aligned LW inst. (2nd fetch)
    if ((pc & 0x00000002) && !(d.info.ir & 0xC0000000)) {
      d.fetch_addr[d.fetches++] = (buffer_addr += 4);
    }
    // LIMM (2nd/3rd fetch)
    if (d.has_limm) {
      d.fetch_addr[d.fetches++] = (buffer_addr += 4);
    }

#ifdef CYCLE_ACC_SIM
    if (sim_opts.cycle_sim)
    { // Get instruction execution cycles
      // FIXME
      d.exe_cycles = (core_arch.isa_cyc) ? core_arch.isa[d.code] : 1 ;
      
      // Set any pipeline model parameters that can be pre-computed during decode
      //
      pipeline->precompute_pipeline_model(d, sys_arch.isa_opts);
    }
#endif /* CYCLE_ACC_SIM */
  }
  
  // If the instruction has long-immediate data, fetch it and install it into the
  // Dcode object. Also, add 4 to the instruction size.
  //
  if (d.has_limm) {
    if ((ecause = fetch32(pc + d.size, d.limm, efa, have_side_effect))) {
      efa = pc + d.size;
      return ecause;
    }
    d.size += 4;
  }
  
  // If this is a branch with a delay slot then fetch the delay-slot instruction
  // and compute the link offset distance accordingly.
  //
  if (d.dslot) {
    if ((ecause = fetch_and_decode (pc + d.size, inst_dslot, state, efa, true, have_side_effect))) {
      return ecause;
    }
    d.link_offset = d.size + inst_dslot.size;
    
    // Note, delay-slot instructions cannot have LIMM data, so check
    // for this and eliminate any erroneous LIMM from the instruction size.
    // If modeling ARCv2 ISA, then link_offset will include the LIMM.
    //
    if (inst_dslot.has_limm) {
      inst_dslot.set_instruction_error (sys_arch.isa_opts, eia_mgr);
      if (!sys_arch.isa_opts.is_isa_a6k()) {
        d.link_offset -= 4;
      }
    }
  } else {
    d.link_offset = d.size;
  }
  return ecause;  
}

      
// -----------------------------------------------------------------------------
// Halt/reset/interrupt/flag etc. logic
//

void
Processor::system_reset ()
{
  system.soft_reset ();
}

void
Processor::reset ()
{
  // Re-initialise GPRs
  //
  init_gprs ();

  // Initialize the pc_mask, lpc_mask and addr_mask state variables.
  // These depend on the pc_size, lpc_size and addr_size configuration
  // parameters.
  //
  state.pc_mask   = (0xffffffffUL >> (32 - sys_arch.isa_opts.pc_size)) ^ 1UL;
  state.lpc_mask  = (0xffffffffUL >> (32 - sys_arch.isa_opts.lpc_size));
  state.addr_mask = (0xffffffffUL >> (32 - sys_arch.isa_opts.addr_size));
  
  // Initialise the same masks in the dummy state structure, used during
  // JIT binary translation.
  //
  state_trace.pc_mask   = state.pc_mask;
  state_trace.lpc_mask  = state.lpc_mask;
  state_trace.addr_mask = state.addr_mask;
  
  // Configure the SmaRT sub-system
  //
  smt.configure(this, sys_arch.isa_opts.smart_stack_size);
  
  // Re-initilise AUX registers
  //
  init_aux_regs ();
  
  // Construct correctly configured MMU instance
  //
  mmu.construct(this, core_arch.mmu_arch);
  
  // Configure the Actionpoints sub-system.
  //
  aps.configure(this, sys_arch.isa_opts.num_actionpoints, sys_arch.isa_opts.aps_full);

  // Initialise cpuState
  //
  
  //If we're in A6kV2.1 with new interrupts, we need to load the start address 
  //from the interrupt vector table rather than just jumping there.
  
  if(sys_arch.isa_opts.new_interrupts) {
    
#ifndef V30_VECTOR_TABLE
    
    uint32 ecause, efa, reset_ev;
    if(ecause = fetch32(state.auxs[AUX_INT_VECTOR_BASE], reset_ev, efa, true))
      enter_exception(ecause, efa, 0);
    state.pc = ((reset_ev & 0xffff) << 16) | ((reset_ev & 0xffff0000) >> 16);
    LOG(LOG_INFO) << "[Reset] Loaded PC from EV 0: 0x" << HEX(state.pc);
    
#else
    
    state.pc = state.auxs[AUX_INT_VECTOR_BASE];
    
#endif
    
  }
  else 
    state.pc = state.auxs[AUX_INT_VECTOR_BASE];
  state.next_pc = 0;
  state.IE = 0;
  state.E  = 0;
  state.ES = 0;
  state.SC = 0;
  state.DZ = 0;
  state.L  = 0;
  state.Z  = 0;
  state.N  = 0;
  state.C  = 0;
  state.V  = 0;
  state.U  = 0;
  state.D  = 0;
  state.AE = 0;
  state.A2 = 0;
  state.A1 = 0;
  state.E2 = 0;
  state.E1 = 0;
  state.H  = 1;
  state.X3 = 0;
  state.X2 = 0;
  state.X1 = 0;
  state.X0 = 0;
  state.stack_top=0;
  state.stack_base=0;
  state.shadow_stack_top=0;
  state.shadow_stack_base=0;
  state.gprs[LP_COUNT] = 0;
  state.lp_end = state.lp_start = 0;
  state.lock_phys_addr = 0;
  state.raise_exception = 0;
  set_operating_mode (KERNEL_MODE);
  
  //All irqs enabled at reset
  state.irq_enabled_bitset_0_31    = 0xFFFFFFFF;
  state.irq_enabled_bitset_32_63   = 0xFFFFFFFF;
  state.irq_enabled_bitset_64_95   = 0xFFFFFFFF;
  state.irq_enabled_bitset_96_127  = 0xFFFFFFFF;
  state.irq_enabled_bitset_128_159 = 0xFFFFFFFF;
  state.irq_enabled_bitset_160_191 = 0xFFFFFFFF;
  state.irq_enabled_bitset_192_223 = 0xFFFFFFFF;
  state.irq_enabled_bitset_224_255 = 0xFFFFFFFF;
  
  //No irqs pending at reset
  state.irq_pending_bitset_0_31    = 0;
  state.irq_pending_bitset_32_63   = 0;
  state.irq_pending_bitset_64_95   = 0;
  state.irq_pending_bitset_96_127  = 0;
  state.irq_pending_bitset_128_159 = 0;
  state.irq_pending_bitset_160_191 = 0;
  state.irq_pending_bitset_192_223 = 0;
  state.irq_pending_bitset_224_255 = 0;
  
  prepare_cpu ();
}


void
Processor::set_operating_mode (OperatingMode mode)
{
  // Choose appropriate decode cache depending on operating mode
  dcode_cache = &(dcode_caches[mode]);
  if(sys_arch.isa_opts.stack_checking){
    //always check because we may have just changed the SC bit by returning form interrupt
    if(sys_arch.isa_opts.is_isa_a6kv2() && state.U != mode)
    {
      // Swap kernel and user stack registers
      uint32 temp = state.stack_base;
      state.stack_base = state.shadow_stack_base;
      state.shadow_stack_base = temp;
      temp = state.stack_top;
      state.stack_top = state.shadow_stack_top;
      state.shadow_stack_top = temp;
    }
  }
  if(sys_arch.isa_opts.new_interrupts && state.U != mode)
  {
    LOG(LOG_DEBUG2) << "Swapping stacks";
    uint32 temp = state.gprs[SP_REG];
    state.gprs[SP_REG] = state.auxs[AUX_USER_SP];
    state.auxs[AUX_USER_SP] = temp;
  }
  
  if ((state.U != mode) && (core_arch.mmu_arch.is_configured)){
    purge_page_cache(PageCache::ALL);
  }
  
  state.U = mode; // modify cpuState to new operating mode
}

// On entry to exceptions, the individual flags in the state structure are live,
// and the STATUS32 bits should be considered stale.
//
void
Processor::enter_exception (uint32 ecr,   // exception cause register
                            uint32 efa,   // exception fault address
                            uint32 eret)  // exception return address
{
  if(sys_arch.isa_opts.new_interrupts)
  {
    LOG(LOG_DEBUG) << "Entering A6KV2.1 Exception @ 0x" << std::hex << std::setw(8) << std::setfill('0') << (state.pc);;
    enter_a6kv21_exception(ecr, efa, eret);
    return;
  }
  
  // On entry to exceptions, the individual flags in the state
  // structure are live, and the STATUS32 bits should be considered stale.
  
  uint32 old_status32     = BUILD_STATUS32(state); // make a snapshot of status32
  uint32 exception_cause  = ecr;

  if (state.AE) { // Check for double exception 
    exception_cause = ECR(sys_arch.isa_opts.EV_MachineCheck, 0, 0);  // raise a machine-check exception with cause code 0 
    if(core_arch.mmu_arch.kind==MmuArch::kMmu)
      mmu.write_pid(state.auxs[AUX_PID] & 0xff);     // disable MMU on EV_MachineCheck
  }

  // Modify STATUS32 for kernel mode and exception entry
  state.E1 = 0;
  state.E2 = 0;
  state.AE = 1;
  state.D  = 0;
  state.L  = 1;
  state.DZ = 0;
  state.ES = 0;
  
  set_operating_mode(MAP_OPERATING_MODE(KERNEL_MODE)); // sets state.U

  if (sys_arch.isa_opts.is_isa_a600()) { // mask out bits not present on A600
    const uint32 a600_status32  = BUILD_STATUS32(state) & kA600AuxStatus32Mask;
    old_status32               &= kA600AuxStatus32Mask;
    state.auxs[AUX_STATUS32_L2] = old_status32;
    state.gprs[ILINK2]          = efa  & 0xfffffffe;
    EXPLODE_STATUS32(state,a600_status32);
  }
  
  // FIXME: @igor - the following aux registers do not exist for A600, writing to
  //        them should not really present a problem but we need to provide a more
  //        flexible implementation
  state.auxs[AUX_ERSTATUS] = old_status32;
  state.auxs[AUX_ERET]     = eret & 0xfffffffe;
  // Why was this masked? this can be the address of a single byte load, 
  // not just an instruction so should not have the bottom bit truncated.
  // @chris 27/3/2012 star 9000531924
  //state.auxs[AUX_EFA]      = efa  & 0xfffffffe;  
  state.auxs[AUX_EFA]      = efa;  
  state.auxs[AUX_ERBTA]    = state.auxs[AUX_BTA];
  state.auxs[AUX_ECR]      = exception_cause;
    
  // compute exception vector address that we should jump to
  const uint32 excpn_pc = state.auxs[AUX_INT_VECTOR_BASE] + ECR_OFFSET(exception_cause);
  
  if (smt.enabled()) { // Push exception entry into SmaRT stack if SmaRT it is enabled
    LOG(LOG_DEBUG4) << "[SMT] Exception entry: ECR = " << HEX(ecr) << ", EFA = " << efa;
    if (((ecr >> 16) & 0xff) == sys_arch.isa_opts.EV_Trap) {
      // Trap exceptions must always set SmaRT SRC_ADDR to address of trap instr
      // even if they are in a delay slot (when state.pc will be the target already)
      //
      smt.push_exception(efa, excpn_pc);
    } else {
      // SmaRT SRC_ADDR is state.pc by default
      smt.push_exception(state.pc, excpn_pc);
    }
  }
  
  // Set PC to the exception vector
  state.pc            = excpn_pc;
  state.gprs[PCL_REG] = excpn_pc & 0xfffffffc;
    
  // Signal fast mode that we are in an exception
  interrupt_stack.push(INTERRUPT_EXCEPTION);  
  state.raise_exception = 1;  // for current inst only
}

void
Processor::exit_exception ()
{
  // Never arrive here in User mode!
  if(sys_arch.isa_opts.new_interrupts)
  {
    return_from_ie_a6kv21();
    return;
  }
  
  if (state.AE || ((state.A1 == 0) && (state.A2 == 0))) {
    // Assert state.AE == 1 or not in exception or interrupt handler
    // Return from exception using ERET, ERSTATUS and ERBTA
    state.next_pc             = state.auxs[AUX_ERET];
    state.auxs[AUX_STATUS32]  = state.auxs[AUX_ERSTATUS];
    state.auxs[AUX_BTA]       = state.auxs[AUX_ERBTA];
    if (interrupt_stack.top() == INTERRUPT_EXCEPTION) {
      interrupt_stack.pop();
    }
  } else {
    // Assert (state.AE == 0) && ((state.A1 == 1) || (state.A2 == 1))
    // Return from interrupt using ILINK1 or ILINK2,
    // STATUS32_L1 or STATUS32_L2, and ERBTA_L1 or ERBTA_L2
    //
    if (state.A2) {
      state.next_pc             = state.gprs[ILINK2];
      state.auxs[AUX_STATUS32]  = state.auxs[AUX_STATUS32_L2];
      state.auxs[AUX_BTA]       = state.auxs[AUX_BTA_L2];
      if (interrupt_stack.top() == INTERRUPT_L2_IRQ) {
        interrupt_stack.pop();
      }
    } else {
      // Assert (state.A1 == 1)
      state.next_pc             = state.gprs[ILINK1];
      state.auxs[AUX_STATUS32]  = state.auxs[AUX_STATUS32_L1];
      state.auxs[AUX_BTA]       = state.auxs[AUX_BTA_L1];
      if (interrupt_stack.top() == INTERRUPT_L1_IRQ) {
        interrupt_stack.pop();
      }
    }
  }
  uint8 old_stateU=state.U;
  EXPLODE_STATUS32(state,state.auxs[AUX_STATUS32]);
  uint8 new_stateU=state.U;
  state.U=old_stateU;
  set_operating_mode (MAP_OPERATING_MODE(new_stateU));
  set_pending_action(kPendingAction_CPU);
}

void
Processor::exit_interrupt (int level)
{
  if(sys_arch.isa_opts.new_interrupts)
  {
    return_from_ie_a6kv21();
    return;
  }
  
  // Apparently, J.F [ILINKn] is allowed in User mode
  if (level == 2) {
    // Assert (state.A2 == 1)
    state.next_pc             = state.gprs[ILINK2];
    state.auxs[AUX_STATUS32]  = state.auxs[AUX_STATUS32_L2];
    state.auxs[AUX_BTA]       = state.auxs[AUX_BTA_L2];
    if (interrupt_stack.top() == INTERRUPT_L2_IRQ) {
      interrupt_stack.pop();
    }
  } else {
    state.next_pc             = state.gprs[ILINK1];
    state.auxs[AUX_STATUS32]  = state.auxs[AUX_STATUS32_L1];
    state.auxs[AUX_BTA]       = state.auxs[AUX_BTA_L1];
    if (interrupt_stack.top() == INTERRUPT_L1_IRQ) {
      interrupt_stack.pop();
    }
  }
  uint8 old_stateU=state.U;
  EXPLODE_STATUS32(state,state.auxs[AUX_STATUS32]);
  uint8 new_stateU=state.U;
  state.U=old_stateU;
  set_operating_mode(MAP_OPERATING_MODE(new_stateU));
  set_pending_action(kPendingAction_CPU);
}

// -----------------------------------------------------------------------------
// Simulator 'interrupt' called when a Watchpoint has been triggered by the
// previously-committed instruction.
//
// This may perform a breakpoint halt (if action == 0) or raise a
// Privilege Violation exception (if action == 1, and state.AE == 0).
// In both cases, the action takes place AFTER the current instruction has
// committed.
//
void
Processor::trigger_watchpoint ()
{
  // Take the action indicated by aps.aps_action
  //
  if (aps.aps_action == 0) {    
    // Update all matching Actionpoint value registers with
    // the value they matched with.
    //
    aps.copy_matching_values();

    // Insert the ASR[7:0] bits into the DEBUG auxiliary register
    //
    state.auxs[AUX_DEBUG] = (state.auxs[AUX_DEBUG] & 0xfffff807UL) | ((aps.aps_matches & 0xffUL) << 3);
    // Implement the breakpoint halt
    //
    break_inst();
  }
  else if (state.AE == 0) {    
    // Update all matching Actionpoint value registers with
    // the value they matched with.
    //
    aps.copy_matching_values();

    // Insert the ASR[7:0] bits into the DEBUG auxiliary register
    //
    state.auxs[AUX_DEBUG] = (state.auxs[AUX_DEBUG] & 0xfffff807UL) | ((aps.aps_matches & 0xffUL) << 3);

    int apn = 0;
    uint32 dbg_aps = aps.aps_hits & 0xffUL;
    
    LOG(LOG_DEBUG4) << "[APS] Raising Watchpoint exception with aps_hits = " 
                    << HEX(aps.aps_hits) << ", aps_matches = " << aps.aps_matches;

    LOG(LOG_DEBUG4) << "[APS] Setting DEBUG = " << HEX(state.auxs[AUX_DEBUG]);
                    
    // find first bit set in aps, assigning bit number to apn
    //
    while (dbg_aps && ((dbg_aps & 0x1) == 0)) {
      ++apn;
      dbg_aps = dbg_aps >> 1;
    }
    
    uint32 ecr_value = ECR(sys_arch.isa_opts.EV_PrivilegeV, ActionPointHit, apn);
    LOG(LOG_DEBUG4) << "[APS] Raising exception with ECR " << HEX(ecr_value);

    enter_exception (ecr_value, state.pc, state.pc);
    TRACE_INST(this, trace_exception());
  }
}

// This function can be called prior to restarting the processor after a
// breakpoint or a halt. It clears the Status[H] bit and also clears the
// Debug[BH] bit.
//
void
Processor::restart_from_halted ()
{
  uint32 debug;

  // Clear the Status.H bit
  //
  state.H                  = false;
  sim_opts.halt_simulation = false;

  // Clear the Debug.BH and Debug.SH bits
  //
  read_aux_register (AUX_DEBUG, &debug, false);
  debug &= 0x9fffffff; // clear bits 30 and 29
  write_aux_register (AUX_DEBUG, debug, false);
}

void
Processor::halt_cpu (bool set_self_halt)
{  
  state.H                  = true;
  sim_opts.halt_simulation = true;
  
  // SH (i.e. self halt bit) indicates that the ARCompact based processor has
  // halted itself with the FLAG instruction.
  //
  if (set_self_halt) {
    uint32 debug;

    read_aux_register (AUX_DEBUG, &debug, false);
    debug |= 0x40000000; // set the DEBUG.SH bit
    write_aux_register(AUX_DEBUG, debug, false);
  }
}

// The flag instruction is executed quite frequently within the Linux kernel,
// so we need to keep the implementation as lean as possible.
//
void
Processor::flag_inst (uint32 opd, uint32 kflag)
{
  if (!state.U) {
    if ((opd & 1) == 0) {
      if (   sys_arch.isa_opts.is_isa_a6k()
          && sys_arch.isa_opts.div_rem_option)
      {
        state.DZ = BITSEL(opd,13);
      }
      if(sys_arch.isa_opts.stack_checking)
        state.SC = BITSEL(opd,14);
      
      LOG(LOG_DEBUG) << "flag inst";
      if (kflag != 0) {
        LOG(LOG_DEBUG) << "kflag != 0";
        if(sys_arch.isa_opts.new_interrupts)
        {
          state.IE = BITSEL(opd, 31);
        }
        else
        {
          state.AE = BITSEL(opd,5);
          state.A2 = BITSEL(opd,4);
          state.A1 = BITSEL(opd,3);
        }
      }
      if(sys_arch.isa_opts.new_interrupts)
      {
        state.E = (opd & 0x1e) >> 1;
      }
      else
      {
        state.E2 = BITSEL(opd,2);
        state.E1 = BITSEL(opd,1);
      }
      
      state.auxs[AUX_DEBUG] &= 0x9fffffff;
    } else {
      halt_cpu();
      return;
    }
  }
  
  // As the interrupts may now be enabled, we should tell
  // the simulation loop to re-check interrupts.
  //
  set_pending_action(kPendingAction_CPU);

  state.Z  = BITSEL(opd,11);
  state.N  = BITSEL(opd,10);
  state.C  = BITSEL(opd,9);
  state.V  = BITSEL(opd,8);
}

void
Processor::break_inst ()
{
  uint32 aux_debug;
  
  state.H                           = sim_opts.exit_on_break;
  sim_opts.halt_simulation          = sim_opts.exit_on_break;
  
  read_aux_register (AUX_DEBUG, &aux_debug, false);
  aux_debug |= 0x20000000; // set the DEBUG.SH bit
  write_aux_register (AUX_DEBUG, aux_debug, false);
  
  LOG(LOG_DEBUG4) << "[BRK] at " << HEX(state.pc)
                  << ", state.H = "         << (int)(state.H)
                  << ", halt_simulation = " << (int)(sim_opts.halt_simulation);
}

void
Processor::sleep_inst (uint32 int_enables)
{
  uint32 aux_debug;

  // Set ZZ flag, but not the H flag
  // Set E1 and E2 according to the low-order 2 bits of operand
  //
  // May safely assume simSleep is only called in Kernel mode
  //
#ifdef VERIFICATION_OPTIONS
  if (!sys_arch.isa_opts.ignore_brk_sleep) {
#endif
    sim_opts.halt_simulation = sim_opts.exit_on_sleep;
    read_aux_register (AUX_DEBUG, &aux_debug, false);
    aux_debug |= (1 << 23); // set the DEBUG.ZZ bit
    write_aux_register (AUX_DEBUG, aux_debug, false);
#ifdef VERIFICATION_OPTIONS
  }
#endif    
  
  if(sys_arch.isa_opts.new_interrupts)
  {
    if((int_enables & 0x10)) 
    {
      state.E = int_enables & 0xF;
      state.IE = 1;
    }
  }
  else 
  {
    state.E1 = (int_enables & 3UL) ? (int_enables & 1UL) : state.E1;
    state.E2 = (int_enables & 3UL) ? (int_enables & 2UL) >> 1 : state.E2;
  }
  
  // As the interrupts may now be enabled, we should tell
  // the simulation loop to re-check interrupts.
  //
  set_pending_action(kPendingAction_CPU);
}


// -----------------------------------------------------------------------------
// This method checks the value in state.pending_actions, and performs the
// actions required for each case. 
// It returns true if the the simulation must be suspended for external
// interactions, and in this case it records and updates the simulation time
// resources.
//
bool
Processor::handle_pending_actions ()
{
  bool requires_action = false;
  
  // ---------------------------------------------------------------------------
  // Retrieve all pending actions
  //
  uint32 pending_actions = get_pending_actions();
  
  // ---------------------------------------------------------------------------
  // 1. Take care of any processor triggered interrupts (e.g. timer interrupt)
  //
  if (pending_actions & kPendingAction_CPU) {
    if (!state.AE) // If there is NO double fault we detect_interrupts
      detect_interrupts();
    clear_pending_action(kPendingAction_CPU);
  }

  // ---------------------------------------------------------------------------
  // 2. Take care of watchpoing interrupts
  //
  if (pending_actions & kPendingAction_WATCHPOINT) {
    trigger_watchpoint();
    clear_pending_action(kPendingAction_WATCHPOINT);
  }
  
  // ---------------------------------------------------------------------------
  // 3. Remove specific translations
  //
  if (pending_actions & kPendingAction_FLUSH_TRANSLATION) {
    purge_dcode_cache ();
    purge_translation_cache();    
    // If any translations have been queued for removal we need to remove them
    while (!invalid_translations_stack.empty()) {
      phys_profile_.remove_translation(invalid_translations_stack.top());
      invalid_translations_stack.pop();
    }
    clear_pending_action(kPendingAction_FLUSH_TRANSLATION);
  }

  // ---------------------------------------------------------------------------
  // 4. Remove all translations
  //
  if (pending_actions & kPendingAction_FLUSH_ALL_TRANSLATIONS) {
    purge_dcode_cache ();
    phys_profile_.remove_translations();
    purge_translation_cache();    
    clear_pending_action(kPendingAction_FLUSH_ALL_TRANSLATIONS);
  }

  // ---------------------------------------------------------------------------
  // 5. Does an external agent want control?
  //
  if (pending_actions & kPendingAction_EXTERNAL_AGENT) {
    // Report that kPendingAction_EXTERNAL_AGENT interrupt requires further action
    requires_action          = true;
    clear_pending_action(kPendingAction_EXTERNAL_AGENT);
  }
  
  // ---------------------------------------------------------------------------
  // 6. Take care of Instrumentation PoinTs (IPTs)
  //
  if (pending_actions & kPendingAction_IPT) {
    // When we hit an IPT in tracing/interpretive mode, we flush translations and
    // traces that contain the particular instruction address
    //
    if (sim_opts.fast && (cur_exec_mode != kExecModeNative)) {
      if (phys_profile_.is_translation_present(state.pc)) {
        LOG(LOG_DEBUG3) << "[IPTManager] Avoid double IPT activation - flush TRANSLATION including @ 0x" << HEX(state.pc);
        phys_profile_.remove_translation(state.pc);
        purge_translation_cache();
      }
      if (phys_profile_.is_trace_present(state.pc)) {
        LOG(LOG_DEBUG3) << "[IPTManager] Avoid double IPT activation - flush TRACE including @ 0x" << HEX(state.pc);
        phys_profile_.remove_trace(state.pc);        
      }
    }
    
    // Report that kPendingAction_IPT interrupt requires further action
    requires_action          = true;
    clear_pending_action(kPendingAction_IPT);
  }

  return requires_action;
}


// -----------------------------------------------------------------------------
// Processor simulation/execution modes (i.e. JIT enabled simulation, single
// stepping, single stepping with tracing etc.)
//

// Interpretive simulation mode without instruction tracing --------------------
//
bool
Processor::run_notrace (uint32 iterations)
{
  bool stepOK = true;
  
  exec_time.start(); // record simulation start time

  state.iterations = iterations; // initialise iteration count

  if (inst_timer_enabled && !sim_opts.cycle_sim) { // INSTRUCTION TIMER MODE ---
    uint32 iter_remaining = state.iterations;
    uint64 prev_sync_time = timer_sync_time; // snapshot of current timer_sync_time
    
    do { // } while (stepOK && !sim_opts.halt_simulation && iter_remaining);
      // compute iterations and set 'state.timer_expiry'
      state.iterations = arcsim::util::min(time_to_expiry(), iter_remaining);
      
      do { // } while (stepOK && !sim_opts.halt_simulation && state.iterations);
        stepOK = step_single_fast(); // perform single step
                
        if (has_pending_actions() && handle_pending_actions())
          sim_opts.halt_simulation = 1; // flag that we need to stop when we get here

        --state.iterations; // decrement executed iteration count
      } while (stepOK && !sim_opts.halt_simulation && state.iterations);
      
      timer_sync(); // sync timers and possibly trigger timer interrupt
      
      // compute remaining iterations
      ASSERT(timer_sync_time >= prev_sync_time);
      const uint32 iter_elapsed = timer_sync_time - prev_sync_time;
      if (iter_remaining > iter_elapsed) { iter_remaining -= iter_elapsed; }
      else                               { iter_remaining  = 0;            }
      prev_sync_time = timer_sync_time; // snapshot of current timer_sync_time
      
      // check to see if advancement of timer resulted in an interrupt
      if (has_pending_actions() && handle_pending_actions())
        sim_opts.halt_simulation = 1; // flag that we need to stop when we get here

    } while (stepOK && !sim_opts.halt_simulation && iter_remaining);
    
  } else {                                        // CYCLE/HOST TIMER MODE -----
    do { // } while (stepOK && !sim_opts.halt_simulation && state.iterations);
      stepOK = step_single_fast(); // perform single step
      
      if (has_pending_actions() && handle_pending_actions()) {
        // FIXME: do we need to call timer_advance_cycles() here?
        sim_opts.halt_simulation = 1; // flag that we need to stop when we get here
      }
      --state.iterations; // decrement executed iteration count
    } while (stepOK && !sim_opts.halt_simulation && state.iterations);    
  }
  
  exec_time.stop(); // record and update simulation end time
  return stepOK;
}

// Interpretive simulation mode with instruction tracing -----------------------
//
bool
Processor::run_trace (uint32 iterations, UpdatePacket* upkt)
{
  bool          stepOK = true;
  UpdatePacket  dummy;
  
  if (upkt == NULL) // no UpdatePacket specified, use dummy
    upkt = &dummy;

  state.iterations = iterations; // initialise iteration count
  exec_time.start(); // record simulation start time

  if (inst_timer_enabled && !sim_opts.cycle_sim) { // INSTRUCTION TIMER MODE ---
    uint32 iter_remaining = state.iterations;
    uint64 prev_sync_time = timer_sync_time; // snapshot of current timer_sync_time
    
    do { // } while (stepOK && !sim_opts.halt_simulation && iter_remaining);
      // compute iterations and set 'state.timer_expiry'
      state.iterations = arcsim::util::min(time_to_expiry(), iter_remaining);
      
      do { // } while (stepOK && !sim_opts.halt_simulation && state.iterations);
        stepOK = step_single(upkt, false); // perform single step
        TRACE_INST(this, trace_emit()); // trace instruction
        
        if (has_pending_actions() && handle_pending_actions())
          sim_opts.halt_simulation = 1; // flag that we need to stop when we get here

        --state.iterations; // decrement executed iteration count
      } while (stepOK && !sim_opts.halt_simulation && state.iterations);
      
      timer_sync(); // sync timers and possibly trigger timer interrupt
      
      // compute remaining iterations
      ASSERT(timer_sync_time >= prev_sync_time);
      const uint32 iter_elapsed = timer_sync_time - prev_sync_time;
      if (iter_remaining > iter_elapsed) { iter_remaining -= iter_elapsed; }
      else                               { iter_remaining  = 0;            }
      prev_sync_time = timer_sync_time; // snapshot of current timer_sync_time
      
      // check to see if advancement of timer resulted in an interrupt
      if (has_pending_actions() && handle_pending_actions())
        sim_opts.halt_simulation = 1; // flag that we need to stop when we get here
      
    } while (stepOK && !sim_opts.halt_simulation && iter_remaining);
      
  } else {                                        // CYCLE/HOST TIMER MODE -----
    
    do { // } while (stepOK && !sim_opts.halt_simulation && state.iterations);
      stepOK = step_single(upkt, false); // perform single step
      TRACE_INST(this, trace_emit());
      
      if (has_pending_actions() && handle_pending_actions()) {
        // FIXME: do we need to call timer_advance_cycles() here?
        sim_opts.halt_simulation = 1; // flag that we need to stop when we get here
      }
      --state.iterations; // decrement executed iteration count
    } while(stepOK && !sim_opts.halt_simulation && state.iterations);
    
  }
  
  exec_time.stop(); // record and update simulation end time
  return stepOK;
}

// JIT compiled simulation mode ------------------------------------------------
//
// For correct termination on a Halt condition, any basic block that sets state.H
// must not be translated.
//
bool
Processor::run (uint32 iterations)
{
  ASSERT(sim_opts.cosim == false);

  bool              stepOK       = true;
  TranslationBlock  native_block = 0;
  
  state.iterations = iterations; // initialise iteration count
  exec_time.start(); // record simulation start time

  do { // } while (stepOK && !sim_opts.halt_simulation && state.iterations);
    // -------------------------------------------------------------------------
    // 1. Execute native translation if lookup in translation cache is successful,
    //    otherwise we need to record traced basic block.
    //
    if (trans_cache.lookup(state.pc, &native_block)) {// NATIVE EXEC - CACHE HIT
      
      cur_exec_mode = kExecModeNative;
      (*native_block)(&state);      // execute translated block
      cur_exec_mode = kExecModeInterpretive;
      
      state.gprs[PCL_REG] = state.pc & 0xfffffffc;
      phys_profile_.reset_all_active_trace_sequences();
      
    } else {                                          // TRACE EXEC - CACHE MISS
      // If the next block starts with a delay-slot instruction DO NOT treat it 
      // as a translatable block. Hence, we call step_single_fast() to interpret
      // the delay-slot instruction. This should only occur when re-issuing a 
      // delay-slot instruction which previously generated an exception.
      //
      if (state.D || state.ES /* a6k */) { // STEP ONE DELAY SLOT INSTRUCTION --
        if (sim_opts.trace_on) {                    // TRACE MODE ON
          stepOK = step_single(0, false);
          TRACE_INST(this, trace_emit());
        } else {                                    // TRACE MODE OFF 
          stepOK = step_single_fast();
        }
        phys_profile_.reset_active_trace_sequence(interrupt_stack.top());
      } else {                             // TRACE ONE BASIC BLOCK ------------
        stepOK = step_block();
      }
    }
    // -------------------------------------------------------------------------
    // 2. After execution/tracing we need to check for any pending actions or
    //    timer expiry.
    //
    if (inst_timer_enabled && !sim_opts.cycle_sim) {
      if (instructions() >= state.timer_expiry)
        timer_sync(); // sync timers and possibly trigger timer interrupt
    }
    if (has_pending_actions() && handle_pending_actions())
      sim_opts.halt_simulation = 1; // flag that we need to stop when we get here

  } while (stepOK && !sim_opts.halt_simulation && state.iterations);
  
  exec_time.stop(); // record and update simulation end time
  return stepOK;
}

// This method is called if a translation cache miss occurs. If a translation
// cache miss occurs we try to find the block in our PhysicalProfile. If a
// translation already exists for that block we execute the translation,
// otherwise we update the profiling information for this block so that it might
// become eligible for JIT translation.
//
bool
Processor::step_block ()
{
  bool    stepOK    = true;
  uint32  phys_addr = 0;  
  uint32  ecause    = 0;

  // To find the required translation, if it exists, we must look for its physical
  // address in the PhysicalProfile of all known pages. This requires that we 
  // first translate from virtual to physical instruction address. The initial
  // lookup uses a side-effect free MMU lookup method, only if this fails do we
  // use an MMU translation method that causes necessary side-effects:
  //  - Raising of ITLB exception and transfer of control flow
  //
  if ( (ecause = mmu.lookup_exec(state.pc, state.U, phys_addr)) ) {

    ASSERT(   (ecause == ECR(sys_arch.isa_opts.EV_ITLBMiss, IfetchTLBMiss, 0))
           || (ecause == ECR(sys_arch.isa_opts.EV_ProtV, IfetchProtV, 0)));

    // translate physical to virtual addr causing control flow to change as a side-effect
    ecause = mmu.translate_exec(state.pc, state.U, phys_addr);
    
    ASSERT(   (ecause == ECR(sys_arch.isa_opts.EV_ITLBMiss, IfetchTLBMiss, 0))
           || (ecause == ECR(sys_arch.isa_opts.EV_ProtV, IfetchProtV, 0)));
    
    enter_exception(ecause, state.pc, state.pc);
    ASSERT(state.raise_exception != 0);
    TRACE_INST(this, trace_exception());
    state.raise_exception = 0;
    return stepOK;
  }
  
  // PHYSICAL to VIRTUAL ADDRESS TRANSLATION SUCCESS ---------------------------
  
  // Find BlockEntry for given physical address (creates BlockEntry if it does not exist)
  arcsim::profile::BlockEntry* block = phys_profile_.get_block_entry(phys_addr,  // physical address
                                                                     state.pc,   // virtual  address
                                                                     MAP_OPERATING_MODE(state.U),
                                                                     zone_);
  
  if (block->is_translated()) {
    // NATIVE EXEC - BLOCK HAS TRANSLATION -------------------------------------
    
    TranslationBlock native_block = block->get_translation();          
    trans_cache.update(state.pc, native_block); // put native block in translation cache
    
    cur_exec_mode = kExecModeNative;
    (*native_block)(&state);      // execute translated block
    cur_exec_mode = kExecModeInterpretive;    
    
    state.gprs[PCL_REG] = state.pc & 0xfffffffc;
    phys_profile_.reset_all_active_trace_sequences();
    return stepOK;
  }
    // TRACE EXEC - BLOCK NOT TRANSLATED ---------------------------------------

  end_of_block   = false;
  prev_had_dslot = false;
  
  // Single-step through instructions in current basic block. 
  if (sim_opts.trace_on) {                       // TRACE MODE ON
    do {
      stepOK = step_single(0, false);
      trace_emit();
      
      // exceptions (e.g. (I|D)TLB miss) change control flow, hence we 'break'
      if (state.raise_exception) 
        break;
      
      // FIXME: avoid creation of new BlockEntries for IPT pending actions
      if (has_pending_actions() && handle_pending_actions())
        sim_opts.halt_simulation = 1; // flag that we need to stop when we get here

    } while (!end_of_block && stepOK && !sim_opts.halt_simulation);
  } else {                                      // TRACE MODE OFF
    do {
      stepOK = step_single_fast();
      
      // exceptions (e.g. (I|D)TLB miss) change control flow, hence we 'break'
      if (state.raise_exception) 
        break;
      
      // FIXME: avoid creation of new BlockEntries for IPT pending actions
      if (has_pending_actions() && handle_pending_actions())
        sim_opts.halt_simulation = 1; // flag that we need to stop when we get here

    } while (!end_of_block && stepOK && !sim_opts.halt_simulation);
  }
  
  // Trace block if it is not translated or in translation
  //
  if (block->is_not_translated()) {
    ++block->interp_count; // Update block interpretation cound
    phys_profile_.trace_block(*block, interrupt_stack.top());      
  }

  --state.iterations;         // decrement iteration count after stepping through block
  ++trace_interp_block_count; // increment count of traced blocks in this interval

  // Are we ready to consider translating some blocks? -----------------------
  //
  if (trace_interp_block_count > sim_opts.trace_interval_size 
      && phys_profile_.has_touched_pages()) {
    dispatch_hot_traces();        // dispatch frequently executed traces
    trace_interp_block_count = 0; // reset count of interpreted instructions
    ++trace_interval;             // increment trace interval
  }
  return stepOK;
}

// ---------------------------------------------------------------------------
// Dispatch frequently executed traces to JIT compilation worker threads
//
int
Processor::dispatch_hot_traces()
{
  trace_time.start(); // start recording trace time
  
  // Create structure holding potential hotspot workload
  size_t wu_count = phys_profile_.touched_pages_count();
  std::valarray<TranslationWorkUnit*>  work_units(wu_count);
  
  // Adaptively modify the hotspot threshold.
  uint32 prev_hotspot_threshold = local_hotspot_threshold;
  local_hotspot_threshold       = phys_profile_.determine_hotspot_threshold(*this);
  
  if (prev_hotspot_threshold != local_hotspot_threshold) {
    LOG(LOG_DEBUG) << "[CPU" << core_id << "] Adapting hotspot threshold from '"
                   << prev_hotspot_threshold  << "' to '"
                   << local_hotspot_threshold << "'.";
  }
  
  // Create translation work unit based on frequency of exectued traces
  int work_size = phys_profile_.analyse_hotspots(*this,
                                                 work_units,
                                                 sim_opts.fast_trans_mode,
                                                 local_hotspot_threshold);
    
  if (work_size > 0 ) // Dispatch all hot translation work units for compilation
    system.trans_mgr.dispatch_translation_work_units(work_size, work_units);
  
  if (sim_opts.debug)
    timing_restart();
    
  // Remove traces collected during this interval
  phys_profile_.remove_traces();
  // Reset all active trace sequences, we start from scratch for next interval
  phys_profile_.reset_all_active_trace_sequences();
  
  trace_time.stop(); // stop recording trace time
  
  return work_size;
}

      
// -----------------------------------------------------------------------------
// Simple debugging support
// FIXME: This code needs to be refactored!
//
bool
Processor::set_breakpoint (uint32 brk_location, bool& brk_s, uint32& old_instruction)
{
  uint32                  fetch_packet = 0;
  arcsim::isa::arc::Dcode instr;

  // Fetch and decode the instruction at the break address
  // NOTE: the break address MUST BE a PHYSICAL address.
  //
  LOG(LOG_DEBUG) << "Setting breakpoint at: 0x" << HEX(brk_location);

  bool fetchOK = read16 (brk_location, fetch_packet);
  instr.info.ir = fetch_packet << 16;
  fetchOK = read16 (brk_location+2, fetch_packet);
  instr.info.ir |= (fetch_packet & 0x0ffff);
  instr.decode(sys_arch.isa_opts, (uint32)instr.info.ir, brk_location, state, eia_mgr, false);

  // Replace it with either a brk or brk_s
  //
  brk_s = ((instr.size == 2) || (instr.size == 6));
  if (brk_s) {
    old_instruction = instr.info.ir >> 16;
    // FIXME: Magic number
    //
    write16 (brk_location, 0x7fff);
    LOG(LOG_DEBUG) << "brk_s replaces: 0x" << HEX(old_instruction);
  } else {
    // FIXME: Magic number
    //
    old_instruction = instr.info.ir;
    write16 (brk_location,   0x256f);
    write16 (brk_location+2, 0x003f);
    LOG(LOG_DEBUG) << "brk replaces: 0x" << HEX(old_instruction);
  }

  // Shootdown cached version(s) of this instruction in the dcode cache
  // N.B. the dcode cache is indexed by virtual address, so we need
  // must flush the entire dcode cache.
  //
  purge_dcode_cache();


  return fetchOK;
}

// FIXME: clearing of breakpoints must be re-factored with respect to native
//        translations
//
bool
Processor::clear_breakpoint (uint32 brk_location, uint32 new_instruction, bool brk_s)
{
  // Replace the old instruction at the break location
  //
  
  // This cannot use the traditional write as it can generate an exception violation!
  // star_9000532322, triggers privilege violation on write ultimately causing a machine check.


  if (brk_s) {
    //write16 (brk_location, new_instruction);
    write_no_exception (brk_location, new_instruction, kByteTransferSizeHalf);
    LOG(LOG_DEBUG) << "brk_s replaced by: 0x" << HEX(new_instruction);
  } else {
    //write16 (brk_location, new_instruction >> 16);
    //write16 (brk_location+2, new_instruction & 0xffff);
    write_no_exception (brk_location, new_instruction >> 16, kByteTransferSizeHalf);
    write_no_exception (brk_location+2, new_instruction & 0xffff, kByteTransferSizeHalf);
    LOG(LOG_DEBUG) << "brk replaced by: 0x" << HEX(new_instruction);
  }

  // Shootdown any copy of the brk instruction in the Dcode cache
  //
  purge_dcode_entry (brk_location);

  return false;
}

// -----------------------------------------------------------------------------
// Trap emulation
//
void
Processor::emulate_trap ()
{
  system.emulate_syscall (*this,
                          state.gprs[8], state.gprs[0], state.gprs[0],
                          state.gprs[1], state.gprs[2], state.gprs[3],
                          state.gprs[4]);

  /* At this point gpr[0-4] and gpr[8] in an RTL co-simulation model
   * will be stale.
   * The co-simulation environment must be somehow informed that it must
   * copy through the changes to the simulator gprs into the RTL gprs.
   */
}

// -----------------------------------------------------------------------------
// Single Precision floating-point emulation function
//
void
Processor::spfp_emulation(uint32 op, uint32 *dst, uint32 src1, uint32 src2, bool flag_enb)
{
  namespace Op = arcsim::isa::arc; // shorten access
  SpFpRegister x, y, z;

  x.u = src1;
  y.u = src2;

  if (std::isnan(x.f)) {
    *dst = z.u = x.u;
  } else if (std::isnan(y.f)) {
    *dst = z.u = y.u;
  } else {
    switch (op) {
      case Op::OpCode::FMUL: z.f  = x.f * y.f; break;
      case Op::OpCode::FADD: z.f  = x.f + y.f; break;
      case Op::OpCode::FSUB: z.f  = x.f - y.f; break;
      default: UNREACHABLE();
    }
    
    if(std::isnan(z.f)) {
      // On x86 the sign bit is meaningless for NaN. Hence we have to compute it
      // properly.
      // @see: http://en.wikipedia.org/wiki/Extended_precision
      // @see: TechPubs/UserDocs/Output/5161_FPX_Bookshelf/trunk/pdf/FPX_Reference.pdf
      // If a NaN has to be returned, then the value 0x7fc00000 or 0xffc00000 is
      // given to the result, depending on the sign of the result.
      //
      const uint32 res_sign = (x.u ^ y.u) & 0x80000000U;
      z.u = 0x7fc00000U | res_sign;
    }

    *dst = z.u;
  }

  if (flag_enb) {
    state.Z = ((z.u & 0x7fffffff) == 0);
    state.N = (z.u & 0x80000000UL) >> 31;
    state.C = (std::isnan(z.f) != 0);
    state.V = (std::isinf(z.f) != 0);
  }
}

// -----------------------------------------------------------------------------
// Double Precision floating-point emulation function
//
void
Processor::dpfp_emulation (uint32 op, uint32 *dst, uint32 src1, uint32 src2, bool flag_enb)
{
  namespace Op = arcsim::isa::arc; // shorten access
  DpFpRegister x, y, z;
  uint32 *reg_low   = 0;
  uint32 *reg_high  = 0;
  uint32 *dest_low  = 0;
  uint32 *dest_high = 0;

  switch (op) { // choose correct auxiliary registers
    case Op::OpCode::DMULH11: case Op::OpCode::DADDH11:
    case Op::OpCode::DSUBH11: case Op::OpCode::DRSUBH11: {
      reg_low   = &(state.auxs[AUX_DPFP1L]);
      reg_high  = &(state.auxs[AUX_DPFP1H]);
      dest_low  = &(state.auxs[AUX_DPFP1L]);
      dest_high = &(state.auxs[AUX_DPFP1H]);
      break;
    }
    case Op::OpCode::DMULH12: case Op::OpCode::DADDH12:
    case Op::OpCode::DSUBH12: case Op::OpCode::DRSUBH12: {
      reg_low   = &(state.auxs[AUX_DPFP2L]);
      reg_high  = &(state.auxs[AUX_DPFP2H]);
      dest_low  = &(state.auxs[AUX_DPFP1L]);
      dest_high = &(state.auxs[AUX_DPFP1H]);
      break;
    }
    case Op::OpCode::DMULH21: case Op::OpCode::DADDH21:
    case Op::OpCode::DSUBH21: case Op::OpCode::DRSUBH21: {
      reg_low   = &(state.auxs[AUX_DPFP1L]);
      reg_high  = &(state.auxs[AUX_DPFP1H]);
      dest_low  = &(state.auxs[AUX_DPFP2L]);
      dest_high = &(state.auxs[AUX_DPFP2H]);
      break;
    }
    case Op::OpCode::DMULH22: case Op::OpCode::DADDH22:
    case Op::OpCode::DSUBH22: case Op::OpCode::DRSUBH22: {
      reg_low   = &(state.auxs[AUX_DPFP2L]);
      reg_high  = &(state.auxs[AUX_DPFP2H]);
      dest_low  = &(state.auxs[AUX_DPFP2L]);
      dest_high = &(state.auxs[AUX_DPFP2H]);
      break;
    }
    default: UNREACHABLE();
  }

  x.v[1] = *reg_high;
  x.v[0] = *reg_low;

  y.v[1] = src1;
  y.v[0] = src2;

  if (std::isnan(x.d)) {
    z.d = x.d;
    *dest_high = *dst = x.v[1];
    *dest_low  = x.v[0];
  } else if (std::isnan(y.d)) {
    z.d = y.d;
    *dest_high = *dst = y.v[1];
    *dest_low = y.v[0];
  } else {
    switch (op) {
      case Op::OpCode::DMULH11: case Op::OpCode::DMULH12: 
      case Op::OpCode::DMULH21: case Op::OpCode::DMULH22: {
        z.d  = x.d * y.d;
        break;
      }
      case Op::OpCode::DADDH11: case Op::OpCode::DADDH12:
      case Op::OpCode::DADDH21: case Op::OpCode::DADDH22: {
        z.d  = x.d + y.d;
        break;
      }
      case Op::OpCode::DSUBH11: case Op::OpCode::DSUBH12: 
      case Op::OpCode::DSUBH21: case Op::OpCode::DSUBH22: {
        z.d  = x.d - y.d;
        break;
      }
      case Op::OpCode::DRSUBH11: case Op::OpCode::DRSUBH12:
      case Op::OpCode::DRSUBH21: case Op::OpCode::DRSUBH22: {
        z.d  = y.d - x.d;
        break;
      }
      default: UNREACHABLE();
    }

    if (std::isnan(z.d) ) {
      // On x86 the sign bit is meaningless for NaN. Hence we have to compute it
      // properly.
      // @see: http://en.wikipedia.org/wiki/Extended_precision
      // @see: TechPubs/UserDocs/Output/5161_FPX_Bookshelf/trunk/pdf/FPX_Reference.pdf
      // If a NaN has to be returned, then the value 0x7ff0000080000000 or 
      // 0xfff0000080000000 is given to the result, depending on the sign of the result.
      // 
      const uint32 res_sign = (x.v[1] ^ y.v[1]) & 0x80000000U;
      z.v[0] = 0x80000000U;
      z.v[1] = 0x7FF00000U | res_sign;
    }
    *dest_high = *dst = z.v[1];
    *dest_low = z.v[0];
  }

  if (flag_enb) {
    state.Z = (((z.v[1] & 0x7fffffff) | z.v[0]) == 0);
    state.N = (z.v[1] & 0x80000000U) >> 31;
    state.C = (std::isnan(z.d) != 0);
    state.V = (std::isinf(z.d) != 0);
  }
}

// -----------------------------------------------------------------------------
// Double precision floating point register exchange
//
void
Processor::dexcl_emulation(uint32 op, uint32 *dst, uint32 src1, uint32 src2)
{
  namespace Op = arcsim::isa::arc; // shorten access
  
  switch (op) {
    case Op::OpCode::DEXCL2: {
      *dst = state.auxs[AUX_DPFP2L]; // dest = (D2).LOW
      state.auxs[AUX_DPFP2H] = src1;
      state.auxs[AUX_DPFP2L] = src2;
      break;
    }
    case Op::OpCode::DEXCL1: {
      *dst = state.auxs[AUX_DPFP1L]; // dest = (D1).LOW
      state.auxs[AUX_DPFP1H] = src1;
      state.auxs[AUX_DPFP1L] = src2;
      break;
    }
    default: UNREACHABLE();
  }  
}

// -----------------------------------------------------------------------------
// Tracing related code
//

void
Processor::trace_string (const char* str)
{
  IS.write(str, strlen(str));
}

// Emit instruction assembled in IS stream
//
void
Processor::trace_emit()
{
#ifdef DEBUG_INST_TRACE_EXEC_MODE
  // indicate if instruction was executed in native or interpretive mode
  static const char kInterpretiveExecToken[] = "[I]";
  static const char kNativeExecToken[]       = "[N]";
  
  if (cur_exec_mode == kExecModeNative)
    TS.write(kNativeExecToken,       sizeof(kNativeExecToken)-1);
  else
    TS.write(kInterpretiveExecToken, sizeof(kInterpretiveExecToken)-1);
#endif
  
#ifdef CYCLE_ACC_SIM
  // print commit time of instruction first for cycle accurate mode
  if (sim_opts.cycle_sim) {
    std::stringstream S;
    S << std::left << std::setw(11) << cnt_ctx.cycle_count.get_value();
    TS.write(S);
  }
#endif
  
  // Inject IS (InstructionTrace) into TS (TraceStream)
  //
  TS.write(IS)
    .flush();
  
  // Efficiently flush IS stream so it can be re-used for the next instruction
  //
  IS.clear();
  IS.str(std::string());
}

// Disassemble a given instruction, and its long-immediate
// (limm) operand, if it has one, to the Processor trace
// buffer.
//
void
Processor::trace_instruction (uint32 trace_pc, uint32 instr, uint32 limm)
{
  // Disassemble instruction
  arcsim::isa::arc::Disasm dis (sys_arch.isa_opts, eia_mgr, instr, limm);
  
  static const uint32 kSymDisplayLength = 14;
  
  std::string fun_name;
  bool has_sym = system.get_symbol(state.pc, fun_name);
  IS << std::setw(kSymDisplayLength) << std::setfill(' ') << std::left
     << ((has_sym) ? fun_name.substr(0, kSymDisplayLength) : " ") // function name
     << std::right;
  
  // Output current PC, instruction in binary form, and long immediate if present
  IS << "[" << std::hex << std::setw(8) << std::setfill('0') << std::right
     << trace_pc                                                  // PC
     << "] ";
  if (dis.is_16bit) {
    IS << std::setw(4) << std::setfill('0') << std::right
       << (instr >> 16)                                           // 16-bit instr
       << std::setw(5) << std::setfill(' ') << ' ';
  } else {
    IS << std::setw(8) << std::setfill('0') << std::right
       << instr                                                   // 32-bit instr
       << std::setw(1) << std::setfill(' ') << ' ';
  }
  IS << std::setw(9)
     << ( (dis.has_limm) ? dis.limm_str : " " );                 // limm
  
  
  // Output flags
  //
  
#define SHOW_SC   1 /* 1 => show SC flag in instruction trace */
#if SHOW_SC
     if(sys_arch.isa_opts.stack_checking)
       IS<< ( (state.SC!=0)? " SC " : "    ");
#endif

  
  IS << (state.L ? 'L' : ' ')
     << (state.U ? 'U' : 'K')
     << (state.ES? 'E' : ' ')
     << (state.D ? 'D' : ' ')
     << (state.Z ? 'Z' : ' ')
     << (state.N ? 'N' : ' ')
     << (state.C ? 'C' : ' ')
     << (state.V ? 'V' : ' ')
#define SHOW_E1E2   0 /* 1 => show E2/E1 flags in instruction trace */
#if SHOW_E1E2
     << ( (state.E2)  ? (state.E1 ? "Eb" : "E2") : (state.E1 ? "E1" : "  ") )
#endif
#undef SHOW_E1E2
     << ( (state.auxs[AUX_MACMODE]&0x200) ? 's' : ' ')
     << ( (state.auxs[AUX_MACMODE]&0x010) ? 'S' : ' ');

  // Add disassembly of instruction to IS
  //
  IS.write(dis.buf, dis.len);
}

// Generate trace output to show the values written by an instruction
//
void
Processor::trace_write_back (int wa0, bool wenb0, uint32 *wdata0,
                             int wa1, bool wenb1, uint32 *wdata1)
{
  if (wenb0)
    IS << " : (w0) r" << std::dec << wa0 << " <= 0x" << HEX(*wdata0);
  if (wenb1)
    IS << " : (w1) r" << std::dec << wa1 << " <= 0x" << HEX(*wdata1);
}

void
Processor::trace_commit (bool is_commit)
{
  static const char kCommitToken[]   = " *\n";
  static const char kNoCommitToken[] = "\n";  

  if (is_commit)
    IS.write(kCommitToken,   sizeof(kCommitToken));
  else
    IS.write(kNoCommitToken, sizeof(kNoCommitToken));
}
      
void
Processor::trace_exception (const char* message)
{
#ifdef VERIFICATION_OPTIONS
  // for verification we expect the exception message on the same line as the instruction
  static const char kExceptionMessage[] = " EXCEPTION RAISED: ";
#else
  static const char kExceptionMessage[] = "\n EXCEPTION RAISED: ";
#endif
  
  uint32 aux_status32  = BUILD_STATUS32(state);

  IS << kExceptionMessage
     << ( (message != 0) ? message : " " )
     << "\n\tECR      <= 0x"  << HEX(state.auxs[AUX_ECR])
     << "\n\tERET     <= 0x"  << HEX(state.auxs[AUX_ERET])
     << "\n\tERSTATUS <= 0x"  << HEX(state.auxs[AUX_ERSTATUS])
     << "\n\tBTA      <= 0x"  << HEX(state.auxs[AUX_BTA])
     << "\n\tEFA      <= 0x"  << HEX(state.auxs[AUX_EFA])
     << "\n\tPC       <= 0x"  << HEX(state.pc)
     << "\n\tSTATUS32 <= 0x"  << HEX(aux_status32);
#if SHOW_SC
  if(sys_arch.isa_opts.stack_checking){
    IS << "\n\tKStackBase= 0x"  << HEX(((state.U)?state.shadow_stack_base : state.stack_base))
       << "\n\tKStackTop = 0x"  << HEX(((state.U)?state.shadow_stack_top : state.stack_top))
       << "\n\tUStackBase= 0x"  << HEX(((state.U)?state.stack_base : state.shadow_stack_base))
       << "\n\tUStackTop = 0x"  << HEX(((state.U)?state.stack_top : state.shadow_stack_top))
       << "\n\tSP        = 0x"  << HEX((state.gprs[SP_REG]));
       if(sys_arch.isa_opts.new_interrupts)
         IS << "\n\tUserSP    = 0x" << HEX(state.auxs[AUX_USER_SP]);
  } 
#endif
  IS << std::endl;
}
#undef SHOW_SC
void
Processor::trace_mul64_inst()
{
  IS << ": (MLO) r"  << std::dec << MLO_REG
     << " <= 0x"     << HEX(state.gprs[MLO_REG])
     << ", (MMID) r" << std::dec << MMID_REG 
     << " <= 0x"      << HEX(state.gprs[MMID_REG])
     << ", (MHI) r"  << std::dec << MHI_REG 
     << " <= 0x"     << HEX(state.gprs[MHI_REG]);
}
      
void
Processor::trace_loop_back ()
{
  IS << ": LP -> 0x" << HEX(state.pc);
}

void
Processor::trace_loop_count ()
{
  IS << ": LP_COUNT <= " << state.gprs[LP_COUNT];
}

void
Processor::trace_loop_inst (int taken, uint32 target)
{
  if (taken) {
    IS << ": jumped over loop to " << HEX(target);
  } else {
    IS << ": LP_START <= " << HEX(state.auxs[AUX_LP_START])
       << ", LP_END <= "   << HEX(state.auxs[AUX_LP_END])
       << ", LP_COUNT = "  << std::dec << state.gprs[LP_COUNT];
  }
}
      
void
Processor::trace_load (int fmt, uint32 addr, uint32 data)
{
  char  buf[BUFSIZ];
  int   len;

  addr = addr & state.addr_mask;
  
  switch (fmt) {
    case T_FORMAT_LW:  len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint32)data); break;
    case T_FORMAT_LH:  len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint16)data); break;
    case T_FORMAT_LB:  len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint8)data);  break;
    case T_FORMAT_LWX: len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint32)data); break;
    case T_FORMAT_LHX: len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint16)data); break;
    case T_FORMAT_LBX: len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint8)data);  break;
    default: {
      UNIMPLEMENTED1("ERROR: Wrong trace format used for trace_load()");
      len = 0;
      break;
    }
  }
  IS.write(buf, len);
}

void
Processor::trace_store (int fmt, uint32 addr, uint32 data)
{
  char  buf[BUFSIZ];
  int   len;

  addr = addr & state.addr_mask;

  switch (fmt) {
    case T_FORMAT_SW:  len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint32)data); break;
    case T_FORMAT_SH:  len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint16)data); break;
    case T_FORMAT_SB:  len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint8)data);  break;
    case T_FORMAT_SWX: len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint32)data); break;
    case T_FORMAT_SHX: len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint16)data); break;
    case T_FORMAT_SBX: len = snprintf (buf, sizeof(buf), kTraceFormatStr[fmt], addr, (uint8)data);  break;
    default: {
      UNIMPLEMENTED1("ERROR: Wrong trace format used for trace_store()");
      len = 0;
      break; 
    }
  }
  IS.write(buf, len);
}

void
Processor::trace_lr (uint32 addr, uint32 data, int success)
{
  if (success) {
    IS << ": aux[0x" << std::hex << std::setw(2) << std::setfill('0') << addr
       << "] => "    << std::hex << std::setw(2) << std::setfill('0') << data;
  } else {
    IS << ": failed aux read, ";
  }
}

void
Processor::trace_sr (uint32 addr, uint32 data, int success)
{
  if (success) {
    IS << ": aux[0x" << std::hex << std::setw(2) << std::setfill('0') << addr
       << "] => "    << std::hex << std::setw(2) << std::setfill('0') << data;
  } else {
    IS << ": failed aux write, ";
  }
}

void
Processor::trace_rtie ()
{
  IS << ": PC <= 0x"       << HEX(state.next_pc)
     << ", STATUS32 <= 0x" << HEX(state.auxs[AUX_STATUS32])
     << ", BTA <= 0x"      << HEX(state.auxs[AUX_BTA]);
}

// -----------------------------------------------------------------------------
// Print processor statistics
//

void
Processor::print_stats ()
{ // Only output statistics if this is desired
  //
  if (!sim_opts.verbose) { return; }
    
  if (sim_opts.show_profile) {
    
    uint64 inst_total = cnt_ctx.opcode_freq_hist.get_total();
    
    fprintf (stderr, "Dynamic instruction frequencies, out of total %lld\n", inst_total);
    fprintf (stderr, "--------------------------------------------------\n");

    for (arcsim::util::SortedHistogramValueIter I(cnt_ctx.opcode_freq_hist);
        !I.is_end();
        ++I)
    {
      uint32 inst_opc      = (*I)->get_index();
      uint64 inst_freq_abs = (*I)->get_value();
      double inst_freq_rel = (double)inst_freq_abs/(double)inst_total * 100.0;
      
      fprintf (stderr, " %-10s\t%llu\t%6.2f\n",
               arcsim::isa::arc::OpCode::to_string(static_cast<arcsim::isa::arc::OpCode::Op>(inst_opc)),
               inst_freq_abs,
               inst_freq_rel);

    }            
    fprintf(stderr, "--------------------------------------------------\n");
    fprintf(stderr, " Delay slot instructions: %llu\n", cnt_ctx.dslot_inst_count.get_value());
    fprintf(stderr, " Flag use stall cycles:   %llu\n", cnt_ctx.flag_stall_count.get_value());
    fprintf(stderr, "--------------------------------------------------\n\n");
  }
  
  if (sim_opts.cycle_sim && sim_opts.is_opcode_latency_distrib_recording_enabled) {
    fprintf(stderr, "\n------------------------------------------------------------------\n");
    fprintf(stderr, "Cycles per inst: [inst: [C cycles total ] [C cycles : # frequency]]\n");
    fprintf(stderr, "------------------------------------------------------------------");
    
    // Iterate over all Histograms in MultiHistogram
    //
    for (arcsim::util::MultiHistogramIter HI(cnt_ctx.opcode_latency_multihist);
         !HI.is_end();
         ++HI)
    {
      uint64 total_cycles_per_instruction = 0;
      
      // Iterate over all HistogramEntries in Histogram to calculate total cycles
      //
      for (arcsim::util::HistogramIter I(*(*HI)); !I.is_end(); ++I)
      {
        total_cycles_per_instruction += ((*I)->get_index() * (*I)->get_value());
      }

      fprintf(stderr, "\n %-10s: %6llu [C]\n  ",
              arcsim::isa::arc::OpCode::to_string(static_cast<arcsim::isa::arc::OpCode::Op>((*HI)->get_id())),
              total_cycles_per_instruction);
      
      uint32 line_break = 1;

      // Iterate over all HistogramEntries in Histogram
      //
      for (arcsim::util::HistogramIter I(*(*HI)); !I.is_end(); ++I)
      {
        if (!(line_break++%4)) fprintf(stderr, "\n  ");
        fprintf(stderr, " [%3d [C] : %6d [#]]", (*I)->get_index(), (*I)->get_value());
      }
    }          
    fprintf(stderr, "\n------------------------------------------------------------------\n\n");
  }
  
#ifdef ENABLE_BPRED
  // Print Branch Predictor Stats (cyc acc)
  //

  if ((sim_opts.cycle_sim) && core_arch.bpu.is_configured) {
    bpu->print_stats();
  }
#endif /* ENABLE_BPRED */
  
  // Print Memory Statistics
  //
  if (sim_opts.memory_sim) {
    mem_model->print_stats();
  }
    
#ifdef REGTRACK_SIM
  // Print out Register Usage Summaries
  //
  if (sim_opts.track_regs) {
    fprintf (stderr, "\nRegister Usage Statistics\n");
    fprintf (stderr, "-------------------------------------------------------------------------------\n");
    fprintf (stderr, "Register     Reads         Writes     Total(R+W) Distance[arith] Distance[geom]\n");
    fprintf (stderr, "-------------------------------------------------------------------------------\n");
    uint64 totalread = 0, totalwrite = 0, totalrw;
    for ( int i=0; i<GPR_BASE_REGS; ++i ) {
      totalrw = state.gprs_stats[i].read+state.gprs_stats[i].write;
      totalread  += state.gprs_stats[i].read;
      totalwrite += state.gprs_stats[i].write;
      fprintf (stderr, " %2d   %12llu   %12llu   %12llu   %10.2f   %10.2f\n",
               i,                                    // register number
               state.gprs_stats[i].read,             // reads from register i
               state.gprs_stats[i].write,            // writes to register i
               totalrw,                              // total reads & writes
               state.gprs_stats[i].arith/totalrw,    // avg.distance[arithmetic mean]
               exp(state.gprs_stats[i].geom/totalrw) // avg.distance[geometric mean]
               );
    }
    fprintf (stderr, "-------------------------------------------------------------------------------\n");
    fprintf (stderr, " Total: %12llu[R]%12llu[W]%12llu[R+W]\n",
             totalread,
             totalwrite,
             totalread+totalwrite);
    fprintf (stderr, "-------------------------------------------------------------------------------\n");
  }
#endif
  
  // Print out Instruction Summaries
  //
  fprintf (stderr, "\nInstruction Execution Profile\n");
  fprintf (stderr, "-------------------------------------------------\n");
  fprintf (stderr, "                            Instructions   %%Total\n");
  fprintf (stderr, "-------------------------------------------------\n");
  fprintf (stderr, " Translated instructions:  %12lld%10.2f\n",
           cnt_ctx.native_inst_count.get_value(),
           100.0 * cnt_ctx.native_inst_count.get_value()/instructions());
  fprintf (stderr, " Interpreted instructions: %12lld%10.2f\n",
           cnt_ctx.interp_inst_count.get_value(),
           100.0 * cnt_ctx.interp_inst_count.get_value()/instructions());
  fprintf (stderr, " Total instructions:       %12lld%10.2f\n",
           instructions(),    100.0);
  fprintf (stderr, "-------------------------------------------------\n\n");
    
  // Print overall simulation times and simulated instructions
  //  
  double sim_total  = exec_time.get_elapsed_seconds();
  double trc_total  = trace_time.get_elapsed_seconds();
  
  fprintf (stderr, "Simulation Time Statistics [Seconds]\n");
  fprintf (stderr, "-----------------------------------------------------\n");
  fprintf (stderr, "        Simulation   Tracing    Total\n");
  fprintf (stderr, " Time:     %7.2f   %7.2f     %7.2f\n",
           sim_total - trc_total, trc_total, sim_total);
  fprintf (stderr, "-----------------------------------------------------\n\n");
  
  fprintf (stderr, "Interpreted instructions = %lld\n", cnt_ctx.interp_inst_count.get_value());
  fprintf (stderr, "Translated instructions  = %lld\n", cnt_ctx.native_inst_count.get_value());
  fprintf (stderr, "Total instructions       = %lld\n", instructions());
  
  fprintf (stderr, "\nSimulation time = %0.2f [Seconds]\n", sim_total);
  fprintf (stderr, "Simulation rate = %0.2f [MIPS]\n",
           (sim_total) ? (instructions()*1.0e-6/sim_total) : 0);
  
#ifdef CYCLE_ACC_SIM
  if (sim_opts.cycle_sim) {
    double cpi = cycle_sim_cpi();
    double ipc = cycle_sim_ipc();
    double mhz = (sim_total) ? (instructions()*1.0e-6*cpi/sim_total) : 0;
    fprintf (stderr, "Cycle count     = %lld [Cycles]\n", cnt_ctx.cycle_count.get_value());
    fprintf (stderr, "CPI             = %6.3f\n", cpi);    
    fprintf (stderr, "IPC             = %6.3f\n", ipc);    
    fprintf (stderr, "Effective clock = %0.1f [MHz]\n", mhz);
  }
#endif /* CYCLE_ACC_SIM */
  
  fprintf(stderr, "\n");
} /* END Processor::print_stats  */

} } } //  arcsim::sys::cpu


