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
// =====================================================================

#include "util/Histogram.h"

#include "concurrent/ScopedLock.h"

#include <algorithm>
#include <sstream>

namespace arcsim {
  namespace util {    

    // -------------------------------------------------------------------------
    // HistogramEntry
    //
    HistogramEntry::HistogramEntry()
    : x_index_(0),
    y_value_(0)
    { /* EMPTY */ }
    
    std::string
    HistogramEntry::to_string() const
    {
      std::ostringstream buf;
      buf << "[" << x_index_ << ": " << y_value_ << "]";
      return buf.str();
    }
    
    // -------------------------------------------------------------------------
    // Histogram
    //
    
    // Re-fill HistogramEntry allocation pool
    //
    uint32
    Histogram::refill_historgram_allocation_pool()
    {
      // Register pointer to HistogramEntry chunk
      //
      HistogramEntry* entries = new HistogramEntry [alloc_size_];
      hist_entries_alloc_chunks_.push(entries);
      
      // Fill up hist_alloc_pool_
      //
      for (uint32 i = 0; i < alloc_size_; ++i) {
        hist_entries_alloc_pool_.push(&entries[i]);
      }
      
      return hist_entries_alloc_pool_.size();
    }
    
    // Returns pointer to new HistogramEntry while maintaining all lookup
    // structures
    //
    HistogramEntry*
    Histogram::new_histogram_entry(uint32 idx)
    {
      arcsim::concurrent::ScopedLock lock(hist_entries_mutex_);
      
      // Re-fill allocation pool if it is empty
      //
      if (hist_entries_alloc_pool_.empty()) { refill_historgram_allocation_pool(); }
      
      // Pop entry from allocation pool stack
      //
      HistogramEntry* entry = hist_entries_alloc_pool_.top();
      hist_entries_alloc_pool_.pop();
      
      // Set desired index
      //
      entry->set_index(idx);
      
      // Add entry to histogram entries
      //
      hist_entries_.insert(entry);
      
      // Install mapping into fast lookup map
      //
      hist_map_index_value_[idx] = entry->get_value_ptr();
      
      return entry;
    }
    
    // Set value at index
    //
    void
    Histogram::set_value_at_index(uint32 idx, uint32 val)
    {
      std::map<uint32,uint32*>::const_iterator I;
      if ((I = hist_map_index_value_.find(idx)) != hist_map_index_value_.end()) {
        // FAST PATH ------------------------------------------------------------
        //
        
        // Hit in fast lookup map
        //
        (*I->second) = val;        
      } else {      
        // SLOW PATH ------------------------------------------------------------
        //
        
        // HistogramEntry is not in fast look-up map so we have to create and install
        // a new one via the following method call.
        //
        HistogramEntry* entry = new_histogram_entry(idx);
        entry->set_value(val);
      }
    }
    
    // Get value at index
    //
    uint32
    Histogram::get_value_at_index(uint32 idx)
    {      
      std::map<uint32,uint32*>::const_iterator I;
      if ((I = hist_map_index_value_.find(idx)) != hist_map_index_value_.end()) {
        // FAST PATH ------------------------------------------------------------
        //
        
        // Hit in fast lookup map
        //
        return (*I->second);        
      } else {        
        // SLOW PATH ------------------------------------------------------------
        //
        
        // HistogramEntry is not in fast look-up map so we have to create and install
        // a new one via the following method call.
        //
        HistogramEntry* entry = new_histogram_entry(idx);
        return entry->get_value();
      }
    }
    
    uint32* 
    Histogram::get_value_ptr_at_index(uint32 idx)
    {
      // FAST PATH ------------------------------------------------------------
      //
      
      // First we query fast lookup map
      //
      std::map<uint32,uint32*>::const_iterator I;
      if ((I = hist_map_index_value_.find(idx)) != hist_map_index_value_.end()) {
        // Hit in fast lookup map
        //
        return I->second;
      }
      
      // SLOW PATH ------------------------------------------------------------
      //
      
      // HistogramEntry is not in fast look-up map so we have to create and install
      // a new one via the following method call.
      //
      HistogramEntry* entry = new_histogram_entry(idx);
      return entry->get_value_ptr();
    }
    
    bool
    Histogram::index_exists(uint32 idx) const
    {
      return hist_map_index_value_.find(idx) != hist_map_index_value_.end();
    }
    
    // Increment
    //
    void
    Histogram::inc(uint32 idx)
    {
      std::map<uint32,uint32*>::const_iterator I;
      if ((I = hist_map_index_value_.find(idx)) != hist_map_index_value_.end()) {
        // FAST PATH ------------------------------------------------------------
        //
        
        // Hit in fast lookup map
        //
        ++(*I->second);
      } else {      
        // SLOW PATH ------------------------------------------------------------
        //
        
        // HistogramEntry is not in fast look-up map so we have to create and install
        // a new one via the following method call.
        //
        HistogramEntry* entry = new_histogram_entry(idx);
        entry->inc();
      }
    }
    
    void
    Histogram::inc(uint32 idx, uint32 val)
    {      
      std::map<uint32,uint32*>::const_iterator I;
      if ((I = hist_map_index_value_.find(idx)) != hist_map_index_value_.end()) {
        // FAST PATH ------------------------------------------------------------
        //
        
        // Hit in fast lookup map
        //
        (*I->second) += val;
      } else {
        // SLOW PATH ------------------------------------------------------------
        //
        
        // HistogramEntry is not in fast look-up map so we have to create and install
        // a new one via the following method call.
        //
        HistogramEntry* entry = new_histogram_entry(idx);
        entry->inc(val);      
      }
    }
    
    // Reset everything to 0
    //
    void
    Histogram::clear()
    {
      for (std::set<HistogramEntry*>::const_iterator I = hist_entries_.begin(),
           E = hist_entries_.end();
           I != E; ++I)
      {
        (*I)->clear();
      }
    }
    
    // Sum all 'bars' in histogram
    //
    uint64
    Histogram::get_total(void) const
    {
      uint64 total = 0;
      for (std::set<HistogramEntry*>::iterator
           I = hist_entries_.begin(),
           E = hist_entries_.end();
           I != E; ++I)
      {
        total += (*I)->get_value();
      }
      return total;
    }
    
    std::string
    Histogram::to_string()
    {
      std::set<HistogramEntry*,HistogramEntryComparator> hist_entries_sorted(hist_entries_.begin(),
                                                                             hist_entries_.end());
      
      std::ostringstream buf;
      
      buf << "[HIST:" << name_ << "]";
      
      for (std::set<HistogramEntry*>::iterator
           I = hist_entries_sorted.begin(),
           E = hist_entries_sorted.end();
           I != E; ++I)
      {
        buf << "\n\t" << (*I)->to_string();
      }
      return buf.str();
    }
    
    // Protected set_name() method
    //
    void
    Histogram::set_name(const char *name)
    {
      uint32 i;
      // init name
      //
      for (i = 0; i < kHistogramMaxNameSize - 1 && name[i]; ++i)
        name_[i] = static_cast<uint8>(name[i]);
      name_[i] = '\0';      
    }

    // Protected no args Histogram constructor
    //
    Histogram::Histogram()
    : id_(kInitialHistogramId),
      alloc_size_(kHistogramEntryDefaultAllocSize)
    {
      name_[0] = '\0';
    }

    // Public Histogram constructor using automatic unique ID injection
    //
    Histogram::Histogram(const char* name, uint32 alloc_size)
    : id_(kInitialHistogramId),
      alloc_size_(alloc_size)
    { 
      uint32 i;
      // init name
      //
      for (i = 0; i < kHistogramMaxNameSize - 1 && name[i]; ++i)
        name_[i] = static_cast<uint8>(name[i]);
      name_[i] = '\0';      
    }
    
    Histogram::~Histogram()
    {
      // Remove all dynamically allocated histogram chunks
      //
      while (!hist_entries_alloc_chunks_.empty()) {
        HistogramEntry* entries = hist_entries_alloc_chunks_.top();
        delete [] entries;
        hist_entries_alloc_chunks_.pop();
      }
    }
    
    // -------------------------------------------------------------------------
    // HistogramIter
    //
    HistogramIter::HistogramIter(const Histogram& h)
    : hist(h)
    { 
      entries.insert(hist.hist_entries_.begin(), hist.hist_entries_.end());
      iter = entries.begin();
    }

    HistogramIter::HistogramIter(Histogram * const h)
    : hist(*h)
    { 
      entries.insert(hist.hist_entries_.begin(), hist.hist_entries_.end());
      iter = entries.begin();
    }
    
    HistogramIter::HistogramIter(const HistogramIter& hi)
    : hist(hi.hist)
    { 
      entries.insert(hist.hist_entries_.begin(), hist.hist_entries_.end());
      iter = entries.begin();
    }    

    HistogramIter::HistogramIter(HistogramIter * const hi)
    : hist(hi->hist)
    { 
      entries.insert(hist.hist_entries_.begin(), hist.hist_entries_.end());
      iter = entries.begin();
    }    

    void
    HistogramIter::operator++()
    {
      ++iter;
    }
    
    const HistogramEntry*
    HistogramIter::operator*()
    {
      return *iter;
    }
    
    bool
    HistogramIter::is_end()
    {
      return iter == entries.end();
    }
    
    bool
    HistogramIter::is_begin()
    {
      return iter == entries.begin();
    }

    // -------------------------------------------------------------------------
    // SortedHistogramValueIter
    //
    
    // -------------------------------------------------------------------------
    // HistogramEntryValueComparator implementation - based on value
    //
    struct HistogramEntryValueComparator
    {
      bool operator() (const HistogramEntry* lhs,
                       const HistogramEntry* rhs) const
      { return lhs->get_value() > rhs->get_value() ; }
    } HistogramEntryValueComparator;
        
    SortedHistogramValueIter::SortedHistogramValueIter(const Histogram& h)
    : hist(h)
    { 
      entries.insert(entries.begin(), hist.hist_entries_.begin(), hist.hist_entries_.end());
      std::sort(entries.begin(), entries.end(), HistogramEntryValueComparator);
      iter = entries.begin();
    }
    
    SortedHistogramValueIter::SortedHistogramValueIter(Histogram * const h)
    : hist(*h)
    { 
      entries.insert(entries.begin(), hist.hist_entries_.begin(), hist.hist_entries_.end());
      std::sort(entries.begin(), entries.end(), HistogramEntryValueComparator);
      iter = entries.begin();
    }
    
    SortedHistogramValueIter::SortedHistogramValueIter(const SortedHistogramValueIter& hi)
    : hist(hi.hist)
    { 
      entries.insert(entries.begin(), hist.hist_entries_.begin(), hist.hist_entries_.end());
      std::sort(entries.begin(), entries.end(), HistogramEntryValueComparator);
      iter = entries.begin();
    }    
    
    SortedHistogramValueIter::SortedHistogramValueIter(SortedHistogramValueIter * const hi)
    : hist(hi->hist)
    { 
      entries.insert(entries.begin(), hist.hist_entries_.begin(), hist.hist_entries_.end());
      std::sort(entries.begin(), entries.end(), HistogramEntryValueComparator);
      iter = entries.begin();
    }    
    
    void
    SortedHistogramValueIter::operator++()
    {
      ++iter;
    }
    
    const HistogramEntry*
    SortedHistogramValueIter::operator*()
    {
      return *iter;
    }
    
    bool
    SortedHistogramValueIter::is_end()
    {
      return iter == entries.end();
    }
    
    bool
    SortedHistogramValueIter::is_begin()
    {
      return iter == entries.begin();
    }


} } // namespace arcsim::util
