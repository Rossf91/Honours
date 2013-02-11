//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
//  Memory class represents THE MAP of internal memory (i.e. BlockData)
//  objects that are allocated on demand.
//
//  Reading and writing to physical pages can be accomplished through
//  the read8, read16, read32, write8, write16 and write32 functions.
//
//  These implement memory operations that are effectively at the
//  system bus level, i.e. on physical addresses.
//
//  It is also possible to perform block wise memory reads and writes. 
//
// =====================================================================

#include <algorithm>
#include <iomanip>
#include <cstring>

#include "Assertion.h"

#include "sys/mem/Memory.h"
#include "sys/mem/BlockData.h"

#include "mem/MemoryDeviceInterface.h"

#include "arch/CoreArch.h"

#include "concurrent/ScopedLock.h"

#include "util/Allocate.h"
#include "util/CodeBuffer.h"
#include "util/Log.h"

#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_

namespace arcsim {
  namespace sys  {
    namespace mem {
      // -----------------------------------------------------------------------
      // Constructor
      //
      Memory::Memory (SystemArch& _sys_arch, PageArch& _page_arch)
      : sys_arch(_sys_arch),
        page_arch(_page_arch),
        block_pool_end_(0),
        block_pool_ptr_(0)
      {
        refill_block_pool();
      }
      // -----------------------------------------------------------------------
      // Destructor
      //      
      Memory::~Memory()
      { // Free dynamically allocated BlockData Metadata objects
        //
        for (std::map<uint32,BlockData*>::iterator
               I = mem_blocks_.begin(),
               E = mem_blocks_.end();
               I != E; ++I)
        {
          delete I->second; I->second = 0;
        }
        // Free dynamically allocated BlockData memory
        //
        while (!block_pool_stack_.empty()) {
          arcsim::util::Malloced::DeleteAligned(block_pool_stack_.top());
          block_pool_stack_.pop();
        }
      }
      
      void
      Memory::clear()
      { // Clear memory if explicit initialisation was requested
        //
        if (sys_arch.sim_opts.init_mem_custom)
        { // Clear managed RAM memory
          //
          for (std::map<uint32,BlockData*>::const_iterator
                I = mem_blocks_.begin(),
                E = mem_blocks_.end();
               I != E; ++I)
          {
            const BlockData& block_data = *I->second;
            if (block_data.is_mem_ram()) {
              uint32 * p = block_data.get_mem_ram();
              std::memset(p, sys_arch.sim_opts.init_mem_value, sizeof(p[0]) * page_arch.page_words);              
            }
          }
        }
        // Clear all registered memory devices
        //
        for (std::set<arcsim::mem::MemoryDeviceInterface*>::iterator
              I = mem_devices_set_.begin(),
              E = mem_devices_set_.end();
             I != E; ++I)
        {
          (*I)->mem_dev_clear(sys_arch.sim_opts.init_mem_value);          
        }
      }
      
      // -------------------------------------------------------------------
      // BlockData memory pool management methods
      //
      uint32
      Memory::refill_block_pool()
      {
        LOG(LOG_DEBUG) << "[MEMORY] Re-fill memory pool with '"
                       << (1<<kLogTwoBlockPoolSize)
                       << "' elements. Element size: "
                       << page_arch.page_bytes
                       << "B. Pool size: "
                       << ((page_arch.page_bytes << kLogTwoBlockPoolSize)>>10)
                       << "KB.";
        
        ASSERT(block_pool_ptr_ == block_pool_end_ &&
               "[Memory] Re-filling non empty or overflowed memory pool.");
        
        // Pre-allocate 2^(pool_shift) elements, note that we do NOT initiliase
        // it here. Only when an element is actually requested from the pool
        // will it be initialised (pay as you go policy).
        //
        block_pool_ptr_ = (uint32*)arcsim::util::Malloced::NewAligned(page_arch.page_bytes << kLogTwoBlockPoolSize);
        ASSERT(block_pool_ptr_ != 0 && "[Memory] Memory allocation failed.");
        
        // Compute end of block pointer
        //
        block_pool_end_ = block_pool_ptr_ + (page_arch.page_words << kLogTwoBlockPoolSize);
        // Register
        //
        block_pool_stack_.push(block_pool_ptr_);
        // Return count of mem pool entries allocated
        //
        return 1 << kLogTwoBlockPoolSize;
      }
      
      // -------------------------------------------------------------------
      // Get pointer to new block data memory
      //
      uint32 *
      Memory::new_block() {
        // Pointer to block data memory to be used
        //
        uint32 * p = block_pool_ptr_;
        
        // Initialise memory when requested
        //
        if (sys_arch.sim_opts.init_mem_custom) {
          std::memset(p, sys_arch.sim_opts.init_mem_value, sizeof(p[0]) * page_arch.page_words);
        }
        
        // Increment 'block_data_mem_pool_ptr_'
        //
        block_pool_ptr_ += page_arch.page_words;
        // Have we used up our memory pool?
        //
        if (block_pool_ptr_ == block_pool_end_)
          refill_block_pool();
        return p;
      }
      
      
      // -------------------------------------------------------------------
      // Compare two start ranges of MemoryDevices
      //
      static inline bool
      compare_range(arcsim::mem::MemoryDeviceInterface * const a,
                    arcsim::mem::MemoryDeviceInterface * const b)
      {
        return (a->get_range_begin() < b->get_range_begin());
      }
      
      // -------------------------------------------------------------------
      // Register MemoryDeviceInterface
      //
      bool
      Memory::register_memory_device(arcsim::mem::MemoryDeviceInterface* mem_dev)
      {
        bool success = false;
        
        if ( (success = (mem_devices_set_.find(mem_dev) == mem_devices_set_.end())) ) {
          LOG(LOG_DEBUG) << "[MEMORY] Registering MemoryDevice for range: 0x"
                         << HEX(mem_dev->get_range_begin()) << "  - 0x"
                         << HEX(mem_dev->get_range_end());
          
          // Register device if it is not already present
          //
          mem_devices_set_.insert(mem_dev);
          
          // Add memory device to mem_devices_array and sort mem_devices_array
          // so we can perform efficient binary search.
          //
          mem_devices_array_.push_back(mem_dev);
          std::sort(mem_devices_array_.begin(), mem_devices_array_.end(), compare_range);
          
          // Call initialisation method on memory device
          //
          mem_dev->mem_dev_init(sys_arch.sim_opts.init_mem_value);
        } else {
          LOG(LOG_ERROR) << "[MEMORY] MemoryDevice already registered for range: 0x"
                         << HEX(mem_dev->get_range_begin()) << "  - 0x"
                         << HEX(mem_dev->get_range_end());
        }
        return success;
      }
      
      // -------------------------------------------------------------------
      // Find device responsible for address using binary search. Return '0'
      // if no memory device is responsible
      //
      static inline arcsim::mem::MemoryDeviceInterface*
      find_memory_device(std::vector<arcsim::mem::MemoryDeviceInterface*>&  devtab,
                         const uint32                                       addr,
                         const int                                          size)
      { 
        int low, high, mid;
        
        low  = 0;
        high = size - 1;
        
        while (low <= high) {
          mid = (low + high) / 2;
          if (addr < devtab[mid]->get_range_begin()) {
            high = mid - 1;
          } else if (addr >= devtab[mid]->get_range_end()) {
            low  = mid + 1;
          } else {  // FOUND MATCH
            return devtab[mid];
          }
        }
        return 0;
      }
      
      // -------------------------------------------------------------------
      // Retrieve block of 'memory' that is registered for a given physical
      // address. If a block of 'memory' is accessed the first time it will
      // be allocated on demand.
      //
      BlockData*
      Memory::get_host_page (uint32 phys_byte_addr)
      { // -----------------------------------------------------------------
        // SCOPED LOCK START
        //
        arcsim::concurrent::ScopedLock lock(mem_blocks_mtx_);
        
        uint32 page_frame = page_arch.page_byte_frame(phys_byte_addr);
        
        // -----------------------------------------------------------------
        // If BlockData has already been allocated we return a pointer to it
        //
        if (mem_blocks_.find(page_frame) != mem_blocks_.end())
          return mem_blocks_[page_frame];
        
        // -----------------------------------------------------------------
        // BlockData has not been allocated yet, check which type of BlockData
        // object we need to instantiate
        //        
        BlockData* block_data  = 0;
        
        // -----------------------------------------------------------------
        // If a MemoryDevice is registered for this BlockData instance we will
        // allocate a special BlockData instance that performs the right call-backs
        // to the actual MemoryDevice implementation.
        //
        if (!mem_devices_set_.empty()) {  
          arcsim::mem::MemoryDeviceInterface* mem_dev = 0;
          
          // ---------------------------------------------------------------
          // Efficiently find memory device using binary search.
          // NOTE: 'mem_devices_array' must be sorted for binary search to work.
          //       We guaratee this by sorting 'mem_devices_array' whenever a
          //       Memory device is added.
          //
          if ( (mem_dev = find_memory_device(mem_devices_array_,
                                             phys_byte_addr,
                                             mem_devices_set_.size())) )
          {
            LOG(LOG_DEBUG) << "[MEMORY] Mapping MemoryDevice - page_frame: 0x"
                           << HEX(page_frame) << " phys_byte_addr: 0x" << HEX(phys_byte_addr);
            
            block_data = new BlockData(page_frame, mem_dev);
          }
        }
        
        // ----------------------------------------------------------------- 
        // If no MemoryDevices are registered for this BlockData instance
        // we will allocate a RAM memory BlockData instance.
        //
        if (block_data == 0)
          block_data = new BlockData(page_frame, new_block());
        
        // Insert newly allocated BlockData object into memory block map
        mem_blocks_.insert(std::pair<uint32,BlockData*>(page_frame, block_data));    

        return block_data;
      }

      // -------------------------------------------------------------------
      // Retrieve pointer to host modelling processor memory
      //
      uint32 *
      Memory::get_host_page_ptr(uint32 addr)
      {
        BlockData * const block = get_host_page(addr);
        ASSERT(block && block->is_mem_ram() && "Can not retrieve raw pointer to MemoryDevice."); 
        return block->get_mem_ram();
      }
      
      // -------------------------------------------------------------------
      // Block READ method. Iterates over all BlockData instances within specified
      // memory range and copies data into buffer.
      //
      bool
      Memory::read_block (uint32 addr, uint32 size, uint8 * buf)
      {
        bool         success = true;
        BlockData*   block   = get_host_page(addr);
        const uint32 offset  = (addr % page_arch.page_bytes);
        
        if(size <= (page_arch.page_bytes - offset))
        {             /* 1. We are not crossing the page boundary when copying */
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(buf, ((uint8*)block->get_mem_ram()) + offset, size);
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_read(addr, buf, size);
          }
        } else {                         /* 2. We are crossing page boundaries */
          // Read Lead-In
          //
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(buf, (((uint8*)block->get_mem_ram()) + offset),
                        (page_arch.page_bytes - offset) );
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_read(addr, buf, (page_arch.page_bytes - offset));
          }
          // Adjust buf, addr, and size
          //
          buf   += (page_arch.page_bytes - offset);
          size  -= (page_arch.page_bytes - offset);
          addr  += (page_arch.page_bytes - offset);
          // Copy in whole pages as long as size is larger than page size
          //
          while (size > page_arch.page_bytes)
          { // Retrieve next block
            //
            block = get_host_page(addr);
            if (block->is_mem_ram()) {                   /* RAM Block          */
              std::memcpy(buf, (uint8*)block->get_mem_ram(), page_arch.page_bytes);
            } else {                                    /* Memory Device block */
              block->get_mem_dev()->mem_dev_read(addr, buf, page_arch.page_bytes);
            }
            buf  += page_arch.page_bytes;
            size -= page_arch.page_bytes;
            addr += page_arch.page_bytes;
          }
          // Read Lead-Out
          //
          if(size > 0)
          { // Retrieve next block
            //
            block = get_host_page(addr);
            
            if (block->is_mem_ram()) {                   /* RAM Block            */
              std::memcpy(buf, (uint8*)block->get_mem_ram(), size);
            } else {                                    /* Memory Device block  */
              block->get_mem_dev()->mem_dev_read(addr, buf, size);
            }
          }
        }
        return success;
      }
      
      // -------------------------------------------------------------------
      // Block WRITE method. Iterates over all BlockData instances within specified
      // memory range and write data to memory.
      //
      bool
      Memory::write_block(uint32 addr, uint32 size, uint8 const * buf)
      {
        bool         success = true;
        BlockData*   block   = get_host_page(addr);
        const uint32 offset  = (addr % page_arch.page_bytes);
        
        if(size <= (page_arch.page_bytes - offset))
        {             /* 1. We are not crossing the page boundary when writing */
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(((uint8*)block->get_mem_ram()) + offset, buf, size);
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_write(addr, buf, size);
          }
        } else {                         /* 2. We are crossing page boundaries */
          // Write Lead-In
          //
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy((((uint8*)block->get_mem_ram()) + offset), buf,
                        (page_arch.page_bytes - offset) );
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_write(addr, buf, (page_arch.page_bytes - offset));
          }
          // Adjust buf, addr, and size
          //
          buf   += (page_arch.page_bytes - offset);
          size  -= (page_arch.page_bytes - offset);
          addr  += (page_arch.page_bytes - offset);
          // Write in whole pages as long as size is larger than page size
          //
          while (size > page_arch.page_bytes)
          { // Retrieve next block
            //
            block = get_host_page(addr);
            if (block->is_mem_ram()) {                   /* RAM Block          */
              std::memcpy((uint8*)block->get_mem_ram(), buf, page_arch.page_bytes);
            } else {                                    /* Memory Device block */
              block->get_mem_dev()->mem_dev_write(addr, buf, page_arch.page_bytes);
            }
            buf  += page_arch.page_bytes;
            size -= page_arch.page_bytes;
            addr += page_arch.page_bytes;
          }
          // Write Lead-Out
          //
          if(size > 0)
          { // Retrieve next block
            //
            block = get_host_page(addr);
            if (block->is_mem_ram()) {                   /* RAM Block            */
              std::memcpy((uint8*)block->get_mem_ram(), buf, size);
            } else {                                    /* Memory Device block  */
              block->get_mem_dev()->mem_dev_write(addr, buf, size);
            }
          }
        }
        return success;
      }
      
      // -------------------------------------------------------------------
      // External agent block READ method. Iterates over all BlockData instances
      // within specified memory range and copies data into buffer.
      //
      bool
      Memory::read_block_external_agent (uint32 addr, uint32 size, uint8 * buf, int id)
      {
        bool         success = true;
        BlockData*   block   = get_host_page(addr);
        const uint32 offset  = (addr % page_arch.page_bytes);
        
        if(size <= (page_arch.page_bytes - offset))
        {             /* 1. We are not crossing the page boundary when copying */
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(buf, ((uint8*)block->get_mem_ram()) + offset, size);
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_read(addr, buf, size, id);
          }
        } else {                         /* 2. We are crossing page boundaries */
          // Read Lead-In
          //
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(buf, (((uint8*)block->get_mem_ram()) + offset),
                        (page_arch.page_bytes - offset) );
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_read(addr, buf, (page_arch.page_bytes - offset), id);
          }
          // Adjust buf, addr, and size
          //
          buf   += (page_arch.page_bytes - offset);
          size  -= (page_arch.page_bytes - offset);
          addr  += (page_arch.page_bytes - offset);
          // Copy in whole pages as long as size is larger than page size
          //
          while (size > page_arch.page_bytes)
          { // Retrieve next block
            //
            block = get_host_page(addr);
            if (block->is_mem_ram()) {                   /* RAM Block          */
              std::memcpy(buf, (uint8*)block->get_mem_ram(), page_arch.page_bytes);
            } else {                                    /* Memory Device block */
              block->get_mem_dev()->mem_dev_read(addr, buf, page_arch.page_bytes, id);
            }
            buf  += page_arch.page_bytes;
            size -= page_arch.page_bytes;
            addr += page_arch.page_bytes;
          }
          // Read Lead-Out
          //
          if(size > 0)
          { // Retrieve next block
            //
            block = get_host_page(addr);
            
            if (block->is_mem_ram()) {                   /* RAM Block            */
              std::memcpy(buf, (uint8*)block->get_mem_ram(), size);
            } else {                                    /* Memory Device block  */
              block->get_mem_dev()->mem_dev_read(addr, buf, size, id);
            }
          }
        }
        return success;
      }
      
      // -------------------------------------------------------------------
      // External agent block WRITE method. Iterates over all BlockData instances
      // within specified memory range and write data to memory.
      //
      bool
      Memory::write_block_external_agent(uint32 addr, uint32 size, uint8 const * buf, int id)
      {
        bool         success = true;
        BlockData*   block   = get_host_page(addr);
        const uint32 offset  = (addr % page_arch.page_bytes);
        
        if(size <= (page_arch.page_bytes - offset))
        {             /* 1. We are not crossing the page boundary when writing */
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(((uint8*)block->get_mem_ram()) + offset, buf, size);
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_write(addr, buf, size, id);
          }
        } else {                         /* 2. We are crossing page boundaries */
          // Write Lead-In
          //
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy((((uint8*)block->get_mem_ram()) + offset), buf,
                        (page_arch.page_bytes - offset) );
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_write(addr, buf, (page_arch.page_bytes - offset), id);
          }
          // Adjust buf, addr, and size
          //
          buf   += (page_arch.page_bytes - offset);
          size  -= (page_arch.page_bytes - offset);
          addr  += (page_arch.page_bytes - offset);
          // Write in whole pages as long as size is larger than page size
          //
          while (size > page_arch.page_bytes)
          { // Retrieve next block
            //
            block = get_host_page(addr);
            if (block->is_mem_ram()) {                   /* RAM Block          */
              std::memcpy((uint8*)block->get_mem_ram(), buf, page_arch.page_bytes);
            } else {                                    /* Memory Device block */
              block->get_mem_dev()->mem_dev_write(addr, buf, page_arch.page_bytes, id);
            }
            buf  += page_arch.page_bytes;
            size -= page_arch.page_bytes;
            addr += page_arch.page_bytes;
          }
          // Write Lead-Out
          //
          if (size > 0)
          { // Retrieve next block
            //
            block = get_host_page(addr);
            if (block->is_mem_ram()) {                   /* RAM Block            */
              std::memcpy((uint8*)block->get_mem_ram(), buf, size);
            } else {                                    /* Memory Device block  */
              block->get_mem_dev()->mem_dev_write(addr, buf, size, id);
            }
          }
        }
        return success;
      }
      
      // ---------------------------------------------------------------------------
      // Methods called by JIT code generator to emit helper methods for memory
      // access.
      //
      typedef enum {
        WRITE_BYTE_UNSIGNED,
        WRITE_HALF_UNSIGNED,
        WRITE_WORD_UNSIGNED,
        READ_BYTE_UNSIGNED,
        READ_BYTE_SIGNED,
        READ_HALF_UNSIGNED,
        READ_HALF_SIGNED,
        READ_WORD,
        NUM_READ_WRITE_VARIANTS
      } MemoryAccessFunctionVariant;
      
      static const char* memory_function_name_map[NUM_READ_WRITE_VARIANTS] =
      {
        "mem_write_byte",         // WRITE_BYTE_UNSIGNED
        "mem_write_half",         // WRITE_HALF_UNSIGNED
        "mem_write_word",         // WRITE_WORD_UNSIGNED
        "mem_read_byte_unsigned", // READ_BYTE_UNSIGNED
        "mem_read_byte_signed",   // READ_BYTE_SIGNED
        "mem_read_half_unsigned", // READ_HALF_UNSIGNED
        "mem_read_half_signed",   // READ_HALF_SIGNED
        "mem_read_word"           // READ_WORD  
      };
      
      // ---------------------------------------------------------------------------
      // Private static local code generation helper functions
      //
      static bool jit_emit_memory_read_access_function (CoreArch&,
                                                        arcsim::util::CodeBuffer&,
                                                        MemoryAccessFunctionVariant);
      
      static bool jit_emit_memory_write_access_function(CoreArch&,
                                                        arcsim::util::CodeBuffer&,
                                                        MemoryAccessFunctionVariant);

      // ---------------------------------------------------------------------------
      // Method emitting code to access memory
      //
      bool
      Memory::jit_emit_memory_access_functions(arcsim::util::CodeBuffer& buf,
                                               CoreArch&                 arch)
      {
        // Memory WRITE functions
        jit_emit_memory_write_access_function(arch, buf, WRITE_WORD_UNSIGNED);
        jit_emit_memory_write_access_function(arch, buf, WRITE_HALF_UNSIGNED);
        jit_emit_memory_write_access_function(arch, buf, WRITE_BYTE_UNSIGNED);
        
        // Memory READ functions
        jit_emit_memory_read_access_function(arch, buf, READ_WORD);
        jit_emit_memory_read_access_function(arch, buf, READ_HALF_SIGNED);
        jit_emit_memory_read_access_function(arch, buf, READ_HALF_UNSIGNED);
        jit_emit_memory_read_access_function(arch, buf, READ_BYTE_SIGNED);
        jit_emit_memory_read_access_function(arch, buf, READ_BYTE_UNSIGNED);
        
        return buf.is_valid();
      }
      
      
      // ---------------------------------------------------------------------------
      // Local code emission helper function emitting memory reads
      //
      static bool
      jit_emit_memory_read_access_function(CoreArch&                    core_arch,
                                           arcsim::util::CodeBuffer&    buf,
                                           MemoryAccessFunctionVariant  variant)
      {
        bool success      = true;        
        const PageArch& p = core_arch.page_arch; // short-cut to page archictecture
        
        // Function definition is always the same
        buf.append("\nstatic inline int %s(cpuState * const s,const uint32 pc,const uint32 addr,const uint16 dst) {\n",
                   memory_function_name_map[variant]);
                
        switch (variant) {
          case READ_BYTE_UNSIGNED: {
            buf.append("EntryPageCache_ const * const p  = s->cache_page_read_ + ((addr >> %d) & 0x%08x);\n",
                       p.byte_index_shift, p.page_index_mask)
               .append("if (p->addr_ == (addr >> %d)) {\n", p.byte_tag_shift)
               .append("  s->gprs[dst] = ((uint8*)(p->block_))[addr & 0x%08x];\n", p.page_byte_offset_mask)
               .append("  return 1;\n}\n")
               .append("s->pc = pc;\n")
               .append("if (p->addr_ == ((addr >> %d) | kMemoryTypeTagDev))\n", p.byte_tag_shift)
               .append("  return cpuIoRead(s->cpu_ctx,(void*)(p->block_),addr,&(s->gprs[dst]),1);\n")
               .append("return cpuReadByte(s->cpu_ctx,addr,&(s->gprs[dst]));\n}\n");
            break;
          }
          case READ_BYTE_SIGNED: {
            buf.append("EntryPageCache_ const * const p  = s->cache_page_read_ + ((addr >> %d) & 0x%08x);\n",
                       p.byte_index_shift, p.page_index_mask)
               .append("if (p->addr_ == (addr >> %d)) {\n", p.byte_tag_shift)
               .append("  s->gprs[dst] = ((sint8*)(p->block_))[addr & 0x%08x];\n", p.page_byte_offset_mask)
               .append("  return 1;\n}\n")
               .append("s->pc = pc;\n")
               .append("if (p->addr_ == ((addr >> %d) | kMemoryTypeTagDev)) {\n", p.byte_tag_shift)
               .append("  int ret = cpuIoRead(s->cpu_ctx,(void*)(p->block_),addr,&(s->gprs[dst]),1);\n")
               .append("  s->gprs[dst] = ((sint8)(s->gprs[dst] & 0xffUL));\n  return ret;\n}\n")
               .append("return cpuReadByteSigned(s->cpu_ctx,addr,&(s->gprs[dst]));\n}\n");
            break;
          }
          case READ_HALF_UNSIGNED: {
            buf.append("EntryPageCache_ const * const p  = s->cache_page_read_ + ((addr >> %d) & 0x%08x);\n",
                       p.byte_index_shift, p.page_index_mask)
               .append("if (p->addr_ == (addr >> %d)) {\n", p.byte_tag_shift)
               .append("  s->gprs[dst] = ((uint16*)(p->block_))[(addr >> %d) & 0x%08x];\n", p.half_bits, p.page_half_offset_mask)
               .append("  return 1;\n}\n")
               .append("s->pc = pc;\n")
               .append("if (p->addr_ == ((addr >> %d) | kMemoryTypeTagDev))\n", p.byte_tag_shift)
               .append("  return cpuIoRead(s->cpu_ctx,(void*)(p->block_),addr,&(s->gprs[dst]),2);\n")
               .append("return cpuReadHalf(s->cpu_ctx,addr,&(s->gprs[dst]));\n}\n");
            break;
          }
          case READ_HALF_SIGNED: {
            buf.append("EntryPageCache_ const * const p  = s->cache_page_read_ + ((addr >> %d) & 0x%08x);\n",
                       p.byte_index_shift, p.page_index_mask)
               .append("if (p->addr_ == (addr >> %d)) {\n", p.byte_tag_shift)
               .append("  s->gprs[dst] = ((sint16*)(p->block_))[(addr >> %d) & 0x%08x];\n", p.half_bits, p.page_half_offset_mask)
               .append("  return 1;\n}\n")
               .append("s->pc = pc;\n")
               .append("if (p->addr_ == ((addr >> %d) | kMemoryTypeTagDev)) {\n", p.byte_tag_shift)
               .append("  int ret = cpuIoRead(s->cpu_ctx,(void*)(p->block_),addr,&(s->gprs[dst]),2);\n")
               .append("  s->gprs[dst] = ((sint16)(s->gprs[dst] & 0xffffUL));\n  return ret;\n}\n")
               .append("return cpuReadHalfSigned(s->cpu_ctx,addr,&(s->gprs[dst]));\n}\n");
            break;
          }
          case READ_WORD: {
            buf.append("EntryPageCache_ const * const p  = s->cache_page_read_ + ((addr >> %d) & 0x%08x);\n",
                       p.byte_index_shift, p.page_index_mask)
               .append("if (p->addr_ == (addr >> %d)) {\n", p.byte_tag_shift)
               .append("  s->gprs[dst] = p->block_[(addr >> %d) & 0x%08x];\n", p.byte_bits, p.page_word_offset_mask)
               .append("  return 1;\n}\n")
               .append("s->pc = pc;\n")
               .append("if (p->addr_ == ((addr >> %d) | kMemoryTypeTagDev))\n", p.byte_tag_shift)
               .append("  return cpuIoRead(s->cpu_ctx,(void*)(p->block_),addr,&(s->gprs[dst]),4);\n")
               .append("return cpuReadWord(s->cpu_ctx,addr,&(s->gprs[dst]));\n}\n");
            break;
          }
          default: {
            UNREACHABLE1("Invalid MemoryAccessFunctionVariant selected during code generation.");
            success = false;
            break;
          }
        }
        
        return success && buf.is_valid();
      }
      
      static bool
      jit_emit_memory_write_access_function(CoreArch&                   core_arch,
                                            arcsim::util::CodeBuffer&   buf,
                                            MemoryAccessFunctionVariant variant)
      {
        bool success      = true;
        const PageArch& p = core_arch.page_arch; // short-cut to page archictecture
        
        // Function definition is always the same
        buf.append("\nstatic inline int %s(cpuState * const s,const uint32 pc,const uint32 addr,const uint32 data) {\n",
                   memory_function_name_map[variant]);
        
        switch (variant) {
          case WRITE_BYTE_UNSIGNED: {
            buf.append("EntryPageCache_ * const p = s->cache_page_write_ + ((addr >> %d) & 0x%08x);\n",
                       p.byte_index_shift, p.page_index_mask)
               .append("if (p->addr_ == (addr >> %d)) {\n", p.byte_tag_shift)
               .append("  ((uint8*)p->block_)[addr & 0x%08x] = (uint8)data;\n", p.page_byte_offset_mask)
               .append("  return 1;\n}\n")
               .append("s->pc = pc;\n")
               .append("if (p->addr_ == ((addr >> %d) | kMemoryTypeTagDev))\n", p.byte_tag_shift)
               .append("  return cpuIoWrite(s->cpu_ctx,(void*)(p->block_),addr,data,1);\n")
               .append("return cpuWriteByte(s->cpu_ctx,addr,data);\n}\n");
            break;
          }
          case WRITE_HALF_UNSIGNED: {
            buf.append("EntryPageCache_ * const p = s->cache_page_write_ + ((addr >> %d) & 0x%08x);\n",
                       p.byte_index_shift, p.page_index_mask)
               .append("if (p->addr_ == (addr >> %d)) {\n", p.byte_tag_shift)
               .append("  ((uint16*)p->block_)[(addr >> %d) & 0x%08x] = (uint16)data;\n", p.half_bits, p.page_half_offset_mask)
               .append("  return 1;\n}\n")
               .append("s->pc = pc;\n")
               .append("if (p->addr_ == ((addr >> %d) | kMemoryTypeTagDev))\n", p.byte_tag_shift)
               .append("  return cpuIoWrite(s->cpu_ctx,(void*)(p->block_),addr,data,2);\n")
               .append("return cpuWriteHalf(s->cpu_ctx,addr,data);\n}\n");
            break;
          }
          case WRITE_WORD_UNSIGNED: {
            buf.append("EntryPageCache_ * const p = s->cache_page_write_ + ((addr >> %d) & 0x%08x);\n",
                       p.byte_index_shift, p.page_index_mask)
               .append("if (p->addr_ == (addr >> %d)) {\n", p.byte_tag_shift)
               .append("  p->block_[(addr >> %d) & 0x%08x] = data;\n", p.byte_bits, p.page_word_offset_mask)
               .append("  return 1;\n}\n")
               .append("s->pc = pc;\n")
               .append("if (p->addr_ == ((addr >> %d) | kMemoryTypeTagDev))\n", p.byte_tag_shift)
               .append("  return cpuIoWrite(s->cpu_ctx,(void*)(p->block_),addr,data,4);\n")
               .append("return cpuWriteWord(s->cpu_ctx,addr,data);\n}\n");
            break;
          }
          default: {
            UNREACHABLE1("Invalid MemoryAccessFunctionVariant selected during code generation.");
            success = false;
            break;
          }
        }
        
        return success && buf.is_valid();
      }
      
} } } // sys::mem
