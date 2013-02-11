//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
//  This class is where all PassManagers are created and maintained.
//
// =====================================================================

#include "translate/TranslationOptManager.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/PassManager.h"

#if (LLVM_VERSION < 30)
#include "llvm/Support/StandardPasses.h"
#else
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/Scalar.h"
#endif

#include "llvm/Target/TargetData.h"

namespace arcsim {
  namespace internal {
    namespace translate {
      
      
      TranslationOptManager::TranslationOptManager()
      : opt_level_count(0),
        n_transitions(0),
        thePMs(0),
        variant(VARIANT_FUNCTIONAL) 
      { /* EMPTY */ }
      
      TranslationOptManager::~TranslationOptManager()
      { /* EMPTY */ }
      
      
      void
      TranslationOptManager::configure(int                    opt_levels,
                                       llvm::ExecutionEngine* engine,
                                       TranslationVariant     _variant)
      {
        variant         = _variant;
        opt_level_count = opt_levels;
        
        // Number of opt levels is the number of levels we can optimise _to_,
        // therefore the total number of levels is actually n+1, therefore:
        // Number of transitions is (n+1)*(n) / 2 - or, n+1 choose 2.
        //
        n_transitions = ( (opt_levels+1) * opt_levels ) / 2;
        
        thePMs = new llvm::PassManager *[n_transitions];
        
        // Allocate the PassManagers
        //
        for (int tr = 0; tr < n_transitions; ++tr) {
          thePMs[tr] = new llvm::PassManager();
          thePMs[tr]->add(new llvm::TargetData(*(engine->getTargetData())));
        }
        
        // Configure the PassManagers for each possible transition.
        //
        for (int lo = 0; lo < opt_levels; ++lo) {
          for (int hi = lo+1; hi <= opt_levels; ++hi) {
            configure_passes(get_pass_manager(lo, hi), lo, hi);
          }
        }
      }
      
      void
      TranslationOptManager::destroy()
      {
        for (int i = 0; i < n_transitions; ++i) {
          delete thePMs[i];
        }
        delete[] thePMs;
      }
      
      
      llvm::PassManager*
      TranslationOptManager::get_pass_manager(int current_opt_level, int target_opt_level)
      {
        // Check the current and target opt levels are sane.
        //
        if (current_opt_level >= target_opt_level
            || current_opt_level > opt_level_count
            || target_opt_level > opt_level_count
            || current_opt_level < 0
            || target_opt_level < 1) {
          return NULL;
        }
        
        // Okay, there has to be a simpler way to do this.
        // Basically I want them to be laid out like:
        // [0,1] [0,2] [0,3] [1,2] [1,3] [2,3]
        // where [x,y] means PassManager which optimises from x to y.
        int offset = 0;
        int i = current_opt_level;
        int j = 0;
        while (i > 0) {
          offset += (opt_level_count - j);
          --i;
          ++j;
        }
        offset += (target_opt_level - ( current_opt_level + 1 ));
        
        return thePMs[offset];
      }
      
      void
      TranslationOptManager::configure_passes(llvm::PassManager *PM,
                                              int current_opt_level,
                                              int target_opt_level)
      {
        // ---------------------------------------------------------------------
        // Optimisations for JIT compilation of functional code
        //
        if (variant == VARIANT_FUNCTIONAL)
        {
          // Add function passes used in FUNCTIONAL mode here!
          //
          //
          if (current_opt_level == 0) {
            // UnitAtATime optimisations
            //
            PM->add(llvm::createGlobalOptimizerPass());     // Optimize out global vars
            PM->add(llvm::createIPSCCPPass());              // IP SCCP
            PM->add(llvm::createDeadArgEliminationPass());  // Dead argument elimination

            // Add all level one optimisations here.
            //
            PM->add(llvm::createInstructionCombiningPass()); // Clean up after IPCP & DAE
            PM->add(llvm::createCFGSimplificationPass());    // Clean up after IPCP & DAE
            PM->add(llvm::createFunctionInliningPass(255));
            PM->add(llvm::createFunctionAttrsPass());        // Set readonly/readnone attrs
          }
          if (current_opt_level <= 1 && target_opt_level > 1) {
            // Add all level two optimisations here.
            //
            PM->add(llvm::createArgumentPromotionPass());   // Scalarize uninlined fn args
          }
          if (current_opt_level <= 2 && target_opt_level > 2) {
            // Add all level three optimisations here.
            //
            PM->add(llvm::createScalarReplAggregatesPass()); // Break up aggregate allocas

            PM->add(llvm::createInstructionCombiningPass()); // Cleanup for scalarrepl.
            PM->add(llvm::createJumpThreadingPass());        // Thread jumps.
            PM->add(llvm::createCFGSimplificationPass());    // Merge & remove BBs
            PM->add(llvm::createInstructionCombiningPass()); // Combine silly seq's

            PM->add(llvm::createReassociatePass());          // Reassociate expressions
            PM->add(llvm::createInstructionCombiningPass()); // Cleanup

            PM->add(llvm::createMemCpyOptPass());            // Remove memcpy / form memset
            PM->add(llvm::createSCCPPass());                 // Constant prop with SCCP
            PM->add(llvm::createInstructionCombiningPass());

            PM->add(llvm::createJumpThreadingPass());        // Thread jumps
            PM->add(llvm::createCorrelatedValuePropagationPass());
            PM->add(llvm::createDeadStoreEliminationPass()); // Delete dead stores
            PM->add(llvm::createAggressiveDCEPass());        // Delete dead instructions
            PM->add(llvm::createCFGSimplificationPass());    // Merge & remove BBs
            PM->add(llvm::createGVNPass());                  // Global Value Numbering pass
            PM->add(llvm::createInstructionCombiningPass()); // Standard cleanup
            PM->add(llvm::createCFGSimplificationPass());    // Standard cleanup

            // UnitAtATime optimisations
            // FIXME: Try running this at the very beginning as well!
            //
            PM->add(llvm::createStripDeadPrototypesPass()); // Get rid of dead prototypes
#if (LLVM_VERSION < 30)
            PM->add(llvm::createDeadTypeEliminationPass()); // Eliminate dead types
#endif            
            PM->add(llvm::createGlobalDCEPass());           // Remove dead fns and globals.
            PM->add(llvm::createConstantMergePass());       // Merge dup global constants

          }
        } else if (variant == VARIANT_CYCLE_ACCURATE) {
          // ---------------------------------------------------------------------
          // Optimisations for JIT compilation of cycle accurate code
          //
          if (current_opt_level == 0) {
            // UnitAtATime optimisations
            //
            PM->add(llvm::createGlobalOptimizerPass());     // Optimize out global vars
            PM->add(llvm::createIPSCCPPass());              // IP SCCP
            PM->add(llvm::createDeadArgEliminationPass());  // Dead argument elimination

            // Add all level one optimisations here.
            //
            PM->add(llvm::createInstructionCombiningPass()); // Clean up after IPCP & DAE
            PM->add(llvm::createCFGSimplificationPass());    // Clean up after IPCP & DAE            
          }
          if (current_opt_level <= 1 && target_opt_level > 1) {
            // Add all level two optimisations here.
            //
            PM->add(llvm::createArgumentPromotionPass());    // Scalarize uninlined fn args

          }
          if (current_opt_level <= 2 && target_opt_level > 2) {
            // Add all level three optimisations here.
            //
            PM->add(llvm::createScalarReplAggregatesPass()); // Break up aggregate allocas

            PM->add(llvm::createInstructionCombiningPass()); // Cleanup for scalarrepl.            
            PM->add(llvm::createCFGSimplificationPass());    // Merge & remove BBs
            PM->add(llvm::createInstructionCombiningPass()); // Combine silly seq's

            PM->add(llvm::createReassociatePass());          // Reassociate expressions
            PM->add(llvm::createInstructionCombiningPass()); // Cleanup

            PM->add(llvm::createMemCpyOptPass());            // Remove memcpy / form memset
            PM->add(llvm::createSCCPPass());                 // Constant prop with SCCP
            PM->add(llvm::createInstructionCombiningPass());
            
            PM->add(llvm::createCorrelatedValuePropagationPass());
            PM->add(llvm::createDeadStoreEliminationPass()); // Delete dead stores
            PM->add(llvm::createAggressiveDCEPass());        // Delete dead instructions
            PM->add(llvm::createCFGSimplificationPass());    // Merge & remove BBs            
            PM->add(llvm::createInstructionCombiningPass()); // Standard cleanup
            PM->add(llvm::createCFGSimplificationPass());    // Standard cleanup

            // UnitAtATime optimisations
            // FIXME: Try running this at the very beginning as well!
            //
            PM->add(llvm::createStripDeadPrototypesPass()); // Get rid of dead prototypes
#if (LLVM_VERSION < 30)
            PM->add(llvm::createDeadTypeEliminationPass()); // Eliminate dead types
#endif
            PM->add(llvm::createGlobalDCEPass());           // Remove dead fns and globals.
            PM->add(llvm::createConstantMergePass());       // Merge dup global constants
          }
        }
      }
      
} } } // namespace arcsim::internal::translate

