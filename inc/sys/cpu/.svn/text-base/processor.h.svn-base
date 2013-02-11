//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2005 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// NOTE: This class is on a diet! You are not allowed to add fields/methods
//       to it without consulting with at least one other person!
//
//  The most intricate aspects of the modeling of a processor and its memory are
//  the way in which pages of physical memory are managed, and the way blocks of
//  translated instructions are kept consistent as the underlying contents of
//  memory change. To support this the processor has a number of data structures.
//
//  Page Translation Structures
//  ---------------------------
//
//  The MMU keeps a set of JTLBs which represent the page-level translations known
//  to the hardware at any instant. In addition, unmapped regions of memory always
//  map using an identity mapping. When required, the processor can interrogate
//  the MMU to obtain a virtual -> physical translation by calling mmu_translate_XXX
//  where XXX represents the type of access, and is one of {read, write, exec, rmw}.
//  It is also possible to call mmu_get_translation (), which will compute the
//  translation if it exists, but will not raise an exception if it does not exist.
//
//  The processor takes advantage of the fact that many references will take place
//  within a small group of pages known as the 'working set'. It maintains three
//  separate hashes on the virtual->physical mapping, one for each basic level of
//  privilege (read, write and execute). If an entry exists in the read-hash table
//  mapping from V->P, then P is the translation of V and it is read-accessible.
//  If an address is read, but is not found in the read-hash table, then the
//  address is looked up using translate_read(). This returns 'true' if the
//  translation exists and does not raise an exception, in which case it also
//  returns the mapped-to physical address. This can be placed in the read-hash
//  table for future accesses. If an exception occurs (TLBmiss, etc) then the
//  translation function returns 'false' and any load/store instruction which
//  generated this read operation must be prevented from committing its result.
//
//  Whenever a JTLB entry is modified, some entries in the three hash tables
//  become potentially stale. It is therefore necessary to maintain consistency
//  between the JTLB and these three page translation hash tables.
//
//  NOTE: When calling MMU related methods for any other reason than as a side-effect
//        of actual simulation, be very careful to call MMU methods that do NOT 
//        have side-effects on processor state - YOU HAVE BEEN WARNED!
//
//  JIT Translation Structures
//  --------------------------
//
//  JIT translation is based on the notion that a frequently executed basic
//  block of code will be executed again many times in the future. For this
//  reason, such blocks of code become candidates for dynamic translation
//  into native instructions. At the start of each basic block the
//  simulator must check whether a translation exists for the basic block
//  starting at the current program counter address. If it exists, it will
//  exist in the form of a function called 'L_0xNNNNNNNN' where NNNNNNNN is
//  the hex physical address of the basic block as stored in target memory.
//  The simulator keeps track of all translated blocks in existence at any
//  instant. It also profiles the execution of blocks that are not yet
//  translated, so that it can identify blocks that are becoming 'hot'
//  and hence becoming candidates for JIT translation.
//  Typically, JIT translation occurs simultaneously for a group of basic
//  blocks that have been executed frequently in the most recent trace interval.
//
//  There are two cross-linked structures which keep track of translated blocks.
//  Firstly, there is a hierarchical collection of BlockEntry objects (one per
//  translated block). At the lower level there are map<> containers pointers
//  to all BlockEntry objects within each page of physical memory, at the
//  higher level there is a map<> containing a pointer to each of the page-level
//  map<> containers.
//  Secondly, there is a similarly hierarchical collection of TranslationModule
//  objects. At the lower level there is a map<> per page, which contains
//  pointers to all of the TranslationModule objects relating to that page.
//  At the higher level there is a map<> containing a pointer to each of the
//  page-level map<> containers.
//  A TranslationModule object maintains all relevant information about a shared
//  library that has been created during the translation of a collection of
//  functions within a single page. Note, each page may have several
//  TranslationModule objects, each created at a different time during simulation.
//
//  When a translated block is modified in target memory
//  ----------------------------------------------------
//
//  A page replacement can be said to have occurred when a JTLB is set up to
//  map a physical page that was previously the target of a different mapping.
//  Usually when this happens, the page will be overwritten with new data,
//  either by the kernel or by a DMA operation (possibly from disk).
//  The act of writing to a page that has some translated sequences, will
//  trigger the consistency mechanism. This will identify the BlockEntry
//  containing the address to be written, and will eliminate that BlockEntry's
//  translation and remove any reference to that BlockEntry from the corresponding
//  TranslationModule object. As each translation is deleted, the number of
//  instructions executed by that translation is accumulated in the overall
//  instruction counter within the Processor class, and also the ref_count
//  of the containing TranslationModule object is decremented. If the ref_count
//  reaches zero, then the corresponding shared library is unloaded, and
//  the TranslationModule object is removed from its containing map<> and deleted.
//  So, for efficiency reasons, each BlockEntry object points to the
//  TranslationModule object which manages the shared library containing that block.
//  There is no need to remove BlockEntry objects, provided their translation
//  is removed and they are re-classified as 'not translated'.
//  Finally, any entry for an instruction in the removed block that may
//  exist in the Dcode cache must also be removed, and any in the translation
//  hash table that matches the removed block must also be invalidated.
//
//  When a new shared library is installed
//  --------------------------------------
//
//  Each shared library produced by JIT translation contains a collection
//  of functions. Each function represents the translated code from a
//  one or more basic blocks. All functions in each shared library come from code
//  that is contained in the same physical page of target memory. This eases
//  the maintenance of translated code on a page-by-page basis.
//
//  Translation Cache
//  ------------------------------------
//
//  To speed up finding a translation for a given PC we use a translation cache.
//  We must make sure to properly maintain the translation cache in the presence
//  of self-modifying code.
//
//
//  Deciding which blocks to translate
//  ----------------------------------
//
//  The aim is to maximise the number of useful block translations that
//  are performed at each translation cycle. We also have to ensure that
//  only blocks from the same page are translated within the same
//  shared library.
//
//  After each trace interval we translate
//
//  Executing and translating in parallel
//  -------------------------------------
//
//  Simulation and JIT compilation runs in parallel using asynchronous threads. On
//  a multi-core processor this has a noticeable speedup.
//
//  @see http://groups.inf.ed.ac.uk/pasta/pub_PLDI_2011.html
//
// =====================================================================

#ifndef INC_SYSTEM_CPU_PROCESSOR_H_
#define INC_SYSTEM_CPU_PROCESSOR_H_

#include <map>
#include <stack>
#include <sstream>

#include "define.h"

#include "api/types.h"
#include "api/api_types.h"
#include "sim_types.h"

#include "ioc/ContextItemInterface.h"

#include "system.h"

#include "arch/CoreArch.h"

#include "isa/arc/DcodeCache.h"


#include "sys/cpu/state.h"
#include "sys/cpu/PageCache.h"
#include "sys/cpu/CounterManager.h"
#include "sys/cpu/EiaExtensionManager.h"
#include "sys/cpu/CcmManager.h"
#include "sys/mmu/Mmu.h"
#include "sys/aps/Actionpoints.h"
#include "sys/smt/Smart.h"

// Tracing/Profiling and JIT dynamic binary translation
#include "profile/PhysicalProfile.h"

#include "translate/TranslationManager.h"
#include "translate/TranslationCache.h"

#include "util/Log.h"
#include "util/Zone.h"

#include "util/TraceStream.h"
#include "util/system/Timer.h"

// ---------------------------------------------------------------------------
// MACROS
//

// Use this macro for instruction tracing
//
#define TRACE_INST(_cpu_, _trace_fun_)                                          \
  do {                                                                          \
    if (!_cpu_->sim_opts.trace_on)  ;                                           \
    else { _cpu_->_trace_fun_; }                                                \
  } while(0)


// ---------------------------------------------------------------------------
// FORWARD DECLARATIONS
// 

class WayMemo;
class Memory;
class MemoryModel;
class BranchPredictorInterface;
class ProcessorPipelineInterface;

namespace arcsim {
  
  namespace ioc {
    class Context;
  }
  
  namespace isa {
    namespace arc {
      class Dcode;
    }
  }
  namespace ise  {
    namespace eia {
      class EiaExtensionInterface;
      class EiaInstructionInterface;
    }
  }
  namespace ipt {
    class IPTManager;
  }
  
  namespace util {
    class CounterTimer;
  }
}


// ---------------------------------------------------------------------------
// TYPES/ENUMS
// 

// Definitions of trace output formats used during tracing of load or store instructions.
//
enum InstructionTraceLoadStoreFormat {
  T_FORMAT_LW = 0,  T_FORMAT_LH,  T_FORMAT_LB,
  T_FORMAT_SW,      T_FORMAT_SH,  T_FORMAT_SB,
  T_FORMAT_SWX,     T_FORMAT_SHX, T_FORMAT_SBX,
  T_FORMAT_LWX,     T_FORMAT_LHX, T_FORMAT_LBX,
  NUM_T_FORMATS
};

namespace arcsim {
  namespace sys {
    namespace cpu {
      
class Processor : public arcsim::util::system::TimerCallbackInterface,
                  public arcsim::ioc::ContextItemInterface
{
public:
  static const int kProcessorMaxNameSize = 256;
  
  // A non-existant interrupt number used to identify 'no interrupt'
  //
  static const uint32 kInvalidInterrupt = 0xffffffff;
  
private:
  uint8  name_[kProcessorMaxNameSize];

  // Decode caches for each operating mode (i.e. kernel and user mode), when the
  // processor switches from one mode to the other the dcode_cache pointer is
  // updated and points to the right cache.
  //
  arcsim::isa::arc::DcodeCache* dcode_cache;                       // pointer to currently active decode cache
  arcsim::isa::arc::DcodeCache  dcode_caches[NUM_OPERATING_MODES]; // decode caches for each operating mode
  
  arcsim::isa::arc::Dcode*      inst;       // pointer to current decoded instruction
  arcsim::isa::arc::Dcode       inst_dslot; // dcode instance used for decoding 
                                            // following dslot instruction
  
  PageCache                     page_cache; // processor page cache
  TranslationCache              trans_cache;// JIT Translation Cache
    
  CcmManager * const            ccm_mgr_;   // CCM device Manager
  
  // Zone used for very fast allocation of objects (e.g. PageProfile and BlockEntry
  // objects are Zone allocated).
  //
  arcsim::util::Zone            zone_;
  
  // When translations should be invalidated and the execution mode is set to 'kExecModeNative'
  // we push all to be invalidated tranlsations onto the 'invalid_translations_stack'
  // variable. Upon reaching a safe point we then remove the given translations.
  //
  std::stack<uint32>  invalid_translations_stack;

  // Free union between single-precision floating-point and uint32.
  // This is used to allow casting between float and uint32
  // whilst avoiding implicit type conversions.
  //
  union SpFpRegister {
    float   f;
    uint32  u;
  };

  // Free union between double-precision floating-point and an
  // array of 2 uint32 values.
  // This is used to allow quick transfers between double and
  // pairs of uint32 register values.
  //
  union DpFpRegister {
    double  d;
    uint32  v[2];
  };
    
  // Fetch and decode an instruction
  //
  uint32 fetch_and_decode (uint32                     pc,
                           arcsim::isa::arc::Dcode&   instr,
                           cpuState&                  state,
                           uint32&                    efa,
                           bool                       in_dslot,
                           bool                       have_side_effect);
  
  // Fetch and Decode an instruction using the DecodeCache as an object/memory pool:
  //  1. Lookup in the DecodeCache if a decoded instruction for that PC already exists
  //  1.a On a CACHE HIT assign the pointer reference to iptr
  //  1.b On a CACHE MISS the memory location/memory space in the cache is used 
  //      to decode the instruction to
  //    
  // ATTENTION: THIS METHOD IS NOT THREAD SAFE AND SHOULD ONLY BE CALLED BY THE
  //            PROCESSORS MAIN SIMULATION LOOP!
  //
  //  
  uint32 decode_instruction_using_cache (uint32                   pc,
                                         arcsim::isa::arc::Dcode* &iptr,
                                         uint32                   &efa,
                                         bool                     in_dslot);
    
#ifdef CYCLE_ACC_SIM
  // Update pipeline timing model in cycle accurate mode.
  // This method is private because it can not just be called from
  // anywhere as it has sideffects and modifies the processor state
  // counters. It is called from Processor::step() and Processor::step_fast()
  // in step.cpp.
  //
  void update_pipeline_timing_model();
#endif
  
  // Processor initialisation methods
  //
  void init_cpu_state ();
  void init_gprs ();
  void init_memory_region_map();
  // Call init_aux_regs () to initialise auxiliary registers. init_aux_regs()
  // will then call the appropriate ISA initialisation method.
  //
  void init_aux_regs ();
  void init_aux_regs_a6k ();
  void init_aux_regs_a700 ();
  void init_aux_regs_a600 ();

  void init_aux_regs_a6kv21();
  
  // Single instruction step or basic-block step methods
  //
  bool step_single (UpdatePacket*, bool);  // Do one inst, collect deltas
  bool step_single_fast ();                // Do one inst, no visibility or EIA
  bool step_block ();                      // Do one basic block of code
  
  // ---------------------------------------------------------------------------
  // Dispatch frequently executed traces to JIT compilation worker
  //
  inline int dispatch_hot_traces();
  
  
  // ---------------------------------------------------------------------------
  // This method checks the value in state.pending_actions, and performs the
  // actions required for each case. 
  // It returns true if the the simulation must be suspended for external
  // interactions, and in this case it records and updates the simulation time
  // resources.
  //
  bool    handle_pending_actions ();
  
  // ---------------------------------------------------------------------------
  // Interrupt detection factored out into two methods, one for A6kv2.1 and one for older systems
  void    detect_interrupts_legacy ();
  void    detect_interrupts_a6kv2 ();
  uint32  get_pending_ints_legacy ();
  uint32  get_pending_ints_a6kv2 ();
  
  // ---------------------------------------------------------------------------
  // Evaluate condition codes
  //
  bool eval_extension_cc (uint8 cc) const;
  
  inline bool eval_cc (uint8 cc) const {
    switch ( cc ) {
      case 0:  return true;
      case 1:  return state.Z;
      case 2:  return !state.Z;
      case 3:  return !state.N;
      case 4:  return state.N;
      case 5:  return state.C;
      case 6:  return !state.C;
      case 7:  return state.V;
      case 8:  return !state.V;
      case 9:  return (state.N && state.V && !state.Z) || (!state.N && !state.V && !state.Z);
      case 10: return (state.N && state.V) || (!state.N && !state.V);
      case 11: return (state.N != state.V);
      case 12: return state.Z || (state.N != state.V);
      case 13: return !state.C && !state.Z;
      case 14: return state.C || state.Z;
      case 15: return !state.N && !state.Z;
      default: {
        if (inst->eia_cond) {
          return eval_extension_cc (cc);
        } else {
          if (cc == 16) return ((state.auxs[AUX_MACMODE] & 0x00000210) != 0);
          if (cc == 17) return ((state.auxs[AUX_MACMODE] & 0x00000210) == 0);
        }
      }
    }
    return false; // should never happen
  }

  
public:

  enum ByteTransferSize {
    kByteTransferSizeByte = 0x1,
    kByteTransferSizeHalf = 0x2,
    kByteTransferSizeWord = 0x4,
  };
  
  enum ExecMode {
    kExecModeInterpretive = 0x0, // interpretive execution mode
    kExecModeNative       = 0x1, // native execution mode
  };
  
  // Current execution mode. This variable set to 'kExecModeNative' just before
  // entering native execution mode, and set to 'kExecModeInterpretive' after
  // exiting native execution mode.
  //
  ExecMode cur_exec_mode;

  // ---------------------------------------------------------------------------
  // Processor architecture configuration
  //
  const int             core_id;    // unique core identifier within a system
  const SystemArch&     sys_arch;   // top-level system architecture config
  CoreArch&             core_arch;  // core architecture config
  SimOptions&           sim_opts;   // simulation options
  
  // ---------------------------------------------------------------------------
  // Most important processor attributes such as the cpuState structure
  //
  System&                   system;       // enclosing system
  arcsim::sys::mem::Memory& mem;          // reference to memory
  Mmu                       mmu;          // memory management unit class
  cpuState                  state;        // cpu state structure
  cpuState                  state_trace;  // state structure used during trace construction
  
  // FIXME: Document the following field
  //
  UpdatePacket              delta;
    
  bool      sim_started;           // indicates if simulation started on this processor
  uint64    t_set_flags;           // stores flag-def time if insn modifies flags
  
  // Helper structures for managing aux register operations
  //
  uint32  aux_mask[BUILTIN_AUX_RANGE];
  uint8   aux_perms[BUILTIN_AUX_RANGE];
  
  // ---------------------------------------------------------------------------
  // Microarchitectural models
  //
  MemoryModel*                 mem_model;
  ProcessorPipelineInterface*  pipeline;
  BranchPredictorInterface*    bpu;
  WayMemo*                     iway_pred;
  WayMemo*                     dway_pred;

  // ---------------------------------------------------------------------------
  // Stringstream used to create current instruction trace
  //
  std::stringstream         IS; // IS - InstructionStream
  
  // Tracestream used for dispatching each traced instruction
  //  
  arcsim::util::TraceStream TS; // TS - TraceStream
      
  // ---------------------------------------------------------------------------
  // Tracing/Profiling related members
  //
  bool        end_of_block;             // true if end of basic block reached
  bool        prev_had_dslot;           // true if previous insn had dslot
  
  uint32      trace_interval;           // current trace interval counter
  uint32      trace_interp_block_count; // current interpretive interval counter

  // Per processor hotspot threshold. This threshold adapts depending on the size
  // of the translation work queue. 
  //
  uint32                            local_hotspot_threshold;
      
  // When an interrupt or exception is entered, that state is pushed on
  // the interrupt_stack data structure. Thus the top-of-stack always denotes
  // the current InterruptState. 
  //
  std::stack<InterruptState>         interrupt_stack;    
      
  // Map of lp_end to lp_start addresses
  //
  std::map<uint32,uint32>            lp_end_to_lp_start_map;

  // --------------------------------------------------------------------------- 
  // Host and Target timer structures
  //
  arcsim::util::system::Timer* timer;
  bool                         use_host_timer;
  bool                         inst_timer_enabled;
  uint32                       count_increment;
  bool                         running0, running1;
  sint64                       vcount0, vcount1;
  uint64                       timer_sync_time;
  
  uint64                       rtc_disabled_ticks;
  uint64                       last_rtc_disable;
  uint64                       last_rtc_clear;
  
  
  // --------------------------------------------------------------------------- 
  // NOTE: Order of declaration matters with the following member variables!
  //
  
  // --------------------------------------------------------------------------- 
  // Data structure holding all information about physical pages and their
  // translations.
  //
  arcsim::profile::PhysicalProfile&  phys_profile_;

  // --------------------------------------------------------------------------- 
  // Counters and time recording
  //
  arcsim::util::CounterTimer& exec_time;    // target binary exeuction time
  arcsim::util::CounterTimer& trace_time;   // trace construction time
  CounterManager              cnt_ctx;      // various processor counters

  // ---------------------------------------------------------------------------  
  // HW Actionpoints, SmART, and EIA support
  //
  Actionpoints              aps;            // actionpoint structures
  Smart                     smt;            // SmaRT structures
  EiaExtensionManager       eia_mgr;        // EIA manager

  // --------------------------------------------------------------------------- 
  // IPT Manager - injected by IOC container (IPT - Instruction PoinT)
  // NOTE: IPTManager retrieves PhysicalProfile from context, hence it has to
  //       be declared AFTER phys_profile_ as member initialisation order in C++
  //       is based on declaration order.
  //
  arcsim::ipt::IPTManager&  ipt_mgr;

  
  // ---------------------------------------------------------------------------
  // Constructor/Destructor
  //
  explicit Processor (System&                   parent,
                      CoreArch&                 core_arch,
                      arcsim::ioc::Context&     cpu_ctx, 
                      arcsim::sys::mem::Memory* memory,
                      MemoryModel*              memory_model,
                      int                       core_id,
                      const char*               name);
  ~Processor ();

  // ---------------------------------------------------------------------------
  // Methods required by ConstantItemInterface
  //
  const uint8* get_name() const { return name_; };
  const Type   get_type() const { return arcsim::ioc::ContextItemInterface::kTProcessor;  };

  // ---------------------------------------------------------------------------
  // Reset to initial state to enable re-use for another simulation run
  //
  bool reset_to_initial_state (bool purge_translations = true);

  void prepare_cpu ();
  void clear_cpu_counters ();

  void reset ();          // reset this processor
  void system_reset ();   // reset at the system level

  // ---------------------------------------------------------------------------
  // Simulation wrapper methods
  //
  void simulation_start();
  void simulation_continued();
  void simulation_end();
  void simulation_stopped();

  // ---------------------------------------------------------------------------
  // Exception and run-mode management functions (processor.cpp)
  //
  void set_operating_mode (OperatingMode);  // 1 => user mode, 0 => kernel mode
  void enter_exception (uint32 ecr, uint32 efa, uint32 eret);
  void exit_exception ();
  void exit_interrupt (int);
  void raise_misaligned_exception (uint32 efa, uint32 eret);

  void acknowledge_interrupt(uint32 PI);
  
  void enter_a6kv21_interrupt(uint8 PI, uint8 W);
  void enter_a6kv21_fast_interrupt(uint8 PI);
  void enter_a6kv21_exception(uint32 ecr, uint32 efa, uint32 eret);
  void return_from_ie_a6kv21();
  void return_from_exception_a6kv21();
  
  // A6kV2.1 involves implicit jump instructions on exception and interrupt entry. This function
  // fetches the given vector and writes it to the PC, and in trace mode inserts a 'dummy' jump.
  // This could raise a memory access exception.
  bool jump_to_vector(uint32 vector, uint32 &ecause, uint32 &efa);
  
  // Swap between the A6KV2.1 Reg banks depending on how the banked registers are configured.
  // Safe to call if no additional reg bank is configured - nothign will happen in this case
  // If the register banks are swapped, the RB field of STATUS32 is updated
  void swap_reg_banks();
  
  // Helper function for swap_reg_banks() - swap all of the registers in r[low] -> r[high] INCLUSIVE
  void swap_reg_group(int low, int high);
  
  //IMPORTANT: in these functions, PI and W represent any PENDING interrupt, NOT the interrupt
  // being returned from. Since these could be kInvalidInterrupt, they must be uint32s
  void return_from_interrupt_a6kv21(uint32 PI, uint8 W);
  void return_from_fast_interrupt_a6kv21(uint32 PI, uint8 W);
  
  // ---------------------------------------------------------------------------
  // Fetch and Decode instruction method
  //
  uint32 decode_instruction (uint32                   pc,
                             arcsim::isa::arc::Dcode& instr,
                             cpuState&                state,
                             uint32&                  efa,
                             bool                     in_dslot);

  // Read only access to currently interpreted instruction
  // NOTE that the returned pointer is only valid during instruction interpretation
  //
  arcsim::isa::arc::Dcode const * const current_interpreted_inst() const { return inst; }
  
  // ---------------------------------------------------------------------------
  // Processor run methods
  //
  bool run         (uint32 num_blocks);    // Simulate for num_blocks basic blocks (JIT)
  bool run_notrace (uint32 num_insts);     // Simulate for num_insts (Normal)
  bool run_trace   (uint32 num_insts, UpdatePacket* upkt = NULL);  // Simulate for num_insts (instruction tracing)

  // ---------------------------------------------------------------------------
  // Run-time emulators and complex instruction interpretation (processor.cpp)
  //
  void halt_cpu (bool set_self_halt = true);
  void restart_from_halted ();  // Restart from halted state
  void break_inst ();
  void sleep_inst (uint32 int_enables);
  void flag_inst  (uint32 opd, uint32 kflag);
  void emulate_trap ();
  
  void spfp_emulation  (uint32 op, uint32 *dst, uint32 s1, uint32 s2, bool flag_enb);
  void dpfp_emulation  (uint32 op, uint32 *dst, uint32 s1, uint32 s2, bool flag_enb);
  void dexcl_emulation (uint32 op, uint32 *dst, uint32 s1, uint32 s2);
  
  bool read_aux_register  (const uint32 aux_addr, uint32* rdata, bool from_sim);
  bool write_aux_register (const uint32 aux_addr, uint32 aux_data, bool from_sim);

  // ---------------------------------------------------------------------------
  // Memory page cache miss routines (processor-memory.cpp)
  //
  bool    read_page_miss  (uint32 virt_byte_addr, uint32 &data, ByteTransferSize size);
  uint32  fetch_page_miss (uint32 virt_byte_addr, uint32 &data, bool have_side_effect);
  bool    write_page_miss (uint32 virt_byte_addr, uint32 data,  ByteTransferSize size);

  // ---------------------------------------------------------------------------
  // Processor interrupt handling methods (processor-interrupts.cpp)
  //
  void    assert_interrupt_line  (int int_num);
  void    rescind_interrupt_line (int int_num);
  void    cancel_interrupt (int int_num);
  void    write_irq_hint (uint32 int_no);
  void    clear_pulse_interrupts (uint32 int_bits);
  void    detect_interrupts ();
  uint32  get_pending_ints ();
  
  // A6KV2.1 Timer IRQs depend on which exceptions are configured
  uint32 get_timer0_irq_num();
  uint32 get_timer1_irq_num();
  
  uint32 get_rtc_low();
  uint32 get_rtc_high();
  uint32 get_rtc_ctrl();
  
  void   set_rtc_low(uint32);
  void   set_rtc_high(uint32);
  void   set_rtc_ctrl(uint32);
  
  // A6KV2.1 function for determining the maximum interrupt vector
  // A possible optimisation would be to have these inlined.
  //
  uint32 max_interrupt_num() const;
  
  // A6KV2.1 Function for determining if an interrupt vector is free for use as an interrupt
  //
  bool is_interrupt_configured(uint32 num) const;
  bool is_interrupt_enabled(uint32 interrupt) const;
  bool is_interrupt_pending(uint32 interrupt) const;
  bool is_interrupt_enabled_and_pending() const;
  void set_interrupt_enabled(uint32 interrupt, bool enable);
  void set_interrupt_pending(uint32 interrupt, bool pending);
  
  // ---------------------------------------------------------------------------
  // Actionpoints handling methods (processor.cpp)
  //
  void    trigger_watchpoint ();

  // ---------------------------------------------------------------------------
  // Timer handling methods, supporting two ARCompact timers (processor-timers.cpp)
  //
  
  // Callback method executed on Timer expiry
  //
  void    on_timer(arcsim::util::system::Timer* timer);
  
  void    init_timers ();
  void    start_timers ();
  void    stop_timers ();
  void    timer_set_count (int timer, uint32 value);
  uint32  timer_get_count (int timer);
  void    timer_set_limit (int timer, uint32 value);
  void    timer_set_control (int timer, uint32 value);
  void    timer_sync ();
  void    timer_int_ack (int timer);
  void    detect_timer_expiry ();
  void    timing_checkpoint ();
  void    timing_restart ();
  uint32  time_to_expiry();
  uint32  timer_advance_cycles();

  // Performance counting and display functions (processor.cpp)
  //
  uint64 instructions  () const;
  double cycle_sim_ipc () const;
  double cycle_sim_cpi () const;
  void   print_stats   ();

  // ---------------------------------------------------------------------------
  // Code tracing functions (processor.cpp)
  //
  void trace_string (const char* str);
  void trace_emit ();
  void trace_commit (bool is_commit);
  
  void trace_instruction (uint32 pc, uint32 inst, uint32 limm);
  void trace_write_back (int wa0, bool wenb0, uint32 *wdata0,
                         int wa1, bool wenb1, uint32 *wdata1);
  void trace_exception  (const char *message = 0);
  void trace_rtie ();

  void trace_mul64_inst ();
  
  void trace_loop_back  ();
  void trace_loop_count ();
  void trace_loop_inst  (int taken, uint32 target);
  
  void trace_load       (int fmt, uint32 addr, uint32 data);
  void trace_store      (int fmt, uint32 addr, uint32 data);
  
  void trace_lr         (uint32 addr, uint32 data, int success);
  void trace_sr         (uint32 addr, uint32 data, int success);

  // ---------------------------------------------------------------------------
  // Functions to support rudimentary debugger (processor.cpp)
  //
  bool set_breakpoint   (uint32 brk_location, bool& brk_s, uint32& old_instruction);
  bool clear_breakpoint (uint32 brk_location, uint32 old_instruction, bool brk_s);

  // ---------------------------------------------------------------------------
  // EIA  methods
  //
  bool register_eia_extension(arcsim::ise::eia::EiaExtensionInterface*);
   
  // ---------------------------------------------------------------------------
  // Performance-critical methods for manipulating a PendingActions
  //
  
  // Set a specific pending action
  inline void set_pending_action(PendingActionKind action) {
    state.pending_actions |= action;
  }
  
  // Check if any actions are pending
  inline bool has_pending_actions() const   {
    return (bool)(state.pending_actions);
  }
  
  // Check if specific action is pending
  inline bool is_pending_action(PendingActionKind action) const   {
    return (bool)(state.pending_actions & action);
  }
  
  // Retrieve currently pending actions kinds
  inline uint32 get_pending_actions() const {
    return state.pending_actions;
  }

  // Clear specific pending action
  inline void
  clear_pending_action(PendingActionKind action) {
    if (action != kPendingAction_NONE) {
      state.pending_actions &= ~action;
    }
  }
  
  // Clear ALL pending actions
  inline void clear_pending_actions()   {
    state.pending_actions = kPendingAction_NONE;
  }
    
  // ---------------------------------------------------------------------------
  // Performance-critical memory/page cache related functions
  //
  inline bool 
  addr_is_misaligned (uint32 addr, int size_mask, uint32 pc) {
    if (addr & size_mask) {
      raise_misaligned_exception (addr, pc);
      return true;
    }
    return false;
  }
  
  // ---------------------------------------------------------------------------
  // CCM manager methods
  //
  inline bool in_ccm_mapped_region(uint32 addr) const { 
    return ccm_mgr_->in_ccm_mapped_region(addr);
  }
  
  inline void register_iccm() { ccm_mgr_->create_or_replace_iccm(); }
  inline void register_dccm() { ccm_mgr_->create_or_replace_dccm(); }
  
  // ---------------------------------------------------------------------------
  // Retrieve BlockData object for page
  //
  inline arcsim::sys::mem::BlockData*
  get_host_page(uint32 addr)
  { // Check if address falls into a configured CCM region
    //
    if (ccm_mgr_->in_ccm_mapped_region(addr)) {
      // CCM is responsible for address, ask CCM manager to wire it up
      return ccm_mgr_->get_host_page(addr);
    }
    // Go off-chip to system level to retrieve page from system memory
    return system.get_host_page(addr);
  }
  
  // ---------------------------------------------------------------------------
  // Block wise read and write of memory at processor level. Note that block wise
  // read/write uses physical addresses as the start address of a block.
  //
  bool read_block (uint32 phys_addr, uint32 size, uint8 *       buf);
  bool write_block(uint32 phys_addr, uint32 size, uint8 const * buf);

  bool read_block_external_agent (uint32        phys_addr,
                                  uint32        size,
                                  uint8 *       buf,
                                  int           agent_id);
  bool write_block_external_agent(uint32        phys_addr,
                                  uint32        size,
                                  uint8 const * buf,
                                  int           agent_id);
  
  
  // ---------------------------------------------------------------------------
  // Returns false if stack checking is enabled and we access an illegal memory area
  // for the current instruction.
  //
  inline bool
  is_stack_check_success_r(uint32 addr) const
  {
    if (!state.SC)
      return true; // exit early if stack checking is disabled
    
    const bool is_opd_SP_REG =   (inst->info.rf_renb0 && (inst->info.rf_ra0 == SP_REG))
                              || (inst->info.rf_renb1 && (inst->info.rf_ra1 == SP_REG));
    if (is_opd_SP_REG) {
        
      const bool is_dst_SP_REG =   (inst->info.rf_wenb0 && (inst->info.rf_wa0 == SP_REG))
                                || (inst->info.rf_wenb1 && (inst->info.rf_wa1 == SP_REG));
      
      if (is_dst_SP_REG) { // is SP_REG modified
        // Are we inside the whole stack: stack is "up side down" base > top
        return ((state.stack_base > addr) && (state.stack_top <= addr));
      }
      // Are we inside the active stack: stack is "up side down" base > top
      return ((state.stack_base > addr) && (state.gprs[SP_REG] <= addr) && (state.stack_top <= addr));
    }
    // Are we outside the stale stack: stack is "up side down" base > top
    return ((state.stack_top > addr) || (state.gprs[SP_REG] <= addr));
  }
  
  inline bool
  is_stack_check_success_w(uint32 addr) const
  {
    if (!state.SC)
      return true; // exit early if stack checking is disabled
    const bool is_opd_SP_REG =   ((inst->info.rf_renb0 && inst->info.rf_ra0 == SP_REG));
    if (is_opd_SP_REG) {

      const bool is_dst_SP_REG =   (inst->info.rf_wenb0 && inst->info.rf_wa0 == SP_REG)
                                || (inst->info.rf_wenb1 && inst->info.rf_wa1 == SP_REG);
      
      if (is_dst_SP_REG) { // is SP_REG modified
        // Are we inside the whole stack: stack is "up side down" base > top
        return ((state.stack_base > addr) && (state.stack_top <= addr));
      }
      // Are we inside the active stack: stack is "up side down" base > top
      return ((state.stack_base > addr) && (state.gprs[SP_REG] <= addr) && (state.stack_top <= addr));
    }
    // Are we outside the stale stack: stack is "up side down" base > top
    return ((state.stack_top > addr) || (state.gprs[SP_REG] <= addr));
  }
  
  inline bool
  is_stack_check_success_x(uint32 addr) const
  {
    if (!state.SC)
      return true; // exit early if stack checking is disabled
    const bool is_opd_SP_REG =   ((inst->info.rf_renb1 && inst->info.rf_ra1 == SP_REG));
    if (is_opd_SP_REG) {
        
      const bool is_dst_SP_REG =   (inst->info.rf_wenb0 && inst->info.rf_wa0 == SP_REG)
                                || (inst->info.rf_wenb1 && inst->info.rf_wa1 == SP_REG);
      
      if (is_dst_SP_REG) { // is SP_REG modified
        // Are we inside the whole stack: stack is "up side down" base > top
        return ((state.stack_base > addr) && (state.stack_top <= addr));
      }
      // Are we inside the active stack: stack is "up side down" base > top
      return ((state.stack_base > addr) && (state.gprs[SP_REG] <= addr) && (state.stack_top <= addr));
    }
    // Are we outside the stale stack: stack is "up side down" base > top
    return ((state.stack_top > addr) || (state.gprs[SP_REG] <= addr));
  }
  
  // ---------------------------------------------------------------------------
  // WRITE - performance-critical hence inlined
  //  
  inline bool
  write8 (uint32 addr, uint32 data)
  {
    EntryPageCache_ * const p = state.cache_page_write_ + core_arch.page_arch.page_byte_index(addr);

    if (p->addr_ == core_arch.page_arch.page_byte_tag(addr))
    {
      uint32 page_offset = core_arch.page_arch.page_offset_byte_index(addr);
      ((uint8*)(p->block_))[page_offset] = (uint8)data;
      return true;
    } else {
      return write_page_miss (addr, data, kByteTransferSizeByte);
    }
  }

  inline bool
  write16 (uint32 addr, uint32 data)
  {
    if (addr_is_misaligned (addr, 1, state.pc)) return false;
    
    EntryPageCache_ * const p = state.cache_page_write_ + core_arch.page_arch.page_byte_index(addr);
      
#if BIG_ENDIAN_SUPPORT
    if (sim_opts.big_endian) { data  = ((data & 0x00ffU) << 8) | ((data & 0xff00U) >> 8); }
#endif

    if (p->addr_ == core_arch.page_arch.page_byte_tag(addr)) {
      uint32 page_offset = core_arch.page_arch.page_offset_half_index(addr);
      ((uint16*)(p->block_))[page_offset] = (uint16)data;
      return true;
    } else {
      return write_page_miss (addr, data, kByteTransferSizeHalf);
    }
  }

  inline bool
  write32 (uint32 addr, uint32 data)
  {
    if (addr_is_misaligned (addr, 3, state.pc)) return false;
    
    EntryPageCache_ * const p = state.cache_page_write_ + core_arch.page_arch.page_byte_index(addr);

#if BIG_ENDIAN_SUPPORT
    if (sim_opts.big_endian) {
#if USE_BSWAP_ASM
      __asm__ __volatile__("bswap %0" : "=r" (data) : "0" (data));
#else
      data  = ((data & 0x000000ffUL) << 24) | ((data & 0x0000ff00UL) << 8)
            | ((data & 0x00ff0000UL) >> 8)  | ((data & 0xff00000fUL) >> 24);
#endif
    }
#endif
    
    if (p->addr_ == core_arch.page_arch.page_byte_tag(addr)) {
      uint32 page_offset = core_arch.page_arch.page_offset_word_index(addr);
      p->block_[page_offset] = data;
      return true;
    } else {
      return write_page_miss (addr, data, kByteTransferSizeWord);
    }
  }
  
  
  bool  write_no_exception(uint32 addr, uint32 data, ByteTransferSize size);

  // ---------------------------------------------------------------------------
  // Atomic memory exchange function
  //
  // Call this function to atomically exchange the contents of a target
  // instruction operand (usually a register) with a target system memory location.
  //
  // The 'addr' argument must be the address of a location in the simulated
  // system memory. This function will attempt to derive the physical target 
  // address and hence the host address for this location. If this is not
  // successful the function returns false, and may also raise an MMU-related
  // exception on the target processor. This location is both read and written.
  // 
  // The 'p_reg' argument must be a pointer to the processor register, within
  // the state structure for the processor. This location is both read and
  // written.
  //
  bool atomic_exchange (uint32 addr, uint32* p_reg);

  // ---------------------------------------------------------------------------
  // READ - performance-critical hence inlined
  //
  inline bool
  read8 (uint32 addr, uint32 &data)
  {
    EntryPageCache_ const * const p = state.cache_page_read_ + core_arch.page_arch.page_byte_index(addr);

    if (p->addr_ == core_arch.page_arch.page_byte_tag(addr)) {
      uint32 page_offset = core_arch.page_arch.page_offset_byte_index(addr);
      data = ((uint8*)(p->block_))[page_offset];
      return true;
    } else {
      return read_page_miss (addr, data, kByteTransferSizeByte);
    }
  }

  inline bool
  read8_extend (uint32 addr, uint32 &data)
  {
    bool   success = true;
    EntryPageCache_ const * const p = state.cache_page_read_ + core_arch.page_arch.page_byte_index(addr);

    if (p->addr_ == core_arch.page_arch.page_byte_tag(addr)) {
      uint32 page_cache = core_arch.page_arch.page_offset_byte_index(addr);
      data = ((sint8*)(p->block_))[page_cache];
    } else {
      success = read_page_miss (addr, data, kByteTransferSizeByte);
      data = ((sint8) (data & 0xffUL)); /* Add sign extension */
    }
    return success;
  }

  inline bool
  read16 (uint32 addr, uint32 &data)
  {
    bool   success = true;
    
    if (addr_is_misaligned (addr, 1, state.pc)) return false;

    EntryPageCache_ const * const p = state.cache_page_read_ + core_arch.page_arch.page_byte_index(addr);

    if (p->addr_ == core_arch.page_arch.page_byte_tag(addr)) {
      uint32 page_offset = core_arch.page_arch.page_offset_half_index(addr);
      data = ((uint16*)(p->block_))[page_offset];
    } else {
      success = read_page_miss (addr, data, kByteTransferSizeHalf);
    }

#if BIG_ENDIAN_SUPPORT
    if (sim_opts.big_endian) { data  = ((data & 0x00ffU) << 8) | ((data & 0xff00U) >> 8); }
#endif

    return success;
  }

  inline bool
  read16_extend (uint32 addr, uint32 &data)
  {
    bool   success = true;

    if (addr_is_misaligned (addr, 1, state.pc)) return false;

    EntryPageCache_ const * const p = state.cache_page_read_ + core_arch.page_arch.page_byte_index(addr);

    if (p->addr_ == core_arch.page_arch.page_byte_tag(addr)) {
      uint32 page_offset = core_arch.page_arch.page_offset_half_index(addr);
      data = ((uint16*)(p->block_))[page_offset];
    } else {
      success = read_page_miss (addr, data, kByteTransferSizeHalf);
    }
    
#if BIG_ENDIAN_SUPPORT
    if (sim_opts.big_endian)  { data  = ((data & 0x00ffU) << 8) | ((data & 0xff00U) >> 8); }
#endif
    // Sign extend
    data = (sint16)(data & 0xffffUL);

    return success;
  }

  inline bool
  read32 (uint32 addr, uint32 &data)
  {
    bool   success = true;

    if (addr_is_misaligned (addr, 3, state.pc)) return false;

    EntryPageCache_ const * const p = state.cache_page_read_ + core_arch.page_arch.page_byte_index(addr);

    if (p->addr_ == core_arch.page_arch.page_byte_tag(addr)) {
      uint32 page_offset = core_arch.page_arch.page_offset_word_index(addr);
      data = p->block_[page_offset];
    } else {
      success = read_page_miss (addr, data, kByteTransferSizeWord);
    }

#if BIG_ENDIAN_SUPPORT
    if (sim_opts.big_endian) {
#if USE_BSWAP_ASM
      __asm__ __volatile__("bswap %0" : "=r" (data) : "0" (data));
#else
      data  = ((data & 0x000000ffUL) << 24) | ((data & 0x0000ff00UL) << 8)
            | ((data & 0x00ff0000UL) >> 8)  | ((data & 0xff00000fUL) >> 24);
#endif
    }
#endif

    return success;
  }
  
  // ---------------------------------------------------------------------------
  // FETCH
  //
  inline uint32
  fetch16 (uint32 addr, uint32 &data, bool have_side_effect)
  {
    uint32 retval = 0;
    
    EntryPageCache_ const * const p = state.cache_page_exec_ + core_arch.page_arch.page_byte_index(addr);

    if (p->addr_ == core_arch.page_arch.page_byte_tag(addr)) {
      uint32 page_offset = core_arch.page_arch.page_offset_half_index(addr);
      data = ((uint16*)(p->block_))[page_offset];
    } else {
      retval = fetch_page_miss (addr, data, have_side_effect);
    }

#if BIG_ENDIAN_SUPPORT
    if (sim_opts.big_endian) { data = ((data & 0x00ffU) << 8) | ((data & 0xff00U) >> 8); }
#endif

    return retval;
  }

  inline uint32
  fetch32 (uint32 addr, uint32 &data, uint32 &efa, bool have_side_effect)
  {
    uint32 hdata, ldata;
    uint32 ecause;

    if ( (ecause = fetch16 (addr, hdata, have_side_effect)) ) {
      efa = addr;
      return ecause;
    }
    if ( (ecause = fetch16 (addr+2, ldata, have_side_effect)) ) {
      efa = addr + 2;
      return ecause;
    }
    data = (hdata << 16) | (ldata & (uint32)0xffffUL);
    return 0;
  }

  // ---------------------------------------------------------------------------
  // Query if address is the end of a ZOL, given the appropriate map data structure
  // for lookup
  //
  inline bool
  is_end_of_zero_overhead_loop (const std::map<uint32,uint32>& lp_end_to_lp_start_map,
                                uint32                         pc_addr) const
  {
    return (lp_end_to_lp_start_map.find(pc_addr) != lp_end_to_lp_start_map.end());
  }
  
  // ---------------------------------------------------------------------------
  // Safely remove translations - when current execution mode is 'kExecModeNative'
  //  the translation that should be invalidated and registers a pending action
  //  which in turn will be evaluated at a 'safe-point' (i.e. a point in  execution
  //  when removing translations is safe). 
  //
  //  For all other execution modes it is safe to remove translations right away
  //
  inline void remove_translation(uint32 addr) {
    if (cur_exec_mode == kExecModeNative) {
      // NATIVE MODE - we register translation for removal and set pending action
      invalid_translations_stack.push(addr);
      set_pending_action(kPendingAction_FLUSH_TRANSLATION);
    } else {
      // INTERPRETIVE MODE - we can remove translation directly
      phys_profile_.remove_translation(addr);
      purge_translation_cache();
    }
  }
  
  // Remove all translations
  //
  inline void remove_translations() {
    if (cur_exec_mode == kExecModeNative) {
      // NATIVE MODE - we set corresponding pending action
      set_pending_action(kPendingAction_FLUSH_ALL_TRANSLATIONS);
    } else {
      // INTERPRETIVE MODE - we can remove translations directly
      phys_profile_.remove_translations();
      purge_translation_cache();
    }
  }
  
  // ---------------------------------------------------------------------------
  // Software cache related functions
  //
  
  inline void purge_dcode_cache () {
    for (uint32 i = 0; i < NUM_OPERATING_MODES; ++i) {
      dcode_caches[i].purge();
    }
  }

  inline void purge_dcode_entry (uint32 pc) {
    for (uint32 i = 0; i < NUM_OPERATING_MODES; ++i) {
      dcode_caches[i].purge_entry(pc);
    }
  }

  inline void purge_translation_cache () {
    // only purge translation cache if in fast mode
    if (sim_opts.fast) {
      trans_cache.purge();
    }
  }
  
  inline void purge_page_cache(arcsim::sys::cpu::PageCache::Kind page_kind) {
    page_cache.flush(page_kind);
  }
  
  inline uint32 purge_page_cache_entry(arcsim::sys::cpu::PageCache::Kind page_kind,
                                       uint32                                 addr)
  {
    return page_cache.purge(page_kind, addr);
  }

};

} } } //  arcsim::sys::cpu

#endif  // INC_SYSTEM_CPU_PROCESSOR_H_
