//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================


#include "isa/arc/DcodeCache.h"

#include "Assertion.h"

#include "util/Allocate.h"

namespace arcsim {
  namespace isa {
    namespace arc {

      DcodeCache::DcodeCache()
      : size_(0), cache_(0)
      { /* EMPTY */ }

      DcodeCache::~DcodeCache()
      {
        if (cache_) arcsim::util::Malloced::Delete(cache_);
      }

      void DcodeCache::construct (uint32 size)
      {
        ASSERT(IS_POWER_OF_TWO(size) && "DcodeCache capacity not power of two!");
        size_  = (size >> 1); // an Entry holds two ways hence we divide size by two
        cache_ = (Entry*)arcsim::util::Malloced::New(size_ * sizeof(Entry));
        purge(); // clear out dcode cache
      }
      
} } } //  arcsim::isa::arc

