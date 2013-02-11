//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Class implementing a TranslationUnit plus types and constants used for
// dealing with translations.
//
// =====================================================================

#include <list>

#include "profile/BlockEntry.h"

#include "translate/TranslationWorkUnit.h"

// Explicit TranslationBlockUnit constructor
//
TranslationBlockUnit::TranslationBlockUnit(arcsim::profile::BlockEntry*  entry)
  : entry_(*entry)
{ /* EMPTY */ }

// Default TranslationBlockUnit destructor
//
TranslationBlockUnit::~TranslationBlockUnit()
{ 
  while (!inst_list_.empty()) {
    delete *inst_list_.begin();
    inst_list_.erase(inst_list_.begin());
  }
}

// Explicit TranslationWorkUnit constructor
//
TranslationWorkUnit::TranslationWorkUnit(arcsim::sys::cpu::Processor* _cpu,
                                         uint32                       _timestamp)
  : cpu(_cpu),
    timestamp(_timestamp),
    exec_freq(0),
    module(0)
{ /* EMPTY */ }

// Default TranslationWorkUnit destructor
//
TranslationWorkUnit::~TranslationWorkUnit()
{ 
  while (!blocks.empty()) {
    delete *blocks.begin();
    blocks.erase(blocks.begin());
  }
}


