//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Thread-safe Histogram classes responsible for maintaining and
// instantiating all kinds of profiling counters in a generic way.
//
// A Histogram class consists of HistogramEntries and allows to calculate
// the frequencies of just about anything.
//
// The Histogram is extremely powerful, efficient and easy to use. It grows
// on demand and fully automatic and it integrates very well with our JIT
// compiler.
//
// There is also a HistogramIter iterator that allows for easy iteration
// over histogram entries in a Histogram:
//
// 
// for (arcsim::util::HistogramIter I(histogram_instance);
//      !I.is_end();
//      ++I)
// {
//   fprintf(stderr, "%s\n", (*I)->to_string().c_str());
// }            
//
// FIXME:
//  - Introduce method to 'Histogram' that will return the next non-zero address.
//
//
// =====================================================================

#ifndef INC_UTIL_HISTOGRAM_H_
#define INC_UTIL_HISTOGRAM_H_

#include "api/types.h"

#include "ioc/ContextItemInterface.h"

#include "concurrent/Mutex.h"

#include <set>
#include <map>
#include <vector>
#include <stack>
#include <string>

namespace arcsim {
  namespace util {    

    // -------------------------------------------------------------------------
    // Forward declarations
    //
    class MultiHistogram;
    
    // -------------------------------------------------------------------------
    // HistogramEntry is used to encapsulate a single 'bar' of a histogram.
    //
    class HistogramEntry
    {
    public:
      // Constructor
      //
      HistogramEntry();
      
      void    set_value(uint32 val)       { y_value_  = val;  }
      uint32  get_value()           const { return y_value_;  }
      uint32* get_value_ptr()             { return &y_value_; }
      
      void    set_index(uint32 idx)       { x_index_  = idx;  }
      uint32  get_index()           const { return x_index_;  }
      
      void  inc()                         { ++y_value_;       }
      void  inc(uint32 val)               { y_value_ += val;  }
      
      // Reset to 0
      //
      void  clear()                       { y_value_ = 0;     }
      
      // Return 'name: value' as string
      //
      std::string to_string()       const;
      
    private:
      uint32  x_index_;
      uint32  y_value_;
    };
    
    
    // -------------------------------------------------------------------------
    // HistogramEntryComparator implementation - based on index
    //
    struct HistogramEntryComparator
    {
      bool operator() (const HistogramEntry* lhs,
                       const HistogramEntry* rhs) const
      { return lhs->get_index() < rhs->get_index() ; }
    };
    
    // -------------------------------------------------------------------------
    // Histogram  maintains HistogramEntries allowing fast access and
    // increment operations.
    //
    class Histogram : public arcsim::ioc::ContextItemInterface
    {
      // MultiHistogram, HistogramIter, and SortedHistogramValueIter are our
      // friends and get to access private members and methods.
      //
      friend class MultiHistogram;
      friend class HistogramIter;
      friend class SortedHistogramValueIter;
      
    protected:
      // The following methods are not 'publicly' exposed as we do not want
      // to allow people to modify Histogram IDs and Histogram names after
      // constructor initialisation. We only allow it in cases where the
      // Histogram is embedded in another container, such as the MultiHistogram,
      // where system wide unique Histogram IDs do not make sense because in that
      // case the MultiHistogram is container (i.e. CounterManager) managed.
      //
      
      // No args constructor creating an empty Histogram without unique ID
      // assignment and a name
      //
      explicit Histogram();

      // Set histogram ID
      //
      void set_id(uint32 id)      { id_ = id; }

      // Set histgram name
      //
      void set_name(const char * name);
      
    public:
      
      // Maximum name length
      //
      static const int kHistogramMaxNameSize            = 256;
      
      // Default HistogramEntry allocation size
      //
      static const int kHistogramEntryDefaultAllocSize  = 64;
      
      // Constructor
      //
      explicit Histogram(const char*  name,
                         uint32       alloc_size = kHistogramEntryDefaultAllocSize);
      
      // Destructor
      //
      ~Histogram();
      
      const uint8* get_name()  const { return name_;    }
      const Type   get_type()  const { return arcsim::ioc::ContextItemInterface::kTHistogram; }
      uint32       get_id()    const { return id_;      }
      
      // Set value at index. If index does not exist it will be automatically allocated.
      //
      void set_value_at_index(uint32 idx, uint32 val);
      
      // Get value at index. If index does not exist it will be automatically allocated.
      //
      uint32  get_value_at_index    (uint32 idx);
      
      // Get pointer to value at index. If index does not exist it will be
      // automatically allocated.
      //
      uint32* get_value_ptr_at_index(uint32 idx);
      
      // Check if entry exists at index
      //
      bool index_exists(uint32 idx) const;
      
      // Sum all 'bars' in histogram
      //
      uint64  get_total(void) const;
      
      // Increment. If index does not exist it will be automatically allocated.
      //
      void inc(uint32 idx);
      void inc(uint32 idx, uint32 val);
            
      // Reset everything to 0
      //
      void clear();
      
      // Return histogram as string
      //
      std::string to_string();
      
    private:
      uint32 id_;
      static const uint32 kInitialHistogramId = 0xFFFFFFFF;
      uint8  name_[kHistogramMaxNameSize];
      
      uint32 alloc_size_;
      
      
      // Set of ALL HistogramEntries for this Histogram
      //
      std::set<HistogramEntry*>     hist_entries_;
      
      // Map for fast lookup/increment of HistogramEntry values:
      // key: hist index
      // val: pointer to hist value
      //
      std::map<uint32,uint32*>      hist_map_index_value_;
      
      // In order to allow for allocation to happen in chunks we use an allocation
      // pool that looks like a stack. When adding a new HistogramEntry to the 
      // hist_entries_ set we pop a pre-allocated element from the hist_alloc_pool_
      // stack. Should the hist_alloc_pool_ become empty we will allocate another
      // chunk of HistogramEntries and put them on a stack.
      //
      std::stack<HistogramEntry*>   hist_entries_alloc_pool_;
      
      // Stack of ALL allocated HistogramEntry chunks
      //
      std::stack<HistogramEntry*>   hist_entries_alloc_chunks_;
      
      // Mutex that must be acquired before modifying HistogramEntry book keeping
      // structures
      //
      arcsim::concurrent::Mutex     hist_entries_mutex_;
      
      // Re-fills allocation pool and returns number of instantiated HistogramEntries
      //
      uint32 refill_historgram_allocation_pool();
      
      // Returns pointer to new HistogramEntry and maintains all lookup
      // structures
      //
      HistogramEntry* new_histogram_entry(uint32 idx);
    };
    
    // -------------------------------------------------------------------------
    // HistogramIter iterator allowing easy iteration over HistogramEntries sorted
    // by index.
    //
    class HistogramIter
    {
    private:
      const Histogram& hist;
      std::set<HistogramEntry*,HistogramEntryComparator>                 entries;
      std::set<HistogramEntry*,HistogramEntryComparator>::const_iterator iter;
    public:
      HistogramIter(const Histogram& h);
      HistogramIter(Histogram * const h);
      HistogramIter(const HistogramIter& hi);
      HistogramIter(HistogramIter * const hi);
      void                  operator++();
      const HistogramEntry* operator*();
      bool                  is_begin();
      bool                  is_end();
    };

    // -------------------------------------------------------------------------
    // SortedHistogramValueIter iterator allowing easy iteration over HistogramEntries
    // sorted by value.
    //
    class SortedHistogramValueIter
    {
    private:
      const Histogram& hist;
      std::vector<HistogramEntry*>                  entries;
      std::vector<HistogramEntry*>::const_iterator  iter;
    public:
      SortedHistogramValueIter(const Histogram& h);
      SortedHistogramValueIter(Histogram * const h);
      SortedHistogramValueIter(const SortedHistogramValueIter& hi);
      SortedHistogramValueIter(SortedHistogramValueIter * const hi);
      void                  operator++();
      const HistogramEntry* operator*();
      bool                  is_begin();
      bool                  is_end();
    };

    
} } // namespace arcsim::util

#endif  // INC_UTIL_HISTOGRAM_H_
