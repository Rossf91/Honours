//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                             
// =====================================================================
//
// Description:
//
// Datastructures and methods for block profiling and analysis.
//
// =====================================================================

#include <iomanip>

#include <map>
#include <valarray>
#include <algorithm>

#include "Assertion.h"

#include "sys/cpu/processor.h"

#include "profile/PhysicalProfile.h"

#include "translate/TranslationModule.h"
#include "translate/TranslationWorkUnit.h"

#include "ioc/Context.h"

#include "util/Allocate.h"
#include "util/Log.h"


namespace arcsim {
  namespace profile {

    // ---------------------------------------------------------------------------
    // Constructor
    //    
    PhysicalProfile::PhysicalProfile (arcsim::ioc::Context& ctx,
                                      const char*           name)
    : ctx_(ctx),
      page_addr_shift_(0),
      page_frame_mask_(0),
      cache_size_(0),
      cache_(0)
    {
      uint32 i;
      for (i = 0; i < kPhysicalProfileMaxNameSize - 1 && name[i]; ++i)
        name_[i] = static_cast<uint8>(name[i]);
      name_[i] = '\0';
      
    }
    
  // ---------------------------------------------------------------------------
  // Destructor
  //
  PhysicalProfile::~PhysicalProfile ()
  { 
    if (cache_) { arcsim::util::Malloced::Delete(cache_); cache_ = 0; }
  }

  void
  PhysicalProfile::construct (uint32                       cache_size,
                              uint32                       page_addr_shift,
                              uint32                       page_frame_mask)
  {
    cache_size_       = cache_size;
    page_addr_shift_  = page_addr_shift;
    page_frame_mask_  = page_frame_mask;
    
    ASSERT(IS_POWER_OF_TWO(cache_size) && "PageProfileCache: Initial Capacity not power of two!");
    
    // Allocate cache
    cache_ = (Entry*)arcsim::util::Malloced::New(cache_size_ * sizeof(cache_[0]));

    // Initialise cache
    Entry* const end = cache_ + cache_size_;
    for (Entry* p = cache_; p < end; ++p) { p->tag_ = kInvalidTag; }
    
    // Initialise trace book-keeping structures
    for (uint32 i = 0; i < NUM_INTERRUPT_STATES; ++i) {
      prev_page_profile_[i] = 0;
    }
    trace_sequence_active_.set();
  }


// ---------------------------------------------------------------------------
// Determine and Analyse HotSpots
//

// Determine current hotspot threshold based on simple heuristic.
//
uint32
PhysicalProfile::determine_hotspot_threshold(arcsim::sys::cpu::Processor&  cpu) const
{
  uint32 hotspot_threshold = DEFAULT_HOTSPOT_THRESHOLD;
  
  if (cpu.system.trans_mgr.get_translation_work_queue_size() > DEFAULT_TRANSLATION_QUEUE_SIZE_THRESHOLD)
  {
    // Increase hotspot threshold according to heuristic only if queue size reaches
    // predefined threshold
    //  
    hotspot_threshold = (DEFAULT_HOTSPOT_THRESHOLD * DEFAULT_HOTSPOT_THRESHOLD_MULTIPLY);    
  }
  return hotspot_threshold;
}


// Analyse all hotspots from last trace interval 
//
int
PhysicalProfile::analyse_hotspots(arcsim::sys::cpu::Processor&         cpu,
                                  std::valarray<TranslationWorkUnit*>& work_units,
                                  CompilationMode                      mode,
                                  uint32                               threshold)
{
  int   work_size = 0;
  
  for (std::map<uint32,PageProfile*>::iterator
        I = touched_pages.begin(),
        E = touched_pages.end();
       I != E; ++I)
  {
    PageProfile * const pp = I->second;
    
    if (pp->has_hotspots(mode, threshold)) {
      TranslationWorkUnit* t = new TranslationWorkUnit(&cpu, cpu.trace_interval);
      
      // populate TranslationWorkUnit with translations
      if (pp->create_translation_work_unit(cpu, mode, *t)) {
        
        // Create TranslationModule for TranslationWorkUnit
        t->module = pp->create_module(cpu.sim_opts);
        
        if (t->module != 0) {               // SUCCESS
          // Mark all blocks in module as 'in translation' and re-set interpretation count
          for (std::list<TranslationBlockUnit*>::const_iterator
               I = t->blocks.begin(), E = t->blocks.end(); I != E; ++I) {
            (*I)->entry_.mark_as_in_translation();
            (*I)->entry_.interp_count = 0;
          }
          work_units[work_size++] = t; // store TranslationWorkUnit pointer
        }
      }
      
      if (t->module == 0) {
        LOG(LOG_DEBUG) << "[PhysicalProfile] TranslationWorkUnit construction failed - freeing resources.";
        delete t;
      }
    }
  }
  return work_size;
}

// ---------------------------------------------------------------------------
// Remove/Query Translations
//

// Returns true if a translation exists for a given physical address
//
bool
PhysicalProfile::is_translation_present (uint32 addr) const
{
  if (PageProfile *p = find_page_profile(addr)) {
    return p->is_block_entry_translation_present(addr);
  }
  return  false;  
}


// Remove all translated blocks that contain the given address
//
int
PhysicalProfile::remove_translation (uint32 addr)
{  
  if (PageProfile* p = find_page_profile(addr)) {
    if (p->is_block_entry_translation_present(addr)) {
      return p->remove_translations();
    }
  }
  return 0;
}

// Remove ALL translations from the simulation
//
int
PhysicalProfile::remove_translations ()
{
  LOG(LOG_DEBUG) << "[PhysicalProfile] removing ALL translations.";
  int removed = 0;
	for (std::map<uint32,PageProfile*>::const_iterator
        I = page_map.begin(), E = page_map.end(); I != E; ++I)
  {  
    removed += I->second->remove_translations();
	}
	return removed;
}

// ---------------------------------------------------------------------------
// Create/Remove/Query Traces
//

// This method is critical during tracing, it is called for each yet unseen and
// untranslated block during a trace interval.
//
void
PhysicalProfile::trace_block(BlockEntry&     block, InterruptState  irq_state)
{
  const uint32 page_frame = block.phys_addr & page_frame_mask_;
  
  // Find the PageProfile for which we want to record the block
  //
  PageProfile* p = 0;
  
  std::map<uint32,PageProfile*>::const_iterator I = touched_pages.find(page_frame);
  
  if (I != touched_pages.end()) {
    // PageProfile has already been touched, return pointer to it
    p = I->second;
  } else {
    // PageProfile has not been touched yet, hence we need to go look for it
		std::map<uint32,PageProfile*>::const_iterator page_map_I = page_map.find (page_frame);
		
		// the following should ALWAYS succeed
		if (page_map_I != page_map.end()) { 
      // Create new entry touched_pages map
      touched_pages.insert(std::pair<uint32,PageProfile*>(page_frame, page_map_I->second));      
      p = page_map_I->second;
		}
  }
  
  ASSERT(p != 0 && "Failed to find PageProfile during tracing.");
    
  // If the previous page we added traces to is not the same as this one, we
  // need to start a new trace sequence for this page by reseting the currently
  // active trace sequence
  //
  if (!is_equal_previous_page_profile(irq_state, p))
    reset_active_trace_sequence(irq_state);
  
  // Add block to PageProfile trace
  p->trace_block (block, irq_state, is_trace_sequence_active(irq_state));
  
  // For the next block this PageProfile will be the previous page profile
  //
  set_previous_page_profile(irq_state, p);
  // We also set the trace sequence as active
  //
  set_active_trace_sequence(irq_state);
}

bool
PhysicalProfile::is_trace_present (uint32 addr) const
{
  const uint32 page_frame = addr & page_frame_mask_;
  
  if (touched_pages.find(page_frame) != touched_pages.end())
    return true;
  
  return false;
}


bool
PhysicalProfile::remove_trace (uint32 addr)
{
	const uint32 page_frame = addr & page_frame_mask_;

  std::map<uint32,PageProfile*>::iterator I = touched_pages.find(page_frame);
  
  if (I != touched_pages.end()) {
    I->second->clear();     // clear collected traces for page
    touched_pages.erase(I); // remove page from map
    
    // Removing one page interrupts the current trace sequence
    // FIXME: here we interrupt ALL trace sequences.
    reset_all_active_trace_sequences();
    
    return true;
  }
  
  return false;
}

void
PhysicalProfile::remove_traces()
{
  while (!touched_pages.empty()) {
    touched_pages.begin()->second->clear(); // clear trace nodes within page
    touched_pages.erase(touched_pages.begin());
  }
  // removing all traced blocks effectively interrupts ALL trace sequences
  reset_all_active_trace_sequences();
}

} } // arcsim::profile
