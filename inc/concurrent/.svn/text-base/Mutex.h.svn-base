//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description: Declaration of a Mutex class.
//
// =====================================================================

#ifndef INC_CONCURRENT_MUTEX_H_
#define INC_CONCURRENT_MUTEX_H_


namespace arcsim {
  namespace concurrent {
    
    // -------------------------------------------------------------------------
    // FORWARD DECLARATIONS
    //
    class ConditionVariable;
    
    // -------------------------------------------------------------------------
    // Platform agnostic Mutex class
    //      
    class Mutex
    {
      // ConditionVariable is allowed to peek at private members of Mutex
      //
      friend class ConditionVariable;
      
    public:
      
      // Initializes the lock but doesn't acquire it. If recursive is set
      // to false, the lock will not be recursive which makes it cheaper but
      // also more likely to deadlock (same thread can't acquire more than
      // once).
      //  
      explicit Mutex(bool recursive = false);
      
      // Releases and removes the lock
      //  
      ~Mutex();
      
    public:
      
      // Unconditionally acquire the lock.
      //  
      bool acquire();
      
      // Unconditionally release the lock.
      // returns false if any kind of error occurs, true otherwise.
      // 
      bool release();
      
      // Try to acquire the lock. Returns false if any kind of error occurs or
      // the lock is not available, true otherwise.
      //  
      bool tryacquire();
      
    private:
      void* mutex_;
      
      Mutex(const Mutex & m);           // DO NOT COPY
      void operator=(const Mutex &);    // DO NOT ASSIGN
    };
    
  } } // namespace arcsim::concurrent

#endif // INC_CONCURRENT_MUTEX_H_


