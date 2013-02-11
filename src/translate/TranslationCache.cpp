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

#include "define.h"

#include "Assertion.h"

#include "translate/TranslationCache.h"

#include "util/Allocate.h"

TranslationCache::TranslationCache()
  : size_(0), cache_(0)
{ /* EMPTY */ }

TranslationCache::~TranslationCache()
{
  if (cache_) arcsim::util::Malloced::Delete(cache_);
}


void
TranslationCache::construct (uint32 size)
{
  ASSERT(IS_POWER_OF_TWO(size) && "TranslationCache: Initial Capacity not power of two!");
  
  size_ = size;
  
  // Allocate cache
  //
  cache_ = (Entry*)arcsim::util::Malloced::New(size_ * sizeof(cache_[0]));
  
  // Clear out dcode cache
  //
  purge();
}


