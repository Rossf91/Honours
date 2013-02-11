//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Counters struct encapsulating per processor profiling counters and
// statistics.
//
// =====================================================================

#include "sys/cpu/CounterManager.h"

#include "util/Counter.h"
#include "util/Histogram.h"
#include "util/MultiHistogram.h"

#include "ioc/Context.h"
#include "ioc/ContextItemId.h"
#include "ioc/ContextItemInterface.h"

// Short-cut macros for instantiating histograms and counters
#define NEW_HISTOGRAM(_ctx_,_name_)                                             \
  *(Histogram*)(_ctx_.create_item(ContextItemInterface::kTHistogram,_name_))

#define NEW_MULTI_HISTOGRAM(_ctx_,_name_)                                       \
  *(MultiHistogram*)(_ctx_.create_item(ContextItemInterface::kTMultiHistogram,_name_))

#define NEW_COUNTER64(_ctx_,_name_)                                             \
  *(Counter64*)(_ctx_.create_item(ContextItemInterface::kTCounter64,_name_))


namespace arcsim {
  namespace sys {
    namespace cpu {

      // Pull in arcsim::util and arcsim::ioc namespace 
      //
      using namespace arcsim::util;
      using namespace arcsim::ioc;
      
      // -----------------------------------------------------------------------
      // CounterManager
      //
      CounterManager::CounterManager(Context& ctx)
      :
      opcode_freq_hist  (NEW_HISTOGRAM(ctx, ContextItemId::kOpcodeFrequencyHistogram)),
      pc_freq_hist      (NEW_HISTOGRAM(ctx, ContextItemId::kPcFrequencyHistogram)),
      call_freq_hist    (NEW_HISTOGRAM(ctx, ContextItemId::kCallFrequencyHistogram)),
      limm_freq_hist    (NEW_HISTOGRAM(ctx, ContextItemId::kLimmFrequencyHistogram)),
      dkilled_freq_hist (NEW_HISTOGRAM(ctx, ContextItemId::kDkilledFrequencyHistogram)),
      killed_freq_hist  (NEW_HISTOGRAM(ctx, ContextItemId::kKilledFrequencyHistogram)),
      call_graph_multihist(NEW_MULTI_HISTOGRAM(ctx, ContextItemId::kCallGraphMultiHistogram)),
      inst_cycles_hist    (NEW_HISTOGRAM(ctx, ContextItemId::kInstructionCyclesHistogram)),
      opcode_latency_multihist(NEW_MULTI_HISTOGRAM(ctx, ContextItemId::kLatencyDistributionMultiHistogram)),
      icache_miss_freq_hist  (NEW_HISTOGRAM(ctx, ContextItemId::kAddrIcacheMissFrequencyHistogram)),
      dcache_miss_freq_hist  (NEW_HISTOGRAM(ctx, ContextItemId::kAddrDcacheMissFrequencyHistogram)),
      icache_miss_cycles_hist(NEW_HISTOGRAM(ctx, ContextItemId::kAddrIcacheMissCyclesHistogram)),
      dcache_miss_cycles_hist(NEW_HISTOGRAM(ctx, ContextItemId::kAddrDcacheMissCyclesHistogram)),
      dslot_inst_count  (NEW_COUNTER64(ctx, ContextItemId::kDslotInstCount64)),
      flag_stall_count  (NEW_COUNTER64(ctx, ContextItemId::kFlagStallCount64)),
      basic_block_entry_count (NEW_COUNTER64(ctx, ContextItemId::kBasicBlockExecCount64)),
      interp_inst_count (NEW_COUNTER64(ctx, ContextItemId::kInterpInstCount64)),
      native_inst_count (NEW_COUNTER64(ctx, ContextItemId::kNativeInstCount64)),
      cycle_count       (NEW_COUNTER64(ctx, ContextItemId::kCycleCount64))
      { /* EMPTY */ }

      void
      CounterManager::clear()
      {
        opcode_freq_hist.clear();
        pc_freq_hist.clear();
        call_freq_hist.clear();
        limm_freq_hist.clear();
        dkilled_freq_hist.clear();
        killed_freq_hist.clear();
        inst_cycles_hist.clear();
        icache_miss_freq_hist.clear();
        dcache_miss_freq_hist.clear();
        icache_miss_freq_hist.clear();
        dcache_miss_freq_hist.clear();
        dslot_inst_count.clear();
        flag_stall_count.clear();
        basic_block_entry_count.clear();
        interp_inst_count.clear();
        native_inst_count.clear();
        cycle_count.clear();
      }
      
} } } //  arcsim::sys::cpu
