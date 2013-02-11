//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description: Definition of a ConditionVariable class.
//
// =====================================================================


#include <pthread.h>

#include "Assertion.h"

#include "concurrent/Mutex.h"
#include "concurrent/ConditionVariable.h"

#include "util/Allocate.h"

namespace arcsim {
  namespace concurrent {
      
      // Construct a ConditionVariable
      //
      ConditionVariable::ConditionVariable()
      : condvar_(NULL)
      {
        bool success = true;
        pthread_cond_t* condvar = static_cast<pthread_cond_t*>(arcsim::util::Malloced::New(sizeof(pthread_cond_t))); 
        pthread_condattr_t attr;
        
        // Initialise ConditionVariable attributes
        //
        success = (pthread_condattr_init(&attr) == 0); 
        ASSERT(success);
                
        // FINALLY initialise ConditionVariable
        //
        success = (pthread_cond_init(condvar, &attr) == 0);
        ASSERT(success);
        
        // Destroy attributes
        //
        success = (pthread_condattr_destroy(&attr) == 0);
        ASSERT(success);
        
        // Assign fully instantiated ConditionVariable to condvar_ attribute
        //
        condvar_ = condvar;
      }
      
      // Destruct ConditionVariable
      //
      ConditionVariable::~ConditionVariable()
      {
        pthread_cond_t* condvar = static_cast<pthread_cond_t*>(condvar_);
        ASSERT(condvar != NULL);
        pthread_cond_destroy(condvar);
        arcsim::util::Malloced::Delete(condvar);
      }
      
      
      // Unblocks a thread waiting for this condition variable.
      //
      bool
      ConditionVariable::signal()
      {
        bool success = true;
        pthread_cond_t* condvar = static_cast<pthread_cond_t*>(condvar_);
        
        success = ( pthread_cond_signal(condvar) == 0 );
        
        return success;
      }

      // Unblocks all threads waiting for this condition variable.
      //
      bool
      ConditionVariable::broadcast()
      {
        bool success = true;
        pthread_cond_t* condvar = static_cast<pthread_cond_t*>(condvar_);
        
        success = ( pthread_cond_broadcast(condvar) == 0 );
        
        return success;
      }

      // Wait on this condition variable.
      //
      bool
      ConditionVariable::wait(const arcsim::concurrent::Mutex &m)
      {
        bool success = true;
        pthread_cond_t*   condvar = static_cast<pthread_cond_t*>(condvar_);
        pthread_mutex_t*  mutex = static_cast<pthread_mutex_t*>(m.mutex_);
        
        success = ( pthread_cond_wait(condvar, mutex) == 0 );
        
        return success;
      }

      
} } // arcsim::concurrent
