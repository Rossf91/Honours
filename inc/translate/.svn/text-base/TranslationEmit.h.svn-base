//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// TranslationEmit is responsible for generating, formatting, and outputting
// JIT code.
//
// =====================================================================

#ifndef _TranslationEmit_h_
#define _TranslationEmit_h_

#include <string>

#include "translate/Translation.h"

#include "arch/Configuration.h"

// -----------------------------------------------------------------------------
// Forward declaration
//
class MemoryModel;
class ProcessorPipelineInterface;
class SimOptions;
class IsaOptions;
namespace arcsim {
  namespace sys {
    namespace mem {
      class Memory;
    }
    namespace cpu {
      class CounterManager;
    }
  }
  namespace util {
    class CodeBuffer;
  }
}

class TranslationEmit
{
private:
  // Constructor is private to avoid instantiation 
  //
  TranslationEmit()  { /* EMPTY */ }
  
  TranslationEmit(const TranslationEmit & m);   // DO NOT COPY
  void operator=(const TranslationEmit &);      // DO NOT ASSIGN

public:  
  
  // Get translation function label
  //
  static void block_label(arcsim::util::CodeBuffer& buf, uint32 addr);
  
  // Get translation function signature
  //
  static void block_signature(arcsim::util::CodeBuffer& buf, uint32 addr);
  
  // Header code for translation unit
  //
  static bool emit(arcsim::util::CodeBuffer&                  buf,
                   const SimOptions&                          sim_opts,
                   const IsaOptions&                          isa_opts,
                   CoreArch&                                  arch,
                   arcsim::sys::mem::Memory&                  mem,
                   MemoryModel*                               mem_model,
                   ProcessorPipelineInterface*                pipeline,
                   arcsim::sys::cpu::CounterManager&          cnt_ctx);
  
};


#endif /* _TranslationEmit_h_ */
