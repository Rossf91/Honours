//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
// This file implements dynamic translation methods.
//
// FIXME:
//  1. We need a communication channel back to the simulation loop to signal JIT
//     compilation errors!
//
// =====================================================================

#include <dlfcn.h>
#include <limits.h>
#include <unistd.h>
#include <signal.h>

#include <iomanip>
#include <string>
#include <list>
#include <stack>

// -----------------------------------------------------------------------------
// Clang/LLVM specific includes
//

#include "llvm/LLVMContext.h"

#if (LLVM_VERSION < 30)
#include "llvm/Target/TargetSelect.h"
#else
#include "llvm/Support/TargetSelect.h"
#endif

#include "llvm/Function.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/JIT.h"

#include "llvm/Target/TargetData.h"
#include "llvm/PassManager.h"

#include "llvm/Module.h"
#include "llvm/Support/MemoryBuffer.h"

#include "llvm/ADT/StringRef.h"

#include "clang/Basic/TargetInfo.h"

#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/TextDiagnosticBuffer.h"

#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"

#include "clang/CodeGen/ModuleBuilder.h"

#include "llvm/Support/CommandLine.h"

// -----------------------------------------------------------------------------
// ArcSim includes
//

// llvm/Config/config.h re-defines PACKAGE_STRING and Co. To avoid compiler
// warnings we undef these llvm/Config/config.h defines.
//
#undef  PACKAGE_NAME
#undef  PACKAGE_BUGREPORT
#undef  PACKAGE_STRING
#undef  PACKAGE_TARNAME
#undef  PACKAGE_VERSION

#include "Assertion.h"

#include "sys/cpu/processor.h"
#include "profile/PhysicalProfile.h"

#include "concurrent/Mutex.h"
#include "concurrent/ConditionVariable.h"
#include "concurrent/ScopedLock.h"

#include "translate/TranslationEmit.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/TranslationModule.h"

#include "translate/TranslationManager.h"
#include "translate/TranslationWorker.h"

#include "util/system/SharedLibrary.h"

#include "util/CodeBuffer.h"

#include "util/Log.h"

#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_

namespace arcsim {
  namespace internal {
    namespace translate {

      // If release pool fills up to release_pool_size it will be emptied
      //
      static const size_t kReleasePoolSize = 8;
  
      // Static variable indicating if LLVM NativeTarget has been initiliased
      //
      static bool isNativeLlvmTargetInitialised = false;

      
// Constructor
//
TranslationWorker::TranslationWorker(uint32 _id, TranslationManager& _m, SimOptions& _sim_opts)
      : worker_id(_id),
        mgr(_m),
        sim_opts(_sim_opts),
        debug_mode(_m.debug_mode),
        keep_mode(_m.keep_mode),
        use_llvm_jit(_m.use_llvm_jit),
        run_state(TW_STATE_RUN),
        work_state(TW_WORK_STATE_WAITING)
  { 
    // FIXME: @igor - make this configurable
    code_buf_ = new arcsim::util::CodeBuffer((sim_opts.cycle_sim) ? 1024*KB : 512*KB);

    if (use_llvm_jit) {
      if (!llvm::llvm_is_multithreaded()) { llvm::llvm_start_multithreaded(); }

      // ------------------------------------------------------------------------- 
      // To make sure that the initialisation of the native target und command
      // line options really _is_ a singleton we need to lock around the shared
      // variable 'isNativeLlvmTargetInitialised'.
      //
      llvm::llvm_acquire_global_lock();

      if (!isNativeLlvmTargetInitialised) {
        llvm::InitializeNativeTarget();
        isNativeLlvmTargetInitialised = true;
   
        // Disables Loop Strength Reduction (LSR) in the code generator,
        // as this is currently broken in LLVM 2.8. Fortunately, we don't need it!
        //
        const char* options[2] = { "arcsim-jit", "-disable-lsr" };
        llvm::cl::ParseCommandLineOptions(2, (char**)options);
      }
      
      llvm::llvm_release_global_lock();
      //
      // -------------------------------------------------------------------------
      // Initialise Clang/LLVM JIT compiler components
      //
            
      CI_ = new clang::CompilerInstance(); // create new clang compiler instance
      CI_->createDiagnostics(0, 0);
      CI_->getDiagnostics().setClient(new clang::TextDiagnosticBuffer());
      clang::CompilerInvocation::CreateFromArgs(CI_->getInvocation(), 0, 0, CI_->getDiagnostics());
      CI_->getInvocation().getLangOpts().C99 = 1; // set C99 standard
      CI_->createFileManager(); // create file manager
      CI_->setTarget(clang::TargetInfo::CreateTargetInfo(CI_->getDiagnostics(),
                                                         CI_->getTargetOpts()));      
      CTX_ = new llvm::LLVMContext();
      MOD_ = new llvm::Module("module", *CTX_);
      ENG_ = llvm::EngineBuilder(MOD_)
                .setEngineKind(llvm::EngineKind::JIT)   // we really want the JIT
                .setOptLevel(llvm::CodeGenOpt::Default) // -O2,-Os
                .setAllocateGVsWithCode(false)
                .setCodeModel(llvm::CodeModel::Default)
                .setJITMemoryManager(NULL)
                .create();
      
      // Initialise the TranslationOptManager
      // FIXME: What is magic number 3, use typedefed ENUM instead
      //
      TranslationVariant variant = (sim_opts.cycle_sim || sim_opts.memory_sim)
                                    ? VARIANT_CYCLE_ACCURATE
                                    : VARIANT_FUNCTIONAL;
      opt_manager.configure(3, ENG_, variant);
    }
  }

// Destructor
//
TranslationWorker::~TranslationWorker()
{
  delete code_buf_; // free code generation buffer
  delete CI_; // free compiler instance
  if (use_llvm_jit) {
    // Sweep release pools
    sweep_translation_work_unit_release_pool();   // GC translation work unit
    sweep_module_release_pool();                  // GC machine code

    opt_manager.destroy();                        // destroy opt pass managers

    // deleting an ExecutionEngine automatically deletes all owned modules
    delete ENG_;      
    delete CTX_;
  }
}


// -----------------------------------------------------------------------------
// Run method TranslationWorker worker threads
// -----------------------------------------------------------------------------
void
TranslationWorker::run()
{ 
  LOG(LOG_DEBUG1) << "[TW" << worker_id << "] STARTED TRANSLATION WORKER THREAD";
  
  // Check state and change it from RUN to RUNNING
  //
  ASSERT(run_state == TW_STATE_RUN);
  run_state = TW_STATE_RUNNING;  
    
  // ---------------------------------------------------------------------------
  // RUN LOOP START
  // ---------------------------------------------------------------------------
  
  for (; /* ever */ ;)
  {
    bool    success = true;
    // Wait until there is work in the queue
    //
    mgr.mutx_work_queue.acquire();

    while (mgr.trans_work_unit_queue.empty()) {
      // Check if we should stop
      // NOTE: This only breaks out of the 'while' loop. That is why we need to
      //       check after the 'while' loop again to break out of outer 'for' loop.
      //
      if (run_state == TW_STATE_STOP)
        break;
      
      // If queue is empty go to sleep and wait for signal
      //
      mgr.cond_work_queue.wait(mgr.mutx_work_queue);

    }
    // Check if we should stop
    // NOTE: If state was changed to STOP we need to release the MUTEX and break
    //       out of the 'for' loop.
    //
    if (run_state == TW_STATE_STOP) {
      mgr.mutx_work_queue.release();
      break;
    }
        
    // Pop off next work item
    //
    TranslationWorkUnit* work_unit = mgr.trans_work_unit_queue.top();
    mgr.trans_work_unit_queue.pop();
    
    // Once we have grabbed some work set our work state to busy
    //
    work_state = TW_WORK_STATE_BUSY;
    
    // Release mutex as quickly as possible
    //
    mgr.mutx_work_queue.release();
        
    // Perform translations
    //
    code_buf_->clear(); // clear code buffer for code generation
    success = translate_module(*work_unit);
    
    // Check if we should stop
    //
    if (run_state == TW_STATE_STOP) {
      mark_translation_work_unit_for_gc(work_unit); // mark work unit for GC  
      break;
    }

    if (success) {
      success = load_module(*work_unit);
      
      if (success) {
        LOG(LOG_DEBUG) << "[TW" << worker_id
                        << "] JIT: REG  MC - WU Exec Freq: "
                        << work_unit->exec_freq
                        << " WU Disp Inter: "
                        << work_unit->timestamp
                        << " Cur Inter: "
                        << mgr.dispatch_counter
                        << " Cur Queue Len: "
                        << mgr.trans_work_unit_queue.size();
      } else {
        LOG(LOG_WARNING) << "[TW" << worker_id << "] JIT: Skipped MC registration.";
      }
    }
    
    mark_translation_work_unit_for_gc(work_unit); // mark work unit for GC  
    sweep_translation_work_unit_release_pool();   // GC translation work unit
    sweep_module_release_pool();                  // GC machine code
    
    // We are done with our work
    //
    work_state = TW_WORK_STATE_WAITING;

    
    // Check if we should stop
    //
    if (run_state == TW_STATE_STOP)
      break;
  }
  // ---------------------------------------------------------------------------
  // RUN LOOP END
  // ---------------------------------------------------------------------------
  
  LOG(LOG_DEBUG1) << "[TW" << worker_id << "] STOPPED TRANSLATION WORKER THREAD";
  
  // Sweep release pools before we exit
  sweep_translation_work_unit_release_pool();   // GC translation work unit
  sweep_module_release_pool();                  // GC machine code

  // Check state and change it from STOP to STOPPED
  //
  ASSERT(run_state == TW_STATE_STOP);
  run_state = TW_STATE_STOPPED;  
}

      
void
TranslationWorker::mark_module_for_gc(void* m)
{
  arcsim::concurrent::ScopedLock lock(mutx_mod_release_pool_);
  ASSERT(m && "mark_module_for_gc() parameter error.");
  mod_release_pool_.insert(m);   
}
      
void
TranslationWorker::sweep_module_release_pool()
{
  arcsim::concurrent::ScopedLock lock(mutx_mod_release_pool_);
    
  while (!mod_release_pool_.empty()) {
    // GC generated machine code for Module functions and remove llvm::Module
    //
    llvm::Module* m = reinterpret_cast<llvm::Module*>(*mod_release_pool_.begin());
    
    for (llvm::Module::iterator I = m->getFunctionList().begin(), E = m->getFunctionList().end();
         I != E; ++I) {
      ENG_->freeMachineCodeForFunction(I);
    }
    ENG_->removeModule(m);
    LOG(LOG_DEBUG1) << "[TW" << worker_id << "] GC Module '" << m->getModuleIdentifier() << "'.";
    delete m;
    mod_release_pool_.erase(mod_release_pool_.begin());
  }
}
      
void
TranslationWorker::mark_translation_work_unit_for_gc(TranslationWorkUnit* w)
{ 
  twu_release_pool_.push(w);
}

void
TranslationWorker::sweep_translation_work_unit_release_pool()
{
  // Free release pool if threshold is reached
  if (twu_release_pool_.size() > kReleasePoolSize) {
    while (!twu_release_pool_.empty()) {
      delete twu_release_pool_.front();
      twu_release_pool_.pop();
    }
  }
}

      
// -----------------------------------------------------------------------------
//
// Translation related methods
//
// -----------------------------------------------------------------------------

// Perform translations the right way
//
bool
TranslationWorker::translate_module(const TranslationWorkUnit& wu)
{
  bool success          = true;
  
  // Emit the header and local functions for this translation unit
  success = TranslationEmit::emit(*code_buf_,
                                  sim_opts,
                                  wu.cpu->sys_arch.isa_opts,
                                  wu.cpu->core_arch,
                                  wu.cpu->mem,
                                  wu.cpu->mem_model,
                                  wu.cpu->pipeline,
                                  wu.cpu->cnt_ctx);
  if (!success) return false;
  
  // Emit translated blocks for this translation unit
  success = translate_work_unit_to_c(wu);
  if (!success) return false;            
  
  // Compile translation unit
  if (use_llvm_jit)   { success = compile_module_llvm  (*wu.module); }
  else if (debug_mode){ success = compile_module_system(*wu.module); }
  else                { success = compile_module_popen (*wu.module); }
  
  return success;
}

// CLANG/LLVM based JIT compilation
//
bool
TranslationWorker::compile_module_llvm(TranslationModule& m)
{
  // Create llvm::MemoryBuffer from arcsim::util::CodeBuffer
  CI_->createSourceManager(CI_->getFileManager());
  llvm::MemoryBuffer* buf  = llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(code_buf_->get_buffer()), m.get_id());
  CI_->getSourceManager().createMainFileIDForMemBuffer(buf);

  // Create pre-processor and AST context
  CI_->createPreprocessor();
  CI_->createASTContext();

  // Create HEAP allocated CodeGenerator instance
  clang::CodeGenOptions  CG_opts;
  clang::CodeGenerator*  CG = clang::CreateLLVMCodeGen(CI_->getDiagnostics(),
                                                       m.get_id(),
                                                       CG_opts,
                                                       MOD_->getContext());
  CI_->setASTConsumer(CG); // give compiler instance ownership of AST consumer
  
  // Finally we are ready to parse the AST
  clang::ParseAST(CI_->getPreprocessor(),         /* Preprocessor       */
                  CG,                             /* ASTConsumer        */
                  CI_->getASTContext());          /* ASTContext         */

  // Retrieve module and take ownership
  llvm::Module* const llvm_module = CG->ReleaseModule();
  
  if (llvm_module == 0) { // Check if module generation failed
    LOG(LOG_ERROR)  << "[TW" << worker_id << "] TRACE compilation FAILED - SKIPPING. '";
    LOG(LOG_DEBUG4) << "===== Start of translation source =====";
    LOG(LOG_DEBUG4) << code_buf_->get_buffer();
    LOG(LOG_DEBUG4) << "===== End of translation source =====";
    return false;
  }
  
  // Optimise LLVM::Module (i.e. -O3)
  opt_manager.get_pass_manager(0,3)->run(*llvm_module);
  // Register LLVM::Module
  m.set_llvm_module(llvm_module);
  return true;
}


// Compile by piping C source to external compiler using popen(3)
//
bool
TranslationWorker::compile_module_popen (const TranslationModule&   m)
{
  static arcsim::concurrent::Mutex  mutx_popen;
  bool success = true;
	char buf[BUFSIZ];
	char lib[BUFSIZ];
  
  // Build the library target file name
  //
  lib[0] = '\0';
  if (keep_mode) {
    snprintf (lib, sizeof(lib), "%s.dll", m.get_id());
    
    // Dump C-source code to file if '--keep' is enabled
    //
    char src[BUFSIZ];
    src[0] = '\0';
    snprintf (src, sizeof(src), "%s.c"  , m.get_id());
    std::ofstream cfile(src, std::ofstream::out);
    cfile << code_buf_->get_buffer();
    cfile.close();
  } else {
    // Do not overwrite existing --keep data, so use a different name for the dll
    snprintf (lib, sizeof(lib), "%s.tmp.dll", m.get_id());
  }
  
  // Compile assemble and link in ONE command
  //
  buf[0] = '\0';
  snprintf(buf, sizeof(buf), "%s -x cpp-output %s %s -o %s -"
          , sim_opts.fast_cc.c_str()            // JIT compiler to be used
          , JIT_CCOPT                           // pre-defined CC options
          , sim_opts.fast_mode_cc_opts.c_str()  // additional  CC flags
          , lib);                               // name of library file

  // ---------------------------------------------------------------------------
  // START compilation and pipe c-file to compiler
  // NOTE THAT CALL TO popen and fcntl MUST be atomic AND synchronized to enable
  //      calling this method from a parrallel or concurrent context
  // ---------------------------------------------------------------------------
  
  FILE* pipe = NULL;
  
  // ----------------------- SYNCHRONIZED START --------------------------------
  mutx_popen.acquire();
  
  // Execute command
  //
  pipe = popen(buf, "w");
  
  // the descriptor is not inherited by the child process
  // 
  fcntl (fileno(pipe), F_SETFD, FD_CLOEXEC);
  
  mutx_popen.release();
  // ----------------------- SYNCHRONIZED END   --------------------------------
  
  if (pipe != NULL) {
    // Pipe c code to compiler
    //
    fprintf(pipe, "%s\n", code_buf_->get_buffer());
    
    // Flush buffers
    //
    fflush(pipe);
    
    // Close pipe
    //
    if (pclose(pipe) == -1) {
      // pclose failed
      //
      LOG(LOG_ERROR) << "[TW" << worker_id << "] JIT compilation failed.";
      
      success = false;
    }
  } else {
    // Popen failed
    //
    LOG(LOG_ERROR) << "[TW" << worker_id << "] Executing JIT compiler failed.";
    
    success = false;
  }
  
  LOG(LOG_DEBUG) << "[TW" << worker_id << "] '" << buf << "' - "
                  << ( (success) ? "DONE" : "ERROR" );
  
  //---------------------------------------------------------------------------
  // END compilation
  //---------------------------------------------------------------------------
  return success;
}

// Compile by writing C source for file and executing external compiler using system(3)
//
bool
TranslationWorker::compile_module_system (const TranslationModule&  m)
{
  bool success = true;
	char buf[BUFSIZ];
	char lib[BUFSIZ];
  char src[BUFSIZ];
  
	// Build the library target file name
	//
  lib[0] = '\0';
  if (keep_mode) {
    snprintf (lib, sizeof(lib), "%s.dll", m.get_id());
  } else {
    snprintf (lib, sizeof(lib), "%s.tmp.dll", m.get_id());
  }
  
  // Dump C source code to file
  //
  src[0] = '\0';
  snprintf (src, sizeof(src), "%s.c"  , m.get_id());
  std::ofstream cfile(src, std::ofstream::out);
  cfile << code_buf_->get_buffer();
  cfile.close();
  
  // Compile assemble and link in ONE command
  //
  buf[0] ='\0'; 
  snprintf(buf, sizeof(buf), "%s %s %s -g -o %s %s"
          , sim_opts.fast_cc.c_str()            // JIT compiler to be used
          , JIT_CCOPT                           // pre-defined CC options
          , sim_opts.fast_mode_cc_opts.c_str()  // additional  CC flags
          , lib                                 // name of library file
          , src);                               // name of C source file
    
  // Start off compilation command
  //
	int ret = ::system (buf);
	if (WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT))
		success = false;
  
	// Remove temporary files, unless --keep flag is set
	//
	if (!keep_mode && !success) {
		snprintf (buf, sizeof(buf), "/bin/rm -f %s %s", lib, src);
		ret = ::system (buf);
		if (WIFSIGNALED(ret) && (WTERMSIG(ret) == SIGINT)) {
			success = false;
    }
    
    LOG(LOG_DEBUG1) << "[TW" << worker_id << "] '" << buf << "' - "
                    << ( (success) ? "DONE" : "ERROR" );

	}
  return success;
}

bool
TranslationWorker::load_module(const TranslationWorkUnit& work_unit)
{
  bool success               = true;
  TranslationModule& m = *work_unit.module;
  
  // -----------------------------------------------------------------------
  // BEGIN SYNCHRONISED registration of native code
  //
  m.lock();
  
  if (m.is_dirty()) {         /* DIRTY TRANSLATION MODULE - SKIP */
    // If a module is dirty, no translations can be present
    ASSERT((m.get_ref_count() == 0)
           && "[TranslationWorker] Module is dirty but contains references to translations.");
    // re-set translation state for BlockEntries
    
    for (std::list<TranslationBlockUnit*>::const_iterator
         BI = work_unit.blocks.begin(), E = work_unit.blocks.end();
         BI != E; ++BI)
    {
      LOG(LOG_DEBUG) << "[TW" << worker_id << "] reset QUEUED BlockEntry @ 0x"
                     << HEX((*BI)->entry_.phys_addr);
      (*BI)->entry_.mark_as_not_translated();
    }
    
    success = false;
  } else {                     /* JIT COMPILE TRANSLATION MODULE                */
    
    if (use_llvm_jit) {       // link Module and Engine with each other
      m.set_worker_engine(this);
      ENG_->addModule(m.get_llvm_module());
    } else {                  // load module so we can resolve symbols
      success = m.load_shared_library(); 
    }

    // Link BlockEntries to native code
    //  
    if (success) {
      switch (sim_opts.fast_trans_mode)
      {    
        case kCompilationModePageControlFlowGraph: {
          TranslationBlock       native   = 0;
          llvm::Function*        function = 0;
          
          const TranslationBlockUnit& block = *work_unit.blocks.front();
          // Function symbol for block
          arcsim::util::CodeBuffer sym;
          TranslationEmit::block_label(sym, block.entry_.virt_addr);
          
          if (use_llvm_jit) {
            // Retrieve function pointer from the module that was just compiled
            if ((function = m.get_llvm_module()->getFunction(sym.get_buffer()))) {
              // JIT compile function, returning a function pointer.
              native = (TranslationBlock)ENG_->getPointerToFunction(function);
              // Free up IR for function once machine code has been generated
              function->deleteBody();
            } 
          } else { // Lookup function pointer in SharedLibrary
            native = m.get_pointer_to_function(sym.get_buffer());
          }
          
          if (native == 0) { // check for compilation errors
            LOG(LOG_ERROR) << "[TW" << worker_id << "] Error retrieving pointer to function '"
                           << sym.get_buffer() << "'";
            success = false;
          } else {        
            // All Blocks have equal entry point for mode 'kCompilationModePageControlFlowGraph'
            //
            for (std::list<TranslationBlockUnit*>::const_iterator
                 BI = work_unit.blocks.begin(), E = work_unit.blocks.end();
                 BI != E; ++BI)
            {
              m.add_block_entry((*BI)->entry_, native);
            }
            m.mark_as_translated(); // mark module as successfully translated
          }
          
          break;
        } // END case kCompilationModePageControlFlowGraph
          
        case kCompilationModeBasicBlock: {
          
          for (std::list<TranslationBlockUnit*>::const_iterator
               BI = work_unit.blocks.begin(), E = work_unit.blocks.end();
               success && BI != E; ++BI)
          {
            TranslationBlock       native   = 0;
            llvm::Function*        function = 0;
            
            // Function symbol for block
            arcsim::util::CodeBuffer sym;
            TranslationEmit::block_label(sym,(*BI)->entry_.virt_addr);
            
            if (use_llvm_jit) {
              // Retrieve function pointer from the module that was just compiled
              if ((function = m.get_llvm_module()->getFunction(sym.get_buffer()))) {
                // JIT compile function, returning a function pointer.
                native = (TranslationBlock)ENG_->getPointerToFunction(function);
                // Free up IR for function once machine code has been generated
                function->deleteBody();
              } 
            } else { // Lookup function pointer in SharedLibrary
              native = m.get_pointer_to_function(sym.get_buffer());
            }
            
            if (native == 0) { // check for compilation errors
              LOG(LOG_ERROR) << "[TW" << worker_id << "] Error retrieving pointer to function '"
                             << sym.get_buffer() << "'";
              success = false;
            } else {
              m.add_block_entry((*BI)->entry_, native);
            }
          }
          
          if (success) // mark module as successfully translated
            m.mark_as_translated();
          break;
        } // END case kCompilationModeBasicBlock
          
      } // switch (sim_opts.fast_trans_mode)
    } // END success
  }
  
  m.unlock();
  //
  // END SYNCHRONISED
  // -----------------------------------------------------------------------

  return  success;
}

} } } // namespace arcsim::internal::translate

