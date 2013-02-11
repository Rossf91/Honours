//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
// Class representing a TranslationUnit plus types and constants used for
// dealing with translations.
//
// =====================================================================


#ifndef INC_TRANSLATE_TRANSLATIONWORKUNIT_H
#define INC_TRANSLATE_TRANSLATIONWORKUNIT_H


#include <map>
#include <list>

#include "isa/arc/Dcode.h"

namespace arcsim {
  namespace sys {
    namespace cpu {
        class Processor;
  } }
  
  namespace profile {
    class BlockEntry;
  }
}

class TranslationModule;

// Wrapper struct encapsulating instruction for native compilation
struct TranslationInstructionUnit
{
  arcsim::isa::arc::Dcode     inst;
};

class TranslationBlockUnit
{
private:
  // HEAP allocated instructions. delete happens in destructor
  std::list<TranslationInstructionUnit*>    inst_list_;

public:
  explicit TranslationBlockUnit(arcsim::profile::BlockEntry* entry);
  ~TranslationBlockUnit();
  
  // Add/Query instructions
  //
  void add_instruction(TranslationInstructionUnit* inst)    { inst_list_.push_back(inst); }
  TranslationInstructionUnit* get_first_instruction() const { return inst_list_.front(); }
  TranslationInstructionUnit* get_last_instruction()  const { return inst_list_.back();  }
  inline size_t               get_instruction_count() const { return inst_list_.size(); }
  
  // Instruction iterator
  //
  inline std::list<TranslationInstructionUnit*>::const_iterator
  begin() const {
    return inst_list_.begin();
  }

  inline std::list<TranslationInstructionUnit*>::const_iterator
  end() const {
    return inst_list_.end();
  }
  
  arcsim::profile::BlockEntry&              entry_;
  std::list<arcsim::profile::BlockEntry*>   edges_;
};

class TranslationWorkUnit
{
  friend struct PrioritizeTranslationWorkUnits;
public:
  explicit TranslationWorkUnit(arcsim::sys::cpu::Processor* cpu, uint32 timestamp);
  ~TranslationWorkUnit();  
  
  arcsim::sys::cpu::Processor* const cpu; // processor this TranslationWorkUnit belongs to  
  std::map<uint32,uint32>            lp_end_to_lp_start_map; // ZOL LP_END to LP_START mapping

  TranslationModule*                 module; // TranslationModule for this TranslationWorkUnit
  std::list<TranslationBlockUnit*>   blocks; // basic blocks contained in this TranslationWorkUnit
  
  uint32                             exec_freq; // cumulative basic block interpretation frequ.
  uint32                             timestamp; // creation timestamp (i.e. interval num.)   
  
};

// Friend class with function object for comparison of TranslationWorkUnits
//
struct PrioritizeTranslationWorkUnits
{  
  // Dynamic work scheduling policy based on recency and frequency
  //
  bool operator()(TranslationWorkUnit const * x,
                  TranslationWorkUnit const * y )
  {
    if (x->timestamp == y->timestamp) {
      return x->exec_freq < y->exec_freq;
    } else {
      return x->timestamp < y->timestamp;
    }
  }  
};

#endif /* _TranslationWorkUnit_h_ */
