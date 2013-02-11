//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Thread class implementation.
//
// =====================================================================

#include <pthread.h>

#include "Assertion.h"

#include "concurrent/Thread.h"

namespace arcsim {
  namespace concurrent {

      // -----------------------------------------------------------------------
      // ThreadHandle::ThreadHandleData nested class impl.
      //
      class ThreadHandle::ThreadHandleData {
      public:
        pthread_t thread_;
        bool      is_valid_;
        
        explicit ThreadHandleData(Kind k)
        : is_valid_(true)
        {
          init(k);
        }
        
        void
        init(Kind k)
        {
          switch (k) {
            case ThreadHandle::SELF:    { thread_   = pthread_self(); break; }
            case ThreadHandle::INVALID: { is_valid_ = false;          break; }
          }
        }
        
      };

      // -----------------------------------------------------------------------
      // ThreadHandle impl.
      //
      
      // Constructor
      //
      ThreadHandle::ThreadHandle(Kind k)
      {
        data_ = new ThreadHandleData(k);
      }
      
      // Destructor
      //
      ThreadHandle::~ThreadHandle()
      {
        delete data_;
      }
      
      void
      ThreadHandle::init(Kind k)
      {
        data_->init(k);
      }
      
      bool
      ThreadHandle::is_valid() const
      {
        return data_->is_valid_;
      }
      
      bool
      ThreadHandle::is_equal() const
      {
        return pthread_equal(data_->thread_, pthread_self());
      }
      
      // -----------------------------------------------------------------------
      // Thread impl.
      //

      // Constructor
      //
      Thread::Thread() : ThreadHandle(ThreadHandle::INVALID)
      { /* EMPTY */ }
      
      // Destructor
      //
      Thread::~Thread()
      { /* EMPTY */ }
      
      // Special static thread entry method
      // @see: http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-18.html
      //
      static void*
      thread_entry_point(void* arg)
      {
        // Retrieve pointer to Thread instance
        //
        Thread* thread = reinterpret_cast<Thread*>(arg);
        thread->get_thread_handle_data()->thread_  = pthread_self();
        thread->get_thread_handle_data()->is_valid_= true;

        ASSERT(thread->is_valid() && "Thread is invalid!");
        
        // Execute run method on Thread instance
        //
        thread->run();
        
        return NULL;
      }
      
      // Spawn actual thread
      //
      void
      Thread::start()
      {
        pthread_create(&get_thread_handle_data()->thread_,  // pthread id
                       NULL,                                // pthread attributes
                       thread_entry_point,                  // thread entry point (function pointer)
                       this);                               // parameters to entry point function
      }
      
      // Terminate thread
      //
      void
      Thread::join()
      {
        // Only if a thread is valid (i.e. has been started) do we call pthread_join()
        //
        if (is_valid()) {
          pthread_join(get_thread_handle_data()->thread_, NULL);
        }
      }

} } // arcsim::concurrent
