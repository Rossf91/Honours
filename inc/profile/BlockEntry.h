//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class representing a Basic Block entry.
//
// =====================================================================

#ifndef INC_PROFILE_BLOCKENTRY_H_
#define INC_PROFILE_BLOCKENTRY_H_


#include "api/types.h"

#include "Assertion.h"

#include "sim_types.h"

#include "translate/Translation.h"

#include "util/Zone.h"

class TranslationModule;

namespace arcsim {
  namespace profile {

// -----------------------------------------------------------------------------
// BlockEntry
//
class BlockEntry : public arcsim::util::ZoneObject
{
public:
  
  // TranslationStates a basic block can be in
  //
  enum TranslationState {
    kNotTranslated, /* not translated and not queued for translation */
    kInTranslation, /* queued for translation */
    kTranslated     /* translated and exists in native mode */
  };
  
	const uint32        phys_addr; /* location of block in physical address space    */
	const uint32        virt_addr; /* location of block in virtual address space     */
  
  // Note that the instruction count and size of a block is only set when tracing
	uint32 inst_count;          /* number of instructions in the block            */
	uint32 size_bytes;          /* length of block in bytes of code               */  
	uint32 interp_count;        /* total number of interpreted executions         */

	const OperatingMode mode;      /* 0 = kernel, 1 = user mode code, in this block  */

private:
  TranslationState   tstate_;   /* Translation state of this basic block */
  TranslationBlock   native_;   /* Native block registered for this BlockEntry */
  TranslationModule* module_;   /* Module this BlockEntry belongs to */
  
public:
  // Constructor
  explicit BlockEntry(uint32 phys_addr, uint32 virt_addr, OperatingMode mode);
      
  // ---------------------------------------------------------------------------
  // Translation state query methods
  //
  inline bool is_in_translation() const { return tstate_ == kInTranslation; }
  inline bool is_translated()     const { return tstate_ == kTranslated;    }
  inline bool is_not_translated() const { return tstate_ == kNotTranslated; }
    
  // ---------------------------------------------------------------------------
  // Translation state modify methods
  //
  inline void mark_as_in_translation() { tstate_ = kInTranslation; }
  inline void mark_as_not_translated() { tstate_ = kNotTranslated; }
  
  // ---------------------------------------------------------------------------
  // Remove native translation
  //
  void remove_translation ();
    
  // ---------------------------------------------------------------------------
  // Register translation for BlockEntry, by doing this the translation state is
  // set to 'kTranslated'
  //
  inline void             set_translation(TranslationBlock block) {
    ASSERT(block != 0);
    ASSERT(tstate_ == kInTranslation);
    native_   = block;
    tstate_   = kTranslated;
  }
  
  inline TranslationBlock get_translation() const { return native_;  }
  
  inline void               set_module(TranslationModule* mod)  { module_ = mod;   }
  inline TranslationModule* get_module()    const               { return module_;  }
  
};

} } // arcsim::profile

#endif /* INC_PROFILE_BLOCKENTRY_H_ */
