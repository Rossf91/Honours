//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// TranslationWorker responsible for generating JIT compiler native code. 
//
// =====================================================================

#ifndef INC_TRANSLATE_TRANSLATIONWORKER_H_
#define INC_TRANSLATE_TRANSLATIONWORKER_H_

#include <queue>
#include <set>

#include "arch/Configuration.h"

#include "concurrent/Thread.h"

#include "translate/TranslationWorkUnit.h"
#include "translate/TranslationManager.h"
#include "translate/TranslationOptManager.h"

#include "concurrent/Mutex.h"

// =====================================================================
// FORWARD DECLARATIONS
// =====================================================================

class TranslationModule;

namespace llvm {
  class Module;
  class LLVMContext;
  class ExecutionEngine;
  class PassManager;
}

namespace clang {
  class CompilerInstance;
}

namespace arcsim {
  
  namespace util {
    class CodeBuffer;
  }
  
  namespace internal {
    namespace translate {

    // =====================================================================
    // TYPES
    // =====================================================================

    typedef enum {
      TW_WORK_STATE_BUSY,
      TW_WORK_STATE_WAITING
    } TranslationWorkerWorkState;

    typedef enum {
      TW_STATE_RUN,
      TW_STATE_RUNNING,
      TW_STATE_STOP,
      TW_STATE_STOPPED
    } TranslationWorkerRunState;


    // =====================================================================
    // CLASS
    // =====================================================================

    class TranslationWorker : public arcsim::concurrent::Thread {
    public:
      // Translation worker state - mainly used to properly shut down worker threads,
      // even if they are in the middle of doing something useful.
      //
      TranslationWorkerRunState   run_state;
      
      // Transtlation worker compilation state - mainly used to in keep mode when we
      // need to determine when a worker finished JIT compiling (i.e. is in state
      // TW_WORK_STATE_WAITING)
      //
      TranslationWorkerWorkState  work_state;
        
      explicit TranslationWorker(uint32 _id, TranslationManager& m, SimOptions& _sim_opts);
      
      ~TranslationWorker();
      
      // Mark module for garbage collection
      //
      void mark_module_for_gc(void* m);
      
    private:
      TranslationWorker(const TranslationWorker &);   // DO NOT COPY
      void operator=(const TranslationWorker &);      // DO NOT ASSIGN

      uint32                  worker_id;              // TranslationWorker ID
      
      // Reference to TranslationManager
      //
      TranslationManager&     mgr;
      
      // Reference to simulation options
      //
      SimOptions&             sim_opts;
      
      const bool              use_llvm_jit;   // should new LLVM JIT be used
      bool                    verbose_mode;   // output status messages
      bool                    keep_mode;      // keep translations around in files
      bool                    reuse_mode;     // reuse existing translations
      bool                    debug_mode;     // enable debugging mode so JIT generated code
                                              // can be debugged

      // Clang/LLVM JIT compiler components
      //
      clang::CompilerInstance*        CI_;   // clang pre thread compiler instance
      llvm::LLVMContext*              CTX_;  // llvm per thread context
      llvm::Module*                   MOD_;  // llvm per thread main module
      llvm::ExecutionEngine*          ENG_;  // llvm per thread execution engine
      
      arcsim::util::CodeBuffer*       code_buf_;    // C-code generation buffer
      TranslationOptManager           opt_manager;  // custom optimisation manager
      
      // Release pool for garbage collection of TranslationWorkUnits
      //
      std::queue<TranslationWorkUnit*>  twu_release_pool_;
      
      // Release pool for garbage collection of generated code
      //
      arcsim::concurrent::Mutex         mutx_mod_release_pool_;
      std::set<void*>                   mod_release_pool_;

      // ------------------------------------------------------------------------
      // Implementation of run() method 
      //
      void run();
  
      // Add work unit to release pool
      //
      void mark_translation_work_unit_for_gc(TranslationWorkUnit* _wu);

      // Free TranslationWorkUnit release pool
      //
      void sweep_translation_work_unit_release_pool();
      
      // Free machine code release pool
      //
      void sweep_module_release_pool();
      
      // ------------------------------------------------------------------------
      // JIT translation related methods
      // ------------------------------------------------------------------------
      // Compilation pipeline:
      //  1. First TranslationWorkUnits which act as an IR representing profiled
      //     instructions are translated to C (@see TranslationWorker::translate_work_unit_to_c()).
      //  2. Next one can choose to compile the generated C code to a TranslationModule
      //     using the following compilers:
      //      2.1. CLANG - TranslationWorker::compile_module_llvm()
      //      2.2. EXTERNAL COMPILER using popen()  - TranslationWorker::compile_module_popen()
      //      2.3. EXTERNAL COMPILER using system() - TranslationWorker::compile_module_system()
      //  3. Finally the translated module is loaded, and JIT compiled blocks are
      //     resolved and registered.
      //
      // NOTE: The methods TranslationWorker::translate_module() and TranslationWorker::load_module()
      //     act as high-level methods that take care of translation and loading hot blocks
      //     depending on given options.
      //
      
      // Translated TranslationWorkUnit -> TranslationModule
      //
      bool translate_module(const TranslationWorkUnit& w);
      
      // Load TranslationModule and register translated blocks
      //
      bool load_module(const TranslationWorkUnit& w);
      
      // ------------------------------------------------------------------------
      
      // Create C-Module for TranslationWork unit
      // 
      bool translate_work_unit_to_c(const TranslationWorkUnit& w);
      
      // Configuration of passes to be used with LLVM internal JIT
      //
      void configure_optimisation_passes(llvm::PassManager* PM, int optLevel);
      
      // JIT compile using Clang/LLVM
      //
      bool compile_module_llvm(TranslationModule& m);
      
      // Compile by piping C source to external compiler using popen(3)
      //
      bool compile_module_popen(const TranslationModule& m);
      
      // Compile by writing C source for file and executing external compiler using system(3)
      //
      bool compile_module_system(const TranslationModule& m);
      
      
    };

} } } // namespace arcsim::internal::translate

#endif /* INC_TRANSLATE_TRANSLATIONWORKER_H_ */
