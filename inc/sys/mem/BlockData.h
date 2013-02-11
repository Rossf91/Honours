//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:  BlockData is a meta-data structure encapsulating blocks
//               of simulated memory. It is important to keep the size of
//               this class as small as possible.
//
// =====================================================================

#ifndef INC_MEMORY_BLOCKDATA_H_
#define INC_MEMORY_BLOCKDATA_H_

#include "api/types.h"
#include "sys/mem/types.h"

// -----------------------------------------------------------------------
// Forward declare MemoryDeviceInterface
//
namespace arcsim {
  namespace mem {
    class MemoryDeviceInterface;
  }
}

namespace arcsim {
  namespace sys {
    namespace mem {
      
      // -----------------------------------------------------------------------
      // BlockData represents a target memory page
      //
      class BlockData
      {
      private:        
        // BlockData 'block_' either points to:
        //  1. A heap-allocated ALIGNED memory block managed by 'arcsim::sys::mem::Memory'
        //  2. A arcsim::mem::MemoryDeviceInterface
        //
        // @igor: because 'block_' is aligned we can use pointer tagging to indicate
        //        that block_ is 'x_cached' or 'w_cached', and get rid of the two
        //        boolean variables as an indicator.
        //
        void * const    block_;

        bool            x_cached_state_;    // true if block_ is in dcode cache
        bool            w_cached_state_;    // true if block_ is in write page cache
        
        void operator=(const BlockData &);  // DO NOT ASSIGN
        BlockData(const BlockData &);       // DO NOT COPY
        BlockData();                        // PRIVATE DEFAULT CONSTRUCTOR
        
      public:
        const uint32    page_frame;         // page frame address
        const uint32    type_tag;           // tag storing MemoryTypeTag type defined in 'state.def'
        
        // -----------------------------------------------------------------------
        // Constructor creating a BlockData object backed by dynamically alloaced
        // RAM type memory.
        //
        explicit BlockData(uint32   page_frame,
                           uint32*  page_block);

        // -----------------------------------------------------------------------
        // Constructor creating a BlockData object backed by a MemoryDevice
        //
        explicit BlockData(uint32                              page_frame,
                           arcsim::mem::MemoryDeviceInterface* mem_dev);
        
        // Destructor
        //
        ~BlockData();
        
        // ---------------------------------------------------------------------
        // Query BlockData Type
        //
        inline bool is_mem_dev() const { return (type_tag == kMemoryTypeTagDev); }
        inline bool is_mem_ram() const { return (type_tag == kMemoryTypeTagRam); }
        
        // ---------------------------------------------------------------------
        // Query/Set cached flags
        //
        inline bool is_w_cached() const  { return w_cached_state_; }
        inline bool is_x_cached() const  { return x_cached_state_; }
        
        inline void set_w_cached(bool v) { w_cached_state_ = v; }
        inline void set_x_cached(bool v) { x_cached_state_ = v; }
                
        // ---------------------------------------------------------------------
        // BlockData 'block_' accessor methods
        //
        
        // Return reference to memory location
        //
        inline uint32& operator[](int offset) const {
          return (reinterpret_cast<uint32*>(block_))[offset];
        }
        
        // Return pointer to memory location
        //
        inline uint8 * const location (uint32 offset) const {
          return reinterpret_cast<uint8*>(block_) + offset;
        }
        
        inline arcsim::mem::MemoryDeviceInterface * const
        get_mem_dev() const
        {
          return reinterpret_cast<arcsim::mem::MemoryDeviceInterface*>(block_);
        }

        inline uint32 * const
        get_mem_ram() const
        {
          return reinterpret_cast<uint32*>(block_);
        }

      };
      
    } } } // sys::mem

#endif  // INC_MEMORY_BLOCKDATA_H_

