//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//            Copyright (C) 2006 The University of Edinburgh
//                        All Rights Reserved
//
//                                             
// =====================================================================
//
// Description:
//
// This file implements a set of page translation cache miss routines.
// There are three page translation caches, for read, write and exec
// accesses. If an access of type X is not a hit in the page cache for
// access type X (implemented as X_tag[] and X_loc[]) then the 
// corresponding miss-routine, X_page_miss() is called.
//
// =====================================================================

#include <iomanip>

#include "exceptions.h"

#include "sys/cpu/processor.h"
#include "sys/mem/BlockData.h"

#include "mem/MemoryDeviceInterface.h"

#include "util/Log.h"

// shortcut macro for logging memory exceptions and errors
#define LOG_MEM_ERROR(_addr_,_type_)                              \
  LOG(LOG_ERROR) << "MEMORY ERROR: at address 0x"                 \
                 << std::hex << std::setfill('0') << std::setw(8) \
                 << _addr_ << _type_

namespace arcsim {
  namespace sys {
    namespace cpu {
      // -----------------------------------------------------------------------
      // Define the following macro to get debug info about page cache 
      // flushing operations that take place due to self-modifying code.
      // #define DEBUG_SMC_FLUSHES
                  
      // -----------------------------------------------------------------------
      // Raise a misalignement exception on the given address, accessed by
      // the instruction at the given pc. This raises the appropriate 
      // exception cause value, according to the architecture in force.
      //
      void
      Processor::raise_misaligned_exception (uint32 addr, uint32 pc)
      {
        if (sys_arch.isa_opts.is_isa_a6k()) {
          enter_exception (ECR(sys_arch.isa_opts.EV_Maligned,0,0), addr, pc);
        } else {
          enter_exception (ECR(sys_arch.isa_opts.EV_ProtV,MisalignedAccess,0), addr, pc);
        }
      }
      
      // -----------------------------------------------------------------------
      // Read page miss
      //
      bool
      Processor::read_page_miss (uint32           virt_addr,
                                 uint32&          data,
                                 ByteTransferSize size)
      {
        PageArch& page_arch     = core_arch.page_arch;        
        uint32 virt_addr_masked = virt_addr & state.addr_mask;
        uint32 addr_tag         = page_arch.page_byte_tag(virt_addr_masked);
        
        EntryPageCache_ * const page_cache_entry    = state.cache_page_read_ + page_arch.page_byte_index (virt_addr_masked);
        
        if (page_cache_entry->addr_ == page_arch.page_iom_tag(addr_tag))
        { // PAGE CACHE HIT for memory device
          arcsim::sys::mem::BlockData * const block
              = reinterpret_cast<arcsim::sys::mem::BlockData*>(page_cache_entry->block_);
          
          uint32 phys_addr = (block->page_frame | (page_arch.page_byte_offset_mask & virt_addr_masked));
          // Call the block-specific read function.
          //
          data = 0; // initialise 'container' variable for memory read
          if (block->get_mem_dev()->mem_dev_read(phys_addr, (uint8*)&data, size)) {
            LOG_MEM_ERROR(virt_addr_masked," [read_fun]");
            assert_interrupt_line (sys_arch.isa_opts.get_memory_error_irq_num());
            return false;
          }
          // Check for an Actionpoint if the operation did not fail
          //
          if (aps.has_ld_aps()) {
            bool no_caching = true;
            // Check a sequence of potential Actionpoint matches, relying on the
            // short-circuit semantics of || to stop evaluating when a match is detected.
            //
            bool triggered  =  aps.match_load_addr(phys_addr, no_caching)
                            || aps.match_load_data(data);
          }
        } else {
          // PAGE CACHE MISS
          //
          // First, we must detect any Actionpoint matches on this read
          // address, as APs may also disable caching of this page even
          // if there is not a precise match.
          //
          bool no_caching = aps.has_mem_data_aps(); // data APs always disable caching
          
          if (aps.has_ld_addr_aps()) {
            aps.match_load_addr(virt_addr_masked, no_caching);
          } else {
            aps.clear_trigger();
          }
          // Translate from virtual to physical addresses, returning false
          // if a TLB exception occurred.
          //
          uint32 phys_addr;
          if ( !mmu.translate_read (virt_addr_masked, state.U, phys_addr) ) {
            return false;
          }
          
          // Obtain the host page from the System.
          // If it does not exist, the System will create a new page.
          // Pages created in this way, on-demand, are ordinary RAM by default.
          //
          arcsim::sys::mem::BlockData * const block = get_host_page (phys_addr);
          
          // Perform the memory read operation. This is performed right here 
          // if the page is regular RAM, or by the nominated read function
          // if the page is memory-mapped I/O or CCM.
          //
          if (block->type_tag == kMemoryTypeTagDev)
          { // Insert this BlockData object into the Read page cache for future references
            // if we are permitted to do so.
            //
            if (!no_caching) {
              page_cache_entry->addr_  = (addr_tag | block->type_tag);
              page_cache_entry->block_ = reinterpret_cast<uint32*>(block);
            }
            // Call the block-specific read function.
            //
            data = 0; // Initialise 'container' variable for memory read
            if (block->get_mem_dev()->mem_dev_read(phys_addr, (uint8*)&data, size)) {
              LOG_MEM_ERROR(virt_addr_masked," [read_fun]");
              assert_interrupt_line (sys_arch.isa_opts.get_memory_error_irq_num());
              return false;
            }
          } else {
            
            if (!no_caching) {
              page_cache_entry->addr_  = (addr_tag | block->type_tag);
              page_cache_entry->block_ = block->get_mem_ram();
            }
            
            // Fetch pointer to host memory that is backing simulated memory
            uint8 const * const page_offset_ptr = block->location(page_arch.page_offset_byte_index(phys_addr));
            
            switch (size) {
              case kByteTransferSizeByte: { data = *((uint8*) page_offset_ptr); break; }
              case kByteTransferSizeHalf: { data = *((uint16*)page_offset_ptr); break; }
              case kByteTransferSizeWord: { data = *((uint32*)page_offset_ptr); break; }
            }
            // Finally, we must check for an Actionpoint hit on read data values
            //
            if (aps.has_ld_data_aps()) {
              aps.match_load_data(data);
            }
          }
        }
        return true;
      }
      
      // -----------------------------------------------------------------------
      // Fetch page miss
      //
      uint32
      Processor::fetch_page_miss (uint32    virt_addr,
                                  uint32&   data,
                                  bool      have_side_effect)
      {
        PageArch& page_arch     = core_arch.page_arch;
        uint32 virt_addr_masked = virt_addr & state.pc_mask;
        uint32 addr_tag         = page_arch.page_byte_tag(virt_addr_masked);
        uint32 ecause           = 0;

        EntryPageCache_ * const page_cache_entry    = state.cache_page_exec_ + page_arch.page_byte_index(virt_addr_masked);
        
        if (page_cache_entry->addr_ == page_arch.page_iom_tag(addr_tag))
        { // This is a page cache hit to a memory device mapped page
          arcsim::sys::mem::BlockData * const block
              = reinterpret_cast<arcsim::sys::mem::BlockData*>(page_cache_entry->block_);

          uint32 phys_addr = (block->page_frame | (page_arch.page_byte_offset_mask & virt_addr_masked));          
          // Call the block-specific read function.
          //
          data = 0; // Initialise 'container' variable for memory read
          if (block->get_mem_dev()->mem_dev_read(phys_addr, (uint8*)&data, kByteTransferSizeHalf)) {
            LOG_MEM_ERROR(virt_addr_masked," [read_fun]");
            assert_interrupt_line (sys_arch.isa_opts.get_memory_error_irq_num());
            // Return Instruction Fetch Memory Error
            return ECR(sys_arch.isa_opts.EV_ITLBMiss, IfetchTLBMiss, 0);
          }
        } else {
          // This is a page cache miss, so we don't know anything yet.
          //

          // Translate from virtual to physical addresses, returning the exception
          // code if a TLB exception occurs
          //
          uint32 phys_addr;
          if (have_side_effect)
          { // Modify MMU auxiliary register AUX_TLB_PD0 with the faulting address
            // upon virtual to physical translation. Or MPU ECR with faulting region.
            if ( (ecause = mmu.translate_exec (virt_addr_masked, state.U, phys_addr)) ) {
              return ecause;
            }
          } else {
            // Side-effect free MMU virtual to physical translation. This is used
            // during trace construction.
            if ( (ecause = mmu.lookup_exec(virt_addr_masked, state.U, phys_addr)) ) {
              return ecause;
            }
          }
          
          // Obtain the host page from the System.
          //
          arcsim::sys::mem::BlockData * const block = get_host_page (phys_addr);
          
          // If this page was not marked as `cached for execution' then it 
          // could be cached in the write_tag array. For consistency, a
          // page cannot be in both the exec and write page caches, so we
          // must eliminate it from the write page cache.
          //
          if (block->is_w_cached()) {
            block->set_w_cached(false); // reset w_cached flag
            
            // Remove page from writeable cache
            if (block->type_tag == kMemoryTypeTagDev){
              page_cache.purge_entry (arcsim::sys::cpu::PageCache::WRITE,
                                      reinterpret_cast<uint32*>(block));
            } else {
              page_cache.purge_entry (arcsim::sys::cpu::PageCache::WRITE,
                                      block->get_mem_ram());
            }
          }
          // Mark this pages as `cached for execution', so as to trigger
          // flushing of dcode cache if this page is subsequently written.
          //
          block->set_x_cached(true);
          
          // Perform the memory read operation. This is performed right here 
          // if the page is regular RAM, or by the nominated read function
          // if the page is memory-mapped I/O or CCM.
          //
          if (block->type_tag == kMemoryTypeTagDev)
          { // Insert this BlockData object into the Exec page cache for future references
            //
            page_cache_entry->addr_  = (addr_tag | block->type_tag);
            page_cache_entry->block_ = reinterpret_cast<uint32*>(block);
            
            // Call the block-specific read function.
            //
            data = 0; // Initialise 'container' variable for memory read
            if (block->get_mem_dev()->mem_dev_read(phys_addr, (uint8*)&data, kByteTransferSizeHalf)) {
              LOG_MEM_ERROR(virt_addr_masked," [read_fun]");
              assert_interrupt_line (sys_arch.isa_opts.get_memory_error_irq_num());
              // Return Instruction Fetch Memory Error
              return ECR(sys_arch.isa_opts.EV_ITLBMiss, IfetchTLBMiss, 0);
            }
          } else {
            // Insert this page into the Exec page cache for future references
            //
            page_cache_entry->addr_  = (addr_tag | block->type_tag);
            page_cache_entry->block_ = block->get_mem_ram();
            // Compute page offset
            //
            uint32 page_offset = page_arch.page_offset_half_index(phys_addr);
            // Now read the 16-bit fetch packet from the required address
            //
            data = ((uint16*)(page_cache_entry->block_))[page_offset];
          }
        }
        return 0UL;
      }
            
      // ---------------------------------------------------------------------------
      // Atomic memory exchange function
      //
      // Call this function to atomically exchange the contents of a target
      // instruction operand (usually a register) with a target system memory location.
      //
      // The 'unmasked_addr' argument must be the address of a location in the simulated
      // system memory. This function will attempt to derive the physical target 
      // address and hence the host address for this location. If this is not
      // successful the function returns false, and may also raise an MMU-related
      // exception on the target processor. This location is both read and written.
      // 
      // The 'p_reg' argument must be a pointer to the processor register, within
      // the state structure for the processor. This location is both read and
      // written.
      //
      bool
      Processor::atomic_exchange (uint32 virt_unmasked_byte_addr, uint32* p_reg)
      {
        PageArch& page_arch   = core_arch.page_arch;
        uint32 virt_byte_addr = virt_unmasked_byte_addr & state.addr_mask;
        uint32 addr_tag       = page_arch.page_byte_tag   (virt_byte_addr);
        bool   success        = true;
        bool   no_caching     = false;

        // Early exit when address is misaligned
        //
        if (addr_is_misaligned (virt_unmasked_byte_addr, 3, state.pc)) return false;
        
        EntryPageCache_ * const page_cache_entry = state.cache_page_write_ + page_arch.page_byte_index (virt_byte_addr);
        EntryPageCache_ * const page_cache_entry_r = state.cache_page_read_ + page_arch.page_byte_index (virt_byte_addr);

#if BIG_ENDIAN_SUPPORT
        if (sim_opts.big_endian) { // Convert data BEFORE writing to memory
          *p_reg = ((*p_reg & 0x000000ffUL) << 24) | ((*p_reg & 0x0000ff00UL) << 8)
                 | ((*p_reg & 0x00ff0000UL) >> 8)  | ((*p_reg & 0xff00000fUL) >> 24);
        }
#endif  
        if ( (page_cache_entry->addr_ == addr_tag) && (page_cache_entry_r->addr_ == addr_tag) && !aps.has_mem_aps()) 
        { // PAGE CACHE HIT AND the memory location is NOT of type kMemoryTypeTagDev
          // and there are no memory-based Actionpoints to catch.
          //

          // Compute offset within page
          //
          uint32 page_offset = page_arch.page_offset_word_index(virt_byte_addr);
          
          // Retrieve location of memory
          //
          uint32* loc = &(page_cache_entry->block_[page_offset]);

          // -------------------------------------------------------------------
          // ATOMIC EXCHANGE;
          //
          // map ARCompact 'EX' instruction onto simulation hosts 'xchgl' instruction
          //
          __asm__ __volatile__("lock; xchgl %0, %1"
                                 : "+m" (*loc),                 /* output - %0 */
                                   "=a" (*p_reg)                /* output - %1 */
                                 : "1"  (*p_reg)                /* input/output - %1 */
                                 : "cc");                       /* clobber list */
        } else {
          // Next on the list we must check if we have a hit into a kMemoryTypeTagDev
          // device
          //
          if (page_cache_entry->addr_ == page_arch.page_iom_tag(addr_tag))
          { // PAGE CACHE HIT to a memory mapped IO page
            //
            arcsim::sys::mem::BlockData * const block
                = reinterpret_cast<arcsim::sys::mem::BlockData*>(page_cache_entry->block_);
            // Retrieve memory device responsible for this BlockData chunk
            //
            arcsim::mem::MemoryDeviceInterface* const mem_dev = block->get_mem_dev();

            // Call the page-specific write function.
            //
            uint32 phys_addr = (block->page_frame | (page_arch.page_byte_offset_mask & virt_byte_addr));
            
            // FIXME: We need support for atomic exchange for IO devices.
            //        What should we do with exceptions (i.e. error return values)
            //        from IO devices?
            //
            uint32 tmp;
            mem_dev->mem_dev_read (phys_addr, (uint8*) &tmp,      kByteTransferSizeWord);
            mem_dev->mem_dev_write(phys_addr, (uint8*) &(*p_reg), kByteTransferSizeWord);
            *p_reg = tmp;

          } else {
            // PAGE CACHE MISS
            //
            // Translate from virtual to physical addresses
            //
            uint32 phys_addr;
            if ( !mmu.translate_rmw (virt_byte_addr, state.U, phys_addr) ) {
              success = false;
            } else {
              // Obtain the host page.
              //
              arcsim::sys::mem::BlockData * const host_page = get_host_page (phys_addr);
              
              // Check if some part of this page could be cached in the dcode
              // and translation cache.
              //
              if (host_page->is_x_cached()) {
                // This is a write to a page containing instructions so we need to
                // do appropriate book-keeping.
                //
                purge_dcode_cache();
                // Flush executable page cache
                //
                page_cache.flush(arcsim::sys::cpu::PageCache::EXEC);
                // Reset x_cached flag
                //
                host_page->set_x_cached(false);
                
                if (phys_profile_.is_translation_present(phys_addr)) {
                  remove_translation(phys_addr);
                }
                if (sim_opts.fast) {
                  if (phys_profile_.remove_translation(phys_addr)) {
                    purge_translation_cache();
                    LOG(LOG_DEBUG) << "[CPU-MEMORY.EX] REMOVED TRANSLATIONS FOR PAGE."; 
                  }
                  // If any BlockEntries have been touched for this page we must remove
                  // them so they do not get dispatched for compilation
                  //
                  if (phys_profile_.remove_trace(phys_addr)) {
                    LOG(LOG_DEBUG) << "[CPU-MEMORY.EX] REMOVED TRACES FOR PAGE."; 
                  }
                }
              }
              // Check whether the atomic memory exchange operation is within an I/O
              // page or a regular memory page.
              //
              if (host_page->type_tag == kMemoryTypeTagDev)
              {
                uint32 load_data;
                uint32 store_data = *p_reg;
                // Retrieve memory device responsible for this BlockData chunk
                //
                arcsim::mem::MemoryDeviceInterface* const mem_dev = host_page->get_mem_dev();

                // FIXME: We need support for atomic exchange for IO devices.
                //        What should we do with exceptions (i.e. error return values)
                //        from IO devices?
                //
                mem_dev->mem_dev_read (phys_addr, (uint8*) &load_data, kByteTransferSizeWord);
                mem_dev->mem_dev_write(phys_addr, (uint8*) &(*p_reg),  kByteTransferSizeWord);
                *p_reg = load_data;

                if (aps.has_mem_aps()) {
                  // Check a sequence of potential Actionpoint matches, relying on the
                  // short-circuit semantics of || to stop evaluating when a match is detected.
                  //
                  bool triggered  =  aps.match_load_addr(phys_addr, no_caching)
                                  || aps.match_load_data(load_data)
                                  || aps.match_store(phys_addr, no_caching, store_data);
                }
                // Insert this BlockData object into the Write page cache for future 
                // references, if we are permitted to do so.
                //
                if (!no_caching) {
                  page_cache_entry->addr_  = (addr_tag | host_page->type_tag);
                  page_cache_entry->block_ = reinterpret_cast<uint32*>(host_page);
                }              
              } else {
                uint32* page       = host_page->get_mem_ram();
                bool    no_caching = false;
                uint32  tmp        = *p_reg;                // preserve a copy of the store data
                
                // Compute offset within page
                //
                uint32  page_offset = page_arch.page_offset_word_index(phys_addr);
                
                // Retrieve simulation host memory location
                //
                uint32* loc = &(page[page_offset]);
                
                // Map ARCompact 'EX' instruction onto simulation hosts 'xchgl' instruction
                //
                __asm__ __volatile__("lock; xchgl %0, %1"
                                       : "+m" (*loc),                 /* output - %0 */
                                         "=a" (*p_reg)                /* output - %1 */
                                       : "1"  (*p_reg)                /* input/output - %1 */
                                       : "cc");                       /* clobber list */
                // Check for an Actionpoint hit, if any memory Actionpoints are enabled
                //
                if (aps.has_mem_aps()) {
                  // Check a sequence of potential Actionpoint matches, relying on the
                  // short-circuit semantics of || to stop evaluating when a match is detected.
                  // Store operations are checked first, as they have priority in case both
                  // load and store Actionpoints are enabled.
                  //
                  bool triggered  =  aps.match_load_addr(phys_addr, no_caching)
                                  || aps.match_load_data(*p_reg)
                                  || aps.match_store(phys_addr, no_caching, tmp);
                }

                // Insert this page into the Write page cache for future references
                //
                if (!no_caching) {
                  page_cache_entry->addr_  = (addr_tag | host_page->type_tag);
                  page_cache_entry->block_ = page;
                }
              }              
              // Mark this page as `cached for writing' if caching is not disabled
              // by the presence of Actionpoints.
              //
              host_page->set_w_cached(!no_caching); 
            }
          }
        }
#if BIG_ENDIAN_SUPPORT
        if (sim_opts.big_endian) { // Convert data AFTER reading from memory
          *p_reg = ((*p_reg & 0x000000ffUL) << 24) | ((*p_reg & 0x0000ff00UL) << 8)
                 | ((*p_reg & 0x00ff0000UL) >> 8)  | ((*p_reg & 0xff00000fUL) >> 24);
        }
#endif        
        return success;
      }

      // -----------------------------------------------------------------------
      // WRITE page miss method
      //
      bool
      Processor::write_page_miss (uint32            virt_addr,
                                  uint32            data,
                                  ByteTransferSize  size)
      {
        PageArch& page_arch     = core_arch.page_arch;
        uint32 virt_addr_masked = virt_addr & state.addr_mask;
        uint32 addr_tag         = page_arch.page_byte_tag   (virt_addr_masked);
        
        EntryPageCache_ * const page_cache_entry = state.cache_page_write_ + page_arch.page_byte_index (virt_addr_masked);
        
        if (page_cache_entry->addr_ == page_arch.page_iom_tag(addr_tag))
        { // This is a page cache hit to a memory mapped IO page
          //
          arcsim::sys::mem::BlockData * const block
              = reinterpret_cast<arcsim::sys::mem::BlockData*>(page_cache_entry->block_);

          // Call the page-specific write function.
          //
          uint32 phys_addr = (block->page_frame | (page_arch.page_byte_offset_mask & virt_addr_masked));
          if (block->get_mem_dev()->mem_dev_write(phys_addr, (uint8*)&data, size))
          {
            LOG_MEM_ERROR(virt_addr_masked," [write_fun]");
            assert_interrupt_line (sys_arch.isa_opts.get_memory_error_irq_num());
            return false;
          }
          // Check for an Actionpoint if the operation did not fail
          //
          if (aps.has_st_aps()) {
            bool no_caching = true;
            aps.match_store(phys_addr, no_caching, data);
          }
        } else {
          // This is a page cache miss, so we don't know anything yet.
          //
          // First, we must detect any Actionpoint matches on this write
          // address, as APs may also disable caching of this page even
          // if there is not a precise match.
          //
          // Value-based watchpoints always prevent the caching of this
          // page (indeed all pages), as an atomic exchange involves both
          // read and write.
          //
          bool no_caching = aps.has_mem_data_aps();
          
          if (aps.has_st_aps()) {
            aps.match_store(virt_addr_masked, no_caching, data);
          }
          
          // Translate from virtual to physical addresses, returning false
          // if a TLB exception occurred.
          //
          uint32 phys_addr;
          if ( !mmu.translate_write (virt_addr_masked, state.U, phys_addr) ) {
            return false;
          }
          
          // Obtain the block of memory for simulated physical address
          //
          arcsim::sys::mem::BlockData * const block = get_host_page (phys_addr);
          
          if (block->is_x_cached()) {
            // If code for this block has been marked as executable and we are writing
            // to it (i.e. self-modifying code), we need to flush the dcode cache
            // and the executable page cache. If the JIT compiler is enabled we need
            // to remove any translations and traces for this page.
            //
            purge_dcode_cache();
            purge_page_cache(arcsim::sys::cpu::PageCache::EXEC);

            if (sim_opts.fast) {
              // Remove translations
              if (phys_profile_.remove_translation(phys_addr)) {
                purge_translation_cache();
                LOG(LOG_DEBUG) << "[CPU-MEMORY.WRITE] REMOVED TRANSLATIONS FOR PAGE."; 
              }
              // Remove traces
              if (phys_profile_.remove_trace(phys_addr)) {
                LOG(LOG_DEBUG) << "[CPU-MEMORY.WRITE] REMOVED TRACES FOR PAGE."; 
              }            
            }
            // Reset x_cached flag
            //
            block->set_x_cached(false);
          }
          // Mark this page as `cached for writing', if permitted
          //
          block->set_w_cached(!no_caching);
          
          // Perform the memory write operation. This is performed right here 
          // if the page is regular RAM, or by the nominated write function
          // if the page is memory-mapped I/O or CCM.
          //
          if (block->type_tag == kMemoryTypeTagDev)
          {
            // Insert this BlockData object into the Write page cache for future 
            // references, if we are permitted to do so.
            //
            if (!no_caching) {
              page_cache_entry->addr_  = (addr_tag | block->type_tag);
              page_cache_entry->block_ = reinterpret_cast<uint32*>(block);
            }
            
            // Call the page-specific write function.
            //
            if (block->get_mem_dev()->mem_dev_write(phys_addr, (uint8*)&data, size))
            {
              LOG_MEM_ERROR(virt_addr_masked," [write_fun]");
              assert_interrupt_line (sys_arch.isa_opts.get_memory_error_irq_num());
              return false;
            }
          } else {
            if (!no_caching) {
              page_cache_entry->addr_  = (addr_tag | block->type_tag);
              page_cache_entry->block_ = block->get_mem_ram();
            }
            
            // Fetch pointer to host memory that is backing simulated memory
            //
            uint8 * const page_offset_ptr = block->location(page_arch.page_offset_byte_index(phys_addr));
            
            // Now write the memory object we want
            //
            switch (size) {
              case kByteTransferSizeByte: { *((uint8*) page_offset_ptr) = (uint8) data; break; }
              case kByteTransferSizeHalf: { *((uint16*)page_offset_ptr) = (uint16)data; break; }
              case kByteTransferSizeWord: { *((uint32*)page_offset_ptr) = (uint32)data; break; }
            }
          }
        }
        return true;
      }
      
        bool
      Processor::write_no_exception(uint32 addr, uint32 data, ByteTransferSize size){

        uint32 phys_addr;
        bool outcome = mmu.lookup_exec(addr, state.U, phys_addr);

        // Obtain the block of memory for simulated physical address
        //
        arcsim::sys::mem::BlockData * const block = get_host_page (phys_addr);
        if (phys_profile_.remove_translation(phys_addr)) {
          purge_translation_cache();
          LOG(LOG_DEBUG) << "[CPU-MEMORY.WRITE] REMOVED TRANSLATIONS FOR PAGE."; 
        }
        // Remove traces
        if (phys_profile_.remove_trace(phys_addr)) {
          LOG(LOG_DEBUG) << "[CPU-MEMORY.WRITE] REMOVED TRACES FOR PAGE."; 
        }

        // Perform the memory write operation. This is performed right here 
        // if the page is regular RAM, or by the nominated write function
        // if the page is memory-mapped I/O or CCM.
        //
        if (block->type_tag == kMemoryTypeTagDev)
        {

          // Call the page-specific write function.
          //
          if (block->get_mem_dev()->mem_dev_write(phys_addr, (uint8*)&data, size))
          {
            LOG(LOG_ERROR)<<"tried to write_no_exception to a MemDevice that threw memory error";
            return false;
          }
        } else {
          // Fetch pointer to host memory that is backing simulated memory
          //
          uint8 * const page_offset_ptr = block->location(core_arch.page_arch.page_offset_byte_index(phys_addr));

          // Now write the memory object we want
          //
          switch (size) {
            case kByteTransferSizeByte: { *((uint8*) page_offset_ptr) = (uint8) data; break; }
            case kByteTransferSizeHalf: { *((uint16*)page_offset_ptr) = (uint16)data; break; }
            case kByteTransferSizeWord: { *((uint32*)page_offset_ptr) = (uint32)data; break; }
          }
        }
        return outcome;
      }
      
      // -------------------------------------------------------------------
      // Block READ method. Iterates over all BlockData instances within specified
      // memory range and copies data into buffer.
      // 
      bool
      Processor::read_block (uint32 phys_addr, uint32 size, uint8 * buf)
      {
        const uint32                   block_bytes  = core_arch.page_arch.page_bytes;
        const uint32                   offset       = phys_addr % block_bytes;
        arcsim::sys::mem::BlockData*   block        = get_host_page(phys_addr);
        bool                           success = true;
        
        if(size <= (block_bytes - offset))
        {             /* 1. We are not crossing the page boundary when writing */
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(buf, ((uint8*)block->get_mem_ram()) + offset, size);
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_read(phys_addr, buf, size);
          }
        } else {                         /* 2. We are crossing page boundaries */
          // Read Lead-In
          //
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(buf, (((uint8*)block->get_mem_ram()) + offset),
                        (block_bytes - offset) );
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_read(phys_addr, buf,
                                               (block_bytes - offset));
          }          
          // Adjust buf, addr, and size
          //
          buf       += (block_bytes - offset);
          size      -= (block_bytes - offset);
          phys_addr += (block_bytes - offset);
          // Copy in whole pages as long as size is larger than page size
          //
          while (size > core_arch.page_arch.page_bytes)
          { // Retrieve next block
            //
            block = get_host_page(phys_addr);
            if (block->is_mem_ram()) {                   /* RAM Block          */
              std::memcpy(buf, (uint8*)block->get_mem_ram(), core_arch.page_arch.page_bytes);
            } else {                                    /* Memory Device block */
              block->get_mem_dev()->mem_dev_read(phys_addr, buf, core_arch.page_arch.page_bytes);
            }
            buf       += block_bytes;
            size      -= block_bytes;
            phys_addr += block_bytes;
          }
          // Read Lead-Out
          //
          if(size > 0)
          { // Retrieve next block
            //
            block = get_host_page(phys_addr);
            if (block->is_mem_ram()) {                  /* RAM Block            */
              std::memcpy(buf, (uint8*)block->get_mem_ram(), size);
            } else {                                    /* Memory Device block  */
              block->get_mem_dev()->mem_dev_read(phys_addr, buf, size);
            }
          }
        }
        return success;
      }
      
      // -------------------------------------------------------------------
      // Block WRITE method. Iterates over all BlockData instances within specified
      // memory range and write data to memory. Additional book-keeping is performed
      // to maintain a correct state of various internal cache and lookup structures.
      // 
      bool
      Processor::write_block(uint32 phys_addr, uint32 size, uint8 const * buf)
      {
        const uint32                   block_bytes  = core_arch.page_arch.page_bytes;
        const uint32                   offset       = phys_addr % block_bytes;
        arcsim::sys::mem::BlockData*   block        = get_host_page(phys_addr);
        bool                           success      = true;
        bool                           touched_exec = false;
        
        if(size <= (block_bytes - offset))
        {             /* 1. We are not crossing the page boundary when writing */
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(((uint8*)block->get_mem_ram()) + offset, buf, size);
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_write(phys_addr, buf, size);
          }
          // Remember if we have written to a block that contains executable code
          //
          if (!touched_exec && block->is_x_cached()) { touched_exec = true; }
        } else {                         /* 2. We are crossing page boundaries */
          // Write Lead-In
          //
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy((((uint8*)block->get_mem_ram()) + offset), buf,
                        (block_bytes - offset) );
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_write(phys_addr, buf,
                                                (block_bytes - offset));
          }
          // Remember if we have written to a block that contains executable code
          //
          if (!touched_exec && block->is_x_cached()) { touched_exec = true; }
          // Adjust buf, addr, and size
          //
          buf       += (block_bytes - offset);
          size      -= (block_bytes - offset);
          phys_addr += (block_bytes - offset);
          // Write in whole pages as long as size is larger than page size
          //
          while (size > core_arch.page_arch.page_bytes)
          { // Retrieve next block
            //
            block = get_host_page(phys_addr);
            if (block->is_mem_ram()) {                  /* RAM Block           */
              std::memcpy((uint8*)block->get_mem_ram(), buf, core_arch.page_arch.page_bytes);
            } else {                                    /* Memory Device block */
              block->get_mem_dev()->mem_dev_write(phys_addr, buf, core_arch.page_arch.page_bytes);
            }
            // Remember if we have written to a block that contains executable code
            //
            if (!touched_exec && block->is_x_cached()) { touched_exec = true; }
            buf       += block_bytes;
            size      -= block_bytes;
            phys_addr += block_bytes;
          }
          // Write Lead-Out
          //
          if(size > 0)
          { // Retrieve next block
            //
            block = get_host_page(phys_addr);
            if (block->is_mem_ram()) {                  /* RAM Block            */
              std::memcpy((uint8*)block->get_mem_ram(), buf, size);
            } else {                                    /* Memory Device block  */
              block->get_mem_dev()->mem_dev_write(phys_addr, buf, size);
            }
            // Remember if we have written to a block that contains executable code
            //
            if (!touched_exec && block->is_x_cached()) { touched_exec = true; }
          }
        }
        // If we have touched a block that contains executable code we need to
        // clear the dcode and page caches. If the JIT compiler is enabled we
        // do the safe thing and flush all translations, the translation cache,
        // and remove all gathered traces.
        //
        if (touched_exec) {
          purge_dcode_cache();
          purge_page_cache(arcsim::sys::cpu::PageCache::EXEC);
          
          if (sim_opts.fast) { /* If we JIT was enabled we also remove native code and traces */
            purge_translation_cache();
            phys_profile_.remove_translations();
            phys_profile_.remove_traces();
          }
        }
        return success;
      }
      
      // -------------------------------------------------------------------
      // Block READ method. Iterates over all BlockData instances within specified
      // memory range and copies data into buffer.
      // 
      bool
      Processor::read_block_external_agent (uint32  phys_addr,
                                            uint32  size,
                                            uint8 * buf,
                                            int     agent_id)
      {
        const uint32                   block_bytes  = core_arch.page_arch.page_bytes;
        const uint32                   offset       = phys_addr % block_bytes;
        arcsim::sys::mem::BlockData*   block        = get_host_page(phys_addr);
        bool                           success      = true;
        
        if(size <= (block_bytes - offset))
        {             /* 1. We are not crossing the page boundary when writing */
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(buf, ((uint8*)block->get_mem_ram()) + offset, size);
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_read(phys_addr, buf, size, agent_id);
          }
        } else {                         /* 2. We are crossing page boundaries */
          // Read Lead-In
          //
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(buf, (((uint8*)block->get_mem_ram()) + offset),
                        (block_bytes - offset) );
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_read(phys_addr, buf,
                                               (block_bytes - offset),
                                               agent_id);
          }          
          // Adjust buf, addr, and size
          //
          buf       += (block_bytes - offset);
          size      -= (block_bytes - offset);
          phys_addr += (block_bytes - offset);
          // Copy in whole pages as long as size is larger than page size
          //
          while (size > block_bytes)
          { // Retrieve next block
            //
            block = get_host_page(phys_addr);
            if (block->is_mem_ram()) {                   /* RAM Block          */
              std::memcpy(buf, (uint8*)block->get_mem_ram(), block_bytes);
            } else {                                    /* Memory Device block */
              block->get_mem_dev()->mem_dev_read(phys_addr,
                                                 buf,
                                                 block_bytes,
                                                 agent_id);
            }
            buf       += block_bytes;
            size      -= block_bytes;
            phys_addr += block_bytes;
          }
          // Read Lead-Out
          //
          if(size > 0)
          { // Retrieve next block
            //
            block = get_host_page(phys_addr);
            if (block->is_mem_ram()) {                  /* RAM Block            */
              std::memcpy(buf, (uint8*)block->get_mem_ram(), size);
            } else {                                    /* Memory Device block  */
              block->get_mem_dev()->mem_dev_read(phys_addr, buf, size, agent_id);
            }
          }
        }
        return success;
      }
      
      // -------------------------------------------------------------------
      // Block WRITE method. Iterates over all BlockData instances within specified
      // memory range and write data to memory. Additional book-keeping is performed
      // to maintain a correct state of various internal cache and lookup structures.
      // 
      bool
      Processor::write_block_external_agent(uint32        phys_addr,
                                            uint32        size,
                                            uint8 const * buf,
                                            int           agent_id)
      {
        const uint32                   block_bytes  = core_arch.page_arch.page_bytes;
        const uint32                   offset       = phys_addr % block_bytes;
        arcsim::sys::mem::BlockData*   block        = get_host_page(phys_addr);
        bool                           success      = true;
        bool                           touched_exec = false;
        
        if(size <= (block_bytes - offset))
        {             /* 1. We are not crossing the page boundary when writing */
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy(((uint8*)block->get_mem_ram()) + offset, buf, size);
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_write(phys_addr, buf, size, agent_id);
          }
          // Remember if we have written to a block that contains executable code
          //
          if (!touched_exec && block->is_x_cached()) { touched_exec = true; }
        } else {                         /* 2. We are crossing page boundaries */
          // Write Lead-In
          //
          if (block->is_mem_ram()) {                   /* RAM Block            */
            std::memcpy((((uint8*)block->get_mem_ram()) + offset), buf,
                        (block_bytes - offset) );
          } else {                                    /* Memory Device block   */
            block->get_mem_dev()->mem_dev_write(phys_addr,
                                                buf,
                                                (block_bytes - offset),
                                                agent_id);
          }
          // Remember if we have written to a block that contains executable code
          //
          if (!touched_exec && block->is_x_cached()) { touched_exec = true; }
          // Adjust buf, addr, and size
          //
          buf       += (block_bytes - offset);
          size      -= (block_bytes - offset);
          phys_addr += (block_bytes - offset);
          // Write in whole pages as long as size is larger than page size
          //
          while (size > block_bytes)
          { // Retrieve next block
            //
            block = get_host_page(phys_addr);
            if (block->is_mem_ram()) {                  /* RAM Block           */
              std::memcpy((uint8*)block->get_mem_ram(), buf, block_bytes);
            } else {                                    /* Memory Device block */
              block->get_mem_dev()->mem_dev_write(phys_addr,
                                                  buf,
                                                  block_bytes,
                                                  agent_id);
            }
            // Remember if we have written to a block that contains executable code
            //
            if (!touched_exec && block->is_x_cached()) { touched_exec = true; }
            buf       += block_bytes;
            size      -= block_bytes;
            phys_addr += block_bytes;
          }
          // Write Lead-Out
          //
          if(size > 0)
          { // Retrieve next block
            //
            block = get_host_page(phys_addr);
            if (block->is_mem_ram()) {                  /* RAM Block            */
              std::memcpy((uint8*)block->get_mem_ram(), buf, size);
            } else {                                    /* Memory Device block  */
              block->get_mem_dev()->mem_dev_write(phys_addr, buf, size, agent_id);
            }
            // Remember if we have written to a block that contains executable code
            //
            if (!touched_exec && block->is_x_cached()) { touched_exec = true; }
          }
        }
        // If we have touched a block that contains executable code we need to
        // clear the dcode and page caches. If the JIT compiler is enabled we
        // do the safe thing and flush all translations, the translation cache,
        // and remove all gathered traces.
        //
        if (touched_exec) {
          purge_dcode_cache();
          purge_page_cache(arcsim::sys::cpu::PageCache::EXEC);
          
          if (sim_opts.fast) { /* If we JIT was enabled we also remove native code and traces */
            purge_translation_cache();
            phys_profile_.remove_translations();
            phys_profile_.remove_traces();
          }
        }
        return success;
      }

} } } //  arcsim::sys::cpu

