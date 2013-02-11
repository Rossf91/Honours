//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:
//
// TranslationManager class keeps track of TranslationWorkers and translation
// trace priority queue.
//
// =====================================================================

#ifndef _TRANSLATION_MANAGER_H
#define _TRANSLATION_MANAGER_H

// =====================================================================
// HEADERS
// =====================================================================

#include <queue>
#include <vector>
#include <valarray>

#include "concurrent/Mutex.h"
#include "concurrent/ConditionVariable.h"

#include "translate/TranslationWorkUnit.h"

class System;
class SimOptions;

namespace arcsim {
  namespace internal {
    namespace translate {
      
      class TranslationWorker;  // Forward decl.
            
      class TranslationManager {
        
        // Allow the TranslationWorker access to private variables
        //
        friend class TranslationWorker;
        
      public:        
        
        TranslationManager();
        ~TranslationManager();
  
        // Assigns worker to Processor
        //
        bool configure(SimOptions* _sim_opts, size_t num_workers);
        
        // Start TranslationWorker threads
        //
        bool start_workers();
        
        // Stop TranslationWorker threads
        //
        bool stop_workers();
        
        // Add work to translation queue
        //
        bool dispatch_translation_work_units(size_t work_size,
                                             std::valarray<TranslationWorkUnit*>& w);
        
        // Return current size of translation work queue
        //
        uint32 get_translation_work_queue_size() const
        {
          return static_cast<uint32>(trans_work_unit_queue.size());
        }

      private:
        TranslationManager(const TranslationManager & m);   // DO NOT COPY
        void operator=(const TranslationManager &);         // DO NOT ASSIGN
        
        // Simulation options
        //
        SimOptions*  sim_opts;
        
        bool         has_started;
        bool         keep_mode;         // keep translations around in files
        bool         use_llvm_jit;      // should new LLVM JIT be used
        bool         debug_mode;        // enable debugging mode so JIT generated code
                                        // can be debugged

        // The dispatch_counter records the frequency of calls to the
        // dispatch_translation_work_units() method. It acts as an indicator
        // for recency.
        //
        uint64       dispatch_counter;
        
        // ---------------------------------------------------------------------
        //
        // Amount of TranslationWorker threads
        //
        size_t                                  worker_thread_count;
        std::valarray<TranslationWorker*>       worker_list;
        
        // Attributes for synchronised access to shared resources
        //
        arcsim::concurrent::Mutex             mutx_work_queue;
        arcsim::concurrent::ConditionVariable cond_work_queue; 
        
        // SHARED RESOURCE
        //
        // Queue structures containing work passed in from the outside via
        // dispatch_translation_work_units(). This structure is a priority queue using
        // TranslationWorkUnit hotspot thresholds as a sorting criteria
        // NOTE: access to this structure MUST be synchronised
        //
        std::priority_queue<TranslationWorkUnit*,std::vector<TranslationWorkUnit*>,
                            PrioritizeTranslationWorkUnits> trans_work_unit_queue;
        

        // ---------------------------------------------------------------------

      };
      
    
} } } // namespace arcsim::internal::translate


#endif // _TRANSLATION_MANAGER_H
