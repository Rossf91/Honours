//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
//
// TranslationManager Impl.
//
// =====================================================================

#include <limits.h>

#include "translate/TranslationManager.h"
#include "translate/TranslationWorker.h"

#include "arch/Configuration.h"

#include "util/Os.h"
#include "util/Log.h"

// Not all systems define SIZE_T_MAX so make sure it has a sensible value
//
#ifndef SIZE_T_MAX
#define SIZE_T_MAX UINT_MAX
#endif

// FIXME: @igor - once Clang and LLVM use thread-safe initialisation of
//        static members we can remove these includes.
// 
#include "llvm/Support/MemoryBuffer.h"
#include "clang/Basic/Version.h"
#include "clang/Basic/PrettyStackTrace.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "util/CodeBuffer.h"


namespace arcsim {
  namespace internal {
    namespace translate {
      
      
      // FIXME: @igor - To avoid running into CR http://llvm.org/bugs/show_bug.cgi?id=12109
      //        we instantiate some Clang objects here, when we are still single
      //        threaded to avoid init race-conditions for function local static
      //        variables. Remove this once Clang and LLVM are fixed.
      //
      static void ClangInitialisationWorkaround() {
        // @igor - call getClangFullVersion in single-threaded mode to avoid race-conditions
        //         when initiliasing function static variables
        clang::getClangFullVersion();

        // Disable pretty stack trace functionality, which will otherwise be a very
        // poor citizen of the world and set up all sorts of signal handlers.
        //
        llvm::DisablePrettyStackTrace = true;
        
        // @igor - Wire-up minimum amount of clang::* objects to be able to instantiate
        //         an object of class PrettyStackTrace, which in turn will initialise
        //         a function static variable indicating that a CrashReporter has been
        //         registered. This is an awful workaround to avoid the problem outlined
        //         in http://llvm.org/bugs/show_bug.cgi?id=12109
        //
        arcsim::util::CodeBuffer dummy_buf(512);
        dummy_buf.append("int main () {return 0;}"); // crate dummy source buffer

        clang::CompilerInstance* CI_ = new clang::CompilerInstance(); // HEAP allocate clang compiler instance
        CI_->createDiagnostics(0, 0);
        CI_->createFileManager(); // create file manager
        CI_->createSourceManager(CI_->getFileManager()); // create source manager
        llvm::MemoryBuffer* buf  = llvm::MemoryBuffer::getMemBuffer(llvm::StringRef(dummy_buf.get_buffer()), "DUMMY");
        CI_->getSourceManager().createMainFileIDForMemBuffer(buf);
        clang::PrettyStackTraceLoc dummy_loc(CI_->getSourceManager(),
                                             CI_->getSourceManager().getLocForStartOfFile(CI_->getSourceManager().getMainFileID()),
                                             "TOP DUMMY STACK TRACE ENTRY");
        delete CI_; // de-allocate clang compiler instance
      }
      
      
      // Constructor
      //
      TranslationManager::TranslationManager() :
        has_started(false),
        keep_mode(false),
        worker_thread_count(1),
        dispatch_counter(0)
      {
        // FIXME: @igor - once Clang and LLVM use thread-safe initialisation of
        //        static members we can avoid calling this method
        ClangInitialisationWorkaround();
      }


      // Destructor
      //
      TranslationManager::~TranslationManager()
      { 
        // Free heap allocated TranslationWorkers
        //
        if (has_started) {
          for (size_t i = 0; i < worker_list.size(); ++i) {
            TranslationWorker* worker = worker_list[i];
            if (worker != NULL) { delete (worker); }
          }
        }
      }

      // Configure TranslationManager
      //
      bool
      TranslationManager::configure(SimOptions* _sim_opts, size_t _num_workers)
      {
        sim_opts            = _sim_opts;
        keep_mode           = sim_opts->keep_files;
        use_llvm_jit        = sim_opts->fast_use_default_jit;
        debug_mode          = sim_opts->fast_enable_debug;
        
        worker_thread_count = _num_workers;
        
        return true;
      }
      
      bool
      TranslationManager::start_workers()
      {
        if (has_started || (worker_thread_count < 1)) return false;
                
        LOG(LOG_DEBUG1) << "[TM] Starting " << worker_thread_count << " workers.";
        
        // Create right size for list holding translation workers
        //
        worker_list.resize(worker_thread_count);
        
        // Instantiate and Run TranslationWorker threads
        //
        for (size_t i = 0; i < worker_thread_count; ++i) {
          worker_list[i] = new TranslationWorker(i, *this, *sim_opts);
          worker_list[i]->start();
        }
        
        // Remember that we have successfully started
        //
        has_started = true;
        
        return true;
      }
      
      // Copy work-load into local queue and return...
      //
      bool
      TranslationManager::dispatch_translation_work_units(size_t work_size,
                                                          std::valarray<TranslationWorkUnit*>& w)
      {
        bool success = true;
        
        // If there is no work so return immediately
        //
        if (work_size == 0) { return success; }
        
        // ---------------------------------------------------------------------------
        // MUTEX LOCK
        // ---------------------------------------------------------------------------
        mutx_work_queue.acquire();
        
        // Upon each call to this method we increment our dispatch_counter which
        // is used to indicate 'recency' of TranslationWorkUnit (i.e. largest
        // dispatch_counter means most recent TranslationWorkUnit)
        //
        ++dispatch_counter;
        
        // Copy work into local priority queue, the insertion puts the hottest things
        // to the front of the queue.
        //
        for (size_t i = 0; i < work_size; ++i) {          
          TranslationWorkUnit *twu = w[i];
          
          // Set recency of TranslationWorkUnit accordingly
          //
          twu->timestamp = dispatch_counter;

          // Insert this work unit into the priority queue
          //
          trans_work_unit_queue.push(twu);

        }
        
        // Output how much work is being generated
        //
        LOG(LOG_DEBUG) << "[TM] Disp Traces: "
                       << work_size
                       << " Disp Inter: "
                       << dispatch_counter
                       << " Cur Queue Length: "
                       << trans_work_unit_queue.size();
        
        // ---------------------------------------------------------------------------
        // MUTEX UNLOCK
        // ---------------------------------------------------------------------------
        mutx_work_queue.release();
        
        //  Signal that there is work to-do
        //
        cond_work_queue.broadcast();

        
#ifdef BLOCKING_DISPATCH_TRANSLATION_WORK_UNITS
        // Define the following to make 'dispatch_translation_work_units()' blocking
        // until ALL translations have been compiled. This should only be used for
        // debugging and verification purposes of the asynchronouse JIT and should
        // never be enabled in production code!
        //
        
        // Busy loop waiting for workers to finish compilation
        //
        uint32 waiting_workers = 0;
        while (    !trans_work_unit_queue.empty()
               && (waiting_workers != worker_thread_count))
        {
          waiting_workers = 0;
          for (size_t i = 0; i < worker_thread_count; ++i) {
            if (worker_list[i]->work_state == TW_WORK_STATE_WAITING) {
              ++waiting_workers;
            }
          }
        }
#endif // DISPATCH_BLOCKING
        return success;
      }

      // Stop all worker threads.
      // FIXME: The logic in this method is way too complicated! Simplify!
      bool
      TranslationManager::stop_workers()
      {
        bool    success = true;
        size_t  running_workers = 0;
        size_t  stopped_workers = 0;
        
        // Return immediately if we have not been started
        //
        if (!has_started) return success;
        
        LOG(LOG_DEBUG1) << "[TM] Stopping workers.";
        
        if (!keep_mode) {
          
          // FIRST we empty the work queue, we need to acquire the MUTEX as we are
          // modifying a shared resource
          //
          // ---------------------------------------------------------------------------
          // MUTEX LOCK
          // ---------------------------------------------------------------------------
          mutx_work_queue.acquire();
          
          while (!trans_work_unit_queue.empty()) {
            trans_work_unit_queue.pop();
          }
          // ---------------------------------------------------------------------------
          // MUTEX UNLOCK
          // ---------------------------------------------------------------------------
          mutx_work_queue.release();
          
        } else { 
          // We are in keep_mode so we need to wait for workers to finish
          //
          
          // We are in keep_mode and wait until the queue is empty so all translations
          // get written out. We do NOT need to acquire a MUTEX as we are only reading
          // and not modifying a shared resources.
          //
          while (!trans_work_unit_queue.empty()) {
            arcsim::util::Os::sleep_micros(1000);
          }
          
          // Now we need to wait until ALL workers are actually finished with the
          // last translation units.
          //
          for (size_t i = 0; i < worker_thread_count; ++i) {
            if (worker_list[i]->run_state == TW_STATE_RUNNING) { ++running_workers; }
          }
          size_t waiting_workers = 0;
          while (running_workers != waiting_workers) {
            waiting_workers = 0;
            for (size_t i = 0; i < worker_thread_count; ++i) {
              if (worker_list[i]->work_state == TW_WORK_STATE_WAITING) { ++waiting_workers; }
            }
            arcsim::util::Os::sleep_micros(100);
          }
        }
        
        // Then we tell all workers to STOP by changing their state
        //
        running_workers = 0;
        for (size_t i = 0; i < worker_thread_count; ++i) {
          if (worker_list[i]->run_state == TW_STATE_RUNNING) {
            worker_list[i]->run_state = TW_STATE_STOP;
            ++running_workers;
          }
        }
        
        //  Wake up workers that might be asleep so they notice the state change
        //
        cond_work_queue.broadcast();
        
        // Busy-loop waiting for all workers to finish their work
        //
        size_t  loop_count = 0;
        while (running_workers != stopped_workers // 1. All workers stopped nicely
               && loop_count++ != SIZE_T_MAX)     // 2. Bail-out condition to avoid deadlock if
                                                  //    thread does not terminate for some reason
        {
          stopped_workers = 0;
          for (size_t i = 0; i < worker_thread_count; ++i) {
            if (worker_list[i]->run_state == TW_STATE_STOPPED) {
              ++stopped_workers;
            }
          }
          // Make busy-loop slightly 'less' busy
          //
          if ((loop_count % 1024) == 0) { arcsim::util::Os::sleep_micros(10); }
        }
        
        LOG(LOG_DEBUG1) << "[TM] Stopped workers.";
        
        return success;
      }

      
} } } // namespace arcsim::internal::translate
