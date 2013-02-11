//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
//
// =====================================================================

// =====================================================================
// HEADERS
// =====================================================================

#include <pthread.h>

#include "Assertion.h"

#include "concurrent/Mutex.h"

#include "util/Allocate.h"

// =====================================================================
// METHODS
// =====================================================================

namespace arcsim {
  namespace concurrent {

      // Construct a Mutex
      //
      Mutex::Mutex(bool recursive)
        : mutex_(NULL)
      {
        bool success = true;
        pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(arcsim::util::Malloced::New(sizeof(pthread_mutex_t)));
        pthread_mutexattr_t attr;
        
        // Initialise Mutex attributes
        //
        success = ( pthread_mutexattr_init(&attr) == 0 );
        ASSERT(success);
        
        // Initialize the mutex attribute type
        //
        int kind = ( recursive  ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL );
        success = ( pthread_mutexattr_settype(&attr, kind) == 0 );
        ASSERT(success);
        
        // FINALLY initialise mutex
        //
        success = ( pthread_mutex_init(mutex, &attr) == 0 );
        ASSERT(success);
        
        // Destroy attributes
        //
        success = ( pthread_mutexattr_destroy(&attr) == 0 );
        ASSERT(success);
        
        // Assign fully instantiated Mutex to mutex_ attribute
        //
        mutex_ = mutex;
      }

      // Destruct Mutex
      //
      Mutex::~Mutex()
      {
        pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(mutex_);
        ASSERT(mutex != NULL);
        pthread_mutex_destroy(mutex);
        arcsim::util::Malloced::Delete(mutex);
      }
      
      
      // Acquire Mutex
      //
      bool
      Mutex::acquire()
      {
        bool success = true;
        pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(mutex_);
        
        success = ( pthread_mutex_lock(mutex) == 0 );
        
        return success;
      }
      
      // Release Mutex
      //
      bool
      Mutex::release()
      {
        bool success = true;
        pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(mutex_);
        
        success = ( pthread_mutex_unlock(mutex) == 0 );
        
        return success;
      }
      
      // Try to acquire Mutex
      //
      bool
      Mutex::tryacquire()
      {
        bool success = true;
        pthread_mutex_t* mutex = static_cast<pthread_mutex_t*>(mutex_);
        
        success = ( pthread_mutex_trylock(mutex) == 0 );
        
        return success;
      }
      
      
} } // arcsim::concurrent
