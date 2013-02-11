//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Translation lookup cache implementation for faster lookup of JIT translated
// blocks.
//
// =====================================================================

#ifndef INC_TRANSLATE_TRANSLATIONCACHE_H_
#define INC_TRANSLATE_TRANSLATIONCACHE_H_

#include "api/types.h"

#include "translate/Translation.h"

class TranslationCache
{
public:

  // Translation cache entry
  //
  struct Entry {
    uint32            addr_;
    TranslationBlock  code_;
  };

  // Invalid simulated address
  //
  static const uint32 kInvalidAddr = 0x1;
  
  TranslationCache();
  ~TranslationCache();
  
  void construct (uint32 size);
      
  // ---------------------------------------------------------------------------
  // Lookup if native code exists in cache
  //
  inline bool lookup (uint32 addr, TranslationBlock* code) const
  {
    Entry* p = cache_ + ((addr >> 1)& (size_ - 1));
    if (p->addr_ == addr) {                            // CACHE HIT ------------
      *code = p->code_;
      return true;
    }
                                                       // CACHE MISS -----------
    *code  = 0;
    return false;
  }
  
  // ---------------------------------------------------------------------------
  // Modify cache content
  //
  inline void update (uint32 addr, TranslationBlock block)
  {
    Entry* p = cache_ + ((addr >> 1)& (size_ - 1));
    p->addr_ = addr;
    p->code_ = block;
  }

  void purge ()
  { 
    const Entry * end = cache_end();
    for (Entry * p = cache_; p < end; ++p) {
      p->addr_  = kInvalidAddr;
    }
  }

  
private:  
  uint32    size_;   // translation cache size
  
  Entry*    cache_;
  
  Entry* cache_end() const { return cache_ + size_; }
  
  TranslationCache(const TranslationCache & m);   // DO NOT COPY
  void operator=(const TranslationCache &);       // DO NOT ASSIGN  
};


#endif /* INC_TRANSLATE_TRANSLATIONCACHE_H_ */
