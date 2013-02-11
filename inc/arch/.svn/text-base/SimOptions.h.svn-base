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

#ifndef INC_ARCH_SIMOPTIONS_H_
#define INC_ARCH_SIMOPTIONS_H_

#include <string>
#include <map>
#include <set>

#include "api/types.h"
#include "sim_types.h"

// -----------------------------------------------------------------------------
// Forward Declaration
//
class Configuration;

// -----------------------------------------------------------------------------
// SimOptions Class
//
class SimOptions
{  
public:
  std::string   obj_name;
  int           app_args;
  ObjectFormat  obj_format;
  bool          big_endian;
  bool          memory_sim;
  bool          cycle_sim;
  bool          cosim;
  bool          init_mem_custom;
  uint32        init_mem_value;
  bool          fast;
  bool          fast_use_default_jit;
  size_t        fast_num_worker_threads;
  bool          fast_enable_debug;
  bool          fast_use_inline_asm;
  CompilationMode     fast_trans_mode;
  std::string   fast_cc;
  std::string   fast_mode_cc_opts;
  std::string   fast_tmp_dir;
  bool          keep_files;
  bool          reuse_txlation;
  uint64        sim_period;
  bool          emulate_traps;
  bool          trace_on;
  bool          verbose;
  bool          quiet;
  bool          exit_on_break;
  bool          exit_on_sleep;
  std::string   sys_arch_file;
  std::string   isa_file;
  bool          print_sys_arch;
  bool          print_arch_file;
  bool          interactive;
  bool          print_sim_cfg;
  bool          debug;
  bool          show_profile;
  std::string   arcsim_lib_name;
  
  // Page/Memory settings
  //
  uint32      page_size_log2; // log2 page size
  
  // File redirection/Output
  //
  bool        redir_inst_trace_output;
  std::string inst_trace_file;
  int         rinst_trace_fd;
  
  bool        redir_std_input;
  std::string std_in_file;
  int         rin_fd;

  bool        redir_std_output;
  std::string std_out_file;
  int         rout_fd;

  bool        redir_std_error;
  std::string std_error_file;
  int         rerr_fd;
  
  bool        dump_state;
  int         halt_simulation;

  uint32      dcode_cache_size;
  uint32      trans_cache_size;
  
  uint32      trace_interval_size;
  uint32      hotspot_threshold;
  
  bool        track_regs;

  bool        has_mmx;
  
  // Memory Devices
  //
  std::set<std::string>             mem_dev_library_list;
  std::set<std::string>             builtin_mem_dev_list;
  std::map<std::string,std::string> builtin_mem_dev_opts;
  
  // EIA style extensions
  //
  bool                              is_eia_enabled;
  std::set<std::string>             eia_library_list;
  
  // Profiling options
  //   
  bool is_pc_freq_recording_enabled;
  bool is_call_freq_recording_enabled;
  bool is_call_graph_recording_enabled;
  bool is_limm_freq_recording_enabled;
  bool is_dkilled_recording_enabled;
  bool is_killed_recording_enabled;
  bool is_cache_miss_recording_enabled;
  bool is_inst_cycle_recording_enabled;
  bool is_cache_miss_cycle_recording_enabled;
  bool is_opcode_latency_distrib_recording_enabled;
  
  // Constructor/Destructor
  //
  SimOptions();
  ~SimOptions();  
  
  // Parse simulation options
  //
  bool get_sim_opts(Configuration* arch_conf, int argc, char *argv[]);
  
};

#endif  // INC_ARCH_SIMOPTIONS_H_

