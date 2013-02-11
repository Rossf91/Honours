//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// PageProfile declarations.
//
// =====================================================================

#ifndef INC_PROFILE_PAGEPROFILE_H_
#define INC_PROFILE_PAGEPROFILE_H_

#include <map>
#include <list>

#include "api/types.h"

#include "profile/BlockEntry.h"

#include "util/Zone.h"

// ---------------------------------------------------------------------
// FORWARD DECLARATION
//
class TranslationModule;
class TranslationBlockUnit;
class TranslationWorkUnit;
class SimOptions;

namespace arcsim {
  // ---------------------------------------------------------------------
  // Forward decl
  //
  namespace sys {
    namespace cpu {
      class Processor;
    }
  }
  
  namespace profile {
    
    class PageProfile : public arcsim::util::ZoneObject
    {
    private:
      // -----------------------------------------------------------------------
      // Data structures for incremental tracing of dynamic CFG
      //

      // Each BlockProfile contains a map of BlockEntries keyed by addresses
      // NOTE: BlockEntry objects are dynamically allocated in processors Zone.
      //
      std::map<uint32,BlockEntry*>          block_map_;
      
      // number of times page has been touched
      uint32                                interp_count_;
      
      // Map of basic block nodes: std::map<BlockEntry.virt_addr,BlockEntry*>
      std::map<uint32,BlockEntry*>          nodes_;
      
      // Multimap of basic block edges encoding destination blocks for a given
      // basic block entry address: std::multimap<BlockEntry.virt_addr, BlockEntry*>
      std::multimap<uint32,BlockEntry*>     edges_;
      
      // An address that a block can never legally have
      static const uint32                   kInvalidBlockEntryAddress = 1;
      
      // Remember previous block entry for each interrupt state in order to be able
      // to add control flow edges for a new block.
      uint32                                prev_block_[NUM_INTERRUPT_STATES];
      
      // -----------------------------------------------------------------------
      // Natively translated traces
      //
      
      // Map of compiled traces (i.e. TranslationModules) for this PageProfile
      //
      std::map<sint32,TranslationModule*>   module_map_;
      uint32                                module_count_;
      
    public:
      // Page Proile frame address
      //
      const uint32                page_address;
      
      // Constructor/Destructor
      //
      explicit PageProfile (uint32 page_addr);
      ~PageProfile();
      
      // ---------------------------------------------------------------------------
      // Get reference to BlockEntry instance, create BlockEntry instance if it does
      // not exist
      //
      inline BlockEntry*
      get_block_entry(uint32 phys_addr, uint32 virt_addr, OperatingMode m, arcsim::util::Zone& zone)
      {
        if (block_map_.find(phys_addr) != block_map_.end())
          return block_map_[phys_addr]; // BlockEntry exists, return pointer to it
        
        // Allocation of BlockEntries using C++ placement new with custom Zone allocator
        BlockEntry * entry = new (&zone) BlockEntry(phys_addr, virt_addr, m);
        
        // Insert newly allocated BlockEntry object into block_map_
        block_map_.insert(std::pair<uint32,BlockEntry*>(phys_addr, entry));    
        
        return entry;
      }
      
      // --------------------------------------------------------------------------- 
      // Find and return BlockEntry pointer if it exists, otherwise return '0'.
      //
      inline BlockEntry*
      find_block_entry(uint32 addr) const
      {
        std::map<uint32,BlockEntry*>::const_iterator I = block_map_.find(addr);
        if (I != block_map_.end())
          return I->second;
        return 0;
      }
      
      // --------------------------------------------------------------------------- 
      // Determine whether a BlockEntry is present given an address
      //
      bool is_block_entry_present(uint32 addr) const;
      
      // --------------------------------------------------------------------------- 
      // Determine wheter a translation for a BlockEntry is present given an address
      //
      bool is_block_entry_translation_present(uint32 addr) const;

      // --------------------------------------------------------------------------- 
      // Clear intermediate trace data structures
      //
      inline void clear () {
        for (uint32 i = 0; i < NUM_INTERRUPT_STATES; ++i)
          prev_block_[i] = kInvalidBlockEntryAddress;
        interp_count_ = 0;
        nodes_.clear();
        edges_.clear();
      }
      
      // Create TranslationModule for this page
      TranslationModule* create_module (SimOptions& sim_opts);
            
      // Remove all translated blocks registered for this page profile
      int remove_translations();
      
      // This method is critical during tracing. It is called for each yet unseen
      // or untranslated BlockEntry on this page during a trace interval to record
      // the dynamic control flow.
      void trace_block (BlockEntry& block, InterruptState state, bool is_sequence_active);
      
      // Method that determines if this PageProfile has hotspots eligible for JIT translation.
      bool has_hotspots(CompilationMode mode, uint32 threshold) const;
      
      // Create TranslationWorkUnit class that can be passed off to TranslationEngine
      bool create_translation_work_unit(arcsim::sys::cpu::Processor& cpu,
                                        CompilationMode              mode,
                                        TranslationWorkUnit&         work_unit);
      
    private:
      // Create TranslationBlockUnit representing a basic block that is a part of a
      // TranslationWorkUnit.
      bool create_translation_block_unit(arcsim::sys::cpu::Processor& cpu,
                                         BlockEntry&                  block_entry,
                                         TranslationWorkUnit&         work_unit,
                                         TranslationBlockUnit&        block_unit);
      
      // Retrieve edges from a block entry
      void get_block_edges(BlockEntry&                 block,
                           std::list<BlockEntry*>&     dest_blocks) const;
      
    };

} } // arcsim::profile
    
#endif /* INC_PROFILE_PAGEPROFILE_H_ */
