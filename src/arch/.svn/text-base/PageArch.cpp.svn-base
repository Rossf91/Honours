//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//   Class providing details about internal page chunk architecture and
//   convenience methods for modification. 
//
//   The PageArch class is also used in dynamically-generated
//   code sequences which access the page translation cache.
//   
//   A physical address is partitioned into the following four
//   fields during the process of looking-up the address in the
//   page caches for Read, Write and Execute accesses.
//   
//   31                  <-->              13 12          2 1  0
//   +--------------------+------------------+-------------+---+
//   |   PAGE_TAG_BITS    |    INDEX_BITS    | OFFSET_BITS | 2 |
//   +--------------------+------------------+-------------+---+
//   |
//   For correct operation INDEX_BITS >= 6      BYTE_BITS --+
//   
//   00-PAGE_INDEX_MASK-0 1111111111111111111 00000000000000 00
//   0-PAGE_OFFSET_MASK-0 0000000000000000000 11111111111111 00
//   
//   
//   Each page cache is direct-mapped and contains 2**INDEX_BITS
//   entries.
//   
//   Each page contains PAGE_WORDS words, or PAGE_BYTES bytes.
//   These are defined by:
//   
//   PAGE_WORDS = 2**OFFSET_BITS
//   PAGE_BYTES = 2**(OFFSET_BITS + BYTE_BITS)
//   
//   The nominal page size is 8 KB, so OFFSET_BITS = 11.
//   
//   Note that PAGE_OFFSET_BITS+PAGE_INDEX_BITS must be strictly
//   greater than 16, to allow at least one unused upper bit
//   in each 16-bit page cache tag.
//
//
// =====================================================================

#include "define.h"

#include "Assertion.h"

#include "arch/PageArch.h"

#include "sys/mem/types.h"

PageArch::PageArch(uint32 page_size_log2)
:
  // NOTE: ORDER OF DECLARATION AND INITIALISATION MATTERS IN THIS CASE!
  //
  half_bits     (1),

  byte_bits     (2),                          // @see above documentation
  offset_bits   (page_size_log2 - byte_bits), // @see above documentation
  index_bits    (12),                         // @see above documentation
  page_tag_bits (32 - index_bits - offset_bits - byte_bits ),// @see above documentation
  // Capacity configuration
  //
  page_words(1 << offset_bits),
  page_halfs(2 << offset_bits),
  page_bytes(4 << offset_bits),

  page_cache_size(1 << index_bits),

  // Byte shifts needed to extract page TAG and INDEX fields
  //
  byte_index_shift(byte_bits + offset_bits),
  word_index_shift(offset_bits),
  byte_tag_shift  (byte_bits   + offset_bits + index_bits),
  word_tag_shift  (offset_bits + index_bits),
  //  Mask to select only OFFSET_BITS field, after shifting out the
  //  BYTE_BITS field to the right.
  //
  page_word_offset_mask(page_words - 1),
  page_half_offset_mask(page_halfs - 1),
  page_byte_offset_mask(page_bytes - 1),

  page_byte_frame_mask(~page_byte_offset_mask),

  // Mask to select only the INDEX_BITS field, after shifting out
  // the OFFSET_BITS and BYTE_BITS fields to the right.
  //
  page_index_mask(page_cache_size - 1),

  memory_type_tag_iom(kMemoryTypeTagDev)
  { 
    ASSERT((index_bits >= 6) && "For correct operation INDEX_BITS >= 6!");
    ASSERT(((offset_bits + index_bits) > 16) && "Page ((offset_bits + index_bits) > 16)!");
    ASSERT((page_size_log2 >= k512BPageSizeLog2) && (page_size_log2 <= k16KPageSizeLog2));
  }

PageArch::~PageArch()
{ /* EMPTY */ }


