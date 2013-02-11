//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Counters class encapsulating per processor profiling
// counters and statistics.
//
// =====================================================================

#ifndef INC_PROCESSOR_COUNTERS_H_
#define INC_PROCESSOR_COUNTERS_H_

#include "api/types.h"

namespace arcsim {
  
  // Forward declarations
  //
  namespace util {
    class Histogram;
    class MultiHistogram;
    class Counter;
    class Counter64;
  }
  
  namespace ioc {
    class Context;
  }
    
  namespace sys {
    namespace cpu {
      
      class CounterManager
      {
      public:
        explicit CounterManager(arcsim::ioc::Context& ctx);
        
        // Clears ALL counters and histograms
        void clear();
        
        // ---------------------------------------------------------------------
        // Counter fields
        //
        arcsim::util::Histogram& opcode_freq_hist; // Opcode frequency
        arcsim::util::Histogram& pc_freq_hist;     // Instruction frequency
        arcsim::util::Histogram& call_freq_hist;   // Call frequency
        arcsim::util::Histogram& limm_freq_hist;   // Long immediate frequency
        arcsim::util::Histogram& dkilled_freq_hist;// Killed delay slot instruction
        arcsim::util::Histogram& killed_freq_hist; // killed instruction frequency
        arcsim::util::MultiHistogram& call_graph_multihist; // Target frequency for each call site
        arcsim::util::Histogram& inst_cycles_hist; // Cycles per instruction frequency
        arcsim::util::MultiHistogram& opcode_latency_multihist; // Opcode latency distribution
        arcsim::util::Histogram& icache_miss_freq_hist; // I-$ misses per PC
        arcsim::util::Histogram& dcache_miss_freq_hist; // D-$ misses per PC
        arcsim::util::Histogram& icache_miss_cycles_hist; // I-$ misses cycles frequency
        arcsim::util::Histogram& dcache_miss_cycles_hist; // D-$ misses cycles frequency
        
        arcsim::util::Counter64& dslot_inst_count; // Delay slot instruction count
        arcsim::util::Counter64& flag_stall_count; // Flag stall count
        arcsim::util::Counter64& basic_block_entry_count; // Count of basic block entry executions
        arcsim::util::Counter64& interp_inst_count;// Interpreted instruction count
        arcsim::util::Counter64& native_inst_count;// Native instruction count
        
        arcsim::util::Counter64& cycle_count;      // Cycle count
        
      };
} } } //  arcsim::sys::cpu

#endif  // INC_PROCESSOR_COUNTERS_H_
