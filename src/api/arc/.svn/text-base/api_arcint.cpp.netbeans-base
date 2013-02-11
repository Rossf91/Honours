//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// SYNOPSYS CONFIDENTIAL - This is an unpublished, proprietary work of
// Synopsys, Inc., and is fully protected under copyright and trade
// secret laws. You may not view, use, disclose, copy, or distribute
// this file or any information contained herein except pursuant to a
// valid written license from Synopsys.
//
// =====================================================================
// 
// Description:
// 
// This file implements the API between ArcSim and the ARC/MW debugger.
// 
// The functions and objects exported from this file have C-linkage
// but they are written in C++ to allow access to the C++ objects used
// in ArcSim. See 'arcint.h' bundled with the MetaWare distribution for
// more details.
// 
// =======================================================================

#define OEM_USE_OF_DEBUGGER_HEADER_FILES 1

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <cstdlib>
#include <errno.h>

#include <arcint.h>
#include <dbgaccess.h>

#include "define.h"

#ifdef SNPS_ARCSIM_LICENSING
#include "snps/licensing/licensing.h"
#undef DLLEXPORT  // to avoid conflict with DLLEXPORT in api/types.h
#include "snps/licensing/dlimit_arcsim.h"
#endif

#include "api/types.h"
#include "api/arc/api_arc_common.h"
#include "api/api_funs.h"

#include "system.h"
#include "sys/cpu/processor.h"

#include "arch/Configuration.h"

#include "util/CounterTimer.h"
#include "util/system/SharedLibrary.h"
#include "util/Log.h"

#include "util/Counter.h"
#include "ioc/ContextItemId.h"

#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
#include "api/arc/prof_count.h"
#endif
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
#include "api/arc/instr_history.h"
#endif

#define ARCINT_LOG_DEBUG   (LOG_DEBUG)

// For is_simulator() return values:
#define ARC_IS_SIMULATOR 1
#define ARC_RUN_CAPABLE  2
#define ARC_RETURN_HALT  4

#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
#define ARCINT_DEFAULT_PROFILING_ENABLED       (true)      // Match ISS default
#endif
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
#define ARCINT_DEFAULT_INSTR_HISTORY_ENABLED   (true)      // Match ISS default
#define ARCINT_DEFAULT_INSTR_HISTORY_SIZE      (65536*8)   // Default is 1/2 meg, same as ISS
#endif

#define MSG_BUF_SIZE (2048)
#define SPEC_SIZE (256)

#define ARCSIM_PRINT(msg)                       \
  do {                                          \
    if (verbose && sys && dbg_access) dbg_access->print msg;  \
  } while (0)

#define ARCSIM_INFO(msg)                        \
  do {                                          \
    if (dbg_access) dbg_access->print msg;      \
  } while (0)

#define ARCSIM_ERROR(msg) ARCSIM_INFO(msg)

#ifdef _WIN32
// From:
//   http://www.codeguru.com/cpp/w-p/dll/tips/article.php/c3635
// More background/supporting info:
//   http://blogs.msdn.com/b/oldnewthing/archive/2004/10/25/247180.aspx
//   http://blogs.msdn.com/b/oldnewthing/archive/2004/06/14/155107.aspx

#if _MSC_VER >= 1300    // for VC 7.0
  // from ATL 7.0 sources
  #ifndef _delayimp_h
  extern "C" IMAGE_DOS_HEADER __ImageBase;
  #endif

#endif

static HMODULE
arcint_GetCurrentModule()
{
#if _MSC_VER < 1300    // earlier than .NET compiler (VC 6.0)
  // Here's a trick that will get you the handle of the module
  // you're running in without any a-priori knowledge:
  // http://www.dotnet247.com/247reference/msgs/13/65259.aspx
  
  MEMORY_BASIC_INFORMATION mbi;
  static int dummy;
  VirtualQuery( &dummy, &mbi, sizeof(mbi) );
  
  return reinterpret_cast<HMODULE>(mbi.AllocationBase);
  
#else    // VC 7.0
  
  // from ATL 7.0 sources
  return reinterpret_cast<HMODULE>(&__ImageBase);
#endif
}
#endif   // _WIN32

static std::string*
arcint_whoami(void)
{
  std::string *res = 0;
#ifdef _WIN32
  LPTSTR strDLLPath = new TCHAR[_MAX_PATH];
  if (GetModuleFileName( arcint_GetCurrentModule(), strDLLPath, _MAX_PATH)) {
    res = new std::string(strDLLPath);
  }
#else
  Dl_info dl_info;
  if (dladdr((void*)arcint_whoami, &dl_info)) {
    res = new std::string(dl_info.dli_fname);
  }
#endif
  return res;
}

struct ARCsim : ARC {
private:
  System          *sys;          /* ptr to Fast ISS simulator       */
  Configuration    arch_conf;    /* architecture configuration      */
  
  ARC_callback    *callback;     /* callbacks                       */
  Debugger_access *dbg_access;   /* debugger access                 */
  
  bool             print_stats_on_exit;
  bool             print_simulation_time_on_exit;
  
  bool             sim_period_set;

  bool             verbose;

  unsigned         mem_size;

  bool             have_icache;
  char             icache_spec[SPEC_SIZE];        
  bool             have_dcache;
  char             dcache_spec[SPEC_SIZE];
  unsigned         iccm_base, iccm_size;
  unsigned         dccm_base, dccm_size;
  unsigned         mem_init;

  bool             arcsim_mode_fast;
  bool             arcsim_mode_cycles;

  char             msg_buf[MSG_BUF_SIZE];

#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
  Prof_count       *prof_cnts[N_PROFCNTS];
  bool             record_profiling;
#endif

#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
  Instr_history   *instr_history;
#endif

#ifdef SNPS_ARCSIM
  bool             do_early_hostlink;
#endif
 
  const char*      flex_license_dll;
  const char*      flex_license_file;

#if SNPS_ARCSIM_LICENSING
  LicenseManager*   license_manager;
#endif

public:
  
  ARCsim()
  : sys(0),               /* set up later in prepare_for_new_program () */
    callback(0),
    dbg_access(0),
    print_stats_on_exit(false),
    print_simulation_time_on_exit(false),
    sim_period_set(false),
    have_icache(false),
    have_dcache(false),
    iccm_base(0), iccm_size(0),
    dccm_base(0), dccm_size(0),
    more_regs(0),
    mem_init(0),
    flex_license_dll(0),
    flex_license_file(0),
#if SNPS_ARCSIM_LICENSING
    license_manager(0),
#endif
    arcsim_mode_fast(0),
    arcsim_mode_cycles(0),
    verbose(true)
  {
    char * env_arcsim_debug = getenv(QUOTEME(SNPS_PROD_STR_UPC(DEBUG)));
    if (env_arcsim_debug) {
      // useful when debugging arcsim start-up
      arcsim::util::Log::reportingLevel = (TLogLevel)arcsim_strtoul(env_arcsim_debug);
    }

    std::string* whoami = arcint_whoami();
    if (whoami) {
      arch_conf.sys_arch.sim_opts.arcsim_lib_name = *whoami;
      delete whoami;
    }

    // Special explicit load of arcsim .so/.dll in order to enable successful symbol
    // resolution when JIT compiler is enabled
    //
    // This doesn't seem to really be needed on Windows, but it doesn't hurt to do it
    // anyway.
    arcsim::util::system::SharedLibrary::open(std::string(arch_conf.sys_arch.sim_opts.arcsim_lib_name));

#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
    record_profiling = ARCINT_DEFAULT_PROFILING_ENABLED;

    /* Create Address Profile Counters */
    int enabled =
      ADDRCOUNT_ENABLED_CODE |
      ADDRCOUNT_ONLY_CODE;


    prof_cnts[PROF_COUNTER_ICNTS] =
      new Prof_count("arcsim_icnts",
                     "Instruction Counts",
                     "icnt",
                     arcsim::ioc::ContextItemId::kPcFrequencyHistogram,
                     arch_conf.sys_arch.sim_opts.is_pc_freq_recording_enabled,
                     enabled | ADDRCOUNT_MAY_DERIVE_STMT_COUNT );

    prof_cnts[PROF_COUNTER_ICYCLES] =
      new Prof_count("arcsim_icycles",
                     "Instruction cycles",
                     "cycles",
                     arcsim::ioc::ContextItemId::kInstructionCyclesHistogram,
                     arch_conf.sys_arch.sim_opts.is_inst_cycle_recording_enabled,
                     enabled );

    prof_cnts[PROF_COUNTER_LIMMS] =
      new Prof_count("arcsim_limm",
                     "LIMM counts",
                     "LIMM cnt",
                     arcsim::ioc::ContextItemId::kLimmFrequencyHistogram,
                     arch_conf.sys_arch.sim_opts.is_limm_freq_recording_enabled,
                     enabled );

    prof_cnts[PROF_COUNTER_CALLS] =
      new Prof_count("arcsim_calls",
                     "Call target counts",
                     "call count",
                     arcsim::ioc::ContextItemId::kCallFrequencyHistogram,
                     arch_conf.sys_arch.sim_opts.is_call_freq_recording_enabled,
                     enabled );

    prof_cnts[PROF_COUNTER_KILLEDS] =
      new Prof_count("arcsim_killeds",
                     "Killed instruction counts",
                     "killed",
                     arcsim::ioc::ContextItemId::kKilledFrequencyHistogram,
                     arch_conf.sys_arch.sim_opts.is_killed_recording_enabled,
                     enabled );

    prof_cnts[PROF_COUNTER_DELAY_KILLEDS] =
      new Prof_count("arcsim_delay_killeds",
                     "Delay slot killed instruction counts",
                     "delay_killed",
                     arcsim::ioc::ContextItemId::kDkilledFrequencyHistogram,
                     arch_conf.sys_arch.sim_opts.is_dkilled_recording_enabled,
                     enabled );

    prof_cnts[PROF_COUNTER_ICACHE_MISS] =
      new Prof_count("arcsim_icache_miss",
                     "Icache miss counts",
                     "Icache miss",
                     arcsim::ioc::ContextItemId::kAddrIcacheMissFrequencyHistogram,
                     have_icache,
                     enabled );

    prof_cnts[PROF_COUNTER_ICACHE_MISS_CYCLES] =
      new Prof_count("arcsim_icache_miss_cycles",
                     "Icache miss cycles",
                     "Icache miss cycles",
                     arcsim::ioc::ContextItemId::kAddrIcacheMissCyclesHistogram,
                     have_icache,
                     enabled );

    prof_cnts[PROF_COUNTER_DCACHE_MISS] =
      new Prof_count("arcsim_dcache_miss",
                     "Where dcache miss counts",
                     "Where dcache miss",
                     arcsim::ioc::ContextItemId::kAddrDcacheMissFrequencyHistogram,
                     have_dcache,
                     enabled );

    prof_cnts[PROF_COUNTER_DCACHE_MISS_CYCLES] =
      new Prof_count("arcsim_dcache_miss_cycles",
                     "Memory stall cycles",
                     "Memory stalls",
                     arcsim::ioc::ContextItemId::kAddrDcacheMissCyclesHistogram,
                     have_dcache,
                     enabled );
#endif
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
    arch_conf.sys_arch.sim_opts.record_instr_history = ARCINT_DEFAULT_INSTR_HISTORY_ENABLED;
    instr_history = new Instr_history(ARCINT_DEFAULT_INSTR_HISTORY_SIZE);
#endif
#ifdef SNPS_ARCSIM
    do_early_hostlink = true;
#endif
  }
  
  void destroy()
  {
    LOG(ARCINT_LOG_DEBUG) << "[ARCINT] Destroy called";
    // If our system has been instantiated we need to destroy it properly
    //
    if (sys != 0) {
      
      // Stop any devices that may be present
      //
      sys->io_iom_device_manager.stop_devices();
      
      // Print statistics
      //
      if (print_stats_on_exit) {
        bool old_verbose = arch_conf.sys_arch.sim_opts.verbose;
        arch_conf.sys_arch.sim_opts.verbose = true;
        sys->print_stats();
        arch_conf.sys_arch.sim_opts.verbose = old_verbose;
      }
      
      if (print_simulation_time_on_exit) {
        arcsim_print_simulation_time();
      }
      
#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
      // Destroy any profile counters
      for (int pcnt=0; pcnt<N_PROFCNTS; pcnt++) {
        Prof_count* counter = prof_cnts[pcnt];
        if (counter)
          counter->destroy();
      }
#endif

      // Destroy System after simulation to clean up resources
      //
      sys->destroy_system();
      
      // Call proper destructors and clean-up
      //
      delete sys;
    }
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
    delete instr_history;
#endif
  }
  
  int version ()
  {
    return ARCINT_TRACE_VERSION;
  }
  
  const char *id ()
  {
    return QUOTEME(SNPS_PRODUCT_NAME, Version: ARCSIM_VERSION (Build: ARCSIM_BUILD_ID));
  }
  
  int is_simulator ()
  {
    int rval = ARC_IS_SIMULATOR + ARC_RUN_CAPABLE + ARC_RETURN_HALT;
    return rval;
  }
  
  int step()
  {
    sys->cpu[0]->simulation_continued();
    
    // Single-step one instruction
    //
    sys->step();
    
    sys->cpu[0]->simulation_stopped();
    return 1;
  }
  
  int run ()
  {
    // Run simulate method which will 'just do the correct thing' based on the
    // selected simulation mode.
    //
    sys->simulate();
    
    return 1;
  }
  
  int read_memory (ARC_ADDR_TYPE adr, void *buf, ARC_AMOUNT_TYPE amount, int context)
  { 
    uint32 addr;
    uint32 data = 0;
    uint32 *rbuf = (uint32*)buf;
    
    for (addr = adr; addr < adr+amount; addr += 4) {
      sys->cpu[0]->read32(addr, data);      
      *rbuf++ = data;
    }       
    return amount;
  }
  
  int write_memory(ARC_ADDR_TYPE adr, void *buf, ARC_AMOUNT_TYPE amount, int context)
  { 
    uint32 addr;
    uint32 data;
    uint32 *rbuf = (uint32*)buf;
    
    for (addr = adr; addr < adr+amount; addr += 4) {
      data = *rbuf++;
      sys->cpu[0]->write32(addr, data);      
    }
    
    return amount; 
  }
  
  int read_reg(ARC_REG_TYPE r, ARC_REG_VALUE_TYPE *value)
  { 
    if (r >= 0 && r < GPR_BASE_REGS) {
      // Read core register 'r'
      //
      *value = sys->cpu[0]->state.gprs[r];
      return 1;
    }
    
    if (r >= AUX_BASE) {
      // Read aux register 'r-AUX_BASE'
      //
      int ok = sys->cpu[0]->read_aux_register (r-AUX_BASE, (uint32*)value, false);
      return ok;
    }
    
    // Not a known register.
    //
    return 0; 
  }
  
  int write_reg(ARC_REG_TYPE r, ARC_REG_VALUE_TYPE value)
  { 
    if (r >= 0 && r < GPR_BASE_REGS) {
      // Write core register 'r'
      //
      sys->cpu[0]->state.gprs[r] = value;
      return 1;
    }
    
    if (r >= AUX_BASE) {
      // Write aux register 'r-AUX_BASE', where writes are permitted.
      // Return success or failure of write.
      //
      int ok = sys->cpu[0]->write_aux_register (r-AUX_BASE, value, false);
      return ok;
    }
    
    // Not a known register.
    //
    return 0; 
  }

  int read_banked_reg(int bank, ARC_REG_TYPE r, ARC_REG_VALUE_TYPE *value)
  {
    switch(bank) {
      case reg_bank_core: /* fall-through */
      case reg_bank_aux: {
        return read_reg(r, value);
      }
      case reg_bank_extra: {
        switch (r) {
          case ICNT: {
            arcsim_update_icnts(true, value, false);
            return 1;
          }
#ifdef CYCLE_ACC_SIM
          case SCYCLES: {
            arcsim_update_cycles(true, value, false);
            return 1;
          }
          case LAST_CYCLES: {
            arcsim_update_lcycles(true, value, false);
            return 1;
          }
#endif
        }
      }
    }
    *value = 0;
    return 0;
  }

  int write_banked_reg(int bank, ARC_REG_TYPE r, ARC_REG_VALUE_TYPE *value)
  {
    switch(bank) {
      case reg_bank_core: /* fall-through */
      case reg_bank_aux: {
        return write_reg(r, *value);
      }
      case reg_bank_extra: {
        switch (r) {
          case ICNT: {
            arcsim_update_icnts(false, value, false);
            return 1;
          }
#ifdef CYCLE_ACC_SIM
          case SCYCLES: {
            arcsim_update_cycles(false, value, false);
            return 1;
          }
          case LAST_CYCLES: {
            arcsim_update_lcycles(false, value, false);
            return 1;
          }
#endif
        }
      }
    }
    return 0;
  }

  unsigned memory_size()
  {
    // not really supported yet, we only handle it to passify regression tests
    // just return whatever size we recorded in set_memory_size()
    return mem_size;
  }

  int set_memory_size(unsigned S)
  {
    // not really supported yet, we only handle it to passify regression tests
    // we just record whatever size so we can return it in memory_size()
    mem_size = S;
    return 1;
  }

  int prepare_for_new_program (int will_download)
  {
    LOG(ARCINT_LOG_DEBUG) << "[ARCINT] Prepare for new program ...";

    arch_conf.sys_arch.sim_opts.init_mem_custom = true;
    arch_conf.sys_arch.sim_opts.init_mem_value = mem_init;

    if (!sys) {
#if SNPS_ARCSIM_LICENSING
      struct demo_limit_info * lim = arcsim_get_limit();
      bool bypass = ((lim->_lic_bypass ^ (lim->_checksum - lim->_dropdead)) == 0);
      if (!bypass) {
        if (!license_manager) {
          license_manager = LicenseManager::getInstance();
          bool res = license_manager->init(flex_license_dll, flex_license_file);
          if (!res) {
            arcsim_print(LPRINT_ERROR, "%s could not initialize licensing mechansim from %s.\n", QUOTEME(SNPS_PRODUCT_NAME), flex_license_dll);
            return 0;
          }
        }
        license_manager->checkout();
        bool have_base = license_manager->check(SNPS_LICENSE_FEATURE_BASE);
        if (!have_base) {
          arcsim_print(LPRINT_ERROR, "Licensing Failure:\n"
                                     "     A valid license for %s could not be granted.\n\n"
                                     "     The license file path considered is:\n"
                                     "          %s\n",
                       QUOTEME(SNPS_PRODUCT_NAME), flex_license_file
                       );
          return 0;
        }

        if (arcsim_mode_fast) {
          /* Fast mode requested, check license */
          bool have_fast = (license_manager->check(SNPS_LICENSE_FEATURE_TURBO) ||
                            license_manager->check(SNPS_LICENSE_FEATURE_NCAM_TURBO));
          if (!have_fast) {
            arcsim_print(LPRINT_ERROR, "Licensing Failure:\n"
                                       "     A valid license for %s Turbo mode could not be granted.\n\n"
                                       "     The license file path considered is:\n"
                                       "          %s\n",
                         QUOTEME(SNPS_PRODUCT_NAME), flex_license_file
                         );
            arcsim_mode_fast = 0;
            return 0;
          }
        }

        if (arcsim_mode_cycles) {
          /* Cycle approximate mode requested, check license */
          bool have_cycles = license_manager->check(SNPS_LICENSE_FEATURE_NCAM_TURBO);
          if (!have_cycles) {
            arcsim_print(LPRINT_ERROR, "Licensing Failure:\n"
                                       "     A valid license for %s Cycle Approximate mode could not be granted.\n\n"
                                       "     The license file path considered is:\n"
                                       "          %s\n",
                         QUOTEME(SNPS_PRODUCT_NAME), flex_license_file
                         );
            arcsim_mode_cycles = 0;
            return 0;
          }
        }
      }
#endif

      arch_conf.sys_arch.sim_opts.fast = arcsim_mode_fast;
#ifdef CYCLE_ACC_SIM
      arch_conf.sys_arch.sim_opts.cycle_sim  = arcsim_mode_cycles;
      if (arcsim_mode_cycles) {
        // Need to simulate memory models in cycle approximate mode
        arch_conf.sys_arch.sim_opts.memory_sim = true;
      }
#else
      if (arcsim_mode_cycles) {
        ARCSIM_ERROR(("Error: This is not a cycle approximate simulator.\n"));
      }
#endif

      // System does not exist yet so we instantiate a new one
      //
      
      // Read target architecture definition
      //
      arch_conf.read_architecture(arch_conf.sys_arch.sim_opts.sys_arch_file,
                                  arch_conf.sys_arch.sim_opts.print_sys_arch,
                                  arch_conf.sys_arch.sim_opts.print_arch_file); 

#ifdef SNPS_ARCSIM
      arcsim_initialize_mem_models();
#endif

      // Initialise System object for target architecture
      //
      sys = new System (arch_conf);
      
      // Create/configure System object
      //
      sys->create_system ();
      
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
      sys->cpu[0]->instr_history=instr_history;
#endif

    } else {
      // System already exists, reset it to its initial state
      //
      sys->reset_to_initial_state(will_download);
    }
    
#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
    // Initialize and clear counters
    // NOTE: This must happen AFTER the system has been instantiated so the
    //       appropriate counters and histograms have been instantiated by
    //       arcsim and can ben used.
    //
    for (int pcnt=0; pcnt<N_PROFCNTS; pcnt++) {
      prof_cnts[pcnt]->init(sys->id);
    }
    arcsim_prof_counters();
#endif
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
    /* clear instruction history */
    instr_history->clear();
#endif

    /* clear any pseudo registers */
    ARC_REG_VALUE_TYPE reg_val = 0;
    arcsim_update_icnts(false, 0, true);
#ifdef CYCLE_ACC_SIM
    arcsim_update_cycles(false, 0, true);
    arcsim_update_lcycles(false, 0, true);
#endif

    // Make sure that we stop the simulator and that 
    // the debugger gets control back whenever we hit
    // a break instruction.
    // We don't require to get control on a sleep
    // instruction, so don't stop on those (improves performance)
    arch_conf.sys_arch.sim_opts.exit_on_break = true;
    arch_conf.sys_arch.sim_opts.exit_on_sleep = false;
    
    arcsim_display_settings();

    return 1;
  }
  
private:
  
  void arcsim_print(int level, const char * format, ...)
  {
    va_list args;
    va_start (args, format);
    dbg_access->vlprint(level, format, args);
    va_end (args);
  }

  unsigned long arcsim_strtoul_internal(const char *s, char **end)
  {
    long mpy = 1;
    // Allow negative numbers as well.
    if (*s == '-') mpy = -mpy, s++;
    std::string UL;
    if (strchr(s,'_')) {
      for (const char *p = s; *p; p++)
        if (*p != '_') UL += *p;
      s = UL.c_str();
    }
    const char *z;
    std::string Z;
    if (*s == '0' && (s[1] == 'x' || s[1] == 'X') && (z = strchr(s,'z'), z)) {
      s+=2; Z = "0x";
      int len = strlen(s);
      if (len <= 8) {
        for (const char *q = s; q < z; q++) Z += *q;
        for (int i = 0; i < 8-len+1; i++) Z += '0';
        for (const char *q = z+1; *q; q++)
          // Just in case he wrote z multiple times, ignore z.
          if (isxdigit(*q)) Z += *q;
        s = Z.c_str();
      }
    }
    return ::strtoul(s,end,0)*mpy;
  }

  unsigned long arcsim_strtoul(const char *s)
  {
    return arcsim_strtoul_internal(s,0);
  }

  // Recognize suffixes "k", "K", "m" or "M".
  unsigned long arcsim_strtoul_size(const char *s)
  {
    char *end;
    unsigned long v = arcsim_strtoul_internal(s, &end);
    switch(*end) {
      case 'k':
      case 'K': return v * 1024;
      case 'm':
      case 'M': return v * 1024 * 1024;
      default:
        return v;
    }
  }

  bool arcsim_power_of_two(unsigned u, int &power) {
    if (u == 0) return false;	// Watch for bad boundary values.
    unsigned bits = 0; power = -1;
    while (u) { if (u&1) bits++; u >>= 1; power++; }
    return bits == 1;
  }	
  
  int arcsim_log2(unsigned u) {
    int res = 0;
    while (u >>=1) ++res;
    return res;
  }

  unsigned arcsim_ensure_zero_mod_4(unsigned x) {
    while (x % 4) x++; return x;
  }

  void arcsim_make_cache(bool icache, const char *value)
  {
    unsigned size, line_size, ways; char attrib[80];
    const char *spec = value;
  AGAIN: ;
    int items = sscanf(spec,"%i,%i,%i,%s",&size,&line_size,&ways,attrib);
    if (items == 3) strcpy(attrib, "a");
    else if (items != 4) {
      const char *substitute = "2048,16,2,a";
      ARCSIM_ERROR(("Invalid cache specification: '%s'; '%s' substituted.\n",spec,
                    substitute));
      // Build something anyway to prevent crashes.
      spec = substitute;
      goto AGAIN;
    }
    int dummy;
    while (!arcsim_power_of_two(size,dummy)) size++;
    while (!arcsim_power_of_two(line_size,dummy)) line_size++;
    while (!arcsim_power_of_two(ways,dummy)) ways++;
    
    LOG(ARCINT_LOG_DEBUG) << (icache?"I":"D") << " cache: "
                          << "Cache size " << size
                          << ", line size " << line_size
                          << ", ways " << ways;
                          
    for (const char *cp = attrib; *cp; cp++) {
      switch(*cp) {
      case 'a': LOG(ARCINT_LOG_DEBUG) << "  random"; break;
      case 'o': LOG(ARCINT_LOG_DEBUG) << "  round_robin"; break;
      case 'c': LOG(ARCINT_LOG_DEBUG) << "  code RAM"; break;
      case '1': LOG(ARCINT_LOG_DEBUG) << "  single wp"; break;
      default:
        LOG(ARCINT_LOG_DEBUG) << "Invalid cache specification: unknown attribute: "
                              << *cp;
      }
    }
    /* Construct arcsim cache specification */
    char * cache_spec;
    if (icache) {
      have_icache=true;
      cache_spec=icache_spec;
    } else {
      // assume D-cache
      have_dcache=true;
      cache_spec=dcache_spec;
    }
    /* Since arcsim only support Random replacement policy, we ignore all
     * the atrributes and only use the cache size, line size and number of ways
     * that were passed.
     */
    snprintf(cache_spec, SPEC_SIZE, "CACHE    AV2_CACHE_%c  %u %u %u %c %u %u %u",
             icache?'I':'D', size, arcsim_log2(line_size), ways, 'R', 32, 1, 1);

    /* enable cache profile counters
     * note: just enable them unconditionally, any consistency checks
     * one whether cycle approximate mode is enabled etc. will be done in
     * arcsim_prof_counters
     */
    arch_conf.sys_arch.sim_opts.is_cache_miss_recording_enabled = true;
    arch_conf.sys_arch.sim_opts.is_cache_miss_cycle_recording_enabled= true;
#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
    arcsim_prof_counters();
#endif
  }

#ifdef SNPS_ARCSIM
  void arcsim_initialize_mem_models()
  {
    // Ok, this is really kinda messy, but it's the best way to do things
    // right now. Once we get a better method of adding Caches and CCMs
    // on the fly in arcsim, this should be completely redone.
#define ST_CORE         0x0010  // Core
    CoreArch* core_arch = arch_conf.core_list.front();
    int level=ST_CORE;
    if (have_icache && !core_arch->icache.is_configured) {
      LOG(ARCINT_LOG_DEBUG) << "ICache spec: " << icache_spec;
      arch_conf.create_new_cache(icache_spec);
      arch_conf.add_cache(level, core_arch, (char*)"ADD_CACHE     AV2_CACHE_I I");
    }
    if (have_dcache && !core_arch->dcache.is_configured) {
      LOG(ARCINT_LOG_DEBUG) << "DCache spec: " << dcache_spec;
      arch_conf.create_new_cache(dcache_spec);
      arch_conf.add_cache(level, core_arch, (char*)"ADD_CACHE     AV2_CACHE_D D");
    }
    /* update state */
    have_icache = core_arch->icache.is_configured;
    have_dcache = core_arch->dcache.is_configured;

    if (iccm_size>0 && !core_arch->iccm.is_configured) {
      char iccm_spec[SPEC_SIZE];
      snprintf(iccm_spec, SPEC_SIZE, "CCM  AV2_CCM_I %u %u %u %u",
               iccm_base, iccm_size, 32, 1);
      LOG(ARCINT_LOG_DEBUG) << "ICCM spec: " << iccm_spec;
      arch_conf.create_new_spad(iccm_spec);
      arch_conf.add_spad(level, core_arch, (char*)"ADD_CCM     AV2_CCM_I I");
    }

    if (dccm_size>0 && !core_arch->dccm.is_configured) {
      char dccm_spec[SPEC_SIZE];
      snprintf(dccm_spec, SPEC_SIZE, "CCM  AV2_CCM_D %u %u %u %u",
               dccm_base, dccm_size, 32, 1);
      LOG(ARCINT_LOG_DEBUG) << "DCCM spec: " << dccm_spec;
      arch_conf.create_new_spad(dccm_spec);
      arch_conf.add_spad(level, core_arch, (char*)"ADD_CCM     AV2_CCM_D D");
    }
    arch_conf.sys_arch.sim_opts.memory_sim =
      arch_conf.sys_arch.sim_opts.memory_sim  ||
      core_arch->icache.is_configured         ||
      core_arch->dcache.is_configured         ||
      core_arch->iccm.is_configured           ||
      core_arch->dccm.is_configured;
  }
#endif

#ifdef SNPS_ARCSIM_LICENSING
  struct demo_limit_info * arcsim_get_limit()
  {
    return &limits;
  }
#endif

#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
  void arcsim_prof_counters()
  {
    if ((!sys) || (!dbg_access)) return;

    /* consistency checks */
    if (!record_profiling) {
      /* requested no profiling, so don't disable all profile counting
       */ 
      arch_conf.sys_arch.sim_opts.is_pc_freq_recording_enabled = false;
      arch_conf.sys_arch.sim_opts.is_limm_freq_recording_enabled = false;
      arch_conf.sys_arch.sim_opts.is_call_freq_recording_enabled = false;
      arch_conf.sys_arch.sim_opts.is_killed_recording_enabled = false;
      arch_conf.sys_arch.sim_opts.is_dkilled_recording_enabled = false;
      arch_conf.sys_arch.sim_opts.is_inst_cycle_recording_enabled = false;
      arch_conf.sys_arch.sim_opts.is_cache_miss_cycle_recording_enabled = false;
      arch_conf.sys_arch.sim_opts.is_cache_miss_recording_enabled = false;
    }
    if (!arcsim_mode_cycles) {
      /* not running in cycle approximate mode, so ignore any request
       * for cycle approximate profile counters
       */
      arch_conf.sys_arch.sim_opts.is_inst_cycle_recording_enabled = false;
      arch_conf.sys_arch.sim_opts.is_cache_miss_cycle_recording_enabled = false;
    }

    if (! (have_icache || have_dcache)) {
      /* no caches were configured, so ignore any request for cache profile counters
       */
      arch_conf.sys_arch.sim_opts.is_cache_miss_recording_enabled = false;
      arch_conf.sys_arch.sim_opts.is_cache_miss_cycle_recording_enabled= false;
    }

    /* add counters if needed, if we already added them previously
     * that's ok, the debugger will just ignore the new add request.
     */
    if (arch_conf.sys_arch.sim_opts.is_pc_freq_recording_enabled) {
      dbg_access->add_addr_count(prof_cnts[PROF_COUNTER_ICNTS]);
    }
    if (arch_conf.sys_arch.sim_opts.is_inst_cycle_recording_enabled) {
      dbg_access->add_addr_count(prof_cnts[PROF_COUNTER_ICYCLES]);
    }
    if (arch_conf.sys_arch.sim_opts.is_limm_freq_recording_enabled) {
      dbg_access->add_addr_count(prof_cnts[PROF_COUNTER_LIMMS]);
    }
    if (arch_conf.sys_arch.sim_opts.is_call_freq_recording_enabled) {
      dbg_access->add_addr_count(prof_cnts[PROF_COUNTER_CALLS]);
    }
    if (arch_conf.sys_arch.sim_opts.is_killed_recording_enabled) {
      dbg_access->add_addr_count(prof_cnts[PROF_COUNTER_KILLEDS]);
    }
    if (arch_conf.sys_arch.sim_opts.is_dkilled_recording_enabled) {
      dbg_access->add_addr_count(prof_cnts[PROF_COUNTER_DELAY_KILLEDS]);
    }

    if (have_icache) {
      if (arch_conf.sys_arch.sim_opts.is_cache_miss_recording_enabled) {
        dbg_access->add_addr_count(prof_cnts[PROF_COUNTER_ICACHE_MISS]);
      }
      if (arch_conf.sys_arch.sim_opts.is_cache_miss_cycle_recording_enabled) {
        dbg_access->add_addr_count(prof_cnts[PROF_COUNTER_ICACHE_MISS_CYCLES]);
      }
    }

    if (have_dcache) {
      if (arch_conf.sys_arch.sim_opts.is_cache_miss_recording_enabled) {
        dbg_access->add_addr_count(prof_cnts[PROF_COUNTER_DCACHE_MISS]);
      }
      if (arch_conf.sys_arch.sim_opts.is_cache_miss_cycle_recording_enabled) {
        dbg_access->add_addr_count(prof_cnts[PROF_COUNTER_DCACHE_MISS_CYCLES]);
      }
    }
  }
#endif

  void arcsim_version()
  {
    ARCSIM_INFO(("********************\n"));
    ARCSIM_INFO((id()));
    ARCSIM_INFO(("\n"));
    ARCSIM_INFO((QUOTEME(REV1: ARCSIM_REV1)"\n"));
    ARCSIM_INFO((QUOTEME(REV2: ARCSIM_REV2)"\n"));
    ARCSIM_INFO(("********************\n"));
  }

  void arcsim_display_settings(bool show_disabled = false)
  {
    if (show_disabled) {
      ARCSIM_PRINT(("Cycle approximate simulation mode %s.\n",
                    arch_conf.sys_arch.sim_opts.cycle_sim ? "enabled":"disabled"));
      ARCSIM_PRINT(("Fast (JIT DBT) simulation mode %s.\n",
                    arch_conf.sys_arch.sim_opts.fast ? "enabled":"disabled"));
    } else {
      if (arch_conf.sys_arch.sim_opts.cycle_sim) {
        ARCSIM_PRINT(("Cycle approximate simulation mode enabled.\n"));
      }
      if (arch_conf.sys_arch.sim_opts.fast) {
        ARCSIM_PRINT(("Fast (JIT DBT) simulation mode enabled.\n"));
      }
    }
        
  }

  void arcsim_print_simulation_time()
  {
    double  total_sim_time = sys->cpu[0]->exec_time.get_elapsed_seconds();
    uint64  total_insns = sys->cpu[0]->instructions();
    fprintf (stderr, "==============\n");
    fprintf (stderr, "Executed total of %lld instructions in %0.6f seconds (%0.2f MIPS)\n",
             total_insns,
             total_sim_time,
             total_insns*1.0e-6/total_sim_time);
    fprintf (stderr, "==============\n");
  }
  
  void arcsim_update_icnts(bool read, ARC_REG_VALUE_TYPE *value, bool reset)
  {
    static uint64 last_icnts = 0;
    static uint64 discard = 0;
    uint64 cur_icnts = sys->cpu[0]->instructions();
    if (reset) {
      last_icnts = cur_icnts;
      discard = 0;
      extra_regs_val[ICNT] = 0;
      return;
    }
    if (read) {
      uint64 elapsed = cur_icnts - last_icnts;
      uint64 count = extra_regs_val[ICNT] + elapsed;
      extra_regs_val[ICNT] += elapsed;
      *value = extra_regs_val[ICNT];
      if (count >> 32) {
        discard += (count -  (uint64)*value);
        ARCSIM_PRINT(("INSTRUCTION COUNTER HAS OVERFLOWED.\n"));
        ARCSIM_PRINT(("Cumulative icnt discard is %llu.\n", discard));
      }
    } else {
      extra_regs_val[ICNT] = *value;
    }
    last_icnts = cur_icnts;
  }

#ifdef CYCLE_ACC_SIM
  void arcsim_update_cycles(bool read, ARC_REG_VALUE_TYPE *value, bool reset)
  {
    static uint64 last_cycles = 0;
    static uint64 discard = 0;
    uint64 cur_cycles = sys->cpu[0]->cnt_ctx.cycle_count.get_value();
    if (reset) {
      last_cycles = cur_cycles;
      discard = 0;
      extra_regs_val[SCYCLES] = 0;
      return;
    }
    if (read) {
      uint64 elapsed = cur_cycles - last_cycles;
      uint64 count = extra_regs_val[SCYCLES] + elapsed;
      extra_regs_val[SCYCLES] += elapsed;
      *value = extra_regs_val[SCYCLES];
      if (count >> 32) {
        discard += (count -  (uint64)*value);
        ARCSIM_PRINT(("CYCLE COUNTER HAS OVERFLOWED.\n"));
        ARCSIM_PRINT(("Cumulative cycle discard is %llu.\n", discard));
      }
    } else {
      extra_regs_val[SCYCLES] = *value;
    }
    last_cycles = cur_cycles;
  }

  void arcsim_update_lcycles(bool read, ARC_REG_VALUE_TYPE *value, bool reset)
  {
    static uint64 last_cycles = 0;
    uint64 cur_cycles = sys->cpu[0]->cnt_ctx.cycle_count.get_value();
    if (reset) {
      last_cycles = cur_cycles;
      extra_regs_val[LAST_CYCLES] = 0;
      return;
    }
    if (read) {
      uint64 elapsed = cur_cycles - last_cycles;
      extra_regs_val[LAST_CYCLES] = elapsed;
      *value = extra_regs_val[LAST_CYCLES];
    } else {
      extra_regs_val[LAST_CYCLES] = *value;
    }
    last_cycles = cur_cycles;
  }
#endif

#define key_is(key, x) (strcmp(key,x)==0)
#define prop_is(x) key_is(property, QUOTEME(x))
#define sim_prop_is(x) key_is(property, QUOTEME(SNPS_PROD_STR_LWC(x)))

  /* ArcSim ISA properties.
   *
   * These properties correspond to ArcSim isa command-line options.
   * Generally the property name takes the form of 'arcsim_isa_'
   * followed by the isa sub-option name. For example the property
   * arcsim_isa_a6k correspond to the '-o -a6k' command-line option.
   */
  int arcsim_isa_props(const char *property, const char *value)
  {
    if (value) {
      int v = arcsim_strtoul(value);
      
      if ( (sim_prop_is(isa_a6k) ) ||
           (sim_prop_is(isa_av2) ) 
           ) {
        if (v != 0) {
          arch_conf.sys_arch.isa_opts.set_isa(IsaOptions::kIsaA6K);
        }
      } else if (sim_prop_is(isa_pc_size) ) {
        arch_conf.sys_arch.isa_opts.pc_size       = v;
      } else if (sim_prop_is(isa_lpc_size) ) {
        arch_conf.sys_arch.isa_opts.lpc_size      = v;
      } else if (sim_prop_is(isa_addr_size) ) {
        arch_conf.sys_arch.isa_opts.addr_size     = v;
        // If 'addr_size' is 16 bits we need to also lower 'page_arch_offset_bits'
        // from 11UL (i.e. 8k) to 7UL (i.e. 512b) so the internal page size
        // used by ArcSim is 'small' enough so CCM region mapping works correctly.
        //
        if (arch_conf.sys_arch.isa_opts.addr_size == 16) { 
          arch_conf.sys_arch.sim_opts.page_size_log2 = PageArch::k512BPageSizeLog2;
          LOG(ARCINT_LOG_DEBUG) << "Changing default internal memory chunk size from "
                                << "'8k' to '512b' due to address size '16'.";
        }
      } else if (sim_prop_is(isa_shift_option) ) {
        arch_conf.sys_arch.isa_opts.shift_option  = ((v & 2) != 0);
        arch_conf.sys_arch.isa_opts.shas_option   = ((v & 1) != 0);  
      } else if (sim_prop_is(isa_swap_option) ) {
        arch_conf.sys_arch.isa_opts.swap_option   = (v != 0);
      } else if (sim_prop_is(isa_bitscan_option) ) {
        arch_conf.sys_arch.isa_opts.norm_option   = (v != 0); 
        arch_conf.sys_arch.isa_opts.ffs_option    = (v != 0);
      } else if (sim_prop_is(isa_mpy_option) ) {
        if (key_is(value,"none")) {
          arch_conf.sys_arch.isa_opts.mpy16_option = false;
          arch_conf.sys_arch.isa_opts.mpy32_option = false;
          arch_conf.sys_arch.isa_opts.mpy_lat_option = 0;
        } else if (key_is(value,"w")) {
          arch_conf.sys_arch.isa_opts.mpy16_option = true;
          arch_conf.sys_arch.isa_opts.mpy32_option = false;
          arch_conf.sys_arch.isa_opts.mpy_fast = true;
          arch_conf.sys_arch.isa_opts.mpy_lat_option = 1;
        } else if (key_is(value,"wlh1")) {
          arch_conf.sys_arch.isa_opts.mpy16_option = true;
          arch_conf.sys_arch.isa_opts.mpy32_option = true;
          arch_conf.sys_arch.isa_opts.mpy_fast = true;
          arch_conf.sys_arch.isa_opts.mpy_lat_option = 1;
        } else if (key_is(value,"wlh2")) {
          arch_conf.sys_arch.isa_opts.mpy16_option = true;
          arch_conf.sys_arch.isa_opts.mpy32_option = true;
          arch_conf.sys_arch.isa_opts.mpy_fast = true;
          arch_conf.sys_arch.isa_opts.mpy_lat_option = 2;
        } else if (key_is(value,"wlh3")) {
          arch_conf.sys_arch.isa_opts.mpy16_option = true;
          arch_conf.sys_arch.isa_opts.mpy32_option = true;
          arch_conf.sys_arch.isa_opts.mpy_fast = true;
          arch_conf.sys_arch.isa_opts.mpy_lat_option = 3;
        } else if (key_is(value,"wlh4")) {
          arch_conf.sys_arch.isa_opts.mpy16_option = true;
          arch_conf.sys_arch.isa_opts.mpy32_option = true;
          arch_conf.sys_arch.isa_opts.mpy_fast = true;
          arch_conf.sys_arch.isa_opts.mpy_lat_option = 4;
        } else if (key_is(value,"wlh5")) {
          arch_conf.sys_arch.isa_opts.mpy16_option = true;
          arch_conf.sys_arch.isa_opts.mpy32_option = true;
          arch_conf.sys_arch.isa_opts.mpy_lat_option = 9;
        } else if (key_is(value,"mul64")) {
          arch_conf.sys_arch.isa_opts.mul64_option = true;
        }
      } else if (sim_prop_is(isa_div_rem_option) ) {
        arch_conf.sys_arch.isa_opts.div_rem_option   = (v != 0);
      } else if (sim_prop_is(isa_code_density_option) ) {
        arch_conf.sys_arch.isa_opts.density_option   = (v != 0);
      } else if (sim_prop_is(isa_atomic_option) ) {
        arch_conf.sys_arch.isa_opts.atomic_option    = v;
      } else if (sim_prop_is(isa_num_actionpoints) ) {
        arch_conf.sys_arch.isa_opts.num_actionpoints = v;
      } else if (sim_prop_is(isa_aps_feature) ) {
        arch_conf.sys_arch.isa_opts.aps_full         = (v != 0);
      } else if (sim_prop_is(isa_has_dmp_peripheral) ) {
        arch_conf.sys_arch.isa_opts.has_dmp_peripheral = (v != 0);
      } else if (sim_prop_is(isa_dc_uncached_region) ) {
        arch_conf.sys_arch.isa_opts.dc_uncached_region = (v != 0);
      } else if (sim_prop_is(isa_enable_timer_0) ) {
        arch_conf.sys_arch.isa_opts.has_timer0       = (v != 0);
      } else if (sim_prop_is(isa_enable_timer_1) ) {
        arch_conf.sys_arch.isa_opts.has_timer1       = (v != 0);
      } else if (sim_prop_is(isa_host_timer) ) {
        arch_conf.sys_arch.isa_opts.use_host_timer   = (v != 0);
      } else if (sim_prop_is(isa_fmt14) ) {
        arch_conf.sys_arch.isa_opts.new_fmt_14       = (v != 0);
      } else if (sim_prop_is(isa_ic_feature_level) ) {
        arch_conf.sys_arch.isa_opts.ic_feature       = v;
      } else if (sim_prop_is(isa_dc_feature_level) ) {
        arch_conf.sys_arch.isa_opts.dc_feature       = v;
      } else if (sim_prop_is(isa_rgf_num_regs) ) {
        arch_conf.sys_arch.isa_opts.only_16_regs     = (v == 16);
      } else if (sim_prop_is(isa_rgf_wr_ports) ||
                 sim_prop_is(isa_rgf_num_wr_ports)
                 ) {
        if ((v != 1) && (v != 2)) {
          v = DEFAULT_RF_4PORT;
          ARCSIM_ERROR(("The number of register file write ports must be 1 or 2, substituted %d\n", v));
        }
        arch_conf.sys_arch.isa_opts.rf_4port         = (v == 2);
      } else if (sim_prop_is(isa_a700) ) {
        if (v!=0) {
          arch_conf.sys_arch.isa_opts.set_isa(IsaOptions::kIsaA700);
        }
      } else if (sim_prop_is(isa_a600) ) {
        if (v!=0) {
          arch_conf.sys_arch.isa_opts.set_isa(IsaOptions::kIsaA600);
        }
      } else if (sim_prop_is(isa_sat) ) {
        arch_conf.sys_arch.isa_opts.sat_option       = (v != 0);
      } else if (sim_prop_is(isa_mul64) ) {
        arch_conf.sys_arch.isa_opts.mul64_option     = (v != 0);
      } else if (sim_prop_is(isa_fpx) ) {
        arch_conf.sys_arch.isa_opts.fpx_option       = (v != 0);
      } else if (sim_prop_is(isa_intvbase_preset) ) {
        arch_conf.sys_arch.isa_opts.intvbase_preset = v;
      } else if (sim_prop_is(isa_big_endian) ) {
        arch_conf.sys_arch.sim_opts.big_endian       = (v != 0);
      } else if (sim_prop_is(isa_turbo_boost) ) {
        arch_conf.sys_arch.isa_opts.turbo_boost      = (v != 0);
      } else if (sim_prop_is(isa_smart_stack_entries) ) {
        arch_conf.sys_arch.isa_opts.smart_stack_size = v;
      } else if (sim_prop_is(isa_number_of_interrupts) ) {
        arch_conf.sys_arch.isa_opts.num_interrupts = v;
      } else if (sim_prop_is(isa_ic_disable_on_reset) ) {
        arch_conf.sys_arch.isa_opts.ic_disable_on_reset = (v != 0);
      } else if (sim_prop_is(isa_timer_0_int_level) ) {
        arch_conf.sys_arch.isa_opts.timer_0_int_level = v;
      } else if (sim_prop_is(isa_timer_1_int_level) ) {
        arch_conf.sys_arch.isa_opts.timer_1_int_level = v;
      } else if (sim_prop_is(isa_ifq_size) ) {
        arch_conf.sys_arch.isa_opts.ifq_size = v;

      } else {
        return 0;
      }
    } else {
      return 0;
    }
    
    if (arch_conf.sys_arch.isa_opts.is_isa_a6k()) {
      arch_conf.sys_arch.isa_opts.mul64_option   = false;
      arch_conf.sys_arch.isa_opts.sat_option     = false;
    } else if (arch_conf.sys_arch.isa_opts.is_isa_a600()) {
      arch_conf.sys_arch.isa_opts.div_rem_option = false;
      arch_conf.sys_arch.isa_opts.density_option = false;
      arch_conf.sys_arch.isa_opts.ffs_option     = false;
      arch_conf.sys_arch.isa_opts.shas_option    = false;
    } else {
      arch_conf.sys_arch.isa_opts.div_rem_option = false;
      arch_conf.sys_arch.isa_opts.density_option = false;
      arch_conf.sys_arch.isa_opts.ffs_option     = false;
      arch_conf.sys_arch.isa_opts.shas_option    = false;
    }

    return 1;
  }
  
  /* ArcSim specific properties.
   *
   * These properties correspond to arcsim specific options.
   */
  int arcsim_props(const char *property, const char *value)
  {
    if (sim_prop_is(profiling) ) {
#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
      if (value) {
        record_profiling = (arcsim_strtoul(value)!=0);
        arcsim_prof_counters();
      }
#endif
    } else if (sim_prop_is(instr_history_size) ) {
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
      if (value) {
        int sz = arcsim_strtoul(value);
        instr_history->resize(sz);
      }
#endif
    } else if (sim_prop_is(instr_history) ) {
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
      if (value) {
        bool record = (arcsim_strtoul(value)!=0);
        bool old_record = arch_conf.sys_arch.sim_opts.record_instr_history;
        arch_conf.sys_arch.sim_opts.record_instr_history = record;
        if (record && !old_record) {
          /* (re-)enabling instruction history, clear any previous history */
          instr_history->clear();
        }
      }
#endif

      /* some miscellanious properties, amonst other sueful for debugging arcsim */
    } else if (sim_prop_is(print_simulation_time_on_exit) ) {
      if (value) print_simulation_time_on_exit = (arcsim_strtoul(value)!=0);
    } else if (sim_prop_is(print_simulation_time) ) {
      arcsim_print_simulation_time();
    } else if (sim_prop_is(sim_period) ) {
      if (value) {
        arch_conf.sys_arch.sim_opts.sim_period = arcsim_strtoul(value);
        sim_period_set=true;
      }
    } else if (sim_prop_is(print_stats) ) {
      if (value && (arcsim_strtoul(value)!=0)) {
        bool old_verbose = arch_conf.sys_arch.sim_opts.verbose;
        arch_conf.sys_arch.sim_opts.verbose = true;
        if (sys != 0) {
          sys->print_stats();
        }
        arch_conf.sys_arch.sim_opts.verbose = old_verbose;
      }
    } else if (sim_prop_is(print_stats_on_exit) ) {
      if (value) print_stats_on_exit = (arcsim_strtoul(value)!=0);
    } else if (sim_prop_is(display_settings) ) {
      if (value && (arcsim_strtoul(value)!=0)) {
        arcsim_display_settings(true);
      }
    } else if (sim_prop_is(version) ) {
      if (value && (arcsim_strtoul(value)!=0)) {
        arcsim_version();
      }
    } else if (sim_prop_is(do_early_hostlink) ) {
#if SNPS_ARCSIM
      if (value) {
        arch_conf.sys_arch.sim_opts.do_early_hostlink =
          do_early_hostlink = (arcsim_strtoul(value)!=0);
      }
#endif

      /* Licensing related properties */
    } else if (sim_prop_is(flex_license_dll) ) {
      if (value) {
        flex_license_dll = strdup(value);
        LOG(ARCINT_LOG_DEBUG) << "[ARCINT] flex_license_dll=" << flex_license_dll;
      }
    } else if (sim_prop_is(flex_license_file) ) {
      if (value) {
        flex_license_file = strdup(value);
        LOG(ARCINT_LOG_DEBUG) << "[ARCINT] flex_license_file=" << flex_license_file;
      }

    } else {
      return 0;
    }
    
    LOG(ARCINT_LOG_DEBUG) << "[ARCINT] Handled "
                          << QUOTEME(SNPS_PRODUCT_NAME)
                          << " property '"
                          << property
                          << "' '"
                          << ((value) ? value : "*no value*")
                          << "'" ;

    return 1;
  }

  /* ArcSim specific command properties.
   *
   * These properties correspond to ArcSim command-line options.
   * Generally the property name takes the form of 'arcsim_'
   * followed by the command-line options's long form. For example
   * the property arcsim_fast correspond to the --fast command-line
   * option.
   */
  int arcsim_cmd_props(const char *property, const char *value)
  {
    if (sim_prop_is(arch) ) {
      if (value) arch_conf.sys_arch.sim_opts.sys_arch_file = value;
    } else if (sim_prop_is(isa) ) {
      if (value) arch_conf.sys_arch.sim_opts.isa_file = value;
    } else if (sim_prop_is(trace) ) {
      if (value) arch_conf.sys_arch.sim_opts.trace_on = (arcsim_strtoul(value)!=0);
    } else if (sim_prop_is(trace-output) ) {
      if (value) {
        arch_conf.sys_arch.sim_opts.redir_inst_trace_output = true;
        arch_conf.sys_arch.sim_opts.inst_trace_file = value;
        arch_conf.sys_arch.sim_opts.rinst_trace_fd = open(value, O_CREAT | O_TRUNC | O_RDWR, 0666);
        if (arch_conf.sys_arch.sim_opts.rinst_trace_fd == -1) {
          LOG(LOG_ERROR) << "Could not open instruction trace redirection file."; 
          arch_conf.sys_arch.sim_opts.redir_inst_trace_output = false;
        }
        LOG(LOG_INFO) << "Redirecting trace output to '" << arch_conf.sys_arch.sim_opts.inst_trace_file <<"'";
        
      }
    } else if (sim_prop_is(cycle) ) {
      if (value) {
        arcsim_mode_cycles = (arcsim_strtoul(value)!=0);
        arch_conf.sys_arch.sim_opts.is_inst_cycle_recording_enabled = arcsim_mode_cycles;
#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
        arcsim_prof_counters();
#endif
      }
    } else if (sim_prop_is(fast) ) {
      if (value) arcsim_mode_fast = (arcsim_strtoul(value)!=0);
    } else if (sim_prop_is(fast-num-threads) ) {
      if (value) {
        int num_worker_threads = arcsim_strtoul(value);
        if (num_worker_threads <= 0)
          num_worker_threads = 1;
        arch_conf.sys_arch.sim_opts.fast_num_worker_threads = (size_t)num_worker_threads;
      }
    } else if (sim_prop_is(fast-trans-mode) ) {
      if (value) {
        if (key_is(value, "bb"))   arch_conf.sys_arch.sim_opts.fast_trans_mode = kCompilationModeBasicBlock;
        if (key_is(value, "page")) arch_conf.sys_arch.sim_opts.fast_trans_mode = kCompilationModePageControlFlowGraph;
      }
    } else if (sim_prop_is(fast-thresh) ) {
      if (value) arch_conf.sys_arch.sim_opts.hotspot_threshold = arcsim_strtoul(value);
    } else if (sim_prop_is(fast-trace-size) ) {
      if (value) {
        int trace_interval_size = arcsim_strtoul(value);
        if (trace_interval_size <= 0)
          trace_interval_size = 1;
        arch_conf.sys_arch.sim_opts.trace_interval_size = trace_interval_size;
      } 
    } else if (sim_prop_is(emt)) {
      if (value) arch_conf.sys_arch.sim_opts.emulate_traps = (arcsim_strtoul(value)!=0);
    } else if (sim_prop_is(profile) ) {
      if (value) arch_conf.sys_arch.sim_opts.show_profile = (arcsim_strtoul(value)!=0);
      
    } else if (sim_prop_is(eia-lib) ) {
      if (value) {
        arch_conf.sys_arch.sim_opts.is_eia_enabled = true;
        arch_conf.sys_arch.sim_opts.eia_library_list.insert(value);
      }
    } else if (sim_prop_is(mem-init) ) {
      if (value) mem_init = arcsim_strtoul(value);

    } else if (arcsim_isa_props(property, value)) {
      
    } else {
      return 0;
    }    
    
    LOG(ARCINT_LOG_DEBUG) << "[ARCINT] Handled "
                          << QUOTEME(SNPS_PRODUCT_NAME)
                          << " property '"
                          << property
                          << "' '"
                          << ((value) ? value : "*no value*")
                          << "'" ;
    
    return 1;
  }
  
public:
  /* ArcSim properties.
   *
   * These are general properties that may be passed from MDB
   * to any target, including ArcSim.
   */
  int process_property (const char *property, const char *value)
  {
    if (prop_is(deadbeef)) {
      if (value && (arcsim_strtoul(value)!=0)) {
        mem_init = 0xdeadbeef;
      }
    
    } else if (prop_is(download_complete)) {
      if (value && (arcsim_strtoul(value)!=0)) {
        sys->cpu[0]->clear_cpu_counters ();
      }

    } else if (prop_is(run_cycles)) {
      int ival = value ? arcsim_strtoul(value) : 0;
      if (ival > 0 && !sim_period_set) {
        arch_conf.sys_arch.sim_opts.sim_period = ival;
      }

    } else if (prop_is(gdebug)) {
      int level = arcsim_strtoul(value);
      switch (level) {
        case 0:  { arcsim::util::Log::reportingLevel = LOG_DEBUG;  break; }      
        case 1:  { arcsim::util::Log::reportingLevel = LOG_DEBUG1; break; }
        case 2:  { arcsim::util::Log::reportingLevel = LOG_DEBUG2; break; }
        case 3:  { arcsim::util::Log::reportingLevel = LOG_DEBUG3; break; }
        default: { arcsim::util::Log::reportingLevel = LOG_DEBUG4; break; }
      }
    } else if (prop_is(gverbose)) {
      if (value) arch_conf.sys_arch.sim_opts.verbose = verbose = (arcsim_strtoul(value)!=0);

    } else if (prop_is(_hl_blockedPeek)) {
#ifdef SNPS_ARCSIM
      if (value) {
        arch_conf.sys_arch.sim_opts.do_early_hostlink = do_early_hostlink;
        arch_conf.sys_arch.sim_opts.early_hostlink = arcsim_strtoul(value);
      }
#endif

      /* 
       * Profiling related properties, for ISS compatibility
       */
    } else if (prop_is(icnts)) {
#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
      if (value) {
        bool record = (arcsim_strtoul(value)!=0);
        arch_conf.sys_arch.sim_opts.is_pc_freq_recording_enabled = record;
        // Toggle this on icnts; no separate limm cnts or call cnts boolean.
        arch_conf.sys_arch.sim_opts.is_limm_freq_recording_enabled = record;
        arch_conf.sys_arch.sim_opts.is_call_freq_recording_enabled = record;
        record_profiling |= record;
        arcsim_prof_counters();
      }
#endif
    } else if (prop_is(killeds)) {
#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
      if (value) {
        bool record = (arcsim_strtoul(value)!=0);
        arch_conf.sys_arch.sim_opts.is_killed_recording_enabled = record;
        record_profiling |= record;
        arcsim_prof_counters();
      }
#endif
    } else if (prop_is(delay_killeds)) {
#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
      if (value) {
        bool record = (arcsim_strtoul(value)!=0);
        arch_conf.sys_arch.sim_opts.is_dkilled_recording_enabled = record;
        record_profiling |= record;
        arcsim_prof_counters();
      }
#endif
    } else if (prop_is(profiling_enabled) ) {
#ifdef SNPS_ARCSIM_ENABLE_ADDR_COUNT
      if (value) {
        record_profiling = (arcsim_strtoul(value)!=0);
        arcsim_prof_counters();
      }
#endif

      /* 
       * Instruction history related properties, for ISS compatibility
       */
    } else if ((prop_is(maxlastpc)) ||
               (prop_is(trace_buffer_entries))
               ) {
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
      if (value) {
        unsigned sz = arcsim_strtoul(value);
        instr_history->resize((int)sz);
      }
#endif
    } else if (prop_is(trace_enabled)) {
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
        if (value) arch_conf.sys_arch.sim_opts.record_instr_history = (arcsim_strtoul(value)!=0);
#endif

    } else if (prop_is(trace_clear)) {
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
      instr_history->clear();
#endif

      /* 
       * Cycle approximate simulation properties, for ISS compatibility
       */
    } else if (prop_is(cycles) ) {
      if (value) {
        arcsim_mode_cycles = (arcsim_strtoul(value)!=0);
        arch_conf.sys_arch.sim_opts.is_inst_cycle_recording_enabled = arcsim_mode_cycles;
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
        arcsim_prof_counters();
#endif
      }
      
    } else if (!strcmp(property, "arcsim_memory_sim") ) {
      arch_conf.sys_arch.sim_opts.memory_sim = (arcsim_strtoul(value)!=0);


      /*
       * Cache configuration properties
       */
    } else if (prop_is(icache)) {
      // -icache=size,line_size,ways,method
      if (value) arcsim_make_cache(true, value);
    } else if (prop_is(dcache)) {
      // -icache=size,line_size,ways,method
      if (value) arcsim_make_cache(false, value);

      /*
       * CCM configuration properties
       */
    } else if ((prop_is(ldst_ram_base)) ||
               (prop_is(dccm_base))) {
      if (value) dccm_base = arcsim_ensure_zero_mod_4(arcsim_strtoul(value));
    } else if ((prop_is(ldst_ram_size)) ||
               (prop_is(dccm_size))) {
      if (value) dccm_size = arcsim_ensure_zero_mod_4(arcsim_strtoul_size(value));
    } else if (prop_is(iccm_base)) {
      if (value) iccm_base = arcsim_ensure_zero_mod_4(arcsim_strtoul(value));
    } else if (prop_is(iccm_size)) {
      if (value) iccm_size = arcsim_ensure_zero_mod_4(arcsim_strtoul_size(value));

    } else if (arcsim_props(property, value)) {
      return 1;
    } else if (arcsim_cmd_props(property, value)) {
      return 1;

    } else {
      LOG(ARCINT_LOG_DEBUG) << "[ARCINT] Unknown "
                            << QUOTEME(SNPS_PRODUCT_NAME)
                            << " property '"
                            << property
                            << "' '"
                            << ((value) ? value : "*no value*")
                            << "'" ;
      return 0;
    }
    LOG(ARCINT_LOG_DEBUG) << "[ARCINT] Handled "
                          << QUOTEME(SNPS_PRODUCT_NAME)
                          << " property '"
                          << property
                          << "' '"
                          << ((value) ? value : "*no value*")
                          << "'" ;
    return 1;
  }
  
  void receive_callback(ARC_callback *cb) {
    callback = cb;
    LOG(ARCINT_LOG_DEBUG) << "[ARCINT] Received callback: "
                          << (callback ? "yes" : "no");
    dbg_access = 0;
    if (callback != 0 && callback->version() >= 4) {
      dbg_access = callback->get_debugger_access();
    }
    LOG(ARCINT_LOG_DEBUG) << "[ARCINT] Got debugger access: "
                          << (dbg_access ? "yes" : "no");
    if (dbg_access) {
      LOG(ARCINT_LOG_DEBUG) << "[ARCINT] debugger access v" << dbg_access->version();
    }
    /* 
     * We expect a dbg_access object with at least version 13, so that we have vlprint 
     * just check it here, so that we don't have to keep on checking it everywhere else.
     */
    if (dbg_access->version()<Debugger_access_VERSION13) {
      ARCSIM_ERROR(("Error: Debugger access too old ( %d < %d)\n", dbg_access->version(), Debugger_access_VERSION13));
      dbg_access = 0;
    }
        

  }

#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
  int instruction_trace_count() {
    return instr_history->get_count();
  }

  void get_instruction_traces(unsigned *traces) {
    instr_history->get_history(traces);
  }    
#endif  

  int supports_feature() {
    int feature = 0;
#ifdef SNPS_ARCSIM_ENABLE_INSTR_HISTORY
    feature |= ARC_FEATURE_instruction_trace; 
#endif
    feature |= ARC_FEATURE_banked_reg | ARC_FEATURE_additional_regs;
    return feature;
  }

  ARC_additional_regs *more_regs;
  enum ARC_addtl_reg {
    ICNT,
    SCYCLES, LAST_CYCLES,
  };
  static const unsigned EXTRA_REGS = 64;
  ARC_reg_in_bank extra_regs[EXTRA_REGS], *extra_regsp[EXTRA_REGS];
  unsigned extra_regs_val[EXTRA_REGS];
  ARC_bank g_bank;
  ARC_bank *g_banksp[2];
  ARC_additional_regs g_AR;
  unsigned dummy;

  ARC_additional_regs *additional_registers() {
#define add_reg(r) \
    do {                                          \
      *p++ = &(extra_regs[p-extra_regsp] = r);    \
      extra_regs_val[r.number] = 0;               \
    } while (0)

    if (more_regs) return more_regs;
    static const int bits = /*1 << 0 |*/ 1 << 3;
    static const int no_display_after_step = 1<<0;
    ARC_reg_in_bank
      icnt    = { "icnt", ICNT, bits|no_display_after_step, 0},
      cycles  = { "cycles", SCYCLES, bits|no_display_after_step, 0},
      lcycles = { "lcycles", LAST_CYCLES, bits|no_display_after_step, 0};

    ARC_reg_in_bank **p = extra_regsp;
    add_reg(icnt);
#ifdef CYCLE_ACC_SIM
    // NCE: check arcsim_mode_cycles here instead of sim_opts.cycle_sim
    //      as sim_opts.cycle_sim may not be set yet since licensing checks
    //      take place AFTER debugger checks for additional regs.
    if (arcsim_mode_cycles) {
      add_reg(cycles);
      add_reg(lcycles);
    }
#endif
    *p++ = 0;
    if (p == extra_regsp) return 0;
    ARC_bank t_bank = {"sim",4,extra_regsp}; g_bank = t_bank;
    g_banksp[0] = &g_bank; g_banksp[1] = 0;
    ARC_additional_regs t_AR = {2, g_banksp}; g_AR = t_AR;
    more_regs = &g_AR;
    // printf("!more regs: %d\n",p-regsp);
    return more_regs;
  }

};

#define IMPNAME ARCsim
CHECK_ARC_OVERRIDES(ARC)

// -----------------------------------------------------------------------------
// Exported functions
//

#ifdef __cplusplus
extern "C" {
#endif
  
  DLLEXPORT ARC* get_ARC_interface ()
  {
    ARC *p = new ARCsim;  /* create simulator object */
    
    return p;
  }
  
#ifdef __cplusplus
}
#endif

// -----------------------------------------------------------------------------




