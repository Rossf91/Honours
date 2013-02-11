//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
//  Description:
//
//  System Architecture Configuration.
//
// =====================================================================

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <cstring>

#include "define.h"

#include "Globals.h"

#include "arch/Configuration.h"
#include "arch/CoreArch.h"
#include "arch/ModuleArch.h"

#include "util/Log.h"
#include "util/OutputStream.h"

// System Architecture Statements
//
#define ST_CACHE        0x00001  // Cache
#define ST_CCM          0x00002  // CCM
#define ST_BPU          0x00004  // Branch Predictor
#define ST_WPU          0x00008  // Way Predictor
#define ST_MMU          0x00010  // MMU
#define ST_IFQ          0x00020  // IFQ

#define ST_CORE         0x00100  // Core
#define ST_MODULE       0x00200  // Module
#define ST_GROUP        0x00400  // Group (future)
#define ST_SYSTEM       0x00800  // System

#define ST_ADD_CACHE    0x01000
#define ST_ADD_CCM      0x02000
#define ST_ADD_BPU      0x04000
#define ST_ADD_WPU      0x08000
#define ST_ADD_MMU      0x10000
#define ST_ADD_IFQ      0x20000
#define ST_ADD_CORE     0x40000
#define ST_ADD_MODULE   0x80000

#define VALID_ARCH_MASK 0xC0B00

#define STD_STRLEN      255

// System Architecture Statements struct
//
struct arch_statements {
  char  statement[20];
  int    st_level;
  int    st_level_from;
  int    st_level_to;
};

// System Architecture Statements struct var
//
static struct arch_statements statement[] = {
  //{Statement,       level,      from,       to,         }
  {"CACHE",          ST_CACHE,      0,         0,         },
  {"CCM",            ST_CCM,        0,         0,         },
  {"BPU",            ST_BPU,        0,         0,         },
  {"WPU",            ST_WPU,        0,         0,         },
  {"MMU",            ST_MMU,        0,         0,         },
  {"IFQ",            ST_IFQ,        0,         0,         },
  {"CORE",           ST_CORE,       0,         0,         },
  {"MODULE",         ST_MODULE,     0,         0,         },
  {"SYSTEM",         ST_SYSTEM,     0,         0,         },
  {"ADD_CACHE",      ST_ADD_CACHE,  ST_CORE,   ST_SYSTEM, },
  {"ADD_CCM",        ST_ADD_CCM,    ST_CORE,   ST_CORE,   },
  {"ADD_BPU",        ST_ADD_BPU,    ST_CORE,   ST_CORE,   },
  {"ADD_WPU",        ST_ADD_WPU,    ST_CORE,   ST_CORE,   },
  {"ADD_MMU",        ST_ADD_MMU,    ST_CORE,   ST_CORE,   },
  {"ADD_IFQ",        ST_ADD_IFQ,    ST_CORE,   ST_CORE,   },
  {"ADD_CORE",       ST_ADD_CORE,   ST_MODULE, ST_MODULE, },
  {"ADD_MODULE",     ST_ADD_MODULE, ST_SYSTEM, ST_SYSTEM, },
  {"End_of_Struct",  0,             0,         0,         },
};


// -----------------------------------------------------------------------------
//  Configuration class
//
Configuration::Configuration()
{ /* EMPTY */ }

Configuration::~Configuration()
{ /* EMPTY */ }


// -----------------------------------------------------------------------------
//  Create new CACHE
//
int
Configuration::create_new_cache(char *il)
{
  CacheArch cacheArch;
  char      replacement_policy;
  bool      success = true;

  if (sscanf(il, "%*s %s %u %u %u %c %u %u %u",
             cacheArch.name,
             &cacheArch.size,
             &cacheArch.block_bits,
             &cacheArch.ways,
             &replacement_policy,
             &cacheArch.bus_width,
             &cacheArch.latency,
             &cacheArch.bus_clk_div) != 8) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (cache).";
    success = false;
  }

  for (std::list<CacheArch>::const_iterator I = cache_list.begin(), E = cache_list.end();
       I != E; ++I)
  {
    if (!strcmp(cacheArch.name, I->name)) {
      LOG(LOG_ERROR) << "Duplicate System Architecture CACHE Name.";
      success = false;
    }
  }

  switch(replacement_policy) {
    case 'L': {
        cacheArch.repl = LRU_REPL;
        LOG(LOG_ERROR) << "LRU not implemented.";
        success = false;
        break;
    }
    case 'R': { cacheArch.repl = RAND_REPL;  break; }
    case 'F': {
        cacheArch.repl = LFU_REPL;
        LOG(LOG_ERROR) << "LFU not implemented.";
        success = false;
        break;
    }
    default: {
        LOG(LOG_ERROR) << "Invalid Cache Replacement Policy.";
        success = false;
        break;
    }
  }

  if (cacheArch.block_bits < 2) {
    LOG(LOG_ERROR) << "Cache Block Offset must be 2 or more.";
    success = false;
  }

  if (cacheArch.ways < 1) {
    LOG(LOG_ERROR) << "Number of Ways must be 1 or more.";
    success = false;
  }

  uint32 blk_size = 1 << cacheArch.block_bits;
  if (cacheArch.size % blk_size) {
    LOG(LOG_ERROR) << "Cache Size not a multiple of block size.";
    success = false;
  }

  uint32 total_blks = cacheArch.size / blk_size;
  if (total_blks % cacheArch.ways) {
    LOG(LOG_ERROR) << "Total blocks not a multiple of Ways.";
    success = false;
  }

  uint32 sets = total_blks / cacheArch.ways;
  if (!IS_POWER_OF_TWO(sets)) {
    LOG(LOG_ERROR) << "Number of sets not a power of 2.";
    success = false;
  }

  if (!success) {
    // FIXME: Don't just exit here, but continue scanning config file.
    //
    exit (EXIT_FAILURE);
  }

  // Cache architecture configured correctly
  //
  cacheArch.is_configured = true;

  // Add cache architecture to list
  //
  cache_list.push_back(cacheArch);

  return 0;
};

// -----------------------------------------------------------------------------
//  Create new SCRATCHPAD
//
int
Configuration::create_new_spad(char *il)
{
  bool      success = true;
  SpadArch  spadArch;
  
  if (sscanf(il, "%*s %s %u %u %u %u",
             spadArch.name,
             &spadArch.start_addr,
             &spadArch.size,
             &spadArch.bus_width,
             &spadArch.latency) != 5)
  {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (spad).";
    exit (EXIT_FAILURE);
  }
  
  for (std::list<SpadArch>::const_iterator I = spad_list.begin(), E = spad_list.end();
       I != E; ++I)
  {
    if (!strcmp(spadArch.name, I->name)) {
      LOG(LOG_ERROR) << "Duplicate System Architecture SCRATCHPAD Name.";
      success = false;
    }
  }
  
  if (   (spadArch.start_addr & 3UL)
      || (spadArch.size & 3UL)) {
    LOG(LOG_ERROR) << "CCM start address and size must be long word (32-bit) values.";
    success = false;
  }
  
  if (!success) {
    // FIXME: Don't just exit here, but continue scanning config file.
    //
    exit (EXIT_FAILURE);
  }
  
  // Scratchpad architecture configured correctly
  //
  spadArch.is_configured = true;
  
  // Add Scratchpad architecture to list
  //
  spad_list.push_back(spadArch);
  
  return 0;
};


// -----------------------------------------------------------------------------
//  Create new MMU
//
int
Configuration::create_new_mmu(char *il)
{
  MmuArch mmuArch;
  uint32  page_size = 0;
  uint32  jtlb_sets = 0;
  uint32  jtlb_ways = 0;
  bool    success = true;

  int ret = sscanf(il, "%*s %s %u %u",
                   mmuArch.name,
                   &mmuArch.kind,
                   &mmuArch.version);
  if (ret != 3 ) { // there must be 3 initial parameters
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (mmu).";
    success = false;
  }
  if(mmuArch.kind>MmuArch::kMpu) { // only valid kind is MMU(0) or MPU(1)
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (mmu), kind must be 0(MMU) or 1(MPU).";
    success = false;
  }
  switch (mmuArch.kind){
    case MmuArch::kMmu:
    {

      // check MMU version is within supported range
      if (mmuArch.version > MmuArch::kMmuV3) {
        LOG(LOG_ERROR) << "Illegal System Architecture parameters (mmu version).";
        success = false;
      }
      // check MMU page size is within supported range if MMUv3 is used
      if (mmuArch.version == MmuArch::kMmuV3) {

        ret = sscanf(il, "%*s %*s %*u %*u %u %u %u",
                     &page_size,
                     &jtlb_sets,
                     &jtlb_ways);
        if (ret > 3) { // there must be between 3 parameters
          LOG(LOG_ERROR) << "Illegal System Architecture parameters (mmu). MMUv3 only requires: page size, jtlb sets, jtlb ways";
          success = false;
        }
        if (mmuArch.set_page_size(page_size)) {
          // store MMU page size correctly for MMUv3 so PageArch class is instantiated properly
          sys_arch.sim_opts.page_size_log2 = (PageArch::PageSizeLog2)mmuArch.get_page_size_log2();
          LOG(LOG_DEBUG) << "[MMU] MMUv3 configured page size: '" << mmuArch.get_page_size() << "B'.";
        } else {
          LOG(LOG_ERROR) << "Illegal MMU size in bytes: " << page_size;
          success = false;
        }
        // Check that MMU 'set' and 'way' settings are within supported range
        if (jtlb_sets != 0 && mmuArch.set_jtlb_sets(jtlb_sets)) {
          LOG(LOG_DEBUG) << "[MMU] MMUv3 configured JTLB SETS: '" << mmuArch.get_jtlb_sets() << "'.";
        } else {
          LOG(LOG_ERROR) << "Illegal MMU JTLB SET configuration: " << jtlb_sets;
          success = false;
        }
        if (jtlb_ways != 0 && mmuArch.set_jtlb_ways(jtlb_ways)) {
          LOG(LOG_DEBUG) << "[MMU] MMUv3 configured JTLB WAYS: '" << mmuArch.get_jtlb_ways() << "'.";
        } else {
          LOG(LOG_ERROR) << "Illegal MMU JTLB WAY configuration: " << jtlb_ways;
          success = false;
        }
      }
    }break;

    case MmuArch::kMpu:
    {
      if (mmuArch.set_page_size(PageArch::k2KPageSize)) {
        // store MMU page size correctly for MPU so PageArch class is instantiated properly
        sys_arch.sim_opts.page_size_log2 = (PageArch::PageSizeLog2)mmuArch.get_page_size_log2();
        LOG(LOG_DEBUG) << "[MMU] MPU configured page size: '" << mmuArch.get_page_size() << "B'.";
      }
      ret = sscanf(il, "%*s %*s %*u %*u %u",
                   &mmuArch.mpu_num_regions);
      if(ret != 1) { // there must be 1 parameter
        LOG(LOG_ERROR) << "Illegal System Architecture parameters (mmu). MPUv2 requires: num regions";
        success = false;
      }
      
    }break;
  }
  
  for (std::list<MmuArch>::const_iterator I = mmu_list.begin(), E = mmu_list.end();
       I != E; ++I)
  {
    if (!strcmp(mmuArch.name, I->name)) {
      LOG(LOG_ERROR) << "Duplicate System Architecture MMU Name.";
      success = false;
    }
  }
  
  if (!success) {
    // FIXME: Don't just exit here, but continue scanning config file.
    exit (EXIT_FAILURE);
  }

  // MMU architecture configured correctly
  //
  mmuArch.is_configured = true;

  // Add MMU architecture to list
  //
  mmu_list.push_back(mmuArch);

  return 0;
};

// -----------------------------------------------------------------------------
//  Create new IFQ
//
int
Configuration::create_new_ifq(char *il)
{
  IfqArch ifqArch;
  bool    success = true;
  
  if (sscanf(il, "%*s %s %u", ifqArch.name, &ifqArch.size) != 2) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (ifq).";
    success = false;
  }
  
  for (std::list<IfqArch>::const_iterator I = ifq_list.begin(), E = ifq_list.end();
       I != E; ++I)
  {
    if (!strcmp(ifqArch.name, I->name)) {
      LOG(LOG_ERROR) << "Duplicate System Architecture IFQ Name.";
      success = false;
    }
  }
  
  if (!success) {
    // FIXME: Don't just exit here, but continue scanning config file.
    //
    exit (EXIT_FAILURE);
  }
  
  // IFQ architecture configured correctly
  //
  ifqArch.is_configured = true;
  
  // Add IFQ architecture to list
  //
  ifq_list.push_back(ifqArch);
  
  return 0;
};


// -----------------------------------------------------------------------------
//  Create new BPU
//
int
Configuration::create_new_bpu(char *il)
{
  BpuArch bpuArch;
  bool    success = true;
  
  if (sscanf(il, "%*s %s %c ", bpuArch.name, &bpuArch.bp_type) != 2) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (bpu).";
    success = false;
  }
  
  for (std::list<BpuArch>::const_iterator I = bpu_list.begin(), E = bpu_list.end();
       I != E; ++I)
  {
    if (!strcmp(bpuArch.name, I->name)) {
      LOG(LOG_ERROR) << "Duplicate System Architecture BPU Name.";
      success = false;
    }
  }
  
  switch (bpuArch.bp_type) {
    case 'O': // Oracle
    {
      if (sscanf(il, "%*s %s %s %u",
                 bpuArch.name,
                 &bpuArch.bp_type,
                 &bpuArch.miss_penalty) != 3) {
        LOG(LOG_ERROR) << "Illegal System Architecture parameters (O bpu).";
        success = false;
      }
      break;
    }
    case 'A': // GShare predictor
    case 'E': // GSelect predictor
    {
      if (sscanf(il, "%*s %s %s %u %u %u %u %u",
                 bpuArch.name,
                 &bpuArch.bp_type,
                 &bpuArch.bht_entries,
                 &bpuArch.ways,
                 &bpuArch.sets,
                 &bpuArch.ras_entries,
                 &bpuArch.miss_penalty) != 7) {
        LOG(LOG_ERROR) << "Illegal System Architecture parameters (GShare, GSelect bpu).";
        success = false;
      }
      break;
    }
    default:
      LOG(LOG_ERROR) << "Illegal System Architecture Branch Predictor type.";
      success = false;
  }
  
  if (!success) {
    // FIXME: Don't just exit here, but continue scanning config file.
    //
    exit (EXIT_FAILURE);
  }
  
  // BPU architecture configured correctly
  //
  bpuArch.is_configured = true;
  
  // Add BPU architecture to list
  //
  bpu_list.push_back(bpuArch);
  
  
  return 0;
};

// -----------------------------------------------------------------------------
//  Create new WPU
//
int
Configuration::create_new_wpu(char *il)
{
  WpuArch wpuArch;
  bool    success = true;
  char    phased;

  if (sscanf(il, "%*s %s %u %u %u %u %u %c",
             wpuArch.name,
             &wpuArch.entries,
             &wpuArch.indices,
             &wpuArch.ways,
             &wpuArch.size,
             &wpuArch.block_size,
             &phased) != 7) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (wpu).";
    exit (EXIT_FAILURE);
  }
  wpuArch.phased = phased;
  
  
  for (std::list<WpuArch>::const_iterator I = wpu_list.begin(), E = wpu_list.end();
       I != E; ++I)
  {
    if (!strcmp(wpuArch.name, I->name)) {
      LOG(LOG_ERROR) << "Duplicate System Architecture WPU Name.";
      success = false;
    }
  }

  if (!success) {
    // FIXME: Don't just exit here, but continue scanning config file.
    //
    exit (EXIT_FAILURE);
  }

  // WPU architecture configured correctly
  //
  wpuArch.is_configured = true;

  // Add WPU architecture to list
  //
  wpu_list.push_back(wpuArch);

  return 0;
};

// -----------------------------------------------------------------------------
//  Create new CORE
//
void*
Configuration::create_new_core(char *il)
{
  PageArch* pageArch = new PageArch(sys_arch.sim_opts.page_size_log2);

  // FIXME: HEAP allocated PageArch will leak when CoreArch is destructed
  //
  CoreArch* coreArch = new CoreArch(pageArch);
  
  
  char      input_line[STD_STRLEN], st_pipeline[STD_STRLEN];
  int       lineno;
  unsigned int code, latency;
  bool      success = true;

  if (sscanf(il, "%*s %s %u %u %s %u",
             coreArch->name,
             &coreArch->cpu_clock_divisor,
             &coreArch->cpu_data_bus_width,
             st_pipeline,
             &coreArch->cpu_warmup_cycles) != 5) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (CORE).";
    exit (EXIT_FAILURE);
  }
 
  
  for (std::list<CoreArch*>::const_iterator I = core_list.begin(), E = core_list.end();
       I != E; ++I)
  {
    if (!strcmp(coreArch->name, (*I)->name)) {
      LOG(LOG_ERROR) << "Duplicate System Architecture CORE Name.";
      success = false;
    }
  }

  if (!success) {
    // FIXME: Don't just exit here, but continue scanning config file.
    //
    exit (EXIT_FAILURE);
  }

  coreArch->cache_types     = CacheArch::kNoCache;
  coreArch->spad_types      = SpadArch::kNoSpad;
  coreArch->wpu_types       = 0;

  // FIXME: Add const string tokens for known pipeline variants
  //
  if (!strcmp(st_pipeline, "EC5")) {
    coreArch->pipeline_variant = E_PL_EC5;
  } else if (!strcmp(st_pipeline, "EC7")) {
    coreArch->pipeline_variant = E_PL_EC7;
  } else if (!strcmp(st_pipeline, "SKIPJACK")) {
    coreArch->pipeline_variant = E_PL_SKIPJACK;
  } else {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (CORE).";
    LOG(LOG_ERROR) << "Pipeline model '" << st_pipeline << "' unsupported.";
    exit(EXIT_FAILURE); 
  }

  if (!IS_POWER_OF_TWO(coreArch->cpu_data_bus_width >> 3)) {
    LOG(LOG_ERROR) << "Data Bus Width not power of two.";
    exit (EXIT_FAILURE);
  }

  coreArch->cpu_bo = 0;
  int bin = (coreArch->cpu_data_bus_width >> 3) - 1;
  while (bin) {
      bin >>= 1;
      ++coreArch->cpu_bo;
  }

  if (!sys_arch.sim_opts.cycle_sim) {
    // functional simulation
    //
    coreArch->isa_cyc = false;
    LOG(LOG_INFO) << "Using 'functional' target ISA.";
  } else {
    // cycle accurate simulation
    //
    LOG(LOG_INFO) << "Using '" << sys_arch.sim_opts.isa_file << "' ISA file.";
    coreArch->isa_cyc = true;
    std::ifstream fin;
    lineno = 0;
    fin.open(sys_arch.sim_opts.isa_file.c_str());
    if (fin.is_open()) {
      char  char_buf[STD_STRLEN];
      while (!fin.eof()) { // valid .isa file
        lineno++;
        fin.getline(input_line, STD_STRLEN);
        if ((sscanf(input_line, "%s", char_buf) != 1) || (input_line[0] == '#')) {
          // skip empty-line, comment
        } else if (sscanf(input_line, "%i %i", &code, &latency) == 2) {
          coreArch->isa[code] = latency;
        } else { 
          LOG(LOG_ERROR) << "ERROR [FILE:'" << sys_arch.sim_opts.isa_file << "' LINE:" << lineno << "]: "
                         << "Invalid .isa file entry.";
          exit (EXIT_FAILURE);
        }
      }
      fin.clear();
      fin.close();
    } else {
      LOG(LOG_ERROR) << "Invalid Core .isa filename: '" << sys_arch.sim_opts.isa_file << "'.";
      exit (EXIT_FAILURE);
    }
  }

  // coreArch architecture configured correctly
  //
  coreArch->is_configured = true;
  
  // Add coreArch architecture to list
  //
  core_list.push_back(coreArch);
  
  return (void *)coreArch;
};

// -----------------------------------------------------------------------------
//  Create new MODULE
//
void*
Configuration::create_new_module(char *il)
{
  ModuleArch* moduleArch = new ModuleArch();
  bool        success = true;

  if (sscanf(il, "%*s %s", moduleArch->name) != 1)
  {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (module).";
    exit (EXIT_FAILURE);
  }

  for (std::list<ModuleArch*>::const_iterator I = module_list.begin(), E = module_list.end();
       I != E; ++I)
  {
    if (!strcmp(moduleArch->name, (*I)->name)) {
      LOG(LOG_ERROR) << "Duplicate System Architecture MODULE Name.";
      success = false;
    }
  }
  
  if (!success) {
    // FIXME: Don't just exit here, but continue scanning config file.
    //
    exit (EXIT_FAILURE);
  }

  moduleArch->number_core_types = 0;
  moduleArch->cache_types       = CacheArch::kNoCache;

  // ModuleArch is configured correctly
  //
  moduleArch->is_configured = true;
  

  // Add moduleArch architecture to list
  //
  module_list.push_back(moduleArch);
  
  return (void *)moduleArch;
};

// -----------------------------------------------------------------------------
// Create SYSTEM
//
void*
Configuration::create_system(char *il)
{
  if (sscanf(il, "%*s %s %u %u %u %u %u %u", 
             sys_arch.name,
             &sys_arch.master_clock_freq,
             &sys_arch.mem_start_addr,
             &sys_arch.mem_end_addr,
             &sys_arch.mem_dbw,
             &sys_arch.mem_latency,
             &sys_arch.mem_bus_clk_div) != 7) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (system).";
    exit (EXIT_FAILURE);
  }

  sys_arch.master_clock_freq   *= 1000000;

  if ((sys_arch.mem_start_addr & 3UL) || (sys_arch.mem_end_addr & 3UL)) {
    LOG(LOG_ERROR) << "Main memory start and end addresses must be long word (32-bit) aligned.";
    exit (EXIT_FAILURE);
  }

  // System is configured correctly
  //
  sys_arch.is_configured = true;
  
  return (void *)&sys_arch;
}

// -----------------------------------------------------------------------------
// Add a Cache to a component
//
int
Configuration::add_cache(int &level, void *section, char *il)
{
  CacheArch cacheArch;
  char      type;

  if (sscanf(il, "%*s %s %c", cacheArch.name, &type) != 2) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (add cache).";
    exit (EXIT_FAILURE);
  }

  bool found = false;

  for (std::list<CacheArch>::const_iterator I = cache_list.begin(), E = cache_list.end();
       I != E; ++I)
  {
    if (!strcmp(cacheArch.name, I->name)) {
      cacheArch = *I;
      found     = true;
      break;
    }
  }

  if (!found) {
    LOG(LOG_ERROR) << "System Architecture Cache statement not found.";
    exit (EXIT_FAILURE);
  }

  switch (level) {
          // Add cache to Core
    case ST_CORE:
          switch (type) {
            case 'I':
              ((CoreArch *)section)->icache = cacheArch;
              ((CoreArch *)section)->cache_types |= CacheArch::kInstCache;
              break;
            case 'D':
              ((CoreArch *)section)->dcache = cacheArch;
              ((CoreArch *)section)->cache_types |= CacheArch::kDataCache;
              break;
            case 'U':
              ((CoreArch *)section)->icache = cacheArch;
              ((CoreArch *)section)->dcache = cacheArch;
              ((CoreArch *)section)->cache_types |= CacheArch::kUnifiedCache;
              break;
            default:
              LOG(LOG_ERROR) << "Invalid cache type.";
              exit (EXIT_FAILURE);
              break;
          }
          break;
          // Add cache to Module
    case ST_MODULE:
          switch (type) {
            case 'I':
              ((ModuleArch *)section)->icache = cacheArch;
              ((ModuleArch *)section)->cache_types |= CacheArch::kInstCache;
              break;
            case 'D':
              ((ModuleArch *)section)->dcache = cacheArch;
              ((ModuleArch *)section)->cache_types |= CacheArch::kDataCache;
              break;
            case 'U':
              ((ModuleArch *)section)->icache = cacheArch;
              ((ModuleArch *)section)->dcache = cacheArch;
              ((ModuleArch *)section)->cache_types |= CacheArch::kUnifiedCache;
              break;
            default:
              LOG(LOG_ERROR) << "Invalid cache type.";
              exit (EXIT_FAILURE);
              break;
          }
          break;
          // Add cache to System
    case ST_SYSTEM:
          switch (type) {
            case 'I':
              ((SystemArch *)section)->icache = cacheArch;
              ((SystemArch *)section)->cache_types |= CacheArch::kInstCache;
              break;
            case 'D':
              ((SystemArch *)section)->dcache = cacheArch;
              ((SystemArch *)section)->cache_types |= CacheArch::kDataCache;
              break;
            case 'U':
              ((SystemArch *)section)->icache = cacheArch;
              ((SystemArch *)section)->dcache = cacheArch;
              ((SystemArch *)section)->cache_types |= CacheArch::kUnifiedCache;
              break;
            default:
              LOG(LOG_ERROR) << "Invalid cache type.";
              exit (EXIT_FAILURE);
              break;
          }
          break;
    default: {
          LOG(LOG_ERROR) << "Unexpected section to add cache.";
          exit (EXIT_FAILURE);
    }
  }

  return 0;
}

// -----------------------------------------------------------------------------
// Add a Scratchpad to a component
//
int
Configuration::add_spad(int &level, void *section, char *il)
{
  char    st_name[SpadArch::kSpadArchMaxNameSize];
  char    type;

  if (sscanf(il, "%*s %s %c", st_name, &type) != 2) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (add spad).";
    exit (EXIT_FAILURE);
  }

  for (std::list<SpadArch>::const_iterator I = spad_list.begin(), E = spad_list.end();
       I != E; ++I)
  {
    if (!strcmp(st_name, I->name)) {
      switch (level)
      { // Add scratchpad to Core
        //
        case ST_CORE: {
          switch (type) {
            case 'I':
              ((CoreArch *)section)->iccm = *I;
              ((CoreArch *)section)->spad_types |= SpadArch::kSpadInstCcm;
              
              // configure latency for slow memory ICCMs
              if ((I->latency > 0) && (I->latency <= 4)) {
                if(((CoreArch *)section)->iccms[I->latency-1].is_configured == false) {
                  if (I->size <= (8 * arcsim::MB)) { // check maximum size of ICCM
                    ((CoreArch *)section)->iccms[I->latency-1] = *I;
                  } else {
                    LOG(LOG_ERROR) << "Scratchpad too large for region" << (uint32)I->latency;
                  }
                } else {
                  LOG(LOG_ERROR) << "Scratchpad already assigned for region" << (uint32)I->latency;
                }
              }
              
              break;
            case 'D':
              ((CoreArch *)section)->dccm = *I;
              ((CoreArch *)section)->spad_types |= SpadArch::kSpadDataCcm;
              break;
            case 'U':
              ((CoreArch *)section)->iccm = *I;
              ((CoreArch *)section)->dccm = *I;
              ((CoreArch *)section)->spad_types |= SpadArch::kSpadUnifiedCcm;
              break;
            default:
              LOG(LOG_ERROR) << "Invalid scratchpad type.";
              exit (EXIT_FAILURE);
              break;
          } // end switch (type)
          break;
        }
        default: {
          LOG(LOG_ERROR) << "Unexpected section to add scratchpad.";
          exit (EXIT_FAILURE);
        } 
      }
      
      return 0;
    }
  }

  LOG(LOG_ERROR) << "System Architecture Scratchpad statement not found.";
  exit (EXIT_FAILURE);
  
  return 0;
}

// -----------------------------------------------------------------------------
// Add a MMU to a component
//
int
Configuration::add_mmu(int &level, void *section, char *il)
{
  char    st_name[MmuArch::kMmuArchMaxNameSize];
  
  if (sscanf(il, "%*s %s", st_name) != 1) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (add mmu).";
    exit (EXIT_FAILURE);
  }
  
  for (std::list<MmuArch>::const_iterator I = mmu_list.begin(), E = mmu_list.end();
       I != E; ++I)
  {
    if (!strcmp(st_name, I->name)) {
      ((CoreArch *)section)->mmu_arch = *I;
      return 0;
    }
  }
  
  LOG(LOG_ERROR) << "System Architecture MMU statement not found.";
  exit (EXIT_FAILURE);
  return 0;
}

// -----------------------------------------------------------------------------
// Add a IFQ to a component
//
int
Configuration::add_ifq(int &level, void *section, char *il)
{
  char    st_name[IfqArch::kIfqArchMaxNameSize];
  
  if (sscanf(il, "%*s %s", st_name) != 1) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (add ifq).";
    exit (EXIT_FAILURE);
  }
  
  for (std::list<IfqArch>::const_iterator I = ifq_list.begin(), E = ifq_list.end();
       I != E; ++I)
  {
    if (!strcmp(st_name, I->name)) {
      ((CoreArch *)section)->ifq_arch = *I;
      //FIXME: isa_opts shouldn't even have the ifq size, but for some reason it's available as a command line argument
      sys_arch.isa_opts.ifq_size = (*I).size;
      return 0;
    }
  }
  
  LOG(LOG_ERROR) << "System Architecture IFQ statement not found.";
  exit (EXIT_FAILURE);
  return 0;
}


// -----------------------------------------------------------------------------
// Add a BPU to a component
//
int
Configuration::add_bpu(int &level, void *section, char *il)
{
  char    st_name[BpuArch::kBpuArchMaxNameSize];
  char    empty;

  if (sscanf(il, "%*s %s %c", st_name, &empty) != 1) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (add bpu).";
    exit (EXIT_FAILURE);
  }

  bool found = false;

  for (std::list<BpuArch>::const_iterator I = bpu_list.begin(), E = bpu_list.end();
       I != E && !found; ++I)
  {
    if (!strcmp(st_name, I->name)) {
      ((CoreArch *)section)->bpu = *I;
      return 0;
    }
  }

  LOG(LOG_ERROR) << "System Architecture BPU statement not found.";
  exit (EXIT_FAILURE);
  return 0;
}

// -----------------------------------------------------------------------------
// Add a WPU to a component
//
int
Configuration::add_wpu(int &level, void *section, char *il)
{
  char st_name[WpuArch::kWpuArchMaxNameSize];
  char type;

  if (sscanf(il, "%*s %s %c", st_name, &type) != 2) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (add wpu).";
    exit (EXIT_FAILURE);
  }


  for (std::list<WpuArch>::const_iterator I = wpu_list.begin(), E = wpu_list.end();
       I != E; ++I)
  {
    if (!strcmp(st_name, I->name)) {
      switch (level) {
          // Add wpu to Core
        case ST_CORE:
          switch (type) {
            case 'I':
              ((CoreArch *)section)->iwpu = *I;
              ((CoreArch *)section)->wpu_types |= CacheArch::kInstCache;
              break;
            case 'D':
              ((CoreArch *)section)->dwpu = *I;
              ((CoreArch *)section)->wpu_types |= CacheArch::kDataCache;
              break;
            default:
              LOG(LOG_ERROR) << "Invalid wpu type.";
              exit (EXIT_FAILURE);
              break;
          }
          break;
        default:
          LOG(LOG_ERROR) << "Unexpected section to add wpu.";
          exit (EXIT_FAILURE);
          break;
      }      
      return 0;
    }
  }

  LOG(LOG_ERROR) << "System Architecture WPU statement not found.";
  exit (EXIT_FAILURE);
  return 0;
}

// Add a Core to a component
//
int
Configuration::add_core(int &level, void *section, char *il)
{
  char      st_name[CoreArch::kCoreArchMaxNameSize];
  uint32    i[10];

  if (sscanf(il, "%*s %s %u", st_name, &i[0]) != 2) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (add core).";
    exit (EXIT_FAILURE);
  }
  
  for (std::list<CoreArch*>::const_iterator I = core_list.begin(), E = core_list.end();
       I != E; ++I)
  {
    if (!strcmp(st_name, (*I)->name)) {
      switch (level) {
          // Add core to Module
          //
        case ST_MODULE:
        {
          int  nt = ((ModuleArch *)section)->number_core_types++;
          ((ModuleArch *)section)->core_type[nt]     = *I;
          ((ModuleArch *)section)->cores_of_type[nt] = i[0];
          break;
        }
        default: {
          LOG(LOG_ERROR) << "Unexpected section to add core.";
          exit (EXIT_FAILURE);
        }
      }
      return 0;
    }
  }

  LOG(LOG_ERROR) << "System Architecture Core statement not found.";
  exit (EXIT_FAILURE);
  return 0;
}

// Add a Module to a component
//
int
Configuration::add_module(int &level, void *section, char *il)
{
  char    st_name[ModuleArch::kModuleArchMaxNameSize];
  uint32  i[10];

  if (sscanf(il, "%*s %s %u", st_name, &i[0]) != 2) {
    LOG(LOG_ERROR) << "Illegal System Architecture parameters (add module).";
    exit (EXIT_FAILURE);
    return 0;
  }

  for (std::list<ModuleArch*>::const_iterator I = module_list.begin(), E = module_list.end();
       I != E; ++I)
  {
    if (!strcmp(st_name, (*I)->name)) {
      switch (level) {
          // Add core to Module
          //
        case ST_SYSTEM: {
          int  nt = ((SystemArch *)section)->number_module_types++;
          ((SystemArch *)section)->module_type[nt]     = *I;
          ((SystemArch *)section)->modules_of_type[nt] = i[0];
          break;
        }
        default: {
          LOG(LOG_ERROR) << "Unexpected section to add module.";
          exit (EXIT_FAILURE);
        }
      }
      return 0;
    }
  }

  LOG(LOG_ERROR) << "System Architecture Module statement not found.";
  exit (EXIT_FAILURE);
  return 0;
}

bool
Configuration::print_caches(arcsim::util::OutputStream& S,
                            uint32                      cache_types,
                            int                         indent,
                            CacheArch                   ic,
                            CacheArch                   dc)
{

  if (cache_types) {
    if (cache_types & CacheArch::kInstCache) {
      S.Get() << "|" << std::string(indent, ' ')
              << "CACHE: " << ic.name << " (Inst: size=" << ic.size
              << " ways=" << ic.ways << " rp=" << ic.repl << ")" << std::endl;
    }
    if (cache_types & CacheArch::kDataCache) {
      S.Get() << "|" << std::string(indent, ' ')
              << "CACHE: " << dc.name << " (Data: size=" << dc.size
              << " ways=" << dc.ways << " rp=" << dc.repl << ")" << std::endl;
    }
    if (cache_types & CacheArch::kUnifiedCache) {
      S.Get() << "|" << std::string(indent, ' ')
              << "CACHE: " << ic.name << " (Unif: size=" << ic.size
              << " ways=" << ic.ways << " rp=" << ic.repl << ")" << std::endl;
    }
  }
  return 0;
}

bool
Configuration::print_spads(arcsim::util::OutputStream&  S,
                           uint32                       spad_types,
                           int                          indent,
                           SpadArch                     ic,
                           SpadArch                     *ics,
                           SpadArch                     dc)
{
  if (spad_types) {
    if (spad_types & SpadArch::kSpadInstCcm) {
      S.Get() << "|" << std::string(indent, ' ')
              << "CCM: " << ic.name << " (Inst: start=" << ic.start_addr
              << " size=" << ic.size << ")" << std::endl;
      for (int i = 0; i < IsaOptions::kMultipleIccmCount; ++i) {
        S.Get() << "|" << std::string(indent, ' ')
                << "CCM" << i << " :" << ics[i].name << " (Inst: start=" << ics[i].start_addr
                << " size=" << ics[i].size << ")" << std::endl;
      }

    }
    if (spad_types & SpadArch::kSpadDataCcm) {
      S.Get() << "|" << std::string(indent, ' ')
              << "CCM: " << dc.name << " (Data: start=" << dc.start_addr
              << " size=" << dc.size << ")" << std::endl;
    }
    if (spad_types & SpadArch::kSpadUnifiedCcm) {
      S.Get() << "|" << std::string(indent, ' ')
              << "CCM: " << ic.name << " (Unif: start=" << ic.start_addr
              << " size=" << ic.size << ")" << std::endl;      
    }
  }
  return 0;
}

// Print out System Architecture
//
bool
Configuration::print_architecture(arcsim::util::OutputStream& S, std::string &afile)
{
  S.Get() << "System Architecture [FILE:'" << afile << "']\n" 
          << "================================================================================\n"
          << "| SYSTEM: " << sys_arch.name << "\n";
  
  print_caches(S, sys_arch.cache_types, 2, sys_arch.icache, sys_arch.dcache);

  for (uint32 m=0; m < sys_arch.number_module_types; ++m) {
    S.Get() << "|\n";
    ModuleArch* mod = sys_arch.module_type[m];
    S.Get() << "!  MODULE: " << mod->name << "  [x" << sys_arch.modules_of_type[m] << "]\n";
    print_caches(S, mod->cache_types, 4, mod->icache, mod->dcache);

    for (uint32 c=0; c < mod->number_core_types; ++c) {
      S.Get() << "|\n";
      CoreArch* core = mod->core_type[c];
      S.Get() << "!    CORE: '"   <<  core->name;
      
      if (core->mmu_arch.is_configured) {
        S.Get() << " [MMU:V" << core->mmu_arch.version << "]";
      } else {
        S.Get() << " [MMU:none]";
      }
      if (core->ifq_arch.is_configured) {
        S.Get() << " [IFQ:SIZ-" << core->ifq_arch.size << "]";
      } else {
        S.Get() << " [IFQ:none]";
      }
      S.Get() << "' [PL:" <<  core->pipeline_variant << "] [#"
              << mod->cores_of_type[c] << "]\n";
      print_caches(S, core->cache_types, 6,  core->icache,  core->dcache);
      print_spads (S, core->spad_types,  6,  core->iccm, core->iccms,  core->dccm);
    }
  }
  S.Get() << "================================================================================\n";

  return 0;
}

// -----------------------------------------------------------------------------
// Read in simulation options
//
bool
Configuration::read_simulation_options(int argc, char *argv[])
{
  return sys_arch.sim_opts.get_sim_opts(this, argc, argv);
}

// -----------------------------------------------------------------------------
// Read in System Configuration
//
bool
Configuration::read_architecture(std::string& sarch_file,
                                 bool         print_sarch,
                                 bool         print_sarch_file)
{
  std::ifstream fin;
  char *input_line, iline[STD_STRLEN], st_type[STD_STRLEN];
  int level, valid_arch, lineno;
  bool match, order, input_data;
  void *section = 0;
  arcsim::util::OutputStream S(stderr);
  
  lineno = 0;          // line number
  level  = 0;          // statement level
  valid_arch = 0;      // verify minimum valid statements used
  input_line = iline;
  input_data = true;

  fin.open(sarch_file.c_str());
  if (!fin.is_open()) {
    LOG(LOG_ERROR) << "System Architecture File '" << sarch_file << "' not found."; 
    exit (EXIT_FAILURE);
  }

  LOG(LOG_INFO) << "Using '" << sarch_file << "' System Architecture File.";

  while (input_data)
  {
    if (fin.eof()) {
      input_data = false;
    } else { // read in next line
      fin.getline(input_line, STD_STRLEN);
      lineno++;
    }

    if (input_data) {

      match = false;  // matching statement
      order = false;  // statement in correct order

      if ((sscanf(input_line, "%s", st_type) != 1) || (input_line[0] == '#')) {
        match = true;
      } else {
        for (int s=0; !match && statement[s].st_level; s++)
        {
          match = !strcmp(st_type, statement[s].statement);
          if (match)
          {
            valid_arch = valid_arch | statement[s].st_level;
            
            if (print_sarch_file) { S.Get() << input_line << std::endl; }

            switch(statement[s].st_level) {

              case ST_CACHE:
                    if (level <= statement[s].st_level) {
                      order = true;
                      create_new_cache(input_line);
                    }
                    level = statement[s].st_level;
                    break;
              case ST_CCM:
                    if (level <= statement[s].st_level) {
                      order = true;
                      create_new_spad(input_line);
                    }
                    level = statement[s].st_level;
                    break;
              case ST_MMU:
                    if (level <= statement[s].st_level) {
                      order = true;
                      create_new_mmu(input_line);
                    }
                    level = statement[s].st_level;
                    break;
              case ST_IFQ:
                    if (level <= statement[s].st_level) {
                      order = true;
                      create_new_ifq(input_line);
                    }
                    level = statement[s].st_level;
                    break;
              case ST_BPU:
                    if (level <= statement[s].st_level) {
                      order = true;
                      create_new_bpu(input_line);
                    }
                    level = statement[s].st_level;
                    break;
              case ST_WPU:
                    if (level <= statement[s].st_level) {
                      order = true;
                      create_new_wpu(input_line);
                    }
                    level = statement[s].st_level;
                    break;
              case ST_CORE:
                    if (level <= statement[s].st_level) {
                      order = true;
                      section = create_new_core(input_line);
                    }
                    level = statement[s].st_level;
                    break;
              case ST_MODULE:
                    if (level <= statement[s].st_level) {
                      order = true;
                      section = create_new_module(input_line);
                    }
                    level = statement[s].st_level;
                    break;
              case ST_SYSTEM:
                    if (level < statement[s].st_level) {
                      order = true;
                      section = create_system(input_line);
                    }
                    level = statement[s].st_level;
                    break;
              case ST_ADD_CACHE:
                    if ((level >= statement[s].st_level_from) && (level <= statement[s].st_level_to)) {
                      order = true;
                      add_cache(level, section, input_line);
                    }
                    break;
              case ST_ADD_CCM:
                    if ((level >= statement[s].st_level_from) && (level <= statement[s].st_level_to)) {
                      order = true;
                      add_spad(level, section, input_line);
                    }
                    break;
              case ST_ADD_BPU:
                    if ((level >= statement[s].st_level_from) && (level <= statement[s].st_level_to)) {
                      order = true;
                      add_bpu(level, section, input_line);
                    }
                    break;
              case ST_ADD_WPU:
                    if ((level >= statement[s].st_level_from) && (level <= statement[s].st_level_to)) {
                      order = true;
                      add_wpu(level, section, input_line);
                    }
                    break;
              case ST_ADD_MMU:
                    if ((level >= statement[s].st_level_from) && (level <= statement[s].st_level_to)) {
                      order = true;
                      add_mmu(level, section, input_line);
                    }
                    break;
              case ST_ADD_IFQ:
                    if ((level >= statement[s].st_level_from) && (level <= statement[s].st_level_to)) {
                      order = true;
                      add_ifq(level, section, input_line);
                    }
                    break;
              case ST_ADD_CORE:
                    if ((level >= statement[s].st_level_from) && (level <= statement[s].st_level_to)) {
                      order = true;
                      add_core(level, section, input_line);
                    }
                    break;
              case ST_ADD_MODULE:
                    if ((level >= statement[s].st_level_from) && (level <= statement[s].st_level_to)) {
                      order = true;
                      add_module(level, section, input_line);
                    }
                    break;
              default:
                LOG(LOG_ERROR) << "[FILE:'" << sarch_file << "' LINE:" << lineno << "]: "
                               << "Unexpected System Architecture statement Number.";
                exit (EXIT_FAILURE);
                break;
            }
            if (!order) {
              LOG(LOG_ERROR) << "[FILE:'" << sarch_file << "' LINE:" << lineno << "]: "
                             << "Illegal System Architecture statement Order.";
              exit (EXIT_FAILURE);
            }
          }
        }
      }
      if (!match) {
        LOG(LOG_ERROR) << "[FILE:'" << sarch_file << "' LINE:" << lineno << "]: "
                       << "Illegal System Architecture statement.";
        exit (EXIT_FAILURE);
      }
    }
  }

  if ((valid_arch & VALID_ARCH_MASK) != VALID_ARCH_MASK) {
    LOG(LOG_ERROR) << "[FILE:'" << sarch_file << "]: Not a valid System Architecture.";
    exit (EXIT_FAILURE);
  }

  fin.clear();
  fin.close();

  if (print_sarch) {
    // Print out architectural description
    //
    arcsim::util::OutputStream S(stderr);
    print_architecture(S, sarch_file);
  }
  
  return 0;
}


