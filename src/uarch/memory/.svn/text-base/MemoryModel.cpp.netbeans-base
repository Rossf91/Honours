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

#include "uarch/memory/MemoryModel.h"

#include "Assertion.h"

#include "arch/CoreArch.h"
#include "isa/arc/Dcode.h"

#include "util/CodeBuffer.h"

MemoryModel::MemoryModel (SystemArch&       _sys_arch,
                          uint16            _block_offset,
                          CacheModel*       _icache,
                          CacheModel*       _dcache,
                          CCMModel*         _iccm,
                          CCMModel*         _dccm,
                          MainMemoryModel*  _main_mem)
  : sys_arch(_sys_arch),
  iccm_c(_iccm),
  dccm_c(_dccm),
  main_mem_c(_main_mem),
  block_offset(_block_offset),
  cached_limit(0xffffffff),
  icache_c(_icache),
  dcache_c(_dcache),
  dcache_enabled(_dcache != 0),
  icache_enabled(_icache != 0)
{
  // FIXME: improve co-existence of multiple ICCMs
  if (sys_arch.isa_opts.multiple_iccms) {
    for (int i = 0; i < IsaOptions::kMultipleIccmCount; ++i)
      iccms_c[i]= ((CCMModel**)iccm_c)[i];
  }
  if (icache_c) {
    icache_c->memory_model = this;
    latency_cache.i_block_bits = icache_c->block_bits;
  }
  
  if (dcache_c) {
    dcache_c->memory_model = this;
    if (icache_c == dcache_c) {
      latency_cache.d_block_bits = icache_c->block_bits;
    } else {
      latency_cache.d_block_bits = dcache_c->block_bits;
    }
  }
}

MemoryModel::~MemoryModel() 
{ /* EMPTY */ }

// Memory interface
//
void
MemoryModel::print_stats()
{
  // Caches
  //
  if ((icache_c != 0) && (dcache_c == icache_c)) {
    icache_c->print_stats(); // Unified
  } else {
    if (icache_c) { icache_c->print_stats(); } 
    if (dcache_c) { dcache_c->print_stats(); }
  }
  // Closely coupled memories
  //
  if (sys_arch.isa_opts.multiple_iccms) {
    for(int i = 0; i < IsaOptions::kMultipleIccmCount; ++i){
      if (iccms_c[i]) { iccms_c[i]->print_stats(); }
    }
    if (dccm_c) { dccm_c->print_stats(); }
  }else{
    if ((iccm_c != 0) && (iccm_c == dccm_c)) {
      iccm_c->print_stats();  // Unified
    } else {
      if (iccm_c) { iccm_c->print_stats(); }
      if (dccm_c) { dccm_c->print_stats(); }
    }
  }

}

// ---------------------------------------------------------------------------
// Flush all memory components the MemoryModel is responsible for
//
void
MemoryModel::clear()
{
  // Clear out caches we are responsible for
  //
  latency_cache.flush();
  if (icache_c) { icache_c->clear(); }
  if (dcache_c) { dcache_c->clear(); }
}


// ---------------------------------------------------------------------------
// Method emitting code to access memory
//


void
MemoryModel::jit_emit_instr_memory_access(arcsim::util::CodeBuffer&      buf,
                                          const arcsim::isa::arc::Dcode& inst,
                                          const char*                    addr)
{
  if (inst.is_memory_kind_inst()) {
    switch (inst.kind) {
      case arcsim::isa::arc::Dcode::kMemLoad: {
        buf.append("\tmem_model_read(s,%s);\n", addr);
        break;
      }
      case arcsim::isa::arc::Dcode::kMemStore: {
        buf.append("\tmem_model_write(s,%s);\n", addr);
        break;
      }
      case arcsim::isa::arc::Dcode::kMemExchg: {
        buf.append("\t(mem_model_read(s,%s) + mem_model_write(s,%s));\n",
                   addr, addr);
        break;
      }
      default: break;
    }
  }
}


void
MemoryModel::jit_emit_instr_memory_fetch(arcsim::util::CodeBuffer&      buf,
                                         const arcsim::isa::arc::Dcode& inst,
                                         uint32                         addr)
{
  switch (inst.fetches) {
    case 1: {
      buf.append("\tmem_model_fetch_1(s,0x%08x);\n", addr);
      break; 
    }
    case 2: {
      buf.append("\tmem_model_fetch_2(s,0x%08x,0x%08x);\n", addr, inst.fetch_addr[1]);
      break;
    }
    case 3: {
      buf.append("\t(mem_model_fetch_2(s,0x%08x,0x%08x) + mem_model_fetch_1(s,0x%08x));\n",
                 addr, inst.fetch_addr[1], inst.fetch_addr[2]);
      break;
    }
    default:
      UNREACHABLE();
  }
}



// FIXME: Integrate CCM memory model
//
bool
MemoryModel::jit_emit_memory_model_access_functions(arcsim::util::CodeBuffer& buf,
                                                    CoreArch&                 core_arch)
{
  // WRITE latency
  buf.append("\nstatic inline uint16 mem_model_write(cpuState * const s, uint32 addr) {");
  if (core_arch.dcache.is_configured) {
    buf.append("uint32 blk_addr = addr >> %d;\n", core_arch.dcache.block_bits)
       .append("uint32 index = blk_addr & 0x%08x;\n", (DEFAULT_HASH_CACHE_SIZE-1))
       .append("if ((blk_addr | 0x80000000) == s->lat_cache_d_tag[index]) {\n")
       .append("++s->lat_cache_write_hit;\n")
       .append("return s->lat_cache_write_val;\n")
       .append("} else {\n")
       .append("return cpuWriteCycles(s->cpu_ctx, addr); }\n}\n");
  } else {
    buf.append("return cpuWriteCycles(s->cpu_ctx, addr);\n}\n");
  }
  
  // READ latency
  buf.append("\nstatic inline uint16 mem_model_read(cpuState * const s, uint32 addr) {");
  if (core_arch.dcache.is_configured) {
    buf.append("uint32 blk_addr = addr >> %d;\n", core_arch.dcache.block_bits)
       .append("uint32 index = blk_addr & 0x%08x;\n", (DEFAULT_HASH_CACHE_SIZE-1))
       .append("if (blk_addr == (s->lat_cache_d_tag[index] & 0x7FFFFFFF)) {\n")
       .append("++s->lat_cache_read_hit;\n")
       .append("return s->lat_cache_read_val;\n")
       .append("} else {\n")
       .append("return cpuReadCycles(s->cpu_ctx, addr); }\n}\n");
  } else {
    buf.append("return cpuReadCycles(s->cpu_ctx, addr);\n}\n");
  }

  // FETCH ONE latency
  //
  //  inst->fet_cycles = mem_model->fetch(addr);
  //  state.ibuff_addr = addr >> 2;
  //
  buf.append("\nstatic inline uint16 mem_model_fetch_1(cpuState * const s, uint32 addr) {\n");
  if (core_arch.icache.is_configured) {
    buf.append("uint32 blk_addr = addr >> %d;\n", core_arch.icache.block_bits)
       .append("uint32 index = blk_addr & 0x%08x;\n", (DEFAULT_HASH_CACHE_SIZE-1))
       .append("s->ibuff_addr = addr >> 2;\n")
       .append("if (blk_addr == (s->lat_cache_i_tag[index] & 0x7FFFFFFF)) {\n")
       .append("++s->lat_cache_fetch_hit;\n")
       .append("return s->lat_cache_fetch_val;\n")
       .append("} else {\n")
       .append("return cpuFetchCycles(s->cpu_ctx, addr); } \n}\n");
  } else {
    buf.append("s->ibuff_addr = addr >> 2;\n")
       .append("return cpuFetchCycles(s->cpu_ctx, addr); \n}\n");
  }

  // FETCH TWO latency
  //
  // uint32 word_addr = addr1 >> 2;
  // if (state.ibuff_addr != word_addr) {
  //   inst->fet_cycles  = mem_model->fetch(addr1);
  //                     + mem_model->fetch(addr2);
  // } else {
  //   inst->fet_cycles = mem_model->fetch(addr2);
  // }
  // state.ibuff_addr  = addr2 >> 2;
  //
  buf.append("\nstatic inline uint32 mem_model_fetch_2 (cpuState * const s, uint32 addr1, uint32 addr2) {\n");
  if (core_arch.icache.is_configured) {
    buf.append("uint32 lat = 0;\n")
       .append("uint32 word_addr = addr1 >> 2;\n")
       .append("if (word_addr != s->ibuff_addr) {\n")
       .append("uint32 blk_addr1 = addr1 >> %d;\n", core_arch.icache.block_bits)
       .append("uint32 index = blk_addr1 & 0x%08x;\n", (DEFAULT_HASH_CACHE_SIZE-1))
       .append("if (blk_addr1 == (s->lat_cache_i_tag[index] & 0x7FFFFFFF)) {\n")
       .append("++s->lat_cache_fetch_hit;\n")
       .append("lat = s->lat_cache_fetch_val;\n")
       .append("} else {\n")
       .append("lat = cpuFetchCycles(s->cpu_ctx, addr1); }\n")
       .append("} \n")
       .append("uint32 blk_addr2 = addr2 >> %d;\n", core_arch.icache.block_bits)
       .append("uint32 index = blk_addr2 & 0x%08x;\n", (DEFAULT_HASH_CACHE_SIZE-1))
       .append("if (blk_addr2 == (s->lat_cache_i_tag[index] & 0x7FFFFFFF)) {\n")
       .append("++s->lat_cache_fetch_hit;\n")
       .append("lat += s->lat_cache_fetch_val;\n")
       .append("} else {\n")
       .append("lat += cpuFetchCycles(s->cpu_ctx, addr2); }\n")
       .append("s->ibuff_addr = addr2 >> 2;\n")
       .append("return lat;\n}\n");
  } else {
    buf.append("uint32 lat = 0;\n")
       .append("uint32 word_addr = addr1 >> 2;\n")
       .append("if (word_addr != s->ibuff_addr) {\n")
       .append("lat  = cpuFetchCycles(s->cpu_ctx, addr1); }\n")
       .append("lat += cpuFetchCycles(s->cpu_ctx, addr2);\n")
       .append("s->ibuff_addr = addr2 >> 2\n;")
       .append("return lat;\n}\n");
  }
  return buf.is_valid();
}

  // FETCH 3
  //
  // uint32 fetch_addr = addr1 >> 2;
  // if (state.ibuff_addr != fetch_addr) {
  //   inst->fet_cycles  = mem_model->fetch(addr1);
  //                     + mem_model->fetch(addr2);
  //                     + mem_model->fetch(addr3);
  //   state.ibuff_addr  = addr3 >> 2;
  // } else {
  //   inst->fet_cycles  = mem_model->fetch(addr2);
  //                     + mem_model->fetch(addr3);
  // }
  // state.ibuff_addr  = addr3 >> 2;
  //
  // Equivalent to mem_model_fetch_2, followed by mem_model_fetch_1
