//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================

#ifndef INC_SYS_MEM_MMU_INL_H_
#define INC_SYS_MEM_MMU_INL_H_

#include "sys/mmu/Mmu.h"

namespace arcsim {
  namespace sys {
    namespace cpu {

      inline uint32
      Mmu::get_virt_page_num(uint32 pd0) const {
        return pd0 & pd_fmt_->vpn_mask_;
      }

      inline uint32
      Mmu::get_phys_page_num(uint32 pd1) const {
        return pd1 & pd_fmt_->ppn_mask_;
      }

      inline uint32
      Mmu::get_phys_addr(uint32 pd1, uint32 vaddr) const {
        return (pd1 & pd_fmt_->ppn_mask_) | (vaddr & (0xffffffff - pd_fmt_->ppn_mask_));
      }

      inline uint32
      Mmu::get_phys_addr_perm(uint32 pd1) const {
        return pd1 & pd_fmt_->perm_mask_;
      }

      inline uint32
      Mmu::get_valid_global_search_key(uint32 vpn) const {
        return vpn | valid_global_;
      }
      
      inline uint32
      Mmu::get_valid_asid_or_sasid_search_key(uint32 vpn, uint32 vaddr, uint32 sasid) const
      { // is SASID matching enabled
        if (is_shared_library_asid_enabled_ && (vaddr & (1<<pd_fmt_->s_bit_))) { 
          const uint32 shift = (vaddr & pd_fmt_->sasid_mask_);
          if (sasid & (1 << shift)) { // can ASID use shared library
            return vpn | valid_sasid_ | shift ; // match SASID
          }
          return 0; // Fail - ASID can not use shared library so search key is 0
        }
        return vpn | valid_asid_; // match ASID
      }
      
} } } // arcsim::sys::cpu

#endif  // INC_SYS_MEM_MMU_INL_H_
