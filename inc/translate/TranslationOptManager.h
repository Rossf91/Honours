//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// TranslationOptManager is the class with responsibility for creating,
// maintaining, and granting access to PassManagers with differing
// optimisation capabilities.
//
// Note that PassManagers cannot have Passes dynamically removed from them,
// and we would like optimising from level 0 -> 3 to require the same number of
// passes to be run as going from 0 -> 1, and then 1 -> 3. For this reason,
// we need to create PassManagers which handle every possible transition.
// Also note that level 0 means the block has not been translated yet.
//
// =====================================================================

#ifndef _TranslationOptManager_h_
#define _TranslationOptManager_h_

#include "sim_types.h"

namespace llvm {
  class ExecutionEngine;
  class PassManager;
}

namespace arcsim {
  namespace internal {
    namespace translate {
      
      class TranslationOptManager {
      private:
        // Storage for the PassManagers.
        //
        llvm::PassManager **thePMs;
        
        int opt_level_count;
        
        // How many possible transitions exist between the different optimisation levels.
        //
        int n_transitions;

        TranslationVariant variant;
        
        // Private function for adding optimisation passes, will be edited to requirements.
        //
        void configure_passes(llvm::PassManager *PM,
                              int current_opt_level,
                              int target_opt_level);
        
      public:
        // Constructor/Destructor
        //
        TranslationOptManager();
        ~TranslationOptManager();
        
        // Creates the PassManagers, and adds their optimisation passes.
        //
        void configure(int                    opt_levels,
                       llvm::ExecutionEngine* engine,
                       TranslationVariant     variant);
        
        // Should be called on cleanup.
        //
        void destroy();
        
        // Obtain the PassManager which has had all the Passes added to it
        // for getting from <current_opt_level> to <desired_opt_level>.
        //
        llvm::PassManager *get_pass_manager(int current_opt_level, int target_opt_level);
        
      };
      
} } } // namespace arcsim::internal::translate

#endif /* _TranslationOptManager_h_ */
