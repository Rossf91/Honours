//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// Class providing details about internal page chunk architecture and
// convenience methods for modification. 
//
// =====================================================================

#ifndef INC_ARCH_PAGEARCH_H_
#define INC_ARCH_PAGEARCH_H_

#include "api/types.h"

// Supported page architecture sizes
// params -> (str,siz,siz_log2)
#define PAGE_ARCH_SIZE_LIST(V) \
  V(512B,  512, 9) \
  V(1K,   1024,10) \
  V(2K,   2048,11) \
  V(4K,   4096,12) \
  V(8K,   8192,13) \
  V(16K, 16384,14) \

// -----------------------------------------------------------------------------
// PageArch Class
//
class PageArch
{
public:
  // Supported page sizes
  enum PageSize {
#define DECL_SIZE(str_,siz_,siz_log2_) k##str_##PageSize = siz_,
    PAGE_ARCH_SIZE_LIST(DECL_SIZE)
#undef DECL_SIZE
  };

  // Supported page sizes log2
  enum PageSizeLog2 {
#define DECL_SIZE_LOG2(str_,siz_,siz_log2_) k##str_##PageSizeLog2 = siz_log2_,
    PAGE_ARCH_SIZE_LIST(DECL_SIZE_LOG2)
#undef DECL_SIZE_LOG2
  };
  
  // NOTE: ORDER OF DECLARATION AND INITIALISATION MATTERS IN THIS CASE!
  //       COMMENTS INDICATE ORDER!
  
  // Bit width configuration used for page cache in terms of sub address field sizes
  //
  /* 01 */ const uint32 byte_bits;
  /* 02 */ const uint32 half_bits;
  /* 03 */ const uint32 offset_bits;
  /* 04 */ const uint32 index_bits;
  
  // Number of valid tag bits in each page cache tag field
  //
  /* 05 */ const uint32 page_tag_bits;
  
  // Capacity of each physical memory page, in words, half-words and bytes
  //
  /* 06 */ const uint32 page_words;
  /* 07 */ const uint32 page_halfs;
  /* 08 */ const uint32 page_bytes;
  
  // Capacity of each page measured in page entries
  //
  /* 09 */ const uint32 page_cache_size;
  
  // Bit shifts needed to extract INDEX from byte and word address
  //
  /* 10 */ const uint32 byte_index_shift;
  /* 11 */ const uint32 word_index_shift;
  /* 12 */ const uint32 byte_tag_shift;
  /* 13 */ const uint32 word_tag_shift;
  
  // Masks to select OFFSET_BITS, INDEX_BITS etc.
  //
  /* 14 */ const uint32 page_word_offset_mask;
  /* 15 */ const uint32 page_half_offset_mask;
  /* 16 */ const uint32 page_byte_offset_mask;
  
  /* 17 */ const uint32 page_byte_frame_mask;
  /* 18 */ const uint32 page_index_mask;
  
  /* 19 */ const uint32 memory_type_tag_iom;
  
  PageArch(uint32 size);
  ~PageArch();
  
  // ---------------------------------------------------------------------------
  // Methods to select the page INDEX from a byte address or a word address
  //
  inline uint32 page_byte_index(const uint32 addr) const
  {
    return (((addr) >> byte_index_shift) & page_index_mask);
  }
  
  inline uint32 page_word_index(const uint32 addr) const
  {
    return (((addr) >> word_index_shift) & page_index_mask);
  }
  // ---------------------------------------------------------------------------
  // Methods to select the page TAG from a byte address or a word address
  //
  inline uint32 page_byte_tag(const uint32 addr) const
  {
    return (addr >> byte_tag_shift);
  }
  
  inline uint32 page_word_tag(const uint32 addr) const
  {
    return (addr >> word_tag_shift);
  }
  inline uint32 page_iom_tag(const uint32 addr) const
  {
    return (addr | memory_type_tag_iom);
  }
  
  // ---------------------------------------------------------------------------
  // Methods to select the page FRAME from a byte address or a word address
  //
  inline uint32 page_byte_frame(const uint32 addr) const
  {
    return (addr >> byte_index_shift) << byte_index_shift;
  }
  inline uint32 page_word_frame(const uint32 addr) const
  {
    return (addr >> word_index_shift) << word_index_shift;
  }
  
  // ---------------------------------------------------------------------------
  // Methods to compute the offset into a page
  //
  inline uint32 page_offset_byte_index(const uint32 addr) const
  {
    return (addr & page_byte_offset_mask);
  }
  
  inline uint32 page_offset_half_index(const uint32 addr) const
  {
    return ((addr >> half_bits) & page_half_offset_mask);
  }
  
  inline uint32 page_offset_word_index(const uint32 addr) const
  {
    return ((addr >> byte_bits) & page_word_offset_mask);
  }  
};

#endif  // INC_ARCH_PAGEARCH_H_
