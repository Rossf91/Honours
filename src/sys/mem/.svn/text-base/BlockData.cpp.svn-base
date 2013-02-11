//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                            
// =====================================================================
//
// Description:  BlockData is a meta-data structure encapsulating blocks
//               of simulated memory. It is important to keep the size of
//               this class as small as possible.
//
// =====================================================================

#include "sys/mem/BlockData.h"

namespace arcsim {
  namespace sys {
    namespace mem {
      // -----------------------------------------------------------------------
      // Constructor creating a BlockData object backed by self managed memory
      //
      BlockData::BlockData(uint32   p_frame,
                           uint32*  p_block)
      : block_(p_block),
        page_frame(p_frame),
        x_cached_state_(false),
        w_cached_state_(false),
        type_tag(kMemoryTypeTagRam)
      { /* EMPTY */ }

      
      // -----------------------------------------------------------------------
      // Constructor creating a BlockData object backed by a MemoryDevice
      //
      BlockData::BlockData(uint32                              p_frame,
                           arcsim::mem::MemoryDeviceInterface* mem_dev)
      : block_(mem_dev),
        page_frame(p_frame),
        x_cached_state_(false),
        w_cached_state_(false),
        type_tag(kMemoryTypeTagDev)
      { /* EMPTY */ }
      
      // Destructor
      //
      BlockData::~BlockData()
      { /* EMPTY */ }
      
      
} } } // sys::mem
