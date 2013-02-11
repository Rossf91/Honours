//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// 
// =====================================================================

#include "Assertion.h"

#include "sys/cpu/PageCache.h"

#include "sys/cpu/state.h"

#include "arch/PageArch.h"

#include "util/Allocate.h"

namespace arcsim {
  namespace sys {
    namespace cpu {

      // -----------------------------------------------------------------------
      // CONSTANTS
      //
      static uint32* const kInvalidPage = 0;
      
      static const uint32  kInvalidTag  = 0xffff;
      
      // Constructor
      //
      PageCache::PageCache() : state_(0) 
      { /* EMPTY */ }
      
      // Destructor
      //
      PageCache::~PageCache()
      {  /* EMPTY */  }

      
      
      // -----------------------------------------------------------------------
      // Init
      //
      void
      PageCache::init(cpuState* state, PageArch* page_arch)
      {
        state_      = state;
        page_arch_  = page_arch;
                
        const size_t size = page_arch_->page_cache_size;
        ASSERT(IS_POWER_OF_TWO(size) && "PageCache: Initial Capacity not power of two!");
        
        // Allocate caches
        //
        state_->cache_page_read_ = (EntryPageCache_*)arcsim::util::Malloced::New(size * sizeof(state_->cache_page_read_[0]));
        state_->cache_page_write_= (EntryPageCache_*)arcsim::util::Malloced::New(size * sizeof(state_->cache_page_write_[0]));
        state_->cache_page_exec_ = (EntryPageCache_*)arcsim::util::Malloced::New(size * sizeof(state_->cache_page_exec_[0]));

        // Initialise page cache structures
        //
        flush(PageCache::ALL);
      }

      // -----------------------------------------------------------------------
      // Init
      //
      void
      PageCache::destroy()
      {
        if (state_->cache_page_read_ != 0) {
          arcsim::util::Malloced::Delete(state_->cache_page_read_);
        }
        if (state_->cache_page_write_ != 0) {
          arcsim::util::Malloced::Delete(state_->cache_page_write_);
        }
        if (state_->cache_page_exec_ != 0) {
          arcsim::util::Malloced::Delete(state_->cache_page_exec_);
        }
      }

      // -----------------------------------------------------------------------
      // Flush 
      //
      void
      PageCache::flush(Kind kind)
      {
        switch (kind) {
          case ALL: {
            const EntryPageCache_ * end = state_->cache_page_read_ + page_arch_->page_cache_size;
            for (EntryPageCache_ * p_read  = state_->cache_page_read_,
                                 * p_write = state_->cache_page_write_,
                                 * p_exec  = state_->cache_page_exec_;
                 p_read < end;
                 ++p_read, ++p_write, ++p_exec)
            {
              p_exec->addr_  = p_write->addr_  = p_read->addr_  = kInvalidTag;
              p_exec->block_ = p_write->block_ = p_read->block_ = kInvalidPage;
            }
            return;
          }
          case READ: {
            const EntryPageCache_ * end = state_->cache_page_read_ + page_arch_->page_cache_size;
            for (EntryPageCache_ * p  = state_->cache_page_read_; p < end; ++p)
            {
              p->addr_  = kInvalidTag;
              p->block_ = kInvalidPage;
            }
            return;
          }
          case WRITE: {
            const EntryPageCache_ * end = state_->cache_page_write_ + page_arch_->page_cache_size;
            for (EntryPageCache_ * p  = state_->cache_page_write_; p < end; ++p)
            {
              p->addr_  = kInvalidTag;
              p->block_ = kInvalidPage;
            }
            return;
          }
          case EXEC: {
            const EntryPageCache_ * end = state_->cache_page_exec_ + page_arch_->page_cache_size;
            for (EntryPageCache_ * p  = state_->cache_page_exec_; p < end; ++p)
            {
              p->addr_  = kInvalidTag;
              p->block_ = kInvalidPage;
            }            
            return;
          }
        }
      }
      
      // -----------------------------------------------------------------------------
      // Purge
      //
      uint32
      PageCache::purge(Kind kind, uint32 addr)
      {
        uint32 removed = 0;
        const uint32 idx = page_arch_->page_byte_index(addr);
        const uint32 tag = page_arch_->page_byte_tag  (addr);
        switch (kind)
        {
          case ALL: {
            EntryPageCache_ * p = state_->cache_page_read_ + idx;
            if (p->addr_ == tag) {
              p->addr_ = kInvalidTag;
              p->block_= kInvalidPage;
              ++removed;
            }
            p = state_->cache_page_write_ + idx;
            if (p->addr_ == tag) {
              p->addr_ = kInvalidTag;
              p->block_= kInvalidPage;
              ++removed;
            }
            p = state_->cache_page_exec_ + idx;
            if (p->addr_ == tag) {
              p->addr_ = kInvalidTag;
              p->block_= kInvalidPage;
              ++removed;
            }
            break;
           }
            
          case READ: {
            EntryPageCache_ * p = state_->cache_page_read_ + idx;
            if (p->addr_ == tag) {
              p->addr_ = kInvalidTag;
              p->block_= kInvalidPage;
              ++removed;
            }
            break;
          }
            
          case WRITE: {
            EntryPageCache_ * p = state_->cache_page_write_ + idx;
            if (p->addr_ == tag) {
              p->addr_ = kInvalidTag;
              p->block_= kInvalidPage;
              ++removed;
            }
            break;
          }
            
          case EXEC: {
            EntryPageCache_ * p = state_->cache_page_exec_ + idx;
            if (p->addr_ == tag) {
              p->addr_ = kInvalidTag;
              p->block_= kInvalidPage;
              ++removed;
            }
            break;
          }
        }
        return removed;
      }

      
      uint32
      PageCache::purge_entry(Kind kind, uint32 const * const entry)
      {
        uint32 removed = 0;
        switch (kind)
        {
          case ALL: {
            EntryPageCache_ * p;
            for (int idx = 0; idx < page_arch_->page_cache_size; ++idx) {
              p = state_->cache_page_read_ + idx;
              if (p->block_ == entry) { p->addr_ = kInvalidTag; ++removed; }
              
              p = state_->cache_page_write_ + idx;
              if (p->block_ == entry) { p->addr_ = kInvalidTag; ++removed; }
              
              p = state_->cache_page_exec_ + idx;
              if (p->block_ == entry) { p->addr_ = kInvalidTag; ++removed; }
            }
          }
            
          case READ: {
            EntryPageCache_ * p;
            for (int idx = 0; idx < page_arch_->page_cache_size; ++idx) {
              p = state_->cache_page_read_ + idx;
              if (p->block_ == entry) { p->addr_ = kInvalidTag; ++removed; }
            }
          }
            
          case WRITE: {
            EntryPageCache_ * p;
            for (int idx = 0; idx < page_arch_->page_cache_size; ++idx) {
              p = state_->cache_page_write_ + idx;
              if (p->block_ == entry) { p->addr_ = kInvalidTag; ++removed; }
            }
          }
            
          case EXEC: {
            EntryPageCache_ * p;
            for (int idx = 0; idx < page_arch_->page_cache_size; ++idx) {
              p = state_->cache_page_exec_ + idx;
              if (p->block_ == entry) { p->addr_ = kInvalidTag; ++removed; }
            }
          }
        }
        return removed;
      }

      
} } } //  arcsim::sys::cpu
