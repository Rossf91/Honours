//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2005-2006 The University of Edinburgh
//                        All Rights Reserved
//
//
// =====================================================================
//
// Description:
//
// This file implements the methods declared in the System class for
// the instruction set simulator.
//
// External library dependencies:
//
//  There is a dependency on the ELFIO library, which is licensed under
//  the GNU Lesser General Public License (LGPL) and is available from
//  http://sourceforge.net/projects/elfio/
//  This simulator is known to work with ELFIO ver 1.0.0 and will almost
//  certainly also work with more recent versions.
//
// =====================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>

#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "api/api_funs.h"
#include "arch/ModuleArch.h"
#include "uarch/memory/ScratchpadFactory.h"

#include "system.h"
#include "sys/cpu/processor.h"
#include "sys/mem/BlockData.h"

// IODeviceManager responsible for memory mapped devices
//
#include "mem/MemoryDeviceInterface.h"
#include "mem/mmap/IODeviceManager.h"

#include "ioc/Context.h"
#include "ioc/ContextItemId.h"

#include "concurrent/Mutex.h"
#include "concurrent/ScopedLock.h"

#include "util/system/SharedLibrary.h"

#include "util/Log.h"
#include "util/Counter.h"
#include "util/OutputStream.h"
#include "util/Allocate.h"

#include "util/SymbolTable.h"

#include <ELFIO.h>

// -----------------------------------------------------------------------------
// MACROS
//

#define MIDDLE_ENDIAN(_x_) ((((_x_) >> 16) & 0x0ffff) | ((_x_) << 16))
#define POW2_CEILING(x,n) ((((x)+((1<<(n))-1))>>(n))<<(n))
#define INIT_MEM_VALUE(x, y) (uint32)((y)<<16)|(x)

#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_

// -----------------------------------------------------------------------------
// LOCAL VARIABLES
//

// Declaration of static data to keep breakpoint context
//
static uint32 brk_instruction;
static uint32 brk_address;
static bool   brk_s;

static const char* cli_help_msg = "\n\
Command-line interpreter help\n\
-----------------------------\n\
 cont     Continue/run program simulation\n\
 norm     Enable Normal simulation mode\n\
 fast     Enable Fast simulation mode\n\
 func     Enable Functional simulation\n\
 cyc      Enable Cycle Accurate simulation\n\
 tron     Enable instruction tracing\n\
 troff    Disable instruction tracing\n\
 trace N  Trace the next N instructions\n\
 break M  Set breakpoint at address M\n\
 clear    Clear breakpoint\n\
 state    Print processor state\n\
 goto X   Sets pc to X\n\
 set F    Set flag F (ZNCVDU)\n\
 clr F    Clear flag F (ZNCVDU)\n\
 stats    Print simulation statistics\n\
 zero     Zero instruction count\n\
 sim      Print the simulator state\n\
 quit     Quit the simulation\n\
 abort    Abort the simulator\n\
 ctrl-C   Interrupt simulation\n\
 help     Print this Help message\n\
---------------------------------\n";

// -----------------------------------------------------------------------------
// systems_in_use_ indicates how many counters have been instantiated and serves
// as a unique numeric id for each system
//

// @igor - We need to construct the  'systems_in_use_mutex_' in a thread safe
//         way which given that initialisation order in C++ is not really
//         well defined can be a bit of a problem.
//
//         We take advantage of the fact that initiliasation before main() 
//         is called will take place on a SINGLE thread, and that 'instance_'
//         must be zero initialised before dynamic initialisation of static
//         objects takes place (3.6.2 "Initialization of non-local objects")
//
class GlobalSystemsInUseMutex {
public:  
  static arcsim::concurrent::Mutex& Get() {
    if (!instance_) {
      static arcsim::concurrent::Mutex default_instance;
      instance_ = &default_instance;
    }
    return *instance_;
  }
private:     
  static arcsim::concurrent::Mutex * instance_;      
  GlobalSystemsInUseMutex() { }; // DEFAULT CTOR must be private!
};

// NOTE: @igor - order of initialisation matters here: GlobalContextMutex::instance_
//       must be initialised before 'systems_in_use_mutex_'.

// initialise 'GlobalSystemsInUseMutex::instance_' first - THIS IS THREAD-SAFE as it runs BEFORE main
arcsim::concurrent::Mutex * GlobalSystemsInUseMutex::instance_ = &GlobalSystemsInUseMutex::Get();

// now we can safely initiliase 'systems_in_use_mutex_'
static arcsim::concurrent::Mutex&  systems_in_use_mutex_ = GlobalSystemsInUseMutex::Get();
static uint32                      systems_in_use_  = 0;

// -----------------------------------------------------------------------------
// Thread safe system id sequence generator function
//
static uint32
get_next_system_id()
{
  arcsim::concurrent::ScopedLock lock(systems_in_use_mutex_);
  return systems_in_use_++;
}

// -----------------------------------------------------------------------------
// System Constructor
//
System::System (Configuration& _conf)
  : id(get_next_system_id()),
    sys_ctx(*arcsim::ioc::Context::Global().create_context(id, arcsim::ioc::Context::kNSystem)),
    sys_conf(_conf),
    sim_opts(_conf.sys_arch.sim_opts),
    io_plugin_handle(NULL),
    ise_plugin_handle(NULL),
    total_cores(0),
    total_modules(0),
    ext_mem(0),
    ext_mem_c(0),
    dmem(0),
    sym_tab_(*(arcsim::util::SymbolTable*)sys_ctx.create_item(arcsim::ioc::ContextItemInterface::kTSymbolTable,
                                                              arcsim::ioc::ContextItemId::kSymbolTable))
{
  heap_base_ = heap_limit_ = 0x4000000;
  stack_top_ = heap_base_ - 8;  
}

System::~System ()
{  
  // Destroy HEAP allocated processor instances
  for (uint32 i = 0 ; i < total_cores; ++i ) {
    if (cpu[i] != 0) {
      arcsim::ioc::Context* sys_ctx = arcsim::ioc::Context::Global().get_context(id);
      arcsim::ioc::Context* mod_ctx = sys_ctx->get_context(0);      
      // removing a processor means destroying a processor context
      delete mod_ctx->get_context(i);
      cpu[i] = 0;
    }
  }

  
  // Destroy HEAP allocated memory instances
  //
  if (ext_mem_c != 0)  { delete ext_mem_c; ext_mem_c = 0; }
  if (dmem != 0)       { delete dmem;      dmem      = 0; }
  if (ext_mem != 0)    { delete ext_mem;   ext_mem   = 0; }  
}

void
System::reset_to_initial_state(bool purge_translations)
{
  // Reset ALL processors
  //
  for (uint32 i = 0 ; i < total_cores; ++i ) {
    if (cpu[i] != 0) {
      cpu[i]->reset_to_initial_state(purge_translations);
    }
  }
  // Clear main memory model
  //
  if (ext_mem_c) { ext_mem_c->clear(); }
  // Clear main memory
  //
  if (ext_mem)   { ext_mem->clear();   }
}

// Create System
//
void
System::create_system ()
{
  // Create internal PageArch configuration (a page corresponds to a chunk of memory)
  //
  PageArch* page_arch = new PageArch(sim_opts.page_size_log2);

  // Create Memory
  //
  ext_mem             = new arcsim::sys::mem::Memory(sys_conf.sys_arch, *page_arch);
  
  // Create MemoryModel
  //
  if (sim_opts.memory_sim) { ext_mem_c = new MainMemoryModel(sys_conf.sys_arch); }
  
  
  if (sim_opts.cosim) {
    // Create separate copy of memory for cosim
    //
    dmem = new arcsim::sys::mem::Memory(sys_conf.sys_arch, *page_arch);
  }
  
  CacheModel* L3_icache = 0;
  CacheModel* L3_dcache = 0;

  if (   sim_opts.memory_sim
      && sys_conf.sys_arch.cache_types)
  { // Level 3 cache is configured
    //
    if (sys_conf.sys_arch.cache_types & CacheArch::kInstCache) {
      L3_icache = new CacheModel (SYSTEM_LEVEL, CacheArch::kInstCache,
                                  sys_conf.sys_arch.icache,
                                  0, ext_mem_c);
    }
    if (sys_conf.sys_arch.cache_types & CacheArch::kDataCache) {
      L3_dcache = new CacheModel (SYSTEM_LEVEL, CacheArch::kDataCache,
                                  sys_conf.sys_arch.dcache,
                                  0, ext_mem_c);
    }
    if (sys_conf.sys_arch.cache_types & CacheArch::kUnifiedCache) {
      L3_icache = new CacheModel (SYSTEM_LEVEL, CacheArch::kUnifiedCache,
                                  sys_conf.sys_arch.icache,
                                  0, ext_mem_c);
      L3_dcache = L3_icache;
    }
  }
  
  // MODULE Type
  //
  for (uint32 module_type = 0; module_type < sys_conf.sys_arch.number_module_types; ++module_type)
  {
    ModuleArch& moduleArch = *sys_conf.sys_arch.module_type[module_type];
        
    // Number of Modules of Type
    //
    for (uint32 module_number = 0; module_number < sys_conf.sys_arch.modules_of_type[module_type]; ++module_number)
    {      
      // Create Module level context
      //
      arcsim::ioc::Context& mod_ctx = *sys_ctx.create_context(total_modules, arcsim::ioc::Context::kNModule);

      CacheModel* L2_icache = L3_icache;
      CacheModel* L2_dcache = L3_dcache;
      
      if (   sim_opts.memory_sim
          && moduleArch.cache_types)
      { // Level 2 cache is configured
        //
        if (moduleArch.cache_types & CacheArch::kInstCache) {
          L2_icache = new CacheModel (MODULE_LEVEL, CacheArch::kInstCache,
                                      moduleArch.icache, L3_icache, ext_mem_c);
        }
        if (moduleArch.cache_types == CacheArch::kDataCache) {
          L2_dcache = new CacheModel (MODULE_LEVEL, CacheArch::kDataCache,
                                      moduleArch.dcache, L3_dcache, ext_mem_c);
        }
        if (moduleArch.cache_types & CacheArch::kUnifiedCache) {
          L2_icache = new CacheModel (MODULE_LEVEL, CacheArch::kUnifiedCache,
                                      moduleArch.icache, L3_icache, ext_mem_c);
          L2_dcache = L2_icache;
        }
      }      
      
      // CORE Type
      //
      for (uint32 core_type = 0; core_type < moduleArch.number_core_types; ++core_type)
      {
        CoreArch& coreArch = *moduleArch.core_type[core_type];
        
        // Number of Cores of Type
        //
        for (uint32 core_number = 0; core_number < moduleArch.cores_of_type[core_type]; ++core_number)
        {
          // Create Processor level context
          //
          arcsim::ioc::Context& cpu_ctx = *mod_ctx.create_context(core_number,
                                                                  arcsim::ioc::Context::kNProcessor);
          
          MemoryModel*  mem_model        = 0;

          if (sim_opts.memory_sim) {
            // Various memory models for cycle accurate simulation
            //
            CacheModel*   L1_icache  = L2_icache;
            CacheModel*   L1_dcache  = L2_dcache;
            CCMModel*     iccm_model = 0;
            // FIXME: Remove magic number 4 and improve co-existence of multiple ICCMs
            CCMModel*     iccm_models[4] = {0,0,0,0};
            CCMModel*     dccm_model     = 0;

            if (coreArch.cache_types != CacheArch::kNoCache) {
              // Level 1 cache is configured
              //
              if (coreArch.cache_types & CacheArch::kInstCache) {
                L1_icache = new CacheModel (CORE_LEVEL, CacheArch::kInstCache,
                                            coreArch.icache, L2_icache, ext_mem_c);
              }
              if (coreArch.cache_types & CacheArch::kDataCache) {
                L1_dcache = new CacheModel (CORE_LEVEL, CacheArch::kDataCache,
                                            coreArch.dcache, L2_dcache, ext_mem_c);
              }
              if (coreArch.cache_types & CacheArch::kUnifiedCache) {
                L1_icache = new CacheModel (CORE_LEVEL, CacheArch::kUnifiedCache,
                                            coreArch.icache, L2_icache, ext_mem_c);
                L1_dcache = L1_icache;
              }
            }
            
            if (coreArch.spad_types != SpadArch::kNoSpad) {
              // Scratchpad memory is configured
              //
              if (coreArch.spad_types & SpadArch::kSpadInstCcm) {
                if (sys_conf.sys_arch.isa_opts.multiple_iccms) {
                  for (int i = 0; i < IsaOptions::kMultipleIccmCount; i++){
                    iccm_models[i] = ScratchpadFactory::create_scratchpad(coreArch.iccms[i],
                                                                          SpadArch::kSpadInstCcm);
                  }
                } else {
                  iccm_model = ScratchpadFactory::create_scratchpad(coreArch.iccm,
                                                                    SpadArch::kSpadInstCcm);
                }
              }
              if (coreArch.spad_types & SpadArch::kSpadDataCcm) {
                dccm_model = ScratchpadFactory::create_scratchpad(coreArch.dccm,
                                                                  SpadArch::kSpadDataCcm);
              }
              if (coreArch.spad_types & SpadArch::kSpadUnifiedCcm) {
                iccm_model = ScratchpadFactory::create_scratchpad(coreArch.iccm,
                                                                  SpadArch::kSpadUnifiedCcm);
                dccm_model = iccm_model;
              }   
            }                        
            // Create cycle accurate memory model interface
            //
            if (sys_conf.sys_arch.isa_opts.multiple_iccms) {
              mem_model = new MemoryModel(sys_conf.sys_arch,
                                          coreArch.cpu_bo,
                                          L1_icache, L1_dcache,
                                          (CCMModel*)(&iccm_models),dccm_model,
                                          ext_mem_c);
            } else {
              mem_model = new MemoryModel(sys_conf.sys_arch,
                                          coreArch.cpu_bo,
                                          L1_icache, L1_dcache,
                                          iccm_model,dccm_model,
                                          ext_mem_c);
            }
          }
          
          // Create a new HEAP allocated core object
          //
          cpu[total_cores] = new arcsim::sys::cpu::Processor(*this,
                                                             coreArch,
                                                             cpu_ctx,
                                                             ext_mem,
                                                             mem_model,
                                                             total_cores,
                                                             arcsim::ioc::ContextItemId::kProcessor);
          
          // Register this processor instance with the container which takes care
          // of its de-allocation.
          // 
          cpu_ctx.register_item(cpu[total_cores]);
          
          ++total_cores;
        }
      }
      L2_dcache->cycle_count   = &(cpu[0]->cnt_ctx.cycle_count);
      ++total_modules;
    }
  }

  // ---------------------------------------------------------------------------
  // Initiliase builtin memory mapped devices via IODeviceManager
  //
  io_iom_device_manager.create_devices((simContext)this,
                                       (IocContext)(&sys_ctx),
                                       sys_conf.sys_arch);
  
  // ---------------------------------------------------------------------------
  // Load memory device libraries if specified
  //
  if (!sim_opts.mem_dev_library_list.empty()) {
    bool success = false;
    
    // Load all MemoryDevice libraries 
    //
    for (std::set<std::string>::const_iterator
         I = sim_opts.mem_dev_library_list.begin(),
         E = sim_opts.mem_dev_library_list.end();
         I != E; ++I)
    {
      void* library_handle  = NULL;
      void* function_handle = NULL;
      
      // Open shared library
      //
      success = arcsim::util::system::SharedLibrary::open(&library_handle, *I);
      
      if (success) {
        // Lookup initialisation function symbol
        //
        success = arcsim::util::system::SharedLibrary::lookup_symbol(&function_handle,
                                                                     library_handle,
                                                                     "simLoadMemoryDevice");
        if (success) {
          // Cast to proper function type
          //
          void (*mem_dev_load)(simContext) = (void (*)(simContext))function_handle;
          
          // Execute EIA load function
          //
          (*mem_dev_load)((simContext)this);
        }
      }
    }    
  }
  
  // ---------------------------------------------------------------------------
  // Load EIA extensions if specified
  //
  if (sim_opts.is_eia_enabled) {
    bool success = false;
    
    // Load all EIA libraries 
    //
    for (std::set<std::string>::const_iterator
          I = sim_opts.eia_library_list.begin(),
          E = sim_opts.eia_library_list.end();
          I != E; ++I)
    {
      void* library_handle  = NULL;
      void* function_handle = NULL;
      
      // Open shared library
      //
      success = arcsim::util::system::SharedLibrary::open(&library_handle, *I);
      
      if (success) {
        // Lookup initialisation function symbol
        //
        success = arcsim::util::system::SharedLibrary::lookup_symbol(&function_handle,
                                                                     library_handle,
                                                                     "simLoadEiaExtension");
        if (success) {
          // Cast to proper function type
          //
          void (*eia_load)(simContext) = (void (*)(simContext))function_handle;
          
          // Execute EIA load function
          //
          (*eia_load)((simContext)this);
        }
      }
    }    
  }
    
  // ---------------------------------------------------------------------------
  // Create TranslationManager that takes care of JIT compilation
  //
  trans_mgr.configure(&sim_opts, sim_opts.fast_num_worker_threads);
  
  if (sim_opts.fast) {
    trans_mgr.start_workers();    
  }
}

// Clean up resources allocated by create_system(). Note that the order of
// de-allocation is the reverse of allocation!
//
void
System::destroy_system ()
{    
  // Stop translation worker threads
  //
  trans_mgr.stop_workers();
  
  // -----------------------
  // When destroying the following HEAP allocated structures make sure to set
  // them to '0' (i.e. NULL) after calling the destructor as the ~System destructor
  // uses that value to distinguish if re-sources have been properly freed up.
  //
  arcsim::ioc::Context* sys_ctx = arcsim::ioc::Context::Global().get_context(id);
  arcsim::ioc::Context* mod_ctx = sys_ctx->get_context(0);
  
  for (int c=0; c < total_cores; ++c) {
    // removing a processor means destroying a processor context
    delete mod_ctx->get_context(c);
    cpu[c] = 0;
  }
    
  // Finally destroy memory
  if (ext_mem_c)  { delete ext_mem_c; ext_mem_c = 0; }
  if (dmem)       { delete dmem;      dmem      = 0; }
  if (ext_mem)    { delete ext_mem;   ext_mem   = 0; }

  sym_tab_.destroy();
}

// -----------------------------------------------------------------------------

void
System::hard_reset ()
{
  destroy_system ();
  create_system ();
}

void
System::soft_reset ()
{
  if (cpu[0]) {
    cpu[0]->reset ();
  }  
}

void
System::halt_cpu ()
{
  if (cpu[0]) {
    cpu[0]->halt_cpu();
  }
}

uint64
System::clock_ticks ()
{
  return cpu[0]->instructions ();
}

// Write data into memory and take care of external memory, shadow memory and
// any IO device types that are registered for specific memory regions.
//
bool
System::write_to_memory(uint32 addr, uint32 data, int size)
{
  // -------------------------------------------------------------------------
  // If CCMs are configured for a CPU we need to load binaries via the CPU
  // writeXX() methods as data needs to end up in the processor local CCMs
  // and NOT system level memory
  //
  if (cpu[0]->in_ccm_mapped_region(addr)) {
    switch (size) {
      case 4: { cpu[0]->write32(addr, data); return true; } // Write WORD
      case 2: { cpu[0]->write16(addr, data); return true; } // Write HALF
      case 1: { cpu[0]->write8 (addr, data); return true; } // Write BYTE
    } // end switch()     
    return false;
  }
  
  // Retrieve BlockData object
  arcsim::sys::mem::BlockData * const block = get_host_page(addr);
  
  if (block->is_mem_dev()) {
    // -------------------------------------------------------------------------
    // Memory device is registered at given address
    //
    block->get_mem_dev()->mem_dev_write(addr,(unsigned char*)&data, size);
    return true;
  }
  
  // --------------------------------------------------------------------------
  // Memory is located at the given address
  //
  switch (size) {
    case 4: { // Write WORD
      ext_mem->write32(addr, data);
      if (sim_opts.cosim)
        dmem->write32(addr, data);
      return true;
    }
    case 2: { // Write HALF
      ext_mem->write16(addr, data);
      if (sim_opts.cosim)
        dmem->write16(addr, data);
      return true;
    }
    case 1: { // Write BYTE
      ext_mem->write8(addr, data);
      if (sim_opts.cosim) 
        dmem->write8(addr, data);
      return true;
    }
  } // end switch() 
  return false;
}


int
System::load_elf32 (const char *objfile)
{
  arcsim::util::OutputStream  S(stderr);
  IELFI*                      elf_reader;
  
  if ( ERR_ELFIO_NO_ERROR != ELFIO::GetInstance()->CreateELFI( &elf_reader ) ) {
    LOG(LOG_ERROR) << "Can't create ELF reader.";
    return 2;
  }
  
  if ( ERR_ELFIO_NO_ERROR != elf_reader->Load( objfile ) ) {
    LOG(LOG_ERROR) << "Can't open object file '" << objfile << "\'";
    return 3;
  }

#if BIG_ENDIAN_SUPPORT
   if ((elf_reader->GetEncoding() == ELFDATA2MSB) && !sim_opts.big_endian){
     LOG(LOG_INFO) << "ELF binary is big-endian. Switching to big-endian memory ordering."; 
     sim_opts.big_endian = true;
   }
#endif

  const int sections_num = elf_reader->GetSectionsNum();
  
  // Load all loadable sections and initialise .bss
  //
  LOG(LOG_INFO) << "[ELF] section count - '" << sections_num << "'.";
  for ( uint32 i = 0; i < sections_num; ++i )
  {
    const IELFISection* section = elf_reader->GetSection( i );

    // Check if section is allocatable
    if ( (section->GetFlags() & SHF_ALLOC) ) {
        // .data, .rodata, and .text sections all have PROGBITS set
        if ((section->GetType() == SHT_PROGBITS) ) {
          const char* data = section->GetData();
          Elf32_Word  size = section->GetSize();
          Elf32_Addr  start= section->GetAddress();
          Elf32_Addr  end  = start + size;
          
          if (sim_opts.debug) {
            char         buf[BUFSIZ];
            std::string  name = section->GetName();
            snprintf (buf, sizeof(buf), "Loading section %-10s from 0x%08x...0x%08x ",
                                        name.c_str(), start, end-1);
            S.Get() << buf;
          }
          // -------------------------------------------------------------------
          // LOAD PROGBITS (i.e. the actual program) into memory
          //
#if BIG_ENDIAN_SUPPORT
          if (sim_opts.big_endian) {
            // When using big endian we have to load data byte-wise
            //
            while (start < end)
            { // WRITE in BYTE chunks when BIG_ENDIAN is enabled
              uint8 byte = *((uint8*)(data++));
              write_to_memory(start++, byte, 1);
            }            
          } else
#endif
          { // Perform fast bulk load of binary using block wise write
            //
            if (start < end)
            { // When no scratch pad memory is configured we write to system
              // memory directly (i.e. system level, otherwise we need to go
              // via the processor (i.e. processor level)
              //
              if (cpu[0]->core_arch.spad_types == SpadArch::kNoSpad) {
                ext_mem->write_block(start, end - start, (uint8*)data);
              } else {
                cpu[0]->write_block (start, end - start, (uint8*)data);
              }
            }
          }
          //
          // -------------------------------------------------------------------
          
          if (sim_opts.debug) { S.Get() << "done\n"; }
        }
        // .bss also has the write flag set, but not the prog_bits
        //
        else if (section->GetFlags() & SHF_WRITE) {
          Elf32_Word   size  = section->GetSize();
          Elf32_Addr   start = section->GetAddress();
          std::string  name  = section->GetName();
          Elf32_Addr   end   = start + size;

           // Skip any heap or stack sections
           //
          if (   strstr(name.c_str(), "heap")
              || strstr(name.c_str(), "stack")) {
            continue;
          }

          if (sim_opts.debug) {
            char buf[BUFSIZ];
            snprintf(buf, sizeof(buf),  "Initialising section %-10s from 0x%08x...0x%08x ",
                                        name.c_str(), start, end - 1);
            S.Get() << buf;
          }
           while (start < end)
           { /* Endianness does not matter for zero data
              * Also .bss are always aligned to 16-byte boundaries
              * Hence writes are in 32-bit words.
              */
             write_to_memory(start, 0L, 4);
             start += 4;
           }
          if (sim_opts.debug) {
            S.Get() << "done\n";
          }
        }
        // Other sections are ignored
      }
      section->Release();
   }
   
  entry_point_ = elf_reader->GetEntry();

  // For cosimulation PC==0 upon reset. For all other modes we set it to the
  // program entry point
  //
  cpu[0]->state.pc = (sim_opts.cosim) ? 0 : entry_point_;

   // Get symbols from symbol tables if any exist
   //
   bool have_vectors = false;
   for (int i = 0; i < sections_num; ++i ) {
     const IELFISection* elf_section = elf_reader->GetSection(i);
     
     if (SHT_SYMTAB == elf_section->GetType() || SHT_DYNSYM == elf_section->GetType() ) {
        const IELFISymbolTable* sym_tab;
        elf_reader->CreateSectionReader( IELFI::ELFI_SYMBOL, elf_section, (void**)&sym_tab );
        // initiliase symbol table
        sym_tab_.create(sym_tab);
        std::string   name;
        Elf32_Addr    value;
        Elf32_Word    size;
        unsigned char bind;
        unsigned char type;
        Elf32_Half    section;
        Elf32_Word    prog_sym_num = sym_tab->GetSymbolNum();
        
         if ( 0 < prog_sym_num )
         {
            for ( int j = 0; j < prog_sym_num; ++j )
            {
              sym_tab->GetSymbol( j, name, value, size, bind, type, section );
              // Pick up '__reset' symbol
              if (strcmp(name.c_str(), "__reset") == 0) {
                have_vectors = true;
                if (sim_opts.debug)
                   S.Get() << "Found symbol __reset      @ 0x" << HEX(value) << "\n";
               }
               // Pick up '__start_heap' symbol
               if (have_vectors && (strcmp(name.c_str(),"__start_heap") == 0)) {
                  heap_base_ = value;
                 if (sim_opts.debug)
                   S.Get() << "Found symbol __start_heap @ 0x" << HEX(value) << "\n";
               }
               // Pick up '__stack_top' symbol
               if (have_vectors && (strcmp(name.c_str(),"__stack_top") == 0)) {
                 stack_top_ = value;
                 if (sim_opts.debug)
                   S.Get() << "Found symbol __stack_top  @ 0x" << HEX(value) << "\n";
               }
            }
          }
      }
      // Release elf section
      elf_section->Release();
   }

  // Load default startup sequence to clear all registers and jump to entry point
  // defined in the ELF binary if the reset vector is not defined (i.e. using a host library)
  //
   if (!have_vectors && sim_opts.cosim) {
#if BIG_ENDIAN_SUPPORT
      if (sim_opts.big_endian) {
        write_to_memory ( 0, INIT_MEM_VALUE(0xD900, 0xD800), 4);                
        write_to_memory ( 4, INIT_MEM_VALUE(0xDB00, 0xDA00), 4);
        write_to_memory ( 8, INIT_MEM_VALUE(0x70B8, 0x7098), 4);
        write_to_memory (12, INIT_MEM_VALUE(0x70F8, 0x70D8), 4);
        write_to_memory (16, INIT_MEM_VALUE(0x7039, 0x7019), 4);
        write_to_memory (20, INIT_MEM_VALUE(0x7079, 0x7059), 4);
        write_to_memory (24, INIT_MEM_VALUE(0x7508, 0x7408), 4);
        write_to_memory (28, INIT_MEM_VALUE(0x7708, 0x7608), 4);
        write_to_memory (32, INIT_MEM_VALUE(0x703A, 0x701A), 4);
        write_to_memory (36, INIT_MEM_VALUE(0x707A, 0x705A), 4);
        write_to_memory (40, INIT_MEM_VALUE(0x70BA, 0x709A), 4);
        write_to_memory (44, INIT_MEM_VALUE(0x70FA, 0x70DA), 4);
        write_to_memory (48, INIT_MEM_VALUE(0x703B, 0x701B), 4);
        write_to_memory (52, INIT_MEM_VALUE(0x707B, 0x705B), 4);
        write_to_memory (56, INIT_MEM_VALUE(0x70BB, 0x709B), 4);
        write_to_memory (60, INIT_MEM_VALUE(0x70FB, 0x70DB), 4);
      } else
#endif
      { 
        write_to_memory ( 0, INIT_MEM_VALUE(0xD800, 0xD900), 4);                
        write_to_memory ( 4, INIT_MEM_VALUE(0xDA00, 0xDB00), 4);
        write_to_memory ( 8, INIT_MEM_VALUE(0x7098, 0x70B8), 4);
        write_to_memory (12, INIT_MEM_VALUE(0x70D8, 0x70F8), 4);
        write_to_memory (16, INIT_MEM_VALUE(0x7019, 0x7039), 4);
        write_to_memory (20, INIT_MEM_VALUE(0x7059, 0x7079), 4);
        write_to_memory (24, INIT_MEM_VALUE(0x7408, 0x7508), 4);
        write_to_memory (28, INIT_MEM_VALUE(0x7608, 0x7708), 4);
        write_to_memory (32, INIT_MEM_VALUE(0x701A, 0x703A), 4);
        write_to_memory (36, INIT_MEM_VALUE(0x705A, 0x707A), 4);
        write_to_memory (40, INIT_MEM_VALUE(0x709A, 0x70BA), 4);
        write_to_memory (44, INIT_MEM_VALUE(0x70DA, 0x70FA), 4);
        write_to_memory (48, INIT_MEM_VALUE(0x701B, 0x703B), 4);
        write_to_memory (52, INIT_MEM_VALUE(0x705B, 0x707B), 4);
        write_to_memory (56, INIT_MEM_VALUE(0x709B, 0x70BB), 4);
        write_to_memory (60, INIT_MEM_VALUE(0x70DB, 0x70FB), 4);
      }
   }

   if (sim_opts.emulate_traps || sim_opts.cosim) {
     if ((sim_opts.cosim) && !have_vectors) {
#if BIG_ENDIAN_SUPPORT
        if (sim_opts.big_endian) {
          // Initialise the processor stack pointer. This is the only
          // live-in register on entry to ctr0.
          //
          write_to_memory (64, INIT_MEM_VALUE(0x3F80, 0x240A), 4);
          write_to_memory (68, INIT_MEM_VALUE((stack_top_ & 0xffff), (stack_top_>>16)), 4);

          // Explicitly insert code to jump to the entry point
          //
          write_to_memory(72, 0x20200F80, 4);
          write_to_memory(76, entry_point_, 4);
        } else
#endif
        {
          // Initialise the processor stack pointer. This is the only live-in register
          // on entry to ctr0.
          //
          write_to_memory (64, INIT_MEM_VALUE(0x240A, 0x3F80), 4);
          write_to_memory (68, INIT_MEM_VALUE((stack_top_>>16), (stack_top_ & 0xffff)), 4);

          // Explicitly insert code to jump to the entry point
          write_to_memory(72, MIDDLE_ENDIAN(0x20200F80), 4);
          write_to_memory(76, MIDDLE_ENDIAN(entry_point_), 4);
        }
      } else {
         cpu[0]->state.gprs[SP_REG] = stack_top_;
         cpu[0]->state.pc = entry_point_;
      }
   }

  LOG(LOG_INFO) << "[ELF] entry point - '0x" << HEX(entry_point_) << "'.";
  LOG(LOG_INFO) << "[ELF] stack top - '0x"   << HEX(stack_top_)   << "'.";
  return 0;
}

int
System::load_binary_image (const char *imgfile)
{
  std::ifstream in(imgfile, std::ios::binary);
  
  if (in.good()) {
    std::filebuf *buf = in.rdbuf();
    unsigned int bytes = buf->in_avail();
    unsigned int addr = 0;
    unsigned char c;
    
    while (addr < bytes) {
      c = buf->sbumpc();
      write_to_memory(addr++, c, 1);
    }
    in.close();
    
    cpu[0]->state.pc = 0; // always start from address 0
    
    return 0;
  }
  LOG(LOG_ERROR) << "Can't open binary image file '" << imgfile << "'";
  return 3;
}

int
System::load_quicksim_hex (const char *objfile)
{
  std::ifstream in(objfile);
  unsigned int address      = 0;
  unsigned int end_address  = 0;
  unsigned int data;
  char line[255] = "";
  char symbol = 0;
  
#ifdef VERIFICATION_OPTIONS
  // Get the range of addresses for ICCM and DCCM, to prevent
  // wrap-around of addresses during hex loading.
  //
  bool   has_iccm    = cpu[0]->core_arch.iccm.is_configured;
  bool   has_dccm    = cpu[0]->core_arch.dccm.is_configured;
  uint32 addr_size   = cpu[0]->sys_arch.isa_opts.addr_size;
  uint32 region_mask = 0xf0000000 >> (32 - addr_size);
  uint32 iccm_start  = cpu[0]->core_arch.iccm.start_addr;
  uint32 dccm_start  = cpu[0]->core_arch.dccm.start_addr;
  uint32 iccm_end    = cpu[0]->core_arch.iccm.start_addr + cpu[0]->core_arch.iccm.size - 1;
  uint32 dccm_end    = cpu[0]->core_arch.dccm.start_addr + cpu[0]->core_arch.dccm.size - 1;

  uint32 iccm_region = iccm_start & region_mask;
  uint32 dccm_region = dccm_start & region_mask;

  bool   discarded   = false;
  uint32 min_lost    = 0xffffffff;
  uint32 max_lost    = 0;
#endif
  
  if ( in.good() ) {
    
    while ( !in.eof() ) {
      char ch = in.peek();
      // skip comments and empty lines
      if (ch == '#' || ch == '\n') {
        in.getline(line,255); 
        continue;
      }
      
      // bail out if EOF is reached after skipping empty line or comment
      if (in.eof())
        break;
      // load address
      in >> std::hex >> address ;
      if (in.fail()) { LOG(LOG_ERROR) << "file format incorrect"; return 1; }
      
      // read next symbol
      in >> symbol ;
      if (in.fail()) { LOG(LOG_ERROR) << "file format incorrect"; return 1; }
      
      // check for address range construct
      if (symbol == '-' ) { 
        in  >> std::hex >> end_address >> symbol >> data >> symbol;
        if (in.fail()) { LOG(LOG_ERROR) << "file format incorrect"; return 1; }
        
        for ( ; address <= end_address ; ++address) {
          unsigned int adr = (address << 2);

#ifdef VERIFICATION_OPTIONS
          if (   (    !has_iccm
                   || ((adr & region_mask) != iccm_region)
                   || (    (adr >= iccm_start) 
                        && (adr <  iccm_end))
	         )
              && (    !has_dccm 
                   || ((adr & region_mask) != dccm_region)
                   || (    (adr >= dccm_start) 
                        && (adr <  dccm_end))))
          {	    
#endif
            write_to_memory(adr  , ((data >> 24) & 0x000000FF), 1);
            write_to_memory(adr+1, ((data >> 16) & 0x000000FF), 1);
            write_to_memory(adr+2, ((data >> 8)  & 0x000000FF), 1);
            write_to_memory(adr+3, ((data)       & 0x000000FF), 1);
#ifdef VERIFICATION_OPTIONS
          } else {
            discarded = true;
            if (adr > max_lost) max_lost = adr;
            if (adr < min_lost) min_lost = adr;
          }
#endif
        } 
      } else if ( symbol == '/' ) {
        in  >> std::hex >> data >> symbol;
        if (in.fail()) { LOG(LOG_ERROR) << "file format incorrect"; return 1; }

        unsigned int adr = (address << 2);
        
#ifdef VERIFICATION_OPTIONS
          if (   (    !has_iccm
                   || ((adr & region_mask) != iccm_region)
                   || (    (adr >= iccm_start) 
                        && (adr <  iccm_end))
	         )
              && (    !has_dccm 
                   || ((adr & region_mask) != dccm_region)
                   || (    (adr >= dccm_start) 
                        && (adr <  dccm_end))))
          {	    
#endif
          write_to_memory(adr  , ((data >> 24) & 0x000000FF), 1);
          write_to_memory(adr+1, ((data >> 16) & 0x000000FF), 1);
          write_to_memory(adr+2, ((data >> 8)  & 0x000000FF), 1);
          write_to_memory(adr+3, ((data)       & 0x000000FF), 1);
#ifdef VERIFICATION_OPTIONS
          } else {
            discarded = true;
            if (adr > max_lost) max_lost = adr;
            if (adr < min_lost) min_lost = adr;
          }
#endif
      } else {
        LOG(LOG_ERROR) << "file format incorrect";
        return 1;
      }
      // Ignore rest of line
      in.getline(line,255);
      
    } // end while ( !in.eof()
  } else {
    LOG(LOG_ERROR) << "Can't open object file \"" << objfile << "\"";
    return 3;
  }  
#ifdef VERIFICATION_OPTIONS
  if (discarded) {
    LOG(LOG_INFO) << "Hex file contains out-of-range ICCM or DCCM bytes, ignored bytes from " 
                  << std::hex << std::setw(8) << std::setfill('0') 
                  << min_lost << " to " << max_lost;
  }
#endif
  return 0;
}

// Setup simulated stack for simulated application so it will find something
// sensible when it tries to access 'argv[0]'. The value in argv[0] should
// point to a filename that is associated with the process being started by one
// of the exec functions: [@see http://pubs.opengroup.org/onlinepubs/000095399/functions/exec.html]
//
// Furthermore one might have used ArcSim's '--' option to pass further parameters
// to the simulated application which must also be pushed on the simulated stack.
//
void
System::setup_simulated_stack (int argc, int arg0_idx, const char* arg0, char* const argv[])
{
#ifdef VERIFICATION_OPTIONS
  if (sys_conf.sys_arch.isa_opts.disable_stack_setup) {
    LOG(LOG_INFO) << "Simulated application stack setup disabled.";
    return;
  } else {
    LOG(LOG_INFO) << "Setting up simulated application stack.";
  }
#endif
  
   if (arg0_idx >= argc) return;

   uint32 app_args = argc - arg0_idx + 1;

   // Scan arguments, computing total memory space needed to store them.
   //
   uint32 arg_bytes = (argc - arg0_idx) + strlen(arg0) + 1;
   for (int argp = arg0_idx; argp < argc; argp++) {
      arg_bytes += strlen(argv[argp]);
   }

   // In addition to the argument strings, stack space must be allocated for
   // the argv list, for argc and for envp.
   //
   uint32 argv_len = 4*app_args;
   uint32 envp_len = 4;
   uint32 argc_len = 4;
   uint32 arg_offset = argc_len + argv_len + envp_len;

   // Compute actual stack size in bytes and create an 8-byte aligned
   // allocation that is at least that large.
   //
   uint32 stack_alloc = argv_len + envp_len + argc_len + arg_bytes;
   uint32 aligned_stack_alloc = POW2_CEILING(stack_alloc,3);
   uint32 aligned_stack_words = aligned_stack_alloc >> 2;

   // Allocate space on stack for the arguments to the application main()
   //
   stack_top_ -= aligned_stack_alloc;

   // Create temporary buffer in which to build application stack.
   //
   char* buffer = new char [aligned_stack_alloc];
   for (int i = 0; i < aligned_stack_words; ++i) {
      ((uint32*)buffer)[i] = 0xdeadbeef;
   }

   // Populate temporary stack buffer with application stack content.
   //
  
   // 1. First one on the stack is the argument count
   //
   ((uint32*)buffer)[0] = app_args;
   
   // 2. Second on the stack is the argv 'pointer' 
   //
   ((uint32*)buffer)[1] = (uint32)(stack_top_ + arg_offset);
  
   // 3. Fill in actual arguments
   //
   strcpy (buffer+arg_offset, arg0);
   arg_offset += strlen(arg0) + 1;

   for (int argp = arg0_idx; argp < argc; ++argp) {
      ((uint32*)buffer)[argp-arg0_idx+2] = (uint32)(stack_top_ + arg_offset);
      strcpy (buffer+arg_offset, argv[argp]);
      arg_offset += strlen(argv[argp]) + 1;
   }
    
   // Copy temporary stack buffer to system memory
   // FIXME: how should we handle this in a multi-core setting?
   ext_mem->write_block(stack_top_, aligned_stack_alloc, (uint8*)buffer);

   // Relinquish temporary stack buffer.
   //
   delete [] buffer;
}

uint32
System::get_entry_point ()
{
   return entry_point_;
}

bool
System::get_symbol (const uint32 addr, std::string& name)
{
  return sym_tab_.get_symbol(addr, name);
}


// JIT compiled simulation mode ------------------------------------------------
//
bool System::run ()
{
  bool stepOK = true;
  
  // Sanity check
  //
  if (cpu[0] == NULL) {
    LOG(LOG_ERROR) << "[SYSTEM] No processor configured.";
    return false; 
  }

  LOG(LOG_DEBUG3) << "[SYSTEM] FAST simulation mode enabled.";
  
  cpu[0]->simulation_start();
  
  while (stepOK && !sim_opts.halt_simulation) {

    if (!sim_opts.sim_period) {
      // Run for one time-slice, defined as a reasonably
      // large number of instructions.
      //
      stepOK = cpu[0]->run (DEFAULT_RUN_TIMESLICE);      
    } else {
      // Run user specified timeslice
      //
      stepOK = cpu[0]->run (sim_opts.sim_period);
      // After user specified simulation period elapsed we halt the simulation
      //
      sim_opts.halt_simulation = true;
    }
  }
  
  if (cpu[0]->state.H) {
    cpu[0]->simulation_end();
    
    // CPU has halted, and therefore simulation is complete
    // temporarily we print out some statistics.
    //
    sim_opts.halt_simulation = true;
    stepOK                   = false;
  }

  return stepOK;
}

// Interpretive simulation mode without instruction tracing --------------------
//
bool System::run_notrace ()
{
  bool stepOK = true;
  
  // Sanity check
  //
  if (cpu[0] == NULL) {
    LOG(LOG_ERROR) << "[SYSTEM] No processor configured.";
    return false; 
  }
  
  LOG(LOG_DEBUG3) << "[SYSTEM] INTERPRETIVE simulation mode enabled.";
  
  cpu[0]->simulation_start();
  
  while (stepOK && !sim_opts.halt_simulation) {
    
    if (!sim_opts.sim_period) {
      // Run for one time-slice, defined as a reasonably large number of instructions.
      stepOK = cpu[0]->run_notrace (DEFAULT_RUN_TIMESLICE);
      
    } else {
      // Run user specified timeslice
      stepOK = cpu[0]->run_notrace (sim_opts.sim_period);
      
      // After user specified simulation period elapsed we halt the simulation
      sim_opts.halt_simulation = true;
    }
    
  }
  
  if (cpu[0]->state.H) {
    cpu[0]->simulation_end();
    
    // CPU has halted, and therefore simulation is complete
    // temporarily we print out some statistics..
    //
    sim_opts.halt_simulation = true;
    stepOK          = false;
  }
  
  return stepOK;
}

// Interpretive simulation mode with instruction tracing -----------------------
//
bool System::run_trace ()
{
  bool stepOK = true;
  
  // Sanity check
  //
  if (cpu[0] == NULL) {
    LOG(LOG_ERROR) << "[SYSTEM] No processor configured.";
    return false; 
  }
  
  LOG(LOG_DEBUG3) << "[SYSTEM] INTERPRETIVE TRACING simulation mode enabled.";
  
  cpu[0]->simulation_start();
  
  while (stepOK && !sim_opts.halt_simulation) {

    if (!sim_opts.sim_period) {
      // Run for one time-slice, defined as a reasonably large number of instructions.
      //
      stepOK = cpu[0]->run_trace (DEFAULT_RUN_TIMESLICE);
      
    } else {
      // Run user specified timeslice
      stepOK = cpu[0]->run_trace (sim_opts.sim_period);
      
      // After user specified simulation period elapsed we halt the simulation
      sim_opts.halt_simulation = true;
    }

  }
  
  if (cpu[0]->state.H) {
    cpu[0]->simulation_end();
    
    // CPU has halted, and therefore simulation is complete
    // temporarily we print out some statistics..
    //
    sim_opts.halt_simulation = true;
    stepOK                   = false;
  }
  
  return stepOK;
}

// Cosim simulation
//
bool System::step (UpdatePacket* deltas)
{
  bool stepOK = true;

  // Sanity check
  //
  if (cpu[0] == NULL) {
    LOG(LOG_ERROR) << "[SYSTEM] No processor configured.";
    return false; 
  }
  
  LOG(LOG_DEBUG3) << "[SYSTEM] INTERPRETIVE single step simulation mode enabled.";
  
  cpu[0]->restart_from_halted ();
  
  // Perform single-step
  //
  stepOK = cpu[0]->run_trace(1, deltas);

  cpu[0]->halt_cpu();

  return stepOK;
}

// ARC debugger simulation
//
bool System::step ()
{
  bool         stepOK = true;

  // Sanity check
  //
  if (cpu[0] == NULL) {
    LOG(LOG_ERROR) << "[SYSTEM] No processor configured.";
    return false; 
  }

  LOG(LOG_DEBUG3) << "[SYSTEM] INTERPRETIVE single step simulation mode enabled.";
  
  cpu[0]->restart_from_halted ();
    
  // Perform single-step
  //
  stepOK = cpu[0]->run_trace(1);
  
  // FIXME: Eventually we should provide a better way to single step through
  //        instructions and not abuse 'restart_from_halted' and 'halt_cpu'.
  //
  cpu[0]->halt_cpu(false);

  return stepOK;
}

// Cosim simulation
//
bool System::trace (UpdatePacket* deltas)
{
  bool stepOK = true;
  
  // Sanity check
  //
  if (cpu[0] == NULL) {
    LOG(LOG_ERROR) << "[SYSTEM] No processor configured.";
    return false; 
  }

  cpu[0]->timing_restart ();
  cpu[0]->restart_from_halted ();
  
  // Single step simulation until it is halted/interrupted
  //
  while (stepOK && !sim_opts.halt_simulation) {
    stepOK = cpu[0]->run_trace(1, deltas);
  }
  
  cpu[0]->timing_checkpoint ();
  return stepOK;
}

void System::print_stats ()
{
  // Only output statistics if this is desired
  //
  if (!sim_opts.verbose) { return; }
  
  // Print cycle accurate memory model statistics
  //
  if (sim_opts.memory_sim) {
    ext_mem_c->print_stats();
  }
  
  // Print per processor statistics
  //
  for(size_t id = 0; id < total_cores; ++id)
  {
    PRINTF() << "\nCPU" << id << " Statistics\n" 
             << "-----------------------------------------------------\n\n";
    cpu[id]->print_stats();
  }  
}

// Dump processor state to stdout
//
void System::dump_state ()
{
  fprintf(stdout, "\nProcessor State");
  fprintf(stdout, "\n------------------------------------------------------------------------------\n");
  fprintf(stdout, "PC=0x%08x Z=%i N=%i C=%i V=%i D=%i U=%i",
        cpu[0]->state.pc, cpu[0]->state.Z, cpu[0]->state.N,
        cpu[0]->state.C,  cpu[0]->state.V, cpu[0]->state.D,
        cpu[0]->state.U);
  fprintf(stdout, "\n------------------------------------------------------------------------------\n");
  for(int i = 0; i < GPR_BASE_REGS; i++) // dump register contents
    fprintf(stdout, "gpr[%2i]=0x%08X%s",
        i,
        cpu[0]->state.gprs[i],
        ((i+1)%4) ? "  " : "\n");
  fprintf(stdout, "\n------------------------------------------------------------------------------\n");
}

// -----------------------------------------------------------------------------
// Rudimentary debugging support
//

bool System::set_breakpoint (uint32 brk_location, bool& brk_s, uint32& old_instruction)
{
  if (cpu[0])
    return cpu[0]->set_breakpoint (brk_location, brk_s, old_instruction);
  return true;
}

bool System::clear_breakpoint (uint32 brk_location, uint32 old_instruction, bool brk_s)
{
  if (cpu[0])
    return cpu[0]->clear_breakpoint (brk_location, old_instruction, brk_s);
  return true;
}

// -----------------------------------------------------------------------------
// Shadow memory access methods
//

bool
System::writeShadow32 (uint32 addr, uint32 data)
{
  if (dmem) { return dmem->write32( addr, data ); }
  else      { return false; }
}

bool
System::writeShadow16 (uint32 addr, uint32 data)
{
  if (dmem) { return dmem->write16( addr, data ); }
  else      { return false; }
}

bool
System::writeShadow8 (uint32 addr, uint32 data)
{
  if (dmem) { return dmem->write8( addr, data ); }
  else      { return false; }
}

bool
System::readShadow32 (uint32 addr, uint32& data)
{
  if (dmem) { return dmem->read32( addr, data ); }
  else      { return false; }
}

bool
System::readShadow16 (uint32 addr, uint32& data)
{
  if (dmem) { return dmem->read16( addr, data ); }
  else      { return false;                      }
}

bool
System::readShadow8 (uint32 addr, uint32& data)
{
  if (dmem) { return dmem->read8( addr, data ); }
  else      { return false;                     }
}

void
System::dmem_block_write (char *buf, uint32 addr, int size)
{
   // This function is used primarily to emulate system calls.
   // If the simulator is being used in cosimulation mode, the
   // the 'dmem' memory structure will be updated without
   // updating the cache of the processor implemented in the
   // HDL simulator. To counter this, the syscall emulation
   // must perform a cache flush in the HDL model.

   uint32 wdata;

   for (int i = 0; i < size; i++) {
      wdata = buf[i];
      dmem->write8(addr+i, wdata);
   }
}

// -----------------------------------------------------------------------------
// Standard simulation
//
bool
System::start_simulation(int argc, char *argv[])
{
  bool success = false;
  
  // Setup simulated application's cmd-line arguments on it's stack.
  // N.B. this must be done before calling sys.load_elf32, as that function
  // inserts code to initialise %sp
  //
  setup_simulated_stack (argc, sim_opts.app_args, sim_opts.obj_name.c_str(), argv);
  
  // Load target executable
  switch (sim_opts.obj_format) {
    case OF_ELF : { success = (load_elf32(sim_opts.obj_name.c_str()) == 0);        break; }
    case OF_HEX : { success = (load_quicksim_hex(sim_opts.obj_name.c_str()) == 0); break; }
    case OF_BIN : { success = (load_binary_image(sim_opts.obj_name.c_str()) == 0); break; }
  }

  //If we are using the A6kV2.1 interrupts system we need to read out the reset exception vector
  //here so that we start execution in the right place.
  //FIXME: We should probably actually simulate a psuedo jump instruction on exception/interrupt entry
  if(!sim_opts.emulate_traps && cpu[0]->sys_arch.isa_opts.new_interrupts) cpu[0]->reset();
  
  // We know if the target is loaded successfully at this point
  if (success) {        
    if (sim_opts.interactive) { // Run interactive command line simulation
      for (bool active = true; active; )
        active = interact();
      
    } else {                    // Run standard simulation
      for (bool active = true; active && !sim_opts.halt_simulation ; )
        active = simulate();
      
    }    
  }
  
  if (sim_opts.dump_state) dump_state();
  
  return success;
}

// -----------------------------------------------------------------------------
// Standard simulation
//
bool System::simulate ()
{
  bool status              = false;
  sim_opts.halt_simulation = false;
  
  cpu[0]->simulation_continued();
  
  if (sim_opts.fast) {             // Fast simulation using JIT DBT
    status = run ();  
  } else if (sim_opts.trace_on) {  // Instruction tracing simulation
    status = run_trace ();
  } else {                         // Normal interpretive simulation
    status = run_notrace ();
  }
  
  cpu[0]->simulation_stopped();
  
  return status;
}

// -----------------------------------------------------------------------------
// Interactive Simulation
//
bool System::interact ()
{
  bool ret                 = true;
  sim_opts.halt_simulation = false;
  
  fflush (stderr);
  fflush (stdout);
  
  fprintf (stdout, "\n> Type help for a list of commands.\n");
  while (sim_opts.interactive) {
    fprintf (stdout, "\n> ");
    fflush (stdout);
    
    char    lbuf [1024];
    ssize_t llen = read (0, lbuf, 1024);
    lbuf[llen] = 0;
    
    if (!strncmp (lbuf, "cont", 4) ) {
      bool simOk                             = true;
      sim_opts.halt_simulation = false;

      while (simOk && !sim_opts.halt_simulation) {
        simOk = simulate();
      }
    }
    
    if (!strncmp (lbuf, "norm", 4)) {
      sim_opts.fast = false;
      fprintf (stdout, "Normal mode enabled.\n");
    }
    
    if (!strncmp (lbuf, "fast", 4)) {
      sim_opts.fast = true;
      
      // Start translation workers for each CPU
      trans_mgr.start_workers();
      
      fprintf (stdout, "Fast mode enabled.\n");
      if (sim_opts.trace_on) {
        fprintf (stdout, "Tracing disabled.\n");
        sim_opts.trace_on = false;
      }
    }
    
    if (!strncmp (lbuf, "cyc", 3)) {
#ifdef CYCLE_ACC_SIM
      sim_opts.cycle_sim  = true;
      sim_opts.memory_sim = true;
      fprintf (stdout, "Cycle accurate simulation enabled.\n");
#else
      fprintf (stdout, "This is NOT a cycle accurate simulator!\n");
#endif
    }
    
    if (!strncmp (lbuf, "func", 4)) {
#ifdef CYCLE_ACC_SIM
      sim_opts.cycle_sim = false;
      fprintf (stdout, "Cycle accurate simulation disabled.\n");
#else
      fprintf (stdout, "This is NOT a cycle accurate simulator!\n");
#endif
    }
    
    if (!strncmp (lbuf, "tron", 4)) {
      sim_opts.trace_on = true;
      fprintf (stdout, "Tracing enabled.\n");
      if (sim_opts.fast) {
        fprintf (stdout, "Fast mode disabled.\n");
        sim_opts.fast = false;
      }
    }
    
    if (!strncmp (lbuf, "troff", 5)) {
      sim_opts.trace_on = false;
      fprintf (stdout, "Tracing disabled.\n");
    }
    
    if (!strncmp (lbuf, "trace", 5)) {
      int n = 0;
      int res = sscanf(lbuf, "trace %d", &n);
      if (res == 1) {
        // Save context for -t and -f flags
        //
        bool old_trace = sim_opts.trace_on;
        bool old_fast  = sim_opts.fast;
        // Set trace mode and slow mode
        //
        sim_opts.trace_on   = true;
        sim_opts.fast       = false;
        
        // In case the processor was halted we need to restart it
        cpu[0]->restart_from_halted();
        
        // Trace for n instructions
        cpu[0]->run_trace (n);

        // Restore context for -f and -t flags
        sim_opts.trace_on   = old_trace;
        sim_opts.fast       = old_fast;
      }
    }
    
    if (!strncmp (lbuf, "break", 5)) {
      char hexbuf[256];
      sscanf(lbuf, "break %s", hexbuf);
      char *endptr;
      brk_address = (uint32)strtoul (hexbuf, &endptr, 0);
      if (endptr != hexbuf) {
        set_breakpoint (brk_address, brk_s, brk_instruction);
        
        fprintf (stdout, "Breakpoint set at 0x%08x, ", brk_address);
        if (brk_s)
          fprintf (stdout, "16-bit instruction was 0x%04x\n", brk_instruction & 0xffff);
        else
          fprintf (stdout, "32-bit instruction was 0x%08x\n", brk_instruction);
      } else
        fprintf (stdout, "Breakpoint address not recognised.\n");
    }
    
    if (!strncmp (lbuf, "clear", 5)) {
      clear_breakpoint (brk_address, brk_instruction, brk_s);
      fprintf (stdout, "Breakpoint cleared.\n");
    }
    
    if (!strncmp (lbuf, "state", 5)) {
      dump_state();
    }
    
    if (!strncmp (lbuf, "goto", 4)) {
      char hexbuf[256];
      int res = sscanf(lbuf, "goto %s", hexbuf);
      if (res == 1) {
        char *endptr;
        uint32 pc = (uint32)strtoul (hexbuf, &endptr, 0);
        if (endptr != hexbuf)
          cpu[0]->state.pc = pc;
      }
    }
    
    if (!strncmp (lbuf, "set", 3)) {
      char c;
      int res = sscanf(lbuf, "set %c", &c);
      if (res == 1) {
        switch (c) {
          case 'Z': cpu[0]->state.Z = 1; break;
          case 'N': cpu[0]->state.N = 1; break;
          case 'C': cpu[0]->state.C = 1; break;
          case 'V': cpu[0]->state.V = 1; break;
          case 'D': cpu[0]->state.D = 1; break;
          case 'U': cpu[0]->state.U = 1; break;
          default: break;
        }
      }
    }
    
    if (!strncmp (lbuf, "clr", 3)) {
      char c;
      int res = sscanf(lbuf, "clr %c", &c);
      if (res == 1) {
        switch (c) {
          case 'Z': cpu[0]->state.Z = 0; break;
          case 'N': cpu[0]->state.N = 0; break;
          case 'C': cpu[0]->state.C = 0; break;
          case 'V': cpu[0]->state.V = 0; break;
          case 'D': cpu[0]->state.D = 0; break;
          case 'U': cpu[0]->state.U = 0; break;
          default: break;
        }
      }
    }
    
    if (!strncmp (lbuf, "stats", 5)) {
      print_stats ();
    }
    
    if (!strncmp (lbuf, "zero", 4)) {
      cpu[0]->cnt_ctx.interp_inst_count.set_value(0);
      cpu[0]->cnt_ctx.native_inst_count.set_value(0);
    }

    if (!strncmp (lbuf, "sim", 3)) {
      fprintf (stdout, "SIM STATE:\n");
      fprintf (stdout, "fast mode = %i\n", sim_opts.fast);
      fprintf (stdout, "cycle accurate simulation = %i\n", sim_opts.cycle_sim);
      fprintf (stdout, "instruction tracing = %i\n", sim_opts.trace_on);
    }
    
    if (!strncmp (lbuf, "help", 4)) {
      fprintf (stdout, "%s", cli_help_msg);
    }
    
    if (!strncmp(lbuf, "quit", 4)) {
      cpu[0]->simulation_end();
      sim_opts.halt_simulation = true;
      sim_opts.interactive = false;
      ret = false;
    }
    
    if (!strncmp(lbuf, "abort", 5)) {
      exit(0);
    }
  }
  
  return ret;
}


