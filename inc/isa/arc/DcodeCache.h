//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Two way set associative DcodeCache implementation using LRU replacement
// policy.
//
// We are using the PC to index into the cache. Because the PC is half-word
// aligned, the least significant bit of the PC is always zero. Hence we
// right shift the PC by 1 before using it to compute an index into the
// DecodeCache.
//
// =====================================================================

#ifndef INC_ISA_ARC_DCODECACHE_H_
#define INC_ISA_ARC_DCODECACHE_H_

#include "Globals.h"

#include "api/types.h"
#include "isa/arc/Dcode.h"

namespace arcsim {
  namespace isa {
    namespace arc {

      class DcodeCache
      {
      public:
        // Cache hit types
        enum HitType {
          kCacheHitWay0 = 0x0, // NOTE: kCacheHitWay0 must be 0, lru policy depends on this
          kCacheHitWay1 = 0x1, // NOTE: kCacheHitWay1 must be 1, lru policy depends on this
          kCacheMiss    = 0x2,
        };

        // Entry structure
        struct Entry {
          uint8                     lru_way_;   // maintain LRU way per Entry
          uint32                    way0_pc_;
          arcsim::isa::arc::Dcode   way0_inst_;
          uint32                    way1_pc_;
          arcsim::isa::arc::Dcode   way1_inst_;
        };

        DcodeCache();
        ~DcodeCache();

        void construct(uint32 size);

        // -------------------------------------------------------------------------
        // Lookup and return location of Dcode object in cache. Set cache HitType
        // accordingly so the caller knows if the returned location is for a cache
        // hit, or a miss. Flag returned location as valid for the next lookup in
        // cache.
        //
        inline arcsim::isa::arc::Dcode* lookup (uint32 pc, HitType*  hit_type)
        { // Least significant bit of PC is always zero so we right shift: (pc >> 1)
          Entry* const p    = cache_ + ((pc >> 1) & (size_ - 1));

          if (p->way0_pc_ == pc) {                      // hit in WAY 0 ------------
            *hit_type   = kCacheHitWay0;
            p->lru_way_ = kCacheHitWay1;
            return &(p->way0_inst_);
          }

          if (p->way1_pc_ == pc) {                      // hit in WAY 1 ------------
            *hit_type   = kCacheHitWay1;
            p->lru_way_ = kCacheHitWay0;
            return &(p->way1_inst_);
          }

          // CACHE  MISS -----------------------------------------------------------
          *hit_type = kCacheMiss;

          // WAY 1 is least recently used
          if (p->lru_way_) {
            p->lru_way_ = kCacheHitWay0;
            p->way1_pc_ = pc;        // flag way as valid
            return &(p->way1_inst_); // return victim
          }
          // WAY 0 is least recently used
          p->lru_way_ = kCacheHitWay1;
          p->way0_pc_ = pc;          // flag way as valid
          return &(p->way0_inst_);   // return victim
        }

        // ---------------------------------------------------------------------------
        // Purge methods
        //
        inline void purge_entry (uint32 pc)
        { // Least significant bit of PC is always zero so we right shift: (pc >> 1)
          Entry * const p = cache_ + ((pc >> 1) & (size_ - 1));
          p->way0_pc_   = kInvalidPcAddress;
          p->way1_pc_   = kInvalidPcAddress;
        }

        inline void purge ()
        {
          Entry const * const end = cache_end();
          for (Entry * p = cache_; p < end; ++p) {
            p->lru_way_ = 0;
            p->way0_pc_ = kInvalidPcAddress;
            p->way1_pc_ = kInvalidPcAddress;
          }
        }

      private:
        uint32 size_;        // size of cache
        Entry* cache_;

        Entry* cache_end() const { return cache_ + size_; }

        DcodeCache(const DcodeCache & m);     // DO NOT COPY
        void operator=(const DcodeCache &);   // DO NOT ASSIGN
      };

    } } } //  arcsim::isa::arc


#endif /* INC_ISA_ARC_DCODECACHE_H_ */
