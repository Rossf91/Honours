//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// CCM management class implementation.
//
// =====================================================================


#include <iomanip>

#include "Assertion.h"

#include "sys/cpu/CcmManager.h"

#include "arch/PageArch.h"
#include "arch/IsaOptions.h"
#include "arch/SimOptions.h"

#include "sys/mem/BlockData.h"

#include "mem/dma/DMACloselyCoupledMemoryDevice.h"
#include "mem/ccm/CCMemoryDevice.h"

#include "util/Log.h"

namespace arcsim {
  namespace sys {
    namespace cpu {

      // -----------------------------------------------------------------------
      // Constructor
      //
      CcmManager::CcmManager(CoreArch&    core_arch,
                             IsaOptions&  isa_opts,
                             SimOptions&  sim_opts)
      : ccm_mem_kind_((isa_opts.is_ccm_debug_enabled) 
                                ? kCcmMemoryDevice
                                : kCcmDirectMemoryAccessDevice),
        core_arch_(core_arch),
        isa_opts_(isa_opts),
        sim_opts_(sim_opts),
        addr_mapping_kind_((isa_opts.is_isa_a6k()) 
                              ? ((isa_opts.multiple_iccms)
                                  ? kCcmMappingKindMultipleIccms
                                  : kCcmMappingKindRegion)
                              : kCcmMappingKindDirect),
        i_ccm_dev_(0),
        d_ccm_dev_(0),
        i_ccm_dma_(0),
        d_ccm_dma_(0)
      {
        iccm_range_.first = iccm_range_.second = 0;
        dccm_range_.first = dccm_range_.second = 0;
        for (int i = 0; i < IsaOptions::kMultipleIccmCount; ++i) {
          i_ccms_dev_[i] = 0;
          i_ccms_dma_[i] = 0;
          iccm_rgn_size_[i].first  = 0;
          iccm_rgn_size_[i].second = 0;
        }
      }
      
      // -----------------------------------------------------------------------
      // Destructor
      //
      CcmManager::~CcmManager()
      { 
        // Free BlockData objects that were potentially allocated for CCM regions
        //
        for (std::map<uint32,arcsim::sys::mem::BlockData*>::iterator
             I = ccm_blocks.begin(),
             E = ccm_blocks.end();
             I != E; ++I)
        {
          if (I->second) { delete I->second; I->second = 0; }
        }
        
        if (i_ccm_dev_) { delete i_ccm_dev_; i_ccm_dev_ = 0; }
        if (d_ccm_dev_) { delete d_ccm_dev_; d_ccm_dev_ = 0; }
        if (i_ccm_dma_) { delete i_ccm_dma_; i_ccm_dma_ = 0; }
        if (d_ccm_dma_) { delete d_ccm_dma_; d_ccm_dma_ = 0; }
      }

      
      // ------------------------------------------------------------------
      // Configure CCM Manager based on CoreArch configuration
      //
      bool CcmManager::configure()
      {
        bool   success      = true;
        
        // Early exit if CCMs are not configured at all
        //
        if (!is_ccm_enabled())  return success;

        // Create and configure CCMs
        //
        if (is_iccm_enabled() && core_arch_.iccm.is_configured) {
          create_or_replace_iccm();
          iccm_range_.first  = core_arch_.iccm.start_addr;
          iccm_range_.second = core_arch_.iccm.start_addr + core_arch_.iccm.size - 1;
        }

        if (is_dccm_enabled() && core_arch_.dccm.is_configured) {
          create_or_replace_dccm();
          dccm_range_.first  = core_arch_.dccm.start_addr;
          dccm_range_.second = core_arch_.dccm.start_addr + core_arch_.dccm.size - 1;
        }
        
        switch (addr_mapping_kind_)
        {
          // -------------------------------------------------------------------
          //
          case kCcmMappingKindRegion:
          {
            LOG(LOG_INFO) << "[CCMManager] Using Address to Region CCM mapping strategy.";

            // Address to region CCM mapping
            //
            uint32 region_begin = 0;
            uint32 region_end   = 0;

            // First we initiliase the memory region map
            //
            for (uint32 i = 0; i < kMemoryRegionCount; ++i) {
              mem_dev_region_map_[i] = 0;
              mem_dma_region_map_[i] = 0;
            }

            // Compute region(s) covered by ICCM if configured
            //
            if (is_iccm_enabled() && core_arch_.iccm.is_configured) {

              if (!core_arch_.icache.is_configured && !core_arch_.ifq_arch.is_configured) {
                // In the case where there is NO path to external memory (i.e. absence
                // of icache AND absence of instruction fetch queue), an ICCM has to
                // cover the first 8 regions from 0-7. Any hit within these regions will
                // hit into the ICCM and addresses wrap appropriately.
                //
                LOG(LOG_DEBUG) << "[CCMManager] No path to external memory, registering ICCM for ranges 0-7.";

                for (uint32 i = 0; i < 8; ++i) {
                  LOG(LOG_DEBUG) << "[CCMManager] Registering ICCM for memory region '" << i << "'.";
                  mem_dev_region_map_[i] = i_ccm_dev_;
                  mem_dma_region_map_[i] = i_ccm_dma_;
                }
              } else {
                // There is a path to external memory, hence we compute the region
                // this iccm is supposed to cover.
                //

                region_begin = get_memory_region_index(core_arch_.iccm.start_addr);
                region_end   = get_memory_region_index((get_memory_region_base(core_arch_.iccm.start_addr)
                                                        + core_arch_.iccm.size - 1));
                // Sanity check
                //
                ASSERT(region_end >= region_begin && region_end < kMemoryRegionCount
                       && "Mapping of ICCM onto regions failed!");
                // Registration of ICCM - ICCMs can only live BELOW MEMORY_REGION_HALF
                //
                if (region_begin < kMemoryRegionHalf) {
                  for (uint32 i = region_begin; i <= region_end; ++i) {
                    LOG(LOG_DEBUG) << "[CCMManager] Registering ICCM for memory region '"
                                   << i << "' with start address '0x"
                                   << std::hex << std::setw(8) << std::setfill('0')
                                   << core_arch_.iccm.start_addr << "'.";
                    mem_dev_region_map_[i] = i_ccm_dev_;
                    mem_dma_region_map_[i] = i_ccm_dma_;
                  }
                } else {
                  LOG(LOG_INFO) << "[CCMManager] Requested to map ICCM onto region '"
                                << region_begin << "' - ICCM UNREACHABLE.";
                }
              }
            }

            // Compute region(s) covered by DCCM if configured
            //
            if (is_dccm_enabled() && core_arch_.dccm.is_configured) {
              region_begin = get_memory_region_index(core_arch_.dccm.start_addr);
              region_end   = get_memory_region_index((get_memory_region_base(core_arch_.dccm.start_addr)
                                                      + core_arch_.dccm.size - 1));
              // Sanity check
              //
              ASSERT(region_end >= region_begin && region_end < kMemoryRegionCount
                     && "Mapping of DCCM onto regions failed!");
              // Registration of DCCM - DCCMs can only live ABOVE MEMORY_REGION_HALF
              //
              if (region_begin >= kMemoryRegionHalf) {
                for (uint32 i = region_begin; i <= region_end; ++i) {
                  LOG(LOG_DEBUG) << "[CCMManager] Registering DCCM for memory region '"
                                 << i << "' with start address '0x"
                                 << std::hex << std::setw(8) << std::setfill('0')
                                 << core_arch_.dccm.start_addr << "'.";
                  mem_dev_region_map_[i] = d_ccm_dev_;
                  mem_dma_region_map_[i] = d_ccm_dma_;
                }
              } else {
                LOG(LOG_INFO) << "[CCMManager] Requested to map DCCM onto region '"
                              << region_begin << "' - DCCM UNREACHABLE.";
              }
            }
            break;
          }
          // -------------------------------------------------------------------
          //
          case kCcmMappingKindDirect:
          {
            LOG(LOG_INFO) << "[CCMManager] Using Direct address to CCM mapping strategy.";

            // Configure direct address to CCM mapping
            //
            if (is_iccm_enabled() && core_arch_.iccm.is_configured) {
              LOG(LOG_DEBUG) << "[CCMManager] Registering ICCM with from '0x"
                             << std::hex << std::setw(8) << std::setfill('0')
                             << iccm_range_.first
                             << "' to '0x"
                             << std::hex << std::setw(8) << std::setfill('0')
                             << iccm_range_.second
                             << "'.";
            }
            if (is_dccm_enabled() && core_arch_.dccm.is_configured) {
              LOG(LOG_DEBUG) << "[CCMManager] Registering DCCM with from '0x"
                             << std::hex << std::setw(8) << std::setfill('0')
                             << dccm_range_.first
                             << "' to '0x"
                             << std::hex << std::setw(8) << std::setfill('0')
                             << dccm_range_.second
                             << "'.";
            }
            break;
          }
          // -------------------------------------------------------------------
          //
          case kCcmMappingKindMultipleIccms:
          {
            LOG(LOG_INFO) << "[CCMManager] Using Slow Code Memories CCM mapping strategy.";
            // First we initiliase the memory region map
            //
            for (uint32 i = 0; i < kMemoryRegionCount; ++i) {
              mem_dev_region_map_[i] = 0;
              mem_dma_region_map_[i] = 0;
            }
            // Create and configure ICCMs
            //
            for (int i = (IsaOptions::kMultipleIccmCount - 1); i >= 0 ; --i) {
              
              if (core_arch_.iccms[i].is_configured) {
                iccm_rgn_size_[i].first  = get_memory_region_index(core_arch_.iccm.start_addr);
                iccm_rgn_size_[i].second = core_arch_.iccms[i].size;
              }

              // There is a path to external memory, hence we compute the region
              // this iccm is supposed to cover.
              //
              uint32 region_begin = get_memory_region_index(core_arch_.iccms[i].start_addr);
              uint32 region_end   = get_memory_region_index((get_memory_region_base(core_arch_.iccms[i].start_addr)
                                                             + core_arch_.iccms[i].size - 1));
              // Sanity check
              //
              ASSERT(region_end >= region_begin && region_end < kMemoryRegionCount
                     && "Mapping of ICCM onto regions failed!");
              // Registration of ICCM - ICCMs can only live BELOW MEMORY_REGION_HALF
              //
              if (region_begin < kMemoryRegionHalf) {
                for (uint32 r = region_begin; r <= region_end; ++r) {
                  if(core_arch_.iccms[i].is_configured) {
                    LOG(LOG_DEBUG) << "[CCMManager] Registering ICCM" << i << " for memory region '"
                                   << r << "' with start address '0x"
                                   << std::hex << std::setw(8) << std::setfill('0')
                                   << core_arch_.iccms[i].start_addr << "'.";
                    mem_dev_region_map_[r] = i_ccms_dev_[i];
                    mem_dma_region_map_[r] = i_ccms_dma_[i];
                  }
                }
              } else {
                LOG(LOG_INFO) << "[CCMManager] Requested to map ICCM"<<i<<" onto region '"
                              << region_begin << "' - ICCM UNREACHABLE.";
              }
            }

            // Compute region(s) covered by DCCM if configured
            //
            if (is_dccm_enabled() && core_arch_.dccm.is_configured) {
              // Address to region CCM mapping
              uint32 region_begin = 0;
              uint32 region_end   = 0;
              region_begin = get_memory_region_index(core_arch_.dccm.start_addr);
              region_end   = get_memory_region_index((get_memory_region_base(core_arch_.dccm.start_addr)
                                                      + core_arch_.dccm.size - 1));
              // Sanity check
              //
              ASSERT(region_end >= region_begin && region_end < kMemoryRegionCount
                     && "Mapping of DCCM onto regions failed!");
              // Registration of DCCM - DCCMs can only live ABOVE MEMORY_REGION_HALF
              //
              if (region_begin >= kMemoryRegionHalf) {
                for (uint32 i = region_begin; i <= region_end; ++i) {
                  LOG(LOG_DEBUG) << "[CCMManager] Registering DCCM for memory region '"
                                 << i << "' with start address '0x"
                                 << std::hex << std::setw(8) << std::setfill('0')
                                 << core_arch_.dccm.start_addr << "'.";
                  mem_dev_region_map_[i] = d_ccm_dev_;
                  mem_dma_region_map_[i] = d_ccm_dma_;
                }
              } else {
                LOG(LOG_INFO) << "[CCMManager] Requested to map DCCM onto region '"
                              << region_begin << "' - DCCM UNREACHABLE.";
              }
            }
            break;
          }

          default: {
            LOG(LOG_ERROR) << "[CCMManager] Given invalid address mapping strategy";
            break;
          }
        } //End of switch
                
        return success;
      }
      
      // ------------------------------------------------------------------
      // When CCMs are registered programmatically we need to be able to
      // re-create them properly.
      //
      void
      CcmManager::create_or_replace_iccm()
      {
        switch (addr_mapping_kind_)
        {
          case kCcmMappingKindDirect:
          case kCcmMappingKindRegion:
          {
            switch (ccm_mem_kind_)
            {
              case kCcmMemoryDevice: {
                using namespace arcsim::mem::ccm;
                if (i_ccm_dev_) { delete i_ccm_dev_; }
                i_ccm_dev_ = new CCMemoryDevice("ICCM", core_arch_.iccm.start_addr,
                                                core_arch_.iccm.size);
                break;
              }
              case kCcmDirectMemoryAccessDevice: {
                using namespace arcsim::mem::dma;
                if (i_ccm_dma_) { delete i_ccm_dma_; }
                i_ccm_dma_ = new DMACloselyCoupledMemoryDevice("ICCM",
                                                               core_arch_.iccm.start_addr,
                                                               core_arch_.iccm.size);
                // call initialisation method on direct memory device
                i_ccm_dma_->dma_dev_init(sim_opts_.init_mem_value);
                break;
              }
            }
          }break;

          case kCcmMappingKindMultipleIccms:
          {
            switch (ccm_mem_kind_)
            {
              case kCcmMemoryDevice: {
                using namespace arcsim::mem::ccm;
                for (int i = 0; i < IsaOptions::kMultipleIccmCount; ++i) {
                  if (i_ccms_dev_[i])
                    delete i_ccms_dev_[i];
                  if (core_arch_.iccms[i].is_configured) {
                    i_ccms_dev_[i] = new CCMemoryDevice("ICCM",
                                                        core_arch_.iccms[i].start_addr,
                                                        core_arch_.iccms[i].size);
                  }
                }
                break;
              }
              case kCcmDirectMemoryAccessDevice: {
                using namespace arcsim::mem::dma;
                for(int i = 0; i < IsaOptions::kMultipleIccmCount; ++i) {
                  if (i_ccms_dma_[i])
                    delete i_ccms_dma_[i];
                  if (core_arch_.iccms[i].is_configured) {
                    i_ccms_dma_[i] = new DMACloselyCoupledMemoryDevice("ICCM",
                                                                       core_arch_.iccms[i].start_addr,
                                                                       core_arch_.iccms[i].size);
                    // call initialisation method on direct memory device
                    i_ccms_dma_[i]->dma_dev_init(sim_opts_.init_mem_value);
                  }
                }
                break;
              }
            }
          }
        }
      }
      
      void
      CcmManager::create_or_replace_dccm()
      { 
        switch (ccm_mem_kind_)
        {
          case kCcmMemoryDevice: {
            using namespace arcsim::mem::ccm;
            if (d_ccm_dev_) { delete d_ccm_dev_; }
            d_ccm_dev_ = new CCMemoryDevice("DCCM", core_arch_.dccm.start_addr,
                                            core_arch_.dccm.size);
            break;
          }
          case kCcmDirectMemoryAccessDevice: {
            using namespace arcsim::mem::dma;
            if (d_ccm_dma_) { delete d_ccm_dma_; }
            d_ccm_dma_ = new DMACloselyCoupledMemoryDevice("DCCM",
                                                           core_arch_.dccm.start_addr,
                                                           core_arch_.dccm.size);
            // call initialisation method on direct memory device
            d_ccm_dma_->dma_dev_init(sim_opts_.init_mem_value);
            break;
          }
        }
      }

      
      // -----------------------------------------------------------------------
      // If an address 'hits' into a CCM return true
      //
      bool
      CcmManager::in_ccm_mapped_region(uint32 addr) const
      {
        if (is_ccm_enabled())
        { // CCMs are present, determine mapping strategy
          //
          switch (addr_mapping_kind_)
          {
            case kCcmMappingKindDirect:
            {
              if (   iccm_range_.second
                  && (addr >= iccm_range_.first && addr < iccm_range_.second)) {
                return true;
              }
              if (   dccm_range_.second
                  && (addr >= dccm_range_.first && addr < dccm_range_.second)) {
                return true;
              }
              return false;
            } break;

            case kCcmMappingKindRegion:
            case kCcmMappingKindMultipleIccms:
            {
              // address to region CCM mapping (A6k)
              if (ccm_mem_kind_) {
                return (mem_dma_region_map_[get_memory_region_index(addr)] != 0);
              } else {
                return (mem_dev_region_map_[get_memory_region_index(addr)] != 0);
              }
            } break;
          }

        }else {
            // NO CCMs present
            //
            return false;
         }
      }
      
      // -----------------------------------------------------------------------
      // If address 'hits' into a CCM, return the a valid instance of BlockData,
      // otherwise '0' is returned.
      //
      arcsim::sys::mem::BlockData*
      CcmManager::get_host_page(uint32 addr)
      {
        uint32                       page_frame = core_arch_.page_arch.page_byte_frame(addr);
        arcsim::sys::mem::BlockData* block  = 0;
        
        if (ccm_blocks.find(page_frame) != ccm_blocks.end()) {
          // Get pointer to page containing ccm device
          //
          block = ccm_blocks[page_frame];
          
        } else {
          switch (ccm_mem_kind_) {
            case kCcmMemoryDevice: {
              arcsim::mem::MemoryDeviceInterface*             ccm_dev = 0;          
              // Based on CcmMappingKind we register the CCM dev that was hit
              switch (addr_mapping_kind_)
              {
                case kCcmMappingKindDirect:
                {
                // Because at this point we know the address has hit into a CCM,
                // we can short-circuit the range test.
                //
                if (addr > iccm_range_.second) { ccm_dev = d_ccm_dev_; }
                else                           { ccm_dev = i_ccm_dev_; }

                } break;

                case kCcmMappingKindRegion:
                case kCcmMappingKindMultipleIccms:
                {
                // When CCMs are mapped onto regions the lookup is direct.
                ccm_dev = mem_dev_region_map_[get_memory_region_index(addr)];
                } break;

              }
              
              if (ccm_dev) {
                // Page has not been allocated yet, so we need to create it,
                // here we use a memory device as backing store.
                block = new arcsim::sys::mem::BlockData(page_frame, ccm_dev);
                // Insert new Page containing CCM device
                ccm_blocks.insert(std::pair<uint32,arcsim::sys::mem::BlockData*>(page_frame,
                                                                                 block));    
              }
              break;
            }
            case kCcmDirectMemoryAccessDevice: {
              arcsim::mem::DirectMemoryAccessDeviceInterface*     ccm_dma = 0;          
              // Based on CcmMappingKind we register the CCM dev that was hit
              switch (addr_mapping_kind_)
              {
                case kCcmMappingKindDirect:
                {
                // Because at this point we know the address has hit into a CCM,
                // we can short-circuit the range test.
                //
                if (addr > iccm_range_.second) { ccm_dma = d_ccm_dma_; }
                else                           { ccm_dma = i_ccm_dma_; }              
              } break;

                case kCcmMappingKindRegion:
                case kCcmMappingKindMultipleIccms:
                {
                // When CCMs are mapped onto regions the lookup is direct.
                ccm_dma = mem_dma_region_map_[get_memory_region_index(addr)];
                } break;

              }
              
              if (ccm_dma) {
                // Page has not been allocated yet, so we need to create it,
                // here we use a direct memory access device as backing store.
                uint8* block_ptr = 0;
                ccm_dma->dma_dev_location(page_frame, &block_ptr); // get raw pointer
                block = new arcsim::sys::mem::BlockData(page_frame,
                                                        (uint32*)block_ptr);
                // Insert new Page containing CCM device
                ccm_blocks.insert(std::pair<uint32,arcsim::sys::mem::BlockData*>(page_frame,
                                                                                 block));    
              }
              break;
            }
          }
        }
        
        return block;
      }

} } } // arcsim::sys::cpu
