//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Memory Model timing interface class.
//
// =====================================================================


#ifndef _INC_UARCH_MEMORY_MEMORYMODEL_H_
#define _INC_UARCH_MEMORY_MEMORYMODEL_H_

#include <queue>
#include <string>

#include "api/types.h"

#include "define.h"

#include "arch/Configuration.h"

#include "uarch/memory/CacheModel.h"
#include "uarch/memory/CcmModel.h"
#include "uarch/memory/MainMemoryModel.h"
#include "uarch/memory/LatencyCache.h"


// -----------------------------------------------------------------------------
// Forward declaration
//
namespace arcsim {
  namespace isa {
    namespace arc {
      class Dcode;
  } }
  namespace util {
    class CodeBuffer;
} }

class MemoryModel {
private:
  uint16                block_offset;
    
  // System Architecture
  //
  SystemArch&           sys_arch;
  
public:
  // LatencyCache
  //
  arcsim::uarch::memory::LatencyCache latency_cache;
  
  // Memory hierarchy managed by this class
  //
  MainMemoryModel*      main_mem_c;
  CacheModel*           icache_c;
  CacheModel*           dcache_c;
  // FIXME: improve co-existance of multiple ICCMs
  CCMModel*             iccm_c;
  CCMModel*             iccms_c[4];
  CCMModel*             dccm_c;
  
  // Queue containing all addresses a memory instruction has accessed
  // (e.g. load, store, enter, leave, ex)
  //
  std::queue<uint32>    addr_queue;
  
  // I-cache and D-cache enables
  //
  bool                  icache_enabled;
  bool                  dcache_enabled;
  
  uint32                cached_limit;
  
  // ---------------------------------------------------------------------------
  // Constructor/Destructor
  //
  explicit MemoryModel (SystemArch&       sys_arch,
                        uint16            block_offset,
                        CacheModel*       icache,
                        CacheModel*       dcache,
                        CCMModel*         iccm,
                        CCMModel*         dccm,
                        MainMemoryModel*  main_mem);
  ~MemoryModel ();

  // ---------------------------------------------------------------------------
  // Clear all memory components the MemoryModel is responsible for
  //
  void clear();
  
  void print_stats();

  // ---------------------------------------------------------------------------
  // Method emitting code to access memory
  //
  bool jit_emit_memory_model_access_functions(arcsim::util::CodeBuffer& buf,
                                              CoreArch&                 core_arch);
  
  void jit_emit_instr_memory_fetch (arcsim::util::CodeBuffer&      buf,
                                    const arcsim::isa::arc::Dcode& inst,
                                    uint32                         addr);
  void jit_emit_instr_memory_access(arcsim::util::CodeBuffer&      buf,
                                    const arcsim::isa::arc::Dcode& inst,
                                    const char*                    addr);


  // ---------------------------------------------------------------------------
  // Cache enable/disable methods
  //
  void enable_icache  () { icache_enabled = (icache_c != 0);  }
  void disable_icache () { icache_enabled = false;            }
  void enable_dcache  () { dcache_enabled = (dcache_c != 0);  }
  void disable_dcache () { dcache_enabled = false;            }

  // ---------------------------------------------------------------------------
  // Tag checks methods
  //
  
  inline bool is_dc_hit (uint32 addr)
  {
    if (dcache_enabled && (addr <= cached_limit)) 
      return dcache_c->is_hit(addr);
    else
      return false;
  }
  
  inline bool is_dirty_dc_hit (uint32 addr)
  {
    if (dcache_enabled && (addr <= cached_limit))
      return dcache_c->is_dirty_hit(addr);
    else
      return false;
  }

  // ---------------------------------------------------------------------------
  // Generic inlined fetch/read/write methods
  //
  
  inline uint16 fetch (uint32 addr, uint32 pc)
  {
    uint16 cycles = 0;

    // If there is an ICCM, attempt to read from it, and if the resulting
    // latency is non-zero then we are within the ICCM address range.
    //
    // FIXME: improve co-existance of multiple ICCMs
    if (sys_arch.isa_opts.multiple_iccms) {
      for (int i = 0; i < IsaOptions::kMultipleIccmCount; ++i){
        if (iccms_c[i] && (cycles = (iccms_c[i]->read(addr, block_offset))))
          return cycles;
      }
    } else {
      if (iccm_c && (cycles = iccm_c->read(addr, block_offset)))
        return cycles;
    }
    
    // Check ICACHE if present and enabled
    //
    if (icache_enabled) {
      // First check if we have a hit in the latency cache
      //
      if (latency_cache.is_fetch_latency_hit(addr, &cycles)) {
        // We had a hit so increment hit counter and return cycle count
        //
        return cycles;
      }
      
      if (addr <= cached_limit) {
        cycles = icache_c->read(addr, block_offset, pc);
        // Update latency cache
        //
        latency_cache.update_fetch_entry(addr);
      
        return cycles;
      }
    }
    
    // Go to MAIN MEMORY
    //
    cycles = main_mem_c->read(2); // 2 indicates a 4-byte fetch (2^2 bytes)
    return cycles; 
  }

  inline uint16 read (uint32 addr, uint32 pc, bool uncached)
  {
    uint16 cycles = 0;
    
    // If there is a DCCM, attempt to read from it, and if the resulting
    // latency is non-zero then we are within the DCCM address range.
    //
    if (dccm_c && (cycles = dccm_c->read(addr, block_offset)))
      return cycles;

    // If this read is uncached, or outside the cacheable region, or if
    // the dcache is disabled, then return the latency of 4-byte read 
    // from main memory.
    //
    if (uncached || (addr > cached_limit) || !dcache_enabled) {
      return main_mem_c->read(2);  // 2 indicates a 4-byte fetch (2^2 bytes)
    }

    // Cached read: check if we have a hit in the latency cache
    //
    if (latency_cache.is_read_latency_hit(addr, &cycles)) {
      // We had a hit so return cycle count
      //
      return cycles;
    }
    
    // Not a hit in the latency cache, so go to the dcache
    //
    cycles = dcache_c->read(addr, block_offset, pc);

    // Update latency cache with current address
    //
    latency_cache.update_read_entry(addr);

    return cycles;
  }
    
  inline uint16 write (uint32 addr, uint32 pc, bool uncached)
  {
    uint16 cycles = 0;

    // If there is a DCCM, attempt to write to it, and if the resulting
    // latency is non-zero then we are within the DCCM address range.
    //
    if (dccm_c && (cycles = dccm_c->write(addr, block_offset)))
      return cycles;

    // If this write is uncached, or outside the cacheable region, or if
    // the dcache is disabled, then return the latency of 4-byte write 
    // to main memory.
    //
    if (uncached || (addr > cached_limit) || !dcache_enabled) {
      return main_mem_c->write(2);  // 2 indicates a 4-byte fetch (2^2 bytes)
    }

    // Cached write: check if we have a hit in the latency cache
    //
    if (latency_cache.is_write_latency_hit(addr, &cycles)) {
      // We had a hit so return cycle count
      //
      return cycles;
    }
    
    // Not a hit in the latency cache, so go to the dcache
    //
    cycles = dcache_c->write(addr, block_offset, pc);
    
    // Update latency cache with current address
    //
    latency_cache.update_write_entry(addr);

    return cycles;    
  }

};



#endif  // _INC_UARCH_MEMORY_MEMORYMODEL_H_
