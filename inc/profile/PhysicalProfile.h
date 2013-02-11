//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//
// Author: npt
// 
// Revision history:
//                                             
// =====================================================================
//
// Description:
//
//  This file defines the classes used to implement run-time profiling
//  of the simulated object code. Profiling is implemented by keeping
//  instruction counts for each function and for each basic block.
//
// =====================================================================

#ifndef INC_PROFILE_PHYSICALPROFILE_H_
#define INC_PROFILE_PHYSICALPROFILE_H_

#include <map>
#include <valarray>
#include <bitset>

#include "api/types.h"

#include "profile/PageProfile.h"
#include "ioc/ContextItemInterface.h"
#include "util/Zone.h"

namespace arcsim {
  
  // ----------------------------------------------------------------------
  // Forward declarations
  //
  namespace ioc {
    class Context;
  }
  namespace sys {
    namespace cpu {
      class Processor;
  } }
  namespace util {
    class Zone;
  }
  
  namespace profile {
    
    // -----------------------------------------------------------------------
    // CLASS
    //
    class PhysicalProfile : public arcsim::ioc::ContextItemInterface
    {
    public:
      // Maximum name lenght
      //
      static const int kPhysicalProfileMaxNameSize = 256;

      // Constructor/Destructor
      //
      explicit PhysicalProfile(arcsim::ioc::Context& ctx, const char* name);
      ~PhysicalProfile ();
      
      const uint8* get_name() const { return name_; };
      const Type   get_type() const { return arcsim::ioc::ContextItemInterface::kTPhysicalProfile;  };
      
      // ---------------------------------------------------------------------------
      // Properly construct PhysicalProfile object
      //
      void construct (uint32   cache_size,
                      uint32   page_addr_shift,
                      uint32   page_frame_mask);
      
      // ---------------------------------------------------------------------------
      // Get the PageProfile for the page containing the given physical address.
      // Will create a new PageProfile in the page_map mapping if this
      // is the first occasion in which the page has been searched for.
      //
      inline PageProfile*
      get_page_profile (uint32 addr, arcsim::util::Zone& zone)
      {        
        const uint32 frame = addr & page_frame_mask_;
        Entry* entry = cache_ + ((addr >> page_addr_shift_) & (cache_size_ - 1));
        
        if (entry->tag_ == frame) {                // HIT into PageProfile cache
          return entry->page_profile_;
        } else {                                   // MISS into PageProfile cache
          PageProfile* profile = 0;
          std::map<uint32,PageProfile*>::const_iterator I = page_map.find(frame);
          if (I != page_map.end()) { // PageProfile has already been seen before
            profile = I->second;
          } else {                   // PageProfile seen for the first time
            profile = new (&zone) PageProfile (frame);
            page_map.insert(std::pair<uint32,PageProfile*>(frame, profile));
          }
          // Update the page cache
          entry->tag_          = frame;
          entry->page_profile_ = profile;
          return profile;
        }
      }
      
      // ---------------------------------------------------------------------------
      // The 'find_page_profile' method searches for the page profile containing
      // the given address. It returns a pointer to the enclosing page profile if
      // it exists, or '0' if it does not exist.
      //
      inline PageProfile*
      find_page_profile (uint32 addr) const
      {
        const uint32  frame = addr & page_frame_mask_;
        Entry * const entry = cache_ + ((addr >> page_addr_shift_) & (cache_size_ - 1));
        
        if (entry->tag_ == frame)
        { // HIT into PageProfile cache
          return entry->page_profile_;          
        } else {
          // Search for PageProfile in PageProfile map
          std::map<uint32,PageProfile*>::const_iterator I = page_map.find(frame);        
          if (I != page_map.end())
            return I->second;
        }
        return 0; // PageProfile not found
      }

      // ---------------------------------------------------------------------------
      // The 'get_block_entry' method searches for a BlockEntry starting at
      // the given address. If it does not exist it will create it and return
      // a pointer to it.
      //
      inline BlockEntry*
      get_block_entry(uint32                phys_addr,
                      uint32                virt_addr,
                      OperatingMode         mode,
                      arcsim::util::Zone&   zone)
      
      { // get PageProfile, creates one if it does not exist
        PageProfile*  p = get_page_profile(phys_addr, zone);
        // get BlockEntry, creates one if it does not exist
        return        p->get_block_entry(phys_addr, virt_addr, mode, zone);
      }
      
      // ---------------------------------------------------------------------------
      // The 'find_block_entry' method searches for a BlockEntry starting at
      // the given address. It returns a pointer to the BockEntry if it exists,
      // or '0' if it does not exist.
      //
      inline BlockEntry*
      find_block_entry(uint32 addr) const
      {
        PageProfile * const pp = find_page_profile(addr);
        if (pp)
          return pp->find_block_entry(addr);
        return 0; // BlockEntry not found
      }
      
      // ---------------------------------------------------------------------------
      // Query/Remove/Touch blocks
      //
      
      // Trace currently encountered basic block
      //
      void trace_block(BlockEntry& block, InterruptState irq_state);
      
      // Query if any pages have been touched and how many have been touched
      //
      bool    has_touched_pages()   const { return !touched_pages.empty(); }
      size_t  touched_pages_count() const { return touched_pages.size();   }
      
      // Returns true if a trace exists for a given physical address
      //
      bool is_trace_present (uint32 addr) const;
      
      // Remove trace that contains a given address, returns true if something has been removed
      //
      bool remove_trace(uint32 addr);
      
      // Remove all traces
      //
      void remove_traces();
      
      // ---------------------------------------------------------------------------
      // Remove/Query Translations
      //
      
      // Returns true if a translation exists for a given physical address
      //
      bool is_translation_present (uint32 addr) const;
      
      // Remove all translated blocks that contain the given address
      //
      int remove_translation (uint32 addr);
      
      // Remove absolutely all translations from the simulation
      //
      int remove_translations();
      
      // ---------------------------------------------------------------------------
      // Determine and Analyse HotSpots
      //
      
      // Determine current hotspot threshold based on heuristic.
      //
      uint32 determine_hotspot_threshold(arcsim::sys::cpu::Processor&  cpu) const;
      
      // Identify 'hotspots' accumulated during last trace interval
      //
      int analyse_hotspots(arcsim::sys::cpu::Processor&         cpu,
                           std::valarray<TranslationWorkUnit*>& work_units,
                           CompilationMode                      mode,
                           uint32                               threshold);
      
      // ---------------------------------------------------------------------------
      // The following methods help to update and query state variables used for
      // tracing block sequences for different interrupt and exception states.
      //
      
      inline bool is_trace_sequence_active(InterruptState s) const {
        return trace_sequence_active_[s];
      }
      inline void set_active_trace_sequence(InterruptState s) {
        trace_sequence_active_.set(s);
      }
      inline void reset_active_trace_sequence(InterruptState s) {
        trace_sequence_active_.reset(s);
      }
      inline void reset_all_active_trace_sequences() {
        trace_sequence_active_.reset();
      }
      
      inline void set_previous_page_profile(InterruptState s, PageProfile* pp) {
        prev_page_profile_[s] = pp;
      }
      
      inline bool is_equal_previous_page_profile(InterruptState s, PageProfile* pp) const {
        return (prev_page_profile_[s] == pp);
      }
      
      
    private:
      uint8  name_[kPhysicalProfileMaxNameSize];
      
      // Traced basic blocks (i.e. BlockEntries) added in succession to the same
      // PageProfile belong to a trace sequence. A new trace sequence is started
      // when one of the following occurs:
      //
      //  1. Jumping to a different PageProfile
      //  2. Occurence of an interrupt or exception
      //
      // The following two book keeping variables are needed to discover whether
      // a trace sequence is active or not.
      //
      PageProfile*                      prev_page_profile_[NUM_INTERRUPT_STATES];
      std::bitset<NUM_INTERRUPT_STATES> trace_sequence_active_;
      
      // Hash-based cache of recently-accessed physical pages to speed up search
      // for a physical page. Cache lookup is O(1) (i.e. cache hit is O(1)).
      // Upon a cache miss we consult std::map<page_frame,PageProfile> which
      // has a worst case search time of O(lg n) (note std::map<> uses balanced
      // binary tree - Red-Black-Trees - as its backing data-structure) where
      // n is the number of physical pages.
      //
      static const uint32 kInvalidTag = 0x1;
      struct Entry {
        uint32        tag_;
        PageProfile*  page_profile_;
      };
      
      Entry*          cache_;
      uint32          cache_size_;      // must be a power of 2
      
      uint32          page_addr_shift_;
      uint32          page_frame_mask_;
      
      // Map of all PageProfile objects. These represent all pages of physical
      // memory that have been touched by instruction fetch. Some of the pages
      // will contain translated code blocks.
      // NOTE: PageProfiles are dynamically allocated in processors Zone.
      //
      std::map<uint32,PageProfile*>   page_map;
      
      // Map containg only those PageProfile objects corresponding to
      // physical pages of memory to which an interpretive instruction
      // fetch has been performed. Such pages are the only ones we 
      // need to search at the end of a trace interval when deciding which
      // blocks to translate. Each map entry is a pointer to the 
      // corresponding block in page_map. This is to avoid expensive
      // copying of objects.
      //
      std::map<uint32,PageProfile*>  touched_pages;
      
      // Enclosing context
      //
      arcsim::ioc::Context&   ctx_;
      
      PhysicalProfile(const PhysicalProfile & m);   // DO NOT COPY
      void operator=(const PhysicalProfile &);      // DO NOT ASSIGN

    };

} } // arcsim::profile

#endif  // INC_PROFILE_PHYSICALPROFILE_H_

