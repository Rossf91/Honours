//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// CCM management class declaration.
//
// =====================================================================


#ifndef INC_PROCESSOR_CCMMANAGER_H_
#define INC_PROCESSOR_CCMMANAGER_H_

#include <map>
#include <utility>

#include "api/types.h"
#include "arch/IsaOptions.h"
#include "arch/CoreArch.h"
#include "arch/SpadArch.h"


// -------------------------------------------------------------------------
// Forward declarations
//
class SimOptions;

namespace arcsim {
  
  // -------------------------------------------------------------------------
  // Forward declarations
  //
  namespace mem {
    class MemoryDeviceInterface;
    class DirectMemoryAccessDeviceInterface;
  }
    
  namespace sys {
    
    // ----------------------------------------------------------------------
    // Forward declarations
    //
    namespace mem {
      class BlockData;
    }
    
    namespace cpu {
      // ----------------------------------------------------------------------
      // CcmManager class
      // 
      class CcmManager
      {
      public:
        static const uint32 kMemoryRegionCount = 16;
        static const uint32 kMemoryRegionHalf  = 8;
        
        // ------------------------------------------------------------------
        // CcmMemoryKind - kind indicating how CCMs should be modelled internally.
        //
        enum CcmMemoryKind {
          kCcmMemoryDevice             = 0x0,
          kCcmDirectMemoryAccessDevice = 0x1
        };

        const CcmMemoryKind ccm_mem_kind_;
        
      private:
        // References to core configuration and ISA option classes
        //
        const CoreArch&   core_arch_;
        const IsaOptions& isa_opts_;
        const SimOptions& sim_opts_;
        
        
        // ------------------------------------------------------------------
        // CcmMappingKind - type indicating how address should be mapped onto
        //                  CCMs.
        //
        enum CcmMappingKind {
          kCcmMappingKindDirect         = 0x0,  // Direct address to CCM mapping (A600)
          kCcmMappingKindRegion         = 0x1,  // Address to region CCM mapping (A6k)
          kCcmMappingKindMultipleIccms  = 0x2,  // Slow Code memories (ARCv2.1, EM1.1)
        };
        
        const CcmMappingKind addr_mapping_kind_;
                
        // ------------------------------------------------------------------
        // Instantiated pointers to CCM devices
        //
        arcsim::mem::MemoryDeviceInterface * i_ccm_dev_; // instruction CCM
        arcsim::mem::MemoryDeviceInterface * d_ccm_dev_; // data CCM
        arcsim::mem::MemoryDeviceInterface * i_ccms_dev_[4]; // instruction CCMs

        // ------------------------------------------------------------------
        // Instantiated pointers to DMA CCMs
        //
        arcsim::mem::DirectMemoryAccessDeviceInterface * i_ccm_dma_; // instruction CCM
        arcsim::mem::DirectMemoryAccessDeviceInterface * d_ccm_dma_; // data CCM
        arcsim::mem::DirectMemoryAccessDeviceInterface * i_ccms_dma_[4]; // instruction CCMs
        
        // ------------------------------------------------------------------
        // Map holding references to BlockData instances encapsulating CCM
        // devices.
        //  
        std::map<uint32,arcsim::sys::mem::BlockData*> ccm_blocks;
        
        // ------------------------------------------------------------------
        // Memory region map to potential CCMs, data structure used for
        // 'kCcmMappingKindRegion' to quickly determine what CCM is responsible
        // for a given region.
        //
        arcsim::mem::MemoryDeviceInterface*              mem_dev_region_map_[kMemoryRegionCount];
        arcsim::mem::DirectMemoryAccessDeviceInterface*  mem_dma_region_map_[kMemoryRegionCount];
        
        std::pair<uint32,uint32> iccm_rgn_size_[4];

        // ICCM and DCCM ranges
        //
        std::pair<uint32,uint32> iccm_range_;
        std::pair<uint32,uint32> dccm_range_;
        
      public:        
        
        explicit CcmManager(CoreArch&   core_arch,
                            IsaOptions& isa_opts,
                            SimOptions& sim_opts);
        ~CcmManager();
        
        // ------------------------------------------------------------------
        // Configure CCM Manager based on CoreArch configuration
        //
        bool configure();
        
        // ------------------------------------------------------------------
        // When CCMs are registered programmatically we need to be able to
        // re-create them properly.
        //
        void create_or_replace_iccm();
        void create_or_replace_dccm();

        // ------------------------------------------------------------------
        // Efficiently query CCM availability
        //
        inline bool is_ccm_enabled()  const { return (core_arch_.spad_types != SpadArch::kNoSpad);     }
        inline bool is_iccm_enabled() const { return (core_arch_.spad_types & SpadArch::kSpadInstCcm); }
        inline bool is_dccm_enabled() const { return (core_arch_.spad_types & SpadArch::kSpadDataCcm); }
        
        // ------------------------------------------------------------------
        // Efficiently compute region index and region base address
        //
        inline uint32 get_memory_region_base(uint32 addr) const
        { 
          return ((addr)  &  (0xF << ((isa_opts_.addr_size) - 4)));
        }
        
        inline uint32 get_memory_region_index(uint32 addr) const
        {
          return (((addr) >> ((isa_opts_.addr_size) - 4)) & 0xF);
        }
        
        // ------------------------------------------------------------------
        // If address 'hits' into a CCM, return true
        //
        bool in_ccm_mapped_region(uint32 addr) const;
        
        // ------------------------------------------------------------------
        // If address 'hits' into a CCM, return the a valid instance of BlockData,
        // otherwise '0' is returned
        //
        arcsim::sys::mem::BlockData* get_host_page(uint32 addr);
        
      };
      
} } } // arcsim::sys::cpu

#endif // INC_PROCESSOR_CCMMANAGER_H_
