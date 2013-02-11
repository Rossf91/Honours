//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
//  Description:
//
//  Simulation Options.
//
// =====================================================================


#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdlib>

#include <limits.h>
#include <errno.h>
#include <getopt.h>

#include <fcntl.h>

#include "define.h"

#include "arch/SimOptions.h"
#include "arch/Configuration.h"

#include "util/OutputStream.h"
#include "util/Log.h"

// -----------------------------------------------------------------------------
// Static non-member funcs/structs to assist members of class Globals
//

static const char* help_msg = "\
arcsim [options] -e <program>\n\
\n\
ArcSim Command Line Options\n\
--------------------------------------------------------------------------------\n\n\
Simulation binary types:\n\
 -e | --elf <program>         ELF executable to simulate\n\
 -H | --hex <program>         Text file to simulate\n\
 -B | --bin <program>         Binary image file to simulate\n\
 --                           Pass all subsequent options to application\n\
\n\
Special simulation modes:\n\
 -i | --interactive           Invoke the command line interface\n\
 -f | --fast                  Fast JIT DBT mode using LLVM-JIT\n\
 -g | --memory                Memory model simulation\n\
 -c | --cycle                 Cycle accurate simulation (default pipeline model: SkipJack 3-stage)\n\
 -x | --cosim                 Co-simulation\n\
 -M | --emt                   Emulate OS traps (i.e. system calls)\n\
 -R | --trackregs             Register usage tracking simulation\n\
 -S | --sim   <insns>         Simulation period\n\
\n\
Fast JIT mode options:\n\
 -m | --fast-trans-mode <mode>Fast translation mode [bb|page] (default: page)\n\
 -Q | --fast-num-threads <n>  Specify number of worker threads used for parallel JIT compilation\n\
 -n | --fast-thresh      <n>  Number of interpretations before a block is deemed to be hot\n\
 -D | --fast-trace-size  <n>  Trace interval size (i.e. # of interpreted blocks for one trace interval)\n\
 -J | --fast-cc               Choose different JIT compiler (e.g. clang, gcc)\n\
 -F | --fast-cc-opt <opt,...> Fast mode JIT compilation flags (effective with '--fast-cc')\n\
 -s | --fast-enable-debug     Enable this flag if you want to debug JIT generated code (effective with '--fast-cc')\n\
 -k | --keep                  Keep intermediate fast mode files (effective with '--fast-cc')\n\
 -r | --reuse                 Reuse previous fast mode files (effective with '--fast-cc')\n\
 -j | --fast-tmp-dir          Directory for storing intermediate JIT compilation results (effective with '--fast-cc')\n\
 -Y | --fast-use-inline-asm   Emit inline assembly code during JIT compilation (effective with '--fast-cc')\n\
\n\
Memory configuration options:\n\
-Z | --mem-init       <value> Initialise each memory block with a custom value\n\
-G | --mem-block-size <value> Memory block size in bytes (e.g. 512B,1K,2K,4K,8K,16K - default:8K)\n\
                              Note that MMU/CCM configuration may override memory block size setting.\n\
\n\
Handling of standard input/output/error and trace output:\n\
 -I | --input        <file>   Redirect standard input from file\n\
 -O | --output       <file>   Redirect standard output to file\n\
 -E | --error        <file>   Redirect standard error to file\n\
 -U | --trace-output <file>   Redirect instruction trace to file\n\
\n\
Instruction set options:\n\
 -o | --options <opt,...>     Select ISA family and ISA options\n\
\n\
Supported ISA family (default A700) and ISA options to be used with '-o' or '--options':\n\
  sat                         Enable saturating extended arithmetic [default disabled]\n\
  fpx                         Enable floating-point extensions [default disabled]\n\
  intvbase_preset=[0x]<addr>  Reset value for interrupt vector base address [default 0]\n\
  ccm_enable_debug=[0,1]      Use slower CCM variant that allows detailed debugging [default 0]\n\
\n\
  a6k=[0,1]                   Enable(1) or disable(0) ARC6000 ISA option [default 0]\n\
  a6kv21=[0,1]                Enable(1) or disable(0) ARC6000 v2.1 ISA option [default 0]\n\
     Sub-options available when 'a6k=1' or 'a6kv21=1' is selected:\n\
       mpy_option=<opt>             Select multiplier [default wlh5]\n\
                                    options are: none,w,wlh1,wlh2,wlh3,wlh4,wlh5\n\
       div_rem_option=[0,1,2]       Set DIV and REM instruction option [default 1]\n\
                                    1 = Standard division               \n\
                                    2 = Radix 4 Enhanced Division      \n\
       code_density_option=[0,1,2]  Set code density option [default 1]\n\
       bitscan_option=[0,1]         Set intra-word search option [default 1]\n\
       swap_option=[0,1]            Set swap option [default 1]\n\
       atomic_option=[0,1]          Set LLOCK and SCOND option [default 0]\n\
       shift_option=[0,1,2,3]       Set shift option [default 3]\n\
       rgf_num_regs=[16,32]         Set number of core registers [default 32]\n\
       pc_size=[16,20,24,28,32]     Set PC size [default 32]\n\
       lpc_size=[8,16,20,24,28,32]  Set LP_COUNT size [default 32]\n\
       addr_size=[16,20,24,28,32]   Set address size [default 32]\n\
       ic_feature_level=[0,1,2]     Set I-cache feature level [default 2]\n\
       dc_feature_level=[0,1,2]     Set D-cache feature level [default 2]\n\
       enable_timer_0=[0,1]         Enable(1) or disable(0) timer 0 [default 0]\n\
       enable_timer_1=[0,1]         Enable(1) or disable(0) timer 1 [default 0]\n\
       fmt14=[0,1]                  Enable(1) or disable(0) the new format 0x0E\n\
       num_actionpoints=[0,1,2,4,8] Set number of Actionpoints [default 0]\n\
       aps_feature=[0,1]            Set full(1) or minimum(0) Actionpoint features\n\
       has_dmp_peripheral=[0,1]     Enable(1) or disable(0) peripheral I/O region\n\
       dc_uncached_region=[0,1]     Enable(1) or disable(0) an uncached data region\n\
       host_timer=[0,1]             Timer uses host clock(1) or virtual cycle count(0)\n\
       big_endian=[0,1]             Enable(1) or disable(0) big-endian memory ordering\n\
       turbo_boost=[0,1]            Enable(1) or disable(0) ARCv2EM turbo boost option\n\
       smart_stack_entries=[0,8,16,32,64,128,256,512,1024,2048,4096] \n\
                                    Number of SmaRT stack entries [0 disables SmaRT]\n\
       number_of_interrupts=[3..32] Number of interrupts supported [default 32]\n\
       ic_disable_on_reset=[0,1]    Disable (1) or enable (0) I-cache on reset [default 0]\n\
       rgf_num_wr_ports=[1,2]       Model 1 or 2 register file write ports [default 2]\n\
       timer_0_int_level=[1,2]      Reset value for Timer0 interrupt level [default 1]\n\
       timer_1_int_level=[1,2]      Reset value for Timer1 interrupt level [default 1]\n\
       ifq_size=[0,1,2,4,8]         Model size of instruction fetch queue [default 0]\n\
     Sub-options available only when a6kv21=1 is selected:\n\
       rgf_num_banks=[1,2]         Select number of banked register files [default 1]\n\
       rgf_banked_regs=<opt>       Select the number of register in the secondary bank\n\
                                    Options: [0,4,8,16,32]\n\
       fast_irq=[0,1]              Disable (1) or Enable(0) pushing/popping of state\n\
                                    for P1 interrupts\n\
       overload_vectors=[0,1]      Overload any unused exception vectors for additional\n\
                                    interrupts.\n\
       number_of_interrupts=[1..248]\n\
       number_of_levels=[1..15]\n\
       rtc_option=[0,1]            Enable or disable the new 64-bit real time clock\n\
       code_protect_mask=0x[0..FF]  16bit hex value representing the code protection bits\n\
       stack_checking=[0,1]         Enable(1) or disable(0) stack checking features on a6k and a700\n\
       multiple_iccms=[0,1]         Enable multiple ICCMs for ARCv2.1 / EM1.1\n\
       new_interrupts=[0,1]        Enable EM1.1 interrupt model \n\
\n\
  a600=[0,1]                  Enable(1) or disable(0) ARC600 ISA option [default 0]\n\
     Sub-options available when 'a600' is selected:\n\
       mpy_option=<opt>           Select multiplier [none,w,mul64]\n\
\n\
Memory devices, EIA extensions, and instruction set extension options:\n\
 -K | --mem-dev      <dev,...> Enable builtin memory devices (e.g. uart0,screen,sound,irq,keyboard)\n\
 -L | --mem-dev-x    <opt,...> Options for builtin memory devices extensions\n\
 -N | --mem-dev-lib <file,..>  Load one or more memory device libraries\n\
 -u | --eia-lib     <file,..>  Load one or more dynamic libraries of EIA extensions\n\
\n\
Supported memory device options used with '--mem-dev-x':\n\
  -cpuid=n                    Set effective CPUID as reported by the hardware to n\n\
  -cpunum=n                   Set effective CPUNUM as reported by the hardware to n\n\
                              (please keep in mind that CPUID < CPUNUM)\n\
 -screen-char-size=<n>        Set character size of screen device (default: 8)\n\
 -screen-flip-clr-chr         Flip colour and char values when decoding writes\n\
                              to memory mapped screen locations\n\
\n\
Tracing and debug related options:\n\
 -t | --trace                 Trace each instruction (with symbol table lookup)\n\
 -P | --profile               Show function-level and HotSpot profiling information\n\
 -X | --dump-state            Output CPU state information\n\
 -d | --debug=<n>             Output debugging information\n\
 -q | --quiet                 Minimise output information\n\
 -v | --verbose               Output more information\n\
\n\
Simulator and Architecture configuration options:\n\
 -a | --arch      <file>      Target system architecture file\n\
 -A | --isa       <file>      Target instruction set architecture file\n\
 -w | --exit-on-brk-sleep     Exit on SLEEP or BRK instruction\n\
 -z | --parch                 Print target system architecture\n\
 -b | --parchfile             Print target system architecture file\n\
 -y | --psimcfg               Print simulator configuration\n\
\n\
 -h | --help                  Print this help message\n\
";

#ifdef VERIFICATION_OPTIONS
static const char* verif_opts_msg = "\
Additional Verification Support Options:\n\
 -o ignore_brk_sleep=[0,1]    Converts BRK, BRK_S and SLEEP to NOP, when 1\n\
 -o disable_stack_setup=[0,1] Disables stack setup of simulated binary, when 1\n\n\
";
#endif

static void usage (const char *msg)
{
  FPRINTF(stderr) << ARCSIM_COPYRIGHT << "usage: " << msg << std::endl; 
#ifdef VERIFICATION_OPTIONS
  FPRINTF(stderr) << verif_opts_msg << std::endl; 
#endif
  exit (EXIT_FAILURE);
}


static struct option long_options[] = {
  /* keep the following alphabetically ordered based on short option */
  { "arch",        required_argument, 0, 'a'  },
  { "isa",         required_argument, 0, 'A'  },
  { "parchfile",   no_argument,       0, 'b'  },
  { "bin",         required_argument, 0, 'B'  },
  { "cycle",       no_argument,       0, 'c'  },
  { "cfg",         required_argument, 0, 'C'  },
  { "debug",       optional_argument, 0, 'd'  },
  { "fast-trace-size",required_argument,0, 'D'},
  { "elf",         required_argument, 0, 'e'  },
  { "error",       required_argument, 0, 'E'  },
  { "fast",        no_argument,       0, 'f'  },
  { "fast-cc-opt", required_argument, 0, 'F'  },
  { "memory",      no_argument,       0, 'g'  },  
  { "mem-block-size",required_argument,0, 'G' },  
  { "help",        no_argument,       0, 'h'  },
  { "hex",         required_argument, 0, 'H'  },
  { "interactive", no_argument,       0, 'i'  },
  { "input",       required_argument, 0, 'I'  },
  { "fast-tmp-dir",required_argument, 0, 'j'  },
  { "fast-cc",     required_argument, 0, 'J'  },
  { "keep",        no_argument,       0, 'k'  },
  { "mem-dev",     required_argument, 0, 'K'  },
  { "mem-dev-x",   required_argument, 0, 'L'  },
  { "fast-trans-mode",required_argument,0, 'm'},
  { "emt",         no_argument,       0, 'M'  },
  { "fast-thresh", required_argument, 0, 'n'  },
  { "mem-dev-lib", required_argument, 0, 'N'  },
  { "options",     required_argument, 0, 'o'  },
  { "output",      required_argument, 0, 'O'  },
  { "profile",     no_argument,       0, 'P'  },
  { "quiet",       no_argument,       0, 'q'  },
  { "fast-num-threads", required_argument,0, 'Q'},
  { "reuse",       no_argument,       0, 'r'  },
  { "trackregs",   no_argument,       0, 'R'  },
  { "fast-enable-debug",no_argument,  0, 's'  },
  { "sim",         required_argument, 0, 'S'  },
  { "trace",       no_argument,       0, 't'  },
  { "eia-lib",     required_argument, 0, 'u'  },
  { "trace-output",required_argument, 0, 'U'  },
  { "verbose",     no_argument,       0, 'v'  },
  { "exit-on-brk-sleep", no_argument, 0, 'w'  },
  { "cosim",       no_argument,       0, 'x'  },
  { "dump-state",  no_argument,       0, 'X'  },
  { "psimcfg",     no_argument,       0, 'y'  },
  { "fast-use-inline-asm", no_argument,0,'Y'  },
  { "parch",       no_argument,       0, 'z'  },
  { "mem-init",    required_argument, 0, 'Z'  },
  /* last element in array must be NULL i.e. empty */
  {NULL}
};

const char* isa_tokens[] = {
  "a6k",                /*  0 */
  "pc_size",            /*  1 */
  "lpc_size",           /*  2 */
  "addr_size",          /*  3 */
  "shift_option",       /*  4 */
  "swap_option",        /*  5 */
  "bitscan_option",     /*  6 */
  "mpy_option",         /*  7 */
  "div_rem_option",     /*  8 */
  "code_density_option",/*  9 */
  "atomic_option",      /* 10 */
  "num_actionpoints",   /* 11 */
  "aps_feature",        /* 12 */
  "has_dmp_peripheral", /* 13 */
  "dc_uncached_region", /* 14 */
  "enable_timer_0",     /* 15 */
  "enable_timer_1",     /* 16 */
  "host_timer",         /* 17 */
  "fmt14",              /* 18 */
  "ic_feature_level",   /* 19 */
  "dc_feature_level",   /* 20 */
  "rgf_num_regs",       /* 21 */
  "rgf_wr_ports",       /* 22 */
  "rgf_num_wr_ports",   /* 23 */
  "a700",               /* 24 */
  "a600",               /* 25 */
  "sat",                /* 26 */
  "mul64",              /* 27 */
  "fpx",                /* 28 */
  "intvbase_preset",    /* 29 */
  "big_endian",         /* 30 */
  "turbo_boost",        /* 31 */
  "smart_stack_entries",/* 32 */
  
  /* BEGIN: VERIFICATION OPTIONS */
  "ignore_brk_sleep",   /* 33 */
  "disable_stack_setup",/* 34 */
  /* END: VERIFICATION OPTIONS */
  
  "number_of_interrupts",/* 35*/
  "ic_disable_on_reset", /* 36*/
  "timer_0_int_level",   /* 37 */
  "timer_1_int_level",   /* 38 */
  "ifq_size",            /* 39 */
  "ccm_enable_debug",    /* 40 */ 
  
  "a6kv21",              /* 41 */
  "code_protect_mask",   /* 42 */
  "stack_checking",      /* 43 */
  "rgf_num_banks",       /* 44 */
  "rgf_banked_regs",     /* 45 */
  "fast_irq",            /* 46 */
  "number_of_levels",    /* 47 */
  "rtc_option",          /* 48 */
  "overload_vectors",    /* 49 */
  "multiple_iccms",      /* 50 */
  "new_interrupts",      /* 51 */
  /* last element in array must be NULL i.e. empty */
  NULL
};


// -----------------------------------------------------------------------------
// Constructor

SimOptions::SimOptions()
: sim_period(0),
  obj_format(DEFAULT_OBJECT_FORMAT),
  big_endian(false),
  trace_on(DEFAULT_TRACE_ON),
  sys_arch_file(DEFAULT_SYS_ARCH_FILE),
  isa_file(DEFAULT_ISA_FILE),
  print_sys_arch(DEFAULT_PRINT_SYS_ARCH),
  print_arch_file(DEFAULT_PRINT_ARCH_FILE),
  verbose(DEFAULT_VERBOSITY),
  debug(DEFAULT_DEBUG),
  quiet(DEFAULT_QUIET),
  dump_state(DEFAULT_DUMP_STATE),
  fast(DEFAULT_FAST),
  fast_use_default_jit(DEFAULT_FAST_JIT),
  fast_num_worker_threads(DEFAULT_FAST_NUM_WORKER_THREADS),
  fast_enable_debug(DEFAULT_FAST_ENABLE_DEBUG),
  fast_use_inline_asm(DEFAULT_FAST_USE_INLINE_ASM),
  fast_trans_mode(DEFAULT_FAST_TRANS_MODE),
  fast_cc(DEFAULT_FAST_CC),
  fast_mode_cc_opts(DEFAULT_FAST_MODE_CC_OPTS),
  fast_tmp_dir(DEFAULT_FAST_TMP_DIR),
  cycle_sim(DEFAULT_CYCLE_SIM),
  memory_sim(DEFAULT_MEMORY_SIM),
  keep_files(DEFAULT_KEEP_FILES),
  reuse_txlation(DEFAULT_REUSE_TXLATION),
  cosim(DEFAULT_COSIM),
  show_profile(DEFAULT_SHOW_PROFILE),
  interactive(DEFAULT_INTERACTIVE),
  emulate_traps(DEFAULT_EMULATE_TRAPS),
  init_mem_custom(DEFAULT_INIT_MEM_CUSTOM),
  init_mem_value(0),
  page_size_log2(DEFAULT_LOG2_PAGE_SIZE),
  obj_name(DEFAULT_OBJECT_NAME),
  app_args(0),
  redir_inst_trace_output(false),
  inst_trace_file(""),
  rinst_trace_fd(-1),
  redir_std_input(false),
  std_in_file(""),
  rin_fd(-1),
  redir_std_output(false),
  std_out_file(""),
  rout_fd(-1),
  redir_std_error(false),
  std_error_file(""),
  rerr_fd(-1),
  halt_simulation(0),
  dcode_cache_size(DEFAULT_DCODE_CACHE_SIZE),
  trans_cache_size(DEFAULT_TRANS_CACHE_SIZE),
  trace_interval_size(DEFAULT_TRACE_INTERVAL_SIZE),
  hotspot_threshold(DEFAULT_HOTSPOT_THRESHOLD),
  print_sim_cfg(DEFAULT_PRINT_SIM_CFG),
  has_mmx(DEFAULT_HAS_MMX),
  is_eia_enabled(false),
  track_regs(false),
  exit_on_break(false),
  exit_on_sleep(false),
  arcsim_lib_name(DEFAULT_ARCSIM_LIB_NAME),
  // Profiling
  //
  is_pc_freq_recording_enabled  (DEFAULT_IS_PC_FREQ_RECORDING_ENABLED),
  is_call_freq_recording_enabled(DEFAULT_IS_CALL_FREQ_RECORDING_ENABLED),
  is_call_graph_recording_enabled(DEFAULT_IS_CALL_GRAPH_RECORDING_ENABLED),
  is_limm_freq_recording_enabled(DEFAULT_IS_LIMM_FREQ_RECORDING_ENABLED),
  is_dkilled_recording_enabled  (DEFAULT_IS_DKILLED_RECORDING_ENABLED),
  is_killed_recording_enabled   (DEFAULT_IS_KILLED_RECORDING_ENABLED),
  is_cache_miss_recording_enabled(DEFAULT_IS_CACHE_MISS_RECORDING_ENABLED),
  is_inst_cycle_recording_enabled(DEFAULT_IS_INST_CYCLE_RECORDING_ENABLED),
  is_cache_miss_cycle_recording_enabled(DEFAULT_IS_INST_MISS_CYCLE_RECORDING_ENABLED),
  is_opcode_latency_distrib_recording_enabled(DEFAULT_IS_OPCODE_LATENCY_DISTRIB_RECORDING_ENABLED)
{ /* EMPTY */ }

SimOptions::~SimOptions()
{
  // Clean-up after ourselves
  //
  if (redir_std_input)          { close(rin_fd);         }
  if (redir_std_output)         { close(rout_fd);        }
  if (redir_std_error)          { close(rerr_fd);        }
  if (redir_inst_trace_output)  { close(rinst_trace_fd); }
}


// -----------------------------------------------------------------------------

// Parse simulator options
//
bool
SimOptions::get_sim_opts(Configuration* _arch_conf, int argc, char *argv[])
{
  Configuration& arch_conf = *_arch_conf;
  int	c;
  bool	status = true;
  char *subopts, *value;
  char *tokens[] = {(char *) NULL };
  
  while (1) {
    int option_index = 0;
    c = getopt_long (argc,
                     argv,
                     /* keep the following alphabetically ordered! */
                     "a:A:bB:cC:dD:e:E:fF:gG:hH:iI:j:J:kK:l:L:m:Mn:N:o:O:PqQrRsS:tu:U:vwxXyYzZ:",
                     long_options,
                     &option_index);
    
    if (c == -1)
      break;
    
    switch (c) {
      case 'h':
        status = false;
        usage (help_msg);
        break;
        
      case 't':
        trace_on = true;
        break;
                
      case 'k':
        keep_files = true;
        break;
        
      case 'r':
        reuse_txlation = true;
        break;
        
      case 'R':
#ifndef REGTRACK_SIM
        LOG(LOG_ERROR) << "This is not a REGTRACK_SIM enabled simulator.";
        exit(EXIT_FAILURE);
#else
        track_regs = true;
#endif
        break;
        
      case 'v':
        verbose = true;
        if (!arcsim::util::Log::reportingLevel > LOG_INFO) {
         arcsim::util::Log::reportingLevel = LOG_INFO;
        } 
        break;
        
      case 'w':
        exit_on_break = true;
        exit_on_sleep = true;
        break;
        
      case 'd':
        debug = true;
        if (optarg) {
          int level = atoi(optarg);
          switch (level) {
            case 0:  { arcsim::util::Log::reportingLevel = LOG_DEBUG;  break; }      
            case 1:  { arcsim::util::Log::reportingLevel = LOG_DEBUG1; break; }
            case 2:  { arcsim::util::Log::reportingLevel = LOG_DEBUG2; break; }
            case 3:  { arcsim::util::Log::reportingLevel = LOG_DEBUG3; break; }
            default: { arcsim::util::Log::reportingLevel = LOG_DEBUG4; break; }
          }
        } else {
          arcsim::util::Log::reportingLevel = LOG_DEBUG;
        }
        break;
        
      case 'q':
        quiet = true;
        break;
        
      case 'e':
        obj_format = OF_ELF;
        obj_name = optarg;
        break;
        
      case 'H':
        obj_format = OF_HEX;
        obj_name = optarg;
        break;
        
      case 'B':
        obj_format = OF_BIN;
        obj_name = optarg;
        break;
        
        // -----------------------------------------------------------------------
        // JIT compilation options
        //
        
      case 'f': {
        fast = true;
        break;
      }
      case 'n': {
        hotspot_threshold = atoi(optarg);
        LOG(LOG_INFO) << "JIT HotSpot threshold: '" << hotspot_threshold << "'";
        break;
      }
        // Fast translation mode
        //
      case 'm': {
        if (!strcmp(optarg, "bb"))  fast_trans_mode = kCompilationModeBasicBlock;
        if (!strcmp(optarg, "page"))fast_trans_mode = kCompilationModePageControlFlowGraph;
        LOG(LOG_INFO) << "JIT translation mode: '" << fast_trans_mode << "'";
        break;
      }
      case 'D': {
        trace_interval_size = atoi(optarg);
        if (trace_interval_size < 1) {
          LOG(LOG_ERROR) << "Trace interval size must be > 0."; 
          exit(EXIT_FAILURE);        
        }
        LOG(LOG_INFO) << "Trace interval size: '" << trace_interval_size << "'";
        break;
      }  
      case 'Q': {
        // How many worker should be used for parallel JIT compilation
        //
        int num_worker_threads = atoi(optarg);
        if (num_worker_threads > 0) {
          fast_num_worker_threads = (size_t)num_worker_threads;
        } else {
          LOG(LOG_ERROR) << "Amount of worker threads used for JIT compilation must be > 0.";
          exit(EXIT_FAILURE);
        }
        LOG(LOG_INFO) << "JIT compiler using '" << fast_num_worker_threads << "'";
        break;
      }
        // Override default JIT compiler
        //
      case 'J': {
        // Turn off default JIT compiler
        //
        fast_use_default_jit = false;
        
        // Use specified JIT compiler
        //
        fast_cc  = optarg;
        LOG(LOG_INFO) << "JIT compiler: '" << fast_cc << "'";
        break;
      }
        // Additional JIT compilation flags
        //
      case 'F': {
        subopts = optarg;
        fast_mode_cc_opts = "";             // clear defaults
        while (*subopts != '\0') {          // retrieve suboptions (man getsubopt)
          getsubopt(&subopts, tokens, &value);
#if (defined(__APPLE__) && defined(__MACH__))
          // see 'man getsubopt' on Mac OS for details
          if (suboptarg != NULL)
            fast_mode_cc_opts.append(" ").append(suboptarg);
          if (value != NULL)
            fast_mode_cc_opts.append("=").append(value);
#else
          if (value != NULL) 
            fast_mode_cc_opts.append(" ").append(value);
#endif
        }
        LOG(LOG_INFO) << "Additional JIT compilation flags: '" << fast_mode_cc_opts.c_str() << "'";
        break;
      }
        // Override JIT temporary directory
        //
      case 'j': {
        fast_tmp_dir = optarg;
        break;
      }
      case 'Y': {
        fast_use_inline_asm = true;
        break;
      }
       
        
      // -----------------------------------------------------------------------
      // EIA extensions
      //

      case 'u': {
        is_eia_enabled = true;
        subopts = optarg;
        while (*subopts != '\0') {
          getsubopt(&subopts, tokens, &value);
#if (defined(__APPLE__) && defined(__MACH__))
          // see 'man getsubopt' on Mac OS for details
          if (suboptarg != NULL)
            eia_library_list.insert(suboptarg);
#else
          if (value != NULL) 
            eia_library_list.insert(value);
#endif
        }
        if (verbose) {
          for (std::set<std::string>::const_iterator
               I = eia_library_list.begin(),
               E = eia_library_list.end();
               I != E; ++I)
          {
            LOG(LOG_INFO) << "Using EIA extension library: '" << *I << "'";
          }
        }
        break;
      }
        
        // -----------------------------------------------------------------------
        // MemoryDevice options       
        //
        
        // Builtin MemoryDevices we should use
        //
      case 'K': {
        subopts = optarg;
        while (*subopts != '\0') {
          getsubopt(&subopts, tokens, &value);
#if (defined(__APPLE__) && defined(__MACH__))
          // see 'man getsubopt' on Mac OS for details
          if (suboptarg != NULL)
            builtin_mem_dev_list.insert(suboptarg);
#else
          if (value != NULL) 
            builtin_mem_dev_list.insert(value);
#endif
        }
        if (verbose) {
          std::string devices_str;
          for (std::set<std::string>::const_iterator
               I = builtin_mem_dev_list.begin(),
               E = builtin_mem_dev_list.end();
               I != E; ++I)
          {
            devices_str.append(" ").append(*I);
          }
          LOG(LOG_INFO) << "Enabled Builtin Memory devices: '" << devices_str << "'";
        }
        break;
      }
        
      // -----------------------------------------------------------------------
      // Memory Device libraries to load
      //
      case 'N': {
        subopts = optarg;
        while (*subopts != '\0') {
          getsubopt(&subopts, tokens, &value);
#if (defined(__APPLE__) && defined(__MACH__))
          // see 'man getsubopt' on Mac OS for details
          if (suboptarg != NULL)
            mem_dev_library_list.insert(suboptarg);
#else
          if (value != NULL) 
            mem_dev_library_list.insert(value);
#endif
        }
        if (verbose) {
          for (std::set<std::string>::const_iterator
               I = mem_dev_library_list.begin(),
               E = mem_dev_library_list.end();
               I != E; ++I)
          {
            LOG(LOG_INFO) << "Using MemoryDevice library: '" << *I << "'";
          }
        }
        break;
      }
      
      // -----------------------------------------------------------------------
      // Additional MemoryDevice options
      //
      case 'L': {
        subopts = optarg;
        while (*subopts != '\0') {          // retrieve suboptions (man getsubopt)
          getsubopt(&subopts, tokens, &value);
#if (defined(__APPLE__) && defined(__MACH__))
          // see 'man getsubopt' on Mac OS for details
          if (value != NULL) {
            builtin_mem_dev_opts.insert(std::pair<std::string,std::string>(suboptarg, value));
          } else {
            builtin_mem_dev_opts.insert(std::pair<std::string,std::string>(suboptarg, ""));
          }
#else
          if (value != NULL) {
            std::string opt (value);
            size_t pos = opt.find("="); // search for equals sign
            if (pos == std::string::npos) { // no equals sign
              builtin_mem_dev_opts.insert(std::pair<std::string,std::string>(value,""));
            } else {  // equals sign found
              builtin_mem_dev_opts.insert(std::pair<std::string,std::string>(opt.substr(0, pos),
                                                                       opt.substr(pos+1)));
            }
          }
#endif
        }
        if (verbose) {
          std::string devices_opt_str;
          for (std::map<std::string,std::string>::const_iterator
               I = builtin_mem_dev_opts.begin(),
               E = builtin_mem_dev_opts.end();
               I != E; ++I)
          {
            devices_opt_str.append(" ").append(I->first)
            .append(" ").append(I->second);
          }
          LOG(LOG_INFO) << "Additional Extension IO options: '" << devices_opt_str << "'";
        }
        break;
      } 
        // -----------------------------------------------------------------------
        
      case 'i':
        interactive = true;
        break;
        
      case 'Z':
        // This is a switch that controls the initialisation of undefined
        // memory locations. To reveal memory bugs, which tend to be masked by
        // ubiquitous zeros, each uninitialised memory word are initialised to
        // its address if this switch is set to false. Under some circumstances,
        // however, the above behaviour is not desirable, e.g. to mimic FPGA
        // behaviour. In such case, set this switch true to have 0-init memory
        // pages. -- xqu
        //
        init_mem_custom = true;
        init_mem_value  = (uint32)atoi(optarg);
        break;
        
      case 'M':
        emulate_traps = true;
        // Historically under emulate_traps mode, memory pages that are not
        // defined by the program need to be initialised to all zeros.
        // Otherwise uClibc binaries would fail alledgedly because ``they
        // relied on a zero return value for uninitialised addresses''. This
        // may not be true any more as now .bss sections are correctly
        // initialised as opposed to before. For backward compatibility,
        // however, this behaviour is kept. -- xqu
        //
        init_mem_custom = true;
        init_mem_value  = 0x0;
        break;
        
      case 'U':
        redir_inst_trace_output = true;
        inst_trace_file = optarg;
        rinst_trace_fd = open(inst_trace_file.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0666);
        if (rinst_trace_fd == -1) {
          LOG(LOG_ERROR) << "Could not open instruction trace redirection file."; 
          exit(EXIT_FAILURE);
        }
        LOG(LOG_INFO) << "Redirecting trace output to '" << inst_trace_file <<"'";
        break;
      
      case 'I':
        redir_std_input = true;
        std_in_file = optarg;
        rin_fd = open(std_in_file.c_str(), O_RDONLY);
        if (rin_fd == -1) {
          LOG(LOG_ERROR) << "Could not open std input redirection file."; 
          exit(EXIT_FAILURE);
        }
        LOG(LOG_INFO) << "Redirecting std input to '" << std_in_file <<"'";
        break;
        
      case 'O':
        redir_std_output = true;
        std_out_file = optarg;
        rout_fd = open(std_out_file.c_str(), O_CREAT | O_WRONLY, 0666);
        if (rout_fd == -1) {
          LOG(LOG_ERROR) << "Could not open std output redirection file."; 
          exit(EXIT_FAILURE);
        }
        LOG(LOG_INFO) << "Redirecting std output to '" << std_out_file <<"'";
        break;
        
      case 'E':
        redir_std_error = true;
        std_error_file = optarg;
        rerr_fd = open(std_error_file.c_str(), O_CREAT | O_WRONLY, 0666);
        if (rerr_fd == -1) {
          LOG(LOG_ERROR) << "Could not open std error redirection file.";
          exit(EXIT_FAILURE);
        }
        LOG(LOG_INFO) << "Redirecting std error to '" << std_error_file <<"'";
        break;
        
      case 'o':
        subopts = optarg;
        while (*subopts != '\0')  // retrieve suboptions (man getsubopt)
        {         
          if ((c = getsubopt(&subopts, (char* const*)isa_tokens, &value)) < 0)
          {
            LOG(LOG_ERROR) << "Unrecognised option '" << subopts << "' in argument for '-o'";
            status = false;
            usage (help_msg);
          }  else  {
            int v = 0;
            if (value)
            {              
              v = atoi(value);
              
              switch (c)
              {
                case  0: if (v) arch_conf.sys_arch.isa_opts.set_isa(IsaOptions::kIsaA6K); break;
                case  1: arch_conf.sys_arch.isa_opts.pc_size     = v;        break;    
                case  2: arch_conf.sys_arch.isa_opts.lpc_size    = v;        break;    
                case  3: { 
                  arch_conf.sys_arch.isa_opts.addr_size   = v;
                  // If 'addr_size' is 16 bits we need to also lower 'page_arch_offset_bits'
                  // from 11UL (i.e. 8k) to 7UL (i.e. 512b) so the internal page size
                  // used by ArcSim is 'small' enough so CCM region mapping works correctly.
                  //
                  if (arch_conf.sys_arch.isa_opts.addr_size == 16) {
                    arch_conf.sys_arch.sim_opts.page_size_log2 = PageArch::k512BPageSizeLog2;
                    LOG(LOG_DEBUG) << "Changing default internal memory chunk size from "
                                   << "'8k' to '512b' due to address size '16'.";
                  }
                  break;
                }
                case  4:
                  {
                    arch_conf.sys_arch.isa_opts.shift_option  = ((v & 2) != 0);  
                    arch_conf.sys_arch.isa_opts.shas_option   = ((v & 1) != 0);
                    break;
                  }
                case  5: arch_conf.sys_arch.isa_opts.swap_option = (v != 0); break;
                case  6: 
                  {
                    arch_conf.sys_arch.isa_opts.norm_option   = (v != 0); 
                    arch_conf.sys_arch.isa_opts.ffs_option    = (v != 0);
                    break;
                  }
                case  7: 
                  {
                    if (!strcmp(value,"none"))
                    {
                      arch_conf.sys_arch.isa_opts.mpy16_option   = false;
                      arch_conf.sys_arch.isa_opts.mpy32_option   = false;
                      arch_conf.sys_arch.isa_opts.mpy_lat_option = 0;
                    } else if (!strcmp(value,"w"))
                    {
                      arch_conf.sys_arch.isa_opts.mpy16_option   = true;
                      arch_conf.sys_arch.isa_opts.mpy32_option   = false;
                      arch_conf.sys_arch.isa_opts.mpy_fast       = true;
                      arch_conf.sys_arch.isa_opts.mpy_lat_option = 1;
                    } else if (!strcmp(value,"wlh1"))
                    {
                      arch_conf.sys_arch.isa_opts.mpy16_option   = true;
                      arch_conf.sys_arch.isa_opts.mpy32_option   = true;
                      arch_conf.sys_arch.isa_opts.mpy_fast       = true;
                      arch_conf.sys_arch.isa_opts.mpy_lat_option = 1;
                    } else if (!strcmp(value,"wlh2"))
                    {
                      arch_conf.sys_arch.isa_opts.mpy16_option   = true;
                      arch_conf.sys_arch.isa_opts.mpy32_option   = true;
                      arch_conf.sys_arch.isa_opts.mpy_fast       = true;
                      arch_conf.sys_arch.isa_opts.mpy_lat_option = 2;
                    } else if (!strcmp(value,"wlh3"))
                    {
                      arch_conf.sys_arch.isa_opts.mpy16_option   = true;
                      arch_conf.sys_arch.isa_opts.mpy32_option   = true;
                      arch_conf.sys_arch.isa_opts.mpy_fast       = true;
                      arch_conf.sys_arch.isa_opts.mpy_lat_option = 3;
                    } else if (!strcmp(value,"wlh4"))
                    {
                      arch_conf.sys_arch.isa_opts.mpy16_option   = true;
                      arch_conf.sys_arch.isa_opts.mpy32_option   = true;
                      arch_conf.sys_arch.isa_opts.mpy_fast       = true;
                      arch_conf.sys_arch.isa_opts.mpy_lat_option = 4;
                    } else if (!strcmp(value,"wlh5"))
                    {
                      arch_conf.sys_arch.isa_opts.mpy16_option   = true;
                      arch_conf.sys_arch.isa_opts.mpy32_option   = true;
                      arch_conf.sys_arch.isa_opts.mpy_lat_option = 9;
                    } else if (!strcmp(value,"mul64"))
                    {
                      arch_conf.sys_arch.isa_opts.mul64_option = true;
                    }
                    break;
                  }
                case  8: arch_conf.sys_arch.isa_opts.div_rem_option   = v;         break;
                case  9: arch_conf.sys_arch.isa_opts.density_option   = v;         break;
                case 10: arch_conf.sys_arch.isa_opts.atomic_option    = v;         break;
                case 11: arch_conf.sys_arch.isa_opts.num_actionpoints = v;         break;    
                case 12: arch_conf.sys_arch.isa_opts.aps_full         = (v != 0);  break;
                case 13: arch_conf.sys_arch.isa_opts.has_dmp_peripheral = (v != 0);break;
                case 14: arch_conf.sys_arch.isa_opts.dc_uncached_region = (v != 0);break;
                case 15: arch_conf.sys_arch.isa_opts.has_timer0       = (v != 0);  break;
                case 16: arch_conf.sys_arch.isa_opts.has_timer1       = (v != 0);  break;
                case 17: arch_conf.sys_arch.isa_opts.use_host_timer   = (v != 0);  break;                
                case 18: arch_conf.sys_arch.isa_opts.new_fmt_14       = (v != 0);  break;
                case 19: arch_conf.sys_arch.isa_opts.ic_feature       = v;         break;    
                case 20: arch_conf.sys_arch.isa_opts.dc_feature       = v;         break;
                case 21: arch_conf.sys_arch.isa_opts.only_16_regs     = (v == 16); break;
                case 22:
                case 23: /* deprecated option */ 
                  {
                    if ((v != 1) && (v != 2))
                    {
                      LOG(LOG_ERROR) << "The number of register file write ports must be 1 or 2";
                      exit(EXIT_FAILURE);
                    }
                    arch_conf.sys_arch.isa_opts.rf_4port = (v == 2);  
                    break;
                  }
                case 24: break;
                case 25: if (v) arch_conf.sys_arch.isa_opts.set_isa(IsaOptions::kIsaA600);  break;
                case 26: arch_conf.sys_arch.isa_opts.sat_option       = (v != 0);  break;
                case 27: arch_conf.sys_arch.isa_opts.mul64_option     = (v != 0);  break;
                case 28: arch_conf.sys_arch.isa_opts.fpx_option       = (v != 0);  break;
                case 29:
                  { // set intvbase_preset to given address, optionally preceded by 0x
                    //
                    errno = 0;
                    arch_conf.sys_arch.isa_opts.intvbase_preset = strtol(value, 0, 0);
                    if (errno) {
                      LOG(LOG_ERROR) << "[OPTIONS] intvbase_preset value is beyond 32-bit range";
                      arch_conf.sys_arch.isa_opts.intvbase_preset = 0;
                      status = false;
                    }
                    break;
                  }
                case 30: big_endian = (v != 0);                                    break;
                case 31: arch_conf.sys_arch.isa_opts.turbo_boost = (v != 0);       break;
                case 32: arch_conf.sys_arch.isa_opts.smart_stack_size = v;         break;
                // VERIFICATION_OPTIONS
                case 33: arch_conf.sys_arch.isa_opts.ignore_brk_sleep = (v != 0);  break;
                case 34: arch_conf.sys_arch.isa_opts.disable_stack_setup = (v != 0);break;
                  
                case 35: arch_conf.sys_arch.isa_opts.num_interrupts = v;           break;
                case 36: arch_conf.sys_arch.isa_opts.ic_disable_on_reset = (v != 0);break;
                case 37: arch_conf.sys_arch.isa_opts.timer_0_int_level = v;        break;
                case 38: arch_conf.sys_arch.isa_opts.timer_1_int_level = v;        break;
                case 39: arch_conf.sys_arch.isa_opts.ifq_size = v;                 break;
                case 40: arch_conf.sys_arch.isa_opts.is_ccm_debug_enabled = (v != 0);  break;
                
                // A6KV2.1 options
                case 41: arch_conf.sys_arch.isa_opts.set_isa(IsaOptions::kIsaA6KV2);    break;
                case 42: sscanf(value,"0x%hx",&(arch_conf.sys_arch.isa_opts.code_protect_bits)); break;
                case 43: arch_conf.sys_arch.isa_opts.stack_checking = (v!=0);      break;
                case 44: arch_conf.sys_arch.isa_opts.num_reg_banks = v; break;
                case 45: arch_conf.sys_arch.isa_opts.num_banked_regs = v; break;
                case 46: arch_conf.sys_arch.isa_opts.fast_irq = (v != 0); break;
                case 47: arch_conf.sys_arch.isa_opts.number_of_levels = v; break;
                case 48: arch_conf.sys_arch.isa_opts.rtc_option = (v != 0); break;
                case 49: arch_conf.sys_arch.isa_opts.overload_vectors = (v != 0); break;
                case 50: arch_conf.sys_arch.isa_opts.multiple_iccms = (v!=0);  break;
                case 51: arch_conf.sys_arch.isa_opts.new_interrupts = (v != 0); arch_conf.sys_arch.isa_opts.setup_exception_vectors(); break;
              }
            }
          }
        }
        
        if (arch_conf.sys_arch.isa_opts.is_isa_a6k()) {
          arch_conf.sys_arch.isa_opts.mul64_option   = false;
          arch_conf.sys_arch.isa_opts.sat_option     = false;
        } else if (arch_conf.sys_arch.isa_opts.is_isa_a600()) {
          arch_conf.sys_arch.isa_opts.div_rem_option = 0;
          arch_conf.sys_arch.isa_opts.density_option = false;
          arch_conf.sys_arch.isa_opts.ffs_option     = false;
          arch_conf.sys_arch.isa_opts.shas_option    = false;
        } else {
          arch_conf.sys_arch.isa_opts.div_rem_option = 0;
          arch_conf.sys_arch.isa_opts.density_option = false;
          arch_conf.sys_arch.isa_opts.ffs_option     = false;
          arch_conf.sys_arch.isa_opts.shas_option    = false;
        }        
        break;
        
      case 'P':
        show_profile = true;
        break;
        
      case 'g':
        memory_sim = true;
        break;
        
      case 'G': {
        int siz = atoi(optarg);
        switch (siz) {
          case PageArch::k512BPageSize: arch_conf.sys_arch.sim_opts.page_size_log2 = PageArch::k512BPageSizeLog2; break;
          case PageArch::k1KPageSize:   arch_conf.sys_arch.sim_opts.page_size_log2 = PageArch::k1KPageSizeLog2;   break; 
          case PageArch::k2KPageSize:   arch_conf.sys_arch.sim_opts.page_size_log2 = PageArch::k2KPageSizeLog2;   break;
          case PageArch::k4KPageSize:   arch_conf.sys_arch.sim_opts.page_size_log2 = PageArch::k4KPageSizeLog2;   break;
          case PageArch::k8KPageSize:   arch_conf.sys_arch.sim_opts.page_size_log2 = PageArch::k8KPageSizeLog2;   break;
          case PageArch::k16KPageSize:  arch_conf.sys_arch.sim_opts.page_size_log2 = PageArch::k16KPageSizeLog2;  break;
          default: {
            LOG(LOG_ERROR) << "Illegal memory block size specified.";
            exit(EXIT_FAILURE);
          }
        }
        break;
      }
        
      case 'c':
        cycle_sim  = true;
        memory_sim = true;
#ifndef CYCLE_ACC_SIM
        LOG(LOG_ERROR) << "This is not a CYCLE_ACC_SIM compiled simulator.";
        exit(EXIT_FAILURE);
#endif
        break;
        
      case 'x':
        cosim = true;
#ifndef COSIM_SIM
        LOG(LOG_ERROR) << "This is not a COSIM_SIM compiled simulator.";
        exit(EXIT_FAILURE);
#endif
        break;
        
      case 'X':
        dump_state = true;
        break;
        
      case 'a':
        sys_arch_file = optarg;
        break;
        
      case 'A':
        isa_file = optarg;
        break;
        
      case 'z':
        print_sys_arch = true;
        break;
        
      case 'b':
        print_arch_file = true;
        break;
                
      case 'y':
        print_sim_cfg = true;
        break;
        
      case 's':
        fast_enable_debug    = true;
        keep_files           = true;
        fast_use_default_jit = false;
        break;
        
      case 'S':
        sim_period = atoll(optarg);
        break;
                
      case '?':
        status = false;
        usage (help_msg);
        break;
        
      default:
        LOG(LOG_ERROR) << "?? getopt returned character code " << c << " ??";
        status = false;
        break;
        
    } /* end switch */
  } /* end while */
  
  app_args = optind;

  
  // Output ISA specific options
  //
  if (arch_conf.sys_arch.isa_opts.is_isa_a6k())       LOG(LOG_INFO) << "ARCompact V2 ISA is selected";
  else if (arch_conf.sys_arch.isa_opts.is_isa_a600()) LOG(LOG_INFO) << "ARC 600 ISA is selected";
  else                                                LOG(LOG_INFO) << "ARC 700 ISA is selected";
  
  if (arch_conf.sys_arch.isa_opts.only_16_regs)  LOG(LOG_INFO) << "Reduced set of 16 core registers is selected";
  if (arch_conf.sys_arch.isa_opts.shift_option)  LOG(LOG_INFO) << "Barrel-shifter is enabled";
  if (arch_conf.sys_arch.isa_opts.swap_option)   LOG(LOG_INFO) << "SWAP instruction are enabled";
  if (arch_conf.sys_arch.isa_opts.norm_option)   LOG(LOG_INFO) << "NORM instructions are enabled";
  if (arch_conf.sys_arch.isa_opts.mpy32_option)
    LOG(LOG_INFO) << "MPY, MPYU, MPYH, MPYHU instructions are enabled, with latency " <<
    arch_conf.sys_arch.isa_opts.mpy_lat_option;
  if (arch_conf.sys_arch.isa_opts.mpy16_option)   LOG(LOG_INFO) << "MPYW, MPYWU instructions are enabled";
  if (arch_conf.sys_arch.isa_opts.div_rem_option) LOG(LOG_INFO) << "DIV, DIVU, REM, REMU instructions enabled";
  if (arch_conf.sys_arch.isa_opts.div_rem_option == 2) LOG(LOG_INFO) << "Radix 4 Enhanced division enabled";
  if (arch_conf.sys_arch.isa_opts.density_option > 0)LOG(LOG_INFO) << "code density pack is enabled";
  if (arch_conf.sys_arch.isa_opts.ffs_option)     LOG(LOG_INFO) << "FFS and FLS instructions are enabled";
  if (arch_conf.sys_arch.isa_opts.atomic_option == 1) LOG(LOG_INFO) << "EX instruction is enabled";
  if (arch_conf.sys_arch.isa_opts.atomic_option == 2) LOG(LOG_INFO) << "EX, LLOCK, and SCCOND instructions are enabled";
  if (arch_conf.sys_arch.isa_opts.shas_option)   LOG(LOG_INFO) << "shift-assist instructions are enabled";
  if (arch_conf.sys_arch.isa_opts.sat_option)    LOG(LOG_INFO) << "saturating operations are enabled";
  if (arch_conf.sys_arch.isa_opts.mul64_option)  LOG(LOG_INFO) << "MUL64 and MULU64 instructions are enabled";
  if (arch_conf.sys_arch.isa_opts.fpx_option)    LOG(LOG_INFO) << "floating-point extension (FPX) is enabled";
  LOG(LOG_INFO) << "Number of interrupts is set to " << arch_conf.sys_arch.isa_opts.num_interrupts;
  if (arch_conf.sys_arch.isa_opts.intvbase_preset) 
    LOG(LOG_INFO) << "interrupt vector base = " 
                  << std::hex << std::setw(8) << std::setfill('0') 
                  << arch_conf.sys_arch.isa_opts.intvbase_preset; 
  
  if (arch_conf.sys_arch.isa_opts.is_ccm_debug_enabled) 
    LOG(LOG_INFO) << "Instantiating MemoryDevice based CCM devices." ;
  
  // Check interrupt configuration constraints for ARCv2EM
  //
  if (arch_conf.sys_arch.isa_opts.is_isa_a6kv1()) {
    if (   (arch_conf.sys_arch.isa_opts.num_interrupts < 3) 
         ||(arch_conf.sys_arch.isa_opts.num_interrupts > 32))
    {
      LOG(LOG_ERROR) << "The number of interrupts is '" << arch_conf.sys_arch.isa_opts.num_interrupts
                     << "'. It must be in the range 3 to 32";
      exit(EXIT_FAILURE);
    }

    uint32 timer_ints = arch_conf.sys_arch.isa_opts.has_timer0 ? 1 : 0;
    timer_ints += arch_conf.sys_arch.isa_opts.has_timer1 ? 1 : 0;
 
    if (arch_conf.sys_arch.isa_opts.num_interrupts < (3+timer_ints)){
      LOG(LOG_ERROR) << "With timers enabled, the number of interrupts must be at least " 
                     << (3+timer_ints);
      exit(EXIT_FAILURE);
    }

    if (  (arch_conf.sys_arch.isa_opts.timer_0_int_level != 2)
        &&(arch_conf.sys_arch.isa_opts.timer_0_int_level != 1))
    {
      LOG(LOG_ERROR) << "timer_0_int_level must be 1 or 2";
      exit(EXIT_FAILURE);
    }

    if (  (arch_conf.sys_arch.isa_opts.timer_1_int_level != 2)
        &&(arch_conf.sys_arch.isa_opts.timer_1_int_level != 1))
    {
      LOG(LOG_ERROR) << "timer_0_int_level must be 1 or 2";
      exit(EXIT_FAILURE);
    }

    if (  (arch_conf.sys_arch.isa_opts.ifq_size != 0)
        &&(arch_conf.sys_arch.isa_opts.ifq_size != 1)
        &&(arch_conf.sys_arch.isa_opts.ifq_size != 2)
        &&(arch_conf.sys_arch.isa_opts.ifq_size != 4)
        &&(arch_conf.sys_arch.isa_opts.ifq_size != 8))
    {
      LOG(LOG_ERROR) << "ifq_size must be 0, 1, 2, 4 or 8";
      exit(EXIT_FAILURE);
    }
  }
  
  return status;
}


