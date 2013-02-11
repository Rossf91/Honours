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

#ifndef INC_MEMORY_MEMORY_H_
#define INC_MEMORY_MEMORY_H_

#include <map>
#include <stack>
#include <set>
#include <vector>

#include "arch/Configuration.h"

#include "concurrent/Mutex.h"

namespace arcsim {
  // ---------------------------------------------------------------------
  // Forward declaration
  //  
  namespace mem {
    class MemoryDeviceInterface;
  }
  
  namespace util {
    class CodeBuffer;
  }
  
  namespace sys  {
    namespace mem {

      // ---------------------------------------------------------------------
      // Forward declaration
      //  
      class BlockData;
      
      // ---------------------------------------------------------------------
      // Memory class
      //
      class Memory
      {
      private:
        // ------------------------------------------------------------------
        // Map holding all physical pages of memory. Access to this structure
        // MUST be synchronised for multi-core processor simulation. The good
        // thing is that this structure is ONLY ever modified in 'get_host_page'
        // when a page is created for the first time on demand. 
        //  
        std::map<uint32,BlockData*>                    mem_blocks_;
        arcsim::concurrent::Mutex                      mem_blocks_mtx_;

        // Set of all memory devices that are registered, and array of memory
        // devices sorted by start address of memory device range
        //
        std::set<arcsim::mem::MemoryDeviceInterface*>     mem_devices_set_;
        std::vector<arcsim::mem::MemoryDeviceInterface*>  mem_devices_array_;

        // Simulated system and page Architecture Configuration
        //
        const SystemArch&                                  sys_arch;
        const PageArch&                                    page_arch;
        
        // In order to improve allocation of simulated memory used in BlockData
        // objects, we allocate memory in contiguous blocks, where each block
        // is host memory page aligned
        //
        uint32 *             block_pool_end_;
        uint32 *             block_pool_ptr_;
        std::stack<uint32*>  block_pool_stack_;
        
        // This parameter controls how many elements will be allocated in a pool
        // 2^(kLogTwoBlockPoolSize). If a block is '8K' large and kLogTwoBlockPoolSize
        // is set to '5', enough space to hold 2^5 * 8K blocks (i.e. 32 * 8K = 256K)
        // will be allocated contiguously.
        //
        static const uint32  kLogTwoBlockPoolSize = 5;
        
        // Re-quest new block from block pool. If the pool has been drained it
        // will be re-filled on demand.
        //
        uint32 * new_block();
        
        // Refills block pool when (1) this class is instantiated, and (2) as a
        // side-effect of a call to 'new_block()'. DO NOT CALL THIS METHOD
        // directly without being abosolutely sure that you know what you are doing!
        //
        uint32   refill_block_pool();
                
      public:
        // -------------------------------------------------------------------
        // Constructor/Destructor
        //
        explicit Memory(SystemArch& sys_arch, PageArch& page_arch);
        ~Memory();
        
        // -------------------------------------------------------------------
        // Retrieve block of 'memory' that is registered for a given physical
        // address. If a block of 'memory' is accessed the first time it will
        // be allocated on demand.
        //
        BlockData* get_host_page (uint32 phys_byte_addr);
        
        // Retrieve pointer to block of host memory modelling processor memory.
        // Be carefull not to call this on MemoryDevices!
        //
        uint32 * get_host_page_ptr(uint32 addr);

        
        // -------------------------------------------------------------------
        // Register MemoryDeviceInterface
        //
        bool register_memory_device(arcsim::mem::MemoryDeviceInterface* mem_dev);
        
        
        // -------------------------------------------------------------------
        // Efficient read/write methods for memory access
        //
        
        inline bool write8 (uint32 addr, uint32 data)
        { // Interpret page as pointers to bytes
          //
          uint8* page = (uint8*)(get_host_page_ptr(addr));
          // Assign BYTE 
          //
          page[page_arch.page_offset_byte_index(addr)] = (uint8)(data);
          return true;
        }
        
        inline bool write16 (uint32 addr, uint32 data)
        { // Interpret page as pointers to half words
          //
          uint16* page = (uint16*)(get_host_page_ptr(addr));
          // Assign HALF word
          //    
          page[page_arch.page_offset_half_index(addr)] = (uint16)(data);
          return true;
        }
        
        inline bool write32 (uint32 addr, uint32 data)
        { // Interpret page as pointers to words
          //
          uint32* page = (uint32*)(get_host_page_ptr(addr));
          // Assign WORD
          //
          page[page_arch.page_offset_word_index(addr)] = data;
          return true;
        }
        
        inline bool read8 (uint32 addr, uint32 &data)
        { // Interpret page as pointers to BYTES
          //
          uint8* page = (uint8*)(get_host_page_ptr(addr));
          data = page[page_arch.page_offset_byte_index(addr)];
          return true;
        }
        
        inline bool read16 (uint32 addr, uint32 &data)
        { // Interpret page as pointers to HALF words
          //
          uint16* page = (uint16*)(get_host_page_ptr(addr));
          data  = page[page_arch.page_offset_half_index(addr)];
          return true;
        }
        
        inline bool read32 (uint32 addr, uint32 &data)
        { // Interpret page as pointers to WORDS
          //
          uint32* page = (uint32*)(get_host_page_ptr(addr));
          data  = page[page_arch.page_offset_word_index(addr)];
          return true;
        }
        
        // -------------------------------------------------------------------
        // Block wise read and write of memory
        //
        bool read_block (uint32 addr, uint32 size, uint8 * buf);
        bool write_block(uint32 addr, uint32 size, uint8 const * buf);

        bool read_block_external_agent (uint32 addr, uint32 size, uint8 * buf, int id);
        bool write_block_external_agent(uint32 addr, uint32 size, uint8 const * buf, int id);
        
        // -------------------------------------------------------------------
        // Method emitting code to access memory
        //
        bool jit_emit_memory_access_functions(arcsim::util::CodeBuffer& buf,
                                              CoreArch&                 core_arch);
        
        // -------------------------------------------------------------------
        // Method that resets allocated memory contents to given values (i.e. '0')
        //
        void clear();

      };

} } } // sys::mem


#endif  // INC_MEMORY_MEMORY_H_
