//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================


#include "util/Zone.h"

#include "Assertion.h"

#include "util/Util.h"
#include "util/Log.h"
#include "util/Allocate.h"

namespace arcsim {
  namespace util {
    
    // ------------------------------------------------------------------------
    // Chunks of memory have their start address encoded in the 'this' pointer
    // and a size in bytes. NOTE that chunks are allocated using 'Malloced::New()'
    // and freed with 'Malloced::Delete()'.
    //
    class Chunk {
    public:
      void Init(Chunk* next, int size) {
        next_ = next;
        size_ = size;
      }
      
      Chunk* next()     const { return next_; }
      int    size()     const { return size_; }
      int    capacity() const { return size_ - sizeof(Chunk); }
      
      // Compute address offsets accounting for 'next_' and 'size_'
      Address begin() const { return offset(sizeof(Chunk)); }
      Address end()   const { return offset(size_); }
      
    private:
      // compute offset
      Address offset(int n) const {
        return Address(this) + n;
      }
      
      Chunk* next_;
      int    size_;
    };

    
    // ------------------------------------------------------------------------
    // Zone
    //
    Zone::Zone()
    : pos_(0),
      end_(0),
      head_(0)
    { /* EMPTY */ }
    
    
    void
    Zone::Delete()
    {
      Chunk* cur = head_;
      while (cur != 0) {
        Chunk* nxt = cur->next();
        Malloced::Delete(cur);
        cur = nxt;
      }
      pos_ = end_ =  0;
    }
    
    void*
    Zone::New(int size)
    { 
      size = round_up(size, kAlignment); // round up requested size to fit alignment
      Address result = pos_;
      if (size > end_ - pos_) { // we requested more than we have, allocate new Chunk
        result = NewExpand(size);
      } else {                  // bump pointer
        pos_ += size;
      }
      return result;
    }
    
    // create new chunk at front of chain of chunks
    //
    Chunk*
    Zone::NewChunk(int size)
    {
      Chunk* chunk = reinterpret_cast<Chunk*>(Malloced::New(size));
      if (chunk != 0) {
        chunk->Init(head_, size);
        head_ = chunk;
      }
      return chunk;
    }
    
    Address
    Zone::NewExpand(int size)
    { 
      static const int kChunkHeaderSize = sizeof(Chunk) + kAlignment;
      
      int old_size = (head_ != 0) ? head_->size() : 0 ;
      // double Chunk allocation size incrementally until we reach a certain limit
      int new_size = size + (old_size << 1);
            
      // do not allocate something that is too small
      if (new_size < kMinimumChunkSize) {
        new_size = kMinimumChunkSize;
      } else if (new_size > Zone::kMaximumChunkSize) {
        // do not allocate something that is too large unless specifically requested
        new_size = max(kChunkHeaderSize + size, kMaximumChunkSize);
      }
      // allocate chunk
      Chunk*  chunk = NewChunk(new_size);
      // align 'top_' and 'end_' according to new Chunk
      Address start = round_up(chunk->begin(), kAlignment);
      pos_ = start + size;
      end_ = chunk->end();      
      ASSERT(pos_ <= end_);
      return start;
    }
    
 
} } // namespace arcsim::util
