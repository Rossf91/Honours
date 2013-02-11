//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                             
// =====================================================================
//
// Description:
//
// This class represents a compilation unit (i.e. Module) containing a trace.
//
// =====================================================================

#ifndef _TranslationModule_h_
#define _TranslationModule_h_

#include <map>

#include "api/types.h"

#include "translate/Translation.h"

#include "concurrent/Mutex.h"

// -----------------------------------------------------------------------------
// Forward Declarations
//
class SimOptions;

namespace llvm {
  class Module;
}

namespace arcsim {
  namespace internal {
    namespace translate {
      class TranslationWorker;
    }
  }
  namespace profile {
    class BlockEntry;
  }
}

class TranslationWorkUnit;


// -----------------------------------------------------------------------------
//
// This class defines an object which holds information about a single
// TranslationModule that has been created by JIT compiler and contains a number
// of basic blocks of target code, all from the same page in target memory.
//
class TranslationModule
{
private:
  TranslationModule(const TranslationModule &);           // DO NOT COPY
  void operator=(const TranslationModule &);              // DO NOT ASSIGN
  
  const SimOptions&                   sim_opts_;

  // Module level mutex used for synchronisation. Use corresponding lock()/unlock()
  // methods before reading/modifying state via is_dirty()/is_translated()
  // or mark_as_dirty()/mark_as_translated() methods.
  //
  arcsim::concurrent::Mutex           module_mtx_;

  static const uint8 kDirtyMask      = 0x1;
  static const uint8 kTranslatedMask = 0x2;
  
  // Module state encodes if an 'in translation' module has been invalidated
  // before it was compiled by setting the first bit to 1. A successfully
  // translated module is indicated by setting the second bit to 1. Access to this
  // state variable must be synchronised using using the corresponding lock() and
  // unlock() methods.
  //
  uint8 module_state_;
    
  // Map of target basic-blocks that have been translated in this TranslationModule.
  //
  std::map<uint32,arcsim::profile::BlockEntry*>       block_map_;

  // Underlying compilation unit (e.g. pointer to shared library or llvm::Module)
  //
  void*                                               module_;
  
  // JIT compilation engine that created this module. Needed for machine code
  // garbage collection - memory for machine code must be allocated
  // and freed by the same JIT compilation engine as each engine has its own
  // machine code memory pool. For performance reasons an engine does not
  // synchronise its access to the memory pool, hence we must not asynchronously
  // modify that pool. Hence we only mark a module as ready for deletion so the
  // engine can safely remove it.
  //
  arcsim::internal::translate::TranslationWorker*    engine_;

  
  std::string     name_;       // unique module name  
	const uint32    key_;        // temporal identity of this module
	int             ref_count_;  // number of BlockEntries referencing module

public:
  
  // Constructor/Destructor
  //
  explicit TranslationModule (uint32 key, SimOptions& sim_opts);
  ~TranslationModule ();

  // ---------------------------------------------------------------------------
  // Initialise Module
  //
	bool          init (uint32 page_frame_addr);
  
  // Retrieve unique module name
  //
  const char*   get_id()      const  { return name_.c_str(); }
  
  // ---------------------------------------------------------------------------
  // Reference counting triggered by adding/removing BlockEntries
  //
  inline void  release()             { --ref_count_; }
  inline void  retain()              { ++ref_count_; }
  inline int   get_ref_count() const { return ref_count_; }

  // ---------------------------------------------------------------------------
  // Synchronisation
  //
  void lock()   { module_mtx_.acquire(); }
  void unlock() { module_mtx_.release(); }

  // ---------------------------------------------------------------------------
  // Module translation states - calls to these methods MUST be synchronised
  //
  inline bool is_dirty() const      { return module_state_ & kDirtyMask; }
  inline void mark_as_dirty()       { module_state_ |= kDirtyMask;       }
  
  inline bool is_translated() const { return module_state_ & kTranslatedMask; }
  inline void mark_as_translated()  { module_state_ |= kTranslatedMask;       }
  
  
  // ---------------------------------------------------------------------------
  // Accessors for LLVM modules/engines
  //
  void          set_worker_engine(arcsim::internal::translate::TranslationWorker* e) { engine_ = e; }
  
  void          set_llvm_module(llvm::Module* m)          { module_ = m; }
  llvm::Module* get_llvm_module() const { return reinterpret_cast<llvm::Module*>(module_); }
  
  // ---------------------------------------------------------------------------
  // Accessors for dynamilcally loaded modules
  //
  void          set_dyn_module(void* m) { module_ = m;    }
  void*         get_dyn_module()  const { return module_; }
  
  // ---------------------------------------------------------------------------
  // Load/Close shared library
  //
  bool           load_shared_library ();
  bool           close_shared_library();
  
  // Lookup TranslationBlock (i.e. function pointer) for symbol name in shared library
  //
  TranslationBlock   get_pointer_to_function(const char* symbol);
  
  // Adds BlockEntries to this module
  void          add_block_entry(arcsim::profile::BlockEntry& block,
                                TranslationBlock             native);
  // Removes all BlockEntries and returns count of erased blocks
  int           erase_block_entries();
  
};

#endif /* _TranslationModule_h_ */

