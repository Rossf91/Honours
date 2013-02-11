//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Thread-safe Counter classes responsible for maintaining and  instantiating
// all kinds of profiling counters in a generic way.
//
// There are two types of Counters, 32-bit counters (i.e. Counter), and
// 64-bit counters (i.e. Counter64).
//
// =====================================================================

#ifndef INC_UTIL_COUNTER_H_
#define INC_UTIL_COUNTER_H_

#include "api/types.h"

#include "ioc/ContextItemInterface.h"

#include <string>

namespace arcsim {
  namespace util {    
    // -------------------------------------------------------------------------
    // Counter class encapsulating common operations on 32 bit counters
    //
    class Counter : public arcsim::ioc::ContextItemInterface
    {
    public:
      // Maximum name lenght
      //
      static const int kCounterMaxNameSize = 256;
      
      // Constructor
      //
      explicit Counter(const char* name);
      
      uint32*        get_ptr()              { return &count_;     }
      inline uint32  get_value()      const { return count_;      }
      
      const uint8*   get_name()       const { return name_;       }
      const Type     get_type()       const { return arcsim::ioc::ContextItemInterface::kTCounter; }

      void           set_value(uint32 count){ count_ = count;     }
      
      inline         void  inc()            { ++count_;           }
      inline         void  inc(uint32 incr) { count_ += incr;     }
      
      inline         void  dec()            { --count_;           }
      inline         void  dec(uint32 decr) { count_ -= decr;     }
      
      // Reset to 0
      //
      void  clear()                         { count_ = 0;         }

            
      // Return 'name: value' as string
      //
      std::string to_string();
      
    private:
      uint8  name_[kCounterMaxNameSize];      
      uint32 count_;
    };
    

    // -------------------------------------------------------------------------
    // Counter class encapsulating common operations on 64 bit counters
    //
    class Counter64  : public arcsim::ioc::ContextItemInterface
    {
    public:
      // Maximum name lenght
      //
      static const int kCounterMaxNameSize = 256;
      
      // Constructor
      //
      explicit Counter64(const char* _name);
            
      uint64*        get_ptr()              { return &count_;     }
      inline uint64  get_value()      const { return count_;      }
      
      const uint8*   get_name()       const { return name_;       }
      const Type     get_type()       const { return arcsim::ioc::ContextItemInterface::kTCounter64; }
      
      void           set_value(uint64 count){ count_ = count;     }
      
      inline void    inc()                  { ++count_;           }
      inline void    inc(uint32 incr)       { count_ += incr;     }
      
      inline void    dec()                  { --count_;           }
      inline void    dec(uint32 decr)       { count_ -= decr;     }
      
      // Reset to 0
      //
      void  clear()                         { count_ = 0;         }
      
      // Return 'name: value' as string
      //
      std::string to_string();

    private:
      uint8  name_[kCounterMaxNameSize];      
      uint64 count_;
    };
  
} } // namespace arcsim::util

#endif  // INC_UTIL_COUNTER_H_
