//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Thread-safe MultiHistogram class is responsible for maintaining and
// instantiating profiling data that consist of multiple histograms (e.g.
// a three dimensional array)
//
// A MultiHistogram class consists of Histograms and allows to calculate
// the frequencies of just about anything.
//
// The MultiHistogram is extremely powerful, efficient and easy to use. It
// grows on demand and fully automatic and it integrates very well with our
// JIT compiler.
//
// There is also a MultiHistogramIter iterator that allows for easy iteration
// over Histograms in a MultiHistogram:
//
// // Iterate over all Histograms in MultiHistogram
// //
// for (arcsim::util::MultiHistogramIter HI(multi_histogram_instance);
//      !HI.is_end();
//      ++HI)
// {
//    // Iterate over all HistogramEntries in Histogram
//    //
//    for (arcsim::util::HistogramIter I(*HI);
//         !I.is_end();
//         ++I)
//    {
//      fprintf(stderr, "%s\n", (*I)->to_string().c_str());
//    }
// }            
//
// =====================================================================

#ifndef INC_UTIL_MULTIHISTOGRAM_H_
#define INC_UTIL_MULTIHISTOGRAM_H_

#include "api/types.h"

#include "ioc/ContextItemInterface.h"

#include "concurrent/Mutex.h"

#include <set>
#include <map>
#include <stack>
#include <string>

namespace arcsim {
  namespace util {    

    // -------------------------------------------------------------------------
    // Forward declarations
    //
    class Histogram;
    
    // -------------------------------------------------------------------------
    // HistogramEntryComparator implementation
    //
    struct HistogramComparator
    {
      bool operator() (const Histogram* lhs,
                       const Histogram* rhs) const;
    };
    
    // -------------------------------------------------------------------------
    // MultiHistogram  maintains HistogramEntries allowing fast access and
    // increment operations.
    //
    class MultiHistogram : public arcsim::ioc::ContextItemInterface
    {
      friend class MultiHistogramIter;
    public:
      // Maximum name length
      //
      static const int kMultiHistogramMaxNameSize  = 256;
      
      // Default Histogram allocation size
      //
      static const int kHistogramDefaultAllocSize  = 64;

      // Constructor
      //
      explicit MultiHistogram(const char * name,
                              uint32       alloc_size = kHistogramDefaultAllocSize);
      
      // Histogram
      //
      ~MultiHistogram();
      
      const uint8* get_name()  const { return name_;    }
      const Type   get_type()  const { return arcsim::ioc::ContextItemInterface::kTMultiHistogram; }

      // Retrieve Histogram pointer at index
      //      
      Histogram * const get_hist_ptr_at_index(uint32 idx_hist);
            
      // Check if Histogram exists at index
      //
      bool index_exists(uint32 idx) const;
      
      // Check if Histogram exists and HistogramEntry exists at given index
      //
      bool index_exists_in_histogram(uint32 idx_hist, uint32 idx_entry) const;
            
      // Increment. If index does not exist it will be automatically allocated.
      //
      void inc(uint32 idx_hist, uint32 idx_entry);
      void inc(uint32 idx_hist, uint32 idx_entry, uint32 val);
      
      // Reset everything to 0
      //
      void clear();
      
      // Return histogram as string
      //
      std::string to_string();
      
    private:
      uint8  name_[kMultiHistogramMaxNameSize];            
      uint32 alloc_size_;

      // Set of ALL Histograms for this MultiHistogram
      //
      std::set<Histogram*>              multihist_entries_;
      
      // Map for fast lookup of Histograms:
      // key: hist index
      // val: pointer to histogram
      //
      std::map<uint32,Histogram*>       multihist_map_index_value_;

      // In order to allow for allocation to happen in chunks we use an allocation
      // pool that looks like a stack. When adding a new Histogram to the 
      // histograms_ set we pop a pre-allocated element from the hist_alloc_pool_
      // stack. Should the hist_alloc_pool_ become empty we will allocate another
      // chunk of HistogramEntries and put them on a stack.
      //
      std::stack<Histogram*>            multihist_entries_alloc_pool_;
      
      // Stack of ALL allocated Histogram chunks
      //
      std::stack<Histogram*>            multihist_entries_alloc_chunks_;


      // Mutex that must be acquired before modifying MultiHistogram book keeping
      // structures
      //
      arcsim::concurrent::Mutex         multihist_entries_mutex_;

      // Re-fills allocation pool and returns number of instantiated Histograms
      //
      uint32 refill_historgram_allocation_pool();
      
      // Returns pointer to new Histogram and maintains all lookup structures
      //
      Histogram* new_histogram(uint32 idx);

    };
    
    // -------------------------------------------------------------------------
    // MultiHistogramIter iterator allowing easy iteration over Histograms.
    //
    class MultiHistogramIter
    {
    private:
      const MultiHistogram& hist;
      std::set<Histogram*,HistogramComparator> entries;
      std::set<Histogram*>::const_iterator     iter;
    public:
      MultiHistogramIter(const MultiHistogram& h);
      void                  operator++();
      const Histogram*      operator*();
      bool                  is_begin();
      bool                  is_end();
    };

    
    
} } // namespace arcsim::util


#endif  // INC_UTIL_MULTIHISTOGRAM_H_
