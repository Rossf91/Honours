//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//
//                                             
// =====================================================================

#include <iomanip>

#include "sys/mmu/Mmu-inl.h"

#include "exceptions.h"
#include "sys/cpu/PageCache.h"
#include "sys/cpu/processor.h"
#include "sys/cpu/state.h"

#include "util/Log.h"

#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_

// This macro selects the 4-bit region field from an address, according to the
// optional address size defined for ARCv2EM.
//
#define ARCV2EM_REGION(_addr_) \
  (((_addr_) >> (cpu_->sys_arch.isa_opts.addr_size-4)) & 0xF)

// This macro performs a check of the (physical) address against the code 
// protection bit-mask, returning 1 if the address is in a region that is protected, 
// and 0 if the region is unprotected. This is required by the Code Protection
// option, which is enabled when any region has a code-protection bit set 
// in cpu_->sys_arch.isa_opts.code_protect_bits.
//
#define CODE_PROTECT_CHECK(_addr_) \
 ((cpu_->sys_arch.isa_opts.code_protect_bits >> ARCV2EM_REGION(_addr_)) & 1)

namespace arcsim {
  namespace sys {
    namespace cpu {
            
      // Supported page descriptor formats
      static Mmu::PageDescriptorFormatEntry pd_fmt_tab[Mmu::kPageDescriptorFormatEntryIndexCount] = {
#define DECL_PD_FORMAT_TABLE(name,\
                             mode,\
                             pgsiz,\
                             pgsiz_log2,\
                             vpn_mask,\
                             ppn_mask,\
                             asid_mask,\
                             sasid_mask,\
                             perm_mask,\
                             v_bit,g_bit,s_bit,\
                             rk_bit,wk_bit,ek_bit,\
                             ru_bit,wu_bit,eu_bit,\
                             fc_bit)\
      { #name,\
        mode,\
        pgsiz,\
        pgsiz_log2,\
        vpn_mask,\
        ppn_mask,\
        asid_mask,\
        sasid_mask,\
        perm_mask,\
        v_bit,g_bit,s_bit,\
        rk_bit,wk_bit,ek_bit,\
        ru_bit,wu_bit,eu_bit,\
        fc_bit\
      },
        // Expand structure definition
        PAGE_DESCR_CONFIG_LIST(DECL_PD_FORMAT_TABLE)
#undef DECL_PD_FORMAT_TABLE
      };
      
      // -----------------------------------------------------------------------------
      // Constructor
      //
      Mmu::Mmu()
      : jtlb_(0),
        jtlb_sets_(0),
        jtlb_ways_(0),
        jtlb_sets_log2_(0),
        jtlb_ways_log2_(0),
        jtlb_index_mask_(0),
        jtlb_index_max_(0x00FF),
        rnd_jtlb_way_(0),
        rnd_uitlb_idx_(0),
        rnd_udtlb_idx_(0),
        valid_global_(0),
        valid_asid_(0),
        valid_sasid_(0),
        pd_fmt_(&pd_fmt_tab[kIndexCompat8K]),
        kGlobalTlbEnableMask(0x80000000),
        kSharedLibraryAsidEnableMask(0x20000000),
        is_global_tlb_enabled_(false),
        is_shared_library_asid_enabled_(false),
        unmapped_base_address_(0),
        version_(MmuArch::kMmuV1),
        kind_(MmuArch::kMmu)
      { /* EMPTY */ }
      
      // -----------------------------------------------------------------------------
      // Destructor
      //
      Mmu::~Mmu()
      {
        destroy();
      }
      
      
      void
      Mmu::construct(arcsim::sys::cpu::Processor* cpu, MmuArch& mmu_arch)
      { 
        destroy();  // call destroy first to avoid leaks upon multiple calls to construct()
        
        cpu_            = cpu;              // processor this MMU is responsible for
        version_        = mmu_arch.version; // store MMU version
        kind_           = mmu_arch.kind;
        ASSERT(kind_<2);
        switch(kind_)
        {
          case MmuArch::kMmu:
          {
            jtlb_ways_log2_ = mmu_arch.get_jtlb_ways_log2();
            jtlb_sets_log2_ = mmu_arch.get_jtlb_sets_log2();
            // pre-compute some useful run-time constants
            jtlb_sets_      = 1 << jtlb_sets_log2_;
            jtlb_ways_      = 1 << jtlb_ways_log2_;
            jtlb_index_mask_= jtlb_sets_ - 1;
            jtlb_index_max_ = (jtlb_sets_ * jtlb_ways_) - 1;

            if (mmu_arch.version > MmuArch::kMmuV2) { // set-up correct page descriptor format for MMUv3
              for (uint32 i = kIndexNormal1K; i < kPageDescriptorFormatEntryIndexCount; ++i) {
                if (pd_fmt_tab[i].page_size_ == mmu_arch.get_page_size()) {
                  pd_fmt_ = &pd_fmt_tab[i];
                  break;
                }
              }
            }
            // mask checking 'valid' and 'global' bits
            valid_global_    = (1 << pd_fmt_->v_bit_) | (1 << pd_fmt_->g_bit_);
            // mask checking 'valid', 'sasid enable' and 'sasid'
            valid_sasid_     = (1 << pd_fmt_->s_bit_) | (1 << pd_fmt_->v_bit_);

            LOG(LOG_INFO) << "[MMU] CONFIG - PD-FORMAT: '" << pd_fmt_->name_
                          << "' VERSION:'" << mmu_arch.version
                          << "' SETS:'" << jtlb_sets_ << "' WAYS:'" << jtlb_ways_ << "'";

            // make absolutely shure jtlb_ways_ is power of two so we can compute
            // next random way using bitwise AND instead of using modulo operation
            // @http://en.wikipedia.org/wiki/Modulo_operation#Performance_issues
            ASSERT(IS_POWER_OF_TWO(jtlb_ways_) && "JTLB ways is not power of two");

            // allocate jtlb_
            jtlb_ = new EntryTLB*[jtlb_sets_];
            for (uint32 set = 0; set < jtlb_sets_; ++set) {
              jtlb_[set] = new EntryTLB[jtlb_ways_];
              // clear jtlb_ entries
              for (uint32 way = 0; way < jtlb_ways_; ++way) {
                jtlb_[set][way].virt_pd0 = jtlb_[set][way].phys_pd1 =  0 ;
              }
            }

            // Clear u(I|D)TLB entries when MMU version != 0x1
            for (uint32 i = 0; i < kuITLB_size; ++i) {
              uitlb_[i].virt_pd0 = uitlb_[i].phys_pd1 = 0;
            }
            for (uint32 i = 0; i < kuDTLB_size; ++i) {
              udtlb_[i].virt_pd0 = udtlb_[i].phys_pd1 = 0;
            }
          }break;
          
          case MmuArch::kMpu:
          {
            pd_fmt_ = &pd_fmt_tab[kIndexCompat8K];
            mpu_num_regions=mmu_arch.mpu_num_regions;
            unmapped_base_address_ = 0xFFFFFFFF;
          }

        }

      }
      
      void
      Mmu::destroy()
      {
        if (jtlb_ != 0) { // free dynamically allocated jtlb arrays
          for (uint32 way = 0; way < jtlb_ways_; ++way)
            delete [] jtlb_[way];
          delete [] jtlb_;
        }
      }
      
      // -----------------------------------------------------------------------------
      // Write process identity register or MPU_EN register
      //
      void
      Mmu::write_pid (uint32 pid)
      {
        if(kind_ == MmuArch::kMmu){
          is_global_tlb_enabled_          = pid & kGlobalTlbEnableMask;
          // TODO(iboehm): Enable SASID matching
          //is_shared_library_asid_enabled_ = pid & kSharedLibraryAsidEnableMask;
        
          LOG(LOG_INFO) << "[MMU] MMU-ENABLED:" << is_global_tlb_enabled_;
          LOG(LOG_INFO) << "[MMU] SASID-MATCHING-ENABLED:" << is_shared_library_asid_enabled_;
          // FIXME(iboehm): here we assume OS page tables, interrupt and exception
          //        handlers are located in un-translated memory above 0x80000000.
          //        Where is it documented how the unmapped base address can be set?
          unmapped_base_address_  = pid & kGlobalTlbEnableMask;
          valid_asid_             = (1 << pd_fmt_->v_bit_) | (pid & pd_fmt_->asid_mask_);

        }else{
          ASSERT(kind_ == MmuArch::kMpu);
          if((pid & (1<<30)) == 0) return; //Disabling the MPU does not necessitate flushing the caches
        }
        
        cpu_->purge_dcode_cache();
        cpu_->purge_translation_cache();
        cpu_->purge_page_cache(arcsim::sys::cpu::PageCache::ALL);

      }

      //----------------------------------------------------------------------------
      // MPU permissions lookup. With MPU virt_addr == phys_addr so can take either 
      // as a parameter. Function return the region which was found (0xFF on default)
      uint32
      Mmu::lookup_mpu_perms (uint32 addr, uint32 &perms) const
      {
        ASSERT(kind_==MmuArch::kMpu);
        uint32 mpu_en = cpu_->state.auxs[AUX_MPU_EN];
        if (mpu_en & (1<<30)){
          for(uint32 i=0; i<mpu_num_regions; i++){
            // 64bit values because base+size can be 0x100000000, or 2^33
            uint64 base = cpu_->state.auxs[AUX_MPU_RDB0+2*i];
            bool valid  = (base & 1)!=0;
            base        = base & 0xFFFFF800;
            uint64 size  = cpu_->state.auxs[AUX_MPU_RDP0+2*i]&0xE00;
            size         = (size>>7) | (cpu_->state.auxs[AUX_MPU_RDP0+2*i] & 0x3);
            uint64 size_bits = (size-0xA) + 11;
            size = 1ull << size_bits;
            base >>= size_bits;
            base <<= size_bits;
            if(valid && (addr >= base) && (addr < (base+size)) ){
              perms = cpu_->state.auxs[AUX_MPU_RDP0+2*i] & 0x1F8;
              // Since user mode permissions also allow kernel access with the MPU,
              // We OR the user mode bits onto the kernel mode bits
              perms |= ((perms<<3) & 0x1C0);
              return i;
            }
          }
          //We didn't get a hit in the configurable regions so return the baseline permissions
          perms = mpu_en & 0x1F8;
          // Since user mode permissions also allow kernel access with the MPU,
          // We OR the user mode bits onto the kernel mode bits
          perms |= ((perms<<3) & 0x1C0);
        }else{
          perms = 0x00FFFFFF;//MPU disabled so return full  KU_RWX permissions
        }
        return 0xFF;
      }
      
      // ---------------------------------------------------------------------------
      // Lookup translation for INST 'addr' if one exists, but don't raise an
      // exception if there is none and do NOT modify any MMU state. Return physical
      // address and permissions.
      //
      bool
      Mmu::lookup_inst_addr (uint32 virt_addr, uint32& phys_addr, uint32& perms) const
      {

        if(kind_ == MmuArch::kMpu){
         phys_addr = virt_addr;
         uint32 region = lookup_mpu_perms( phys_addr, perms );
         perms |= (region << 24); //use the perms parameter to return the region hit
         return true;
        }
        ASSERT(kind_ == MmuArch::kMmu);

        const uint32 set = (virt_addr >> pd_fmt_->page_size_log2_) & jtlb_index_mask_;
        const uint32 vpn = get_virt_page_num(virt_addr);
        const uint32 pda = get_valid_asid_or_sasid_search_key(vpn, virt_addr, cpu_->state.auxs[AUX_SASID]);
        const uint32 pdg = get_valid_global_search_key(vpn);
        
        // ---------------------------------------------------------------------------
        // 1. Check uITLB
        //
        for (uint32 i = 0; i < kuITLB_size; ++i) {
          if ( (pda == uitlb_[i].virt_pd0) || (pdg == uitlb_[i].virt_pd0) )
          { // uITLB hit
            phys_addr = get_phys_addr(uitlb_[i].phys_pd1,virt_addr);
            perms     = get_phys_addr_perm(uitlb_[i].phys_pd1);
            return true;
          }
        }
        // ---------------------------------------------------------------------------
        // 2. Check jtlb_
        //
        for (uint32 way = 0; way < jtlb_ways_; ++way) {
          if ( (pda == jtlb_[set][way].virt_pd0) || (pdg == jtlb_[set][way].virt_pd0)) {
            // TLB hit in way 0; get way 0 translation and perms
            phys_addr = get_phys_addr(jtlb_[set][way].phys_pd1,virt_addr);
            perms     = get_phys_addr_perm(jtlb_[set][way].phys_pd1);
            return true;
          }
        }
        // fall-through implies TLB miss
        return false;
      }
      
      // ---------------------------------------------------------------------------
      // Lookup translation for DATA 'addr' if one exists, but don't raise an
      // exception if there is none and do NOT modify any MMU state. Return physical
      // address and permissions.
      //
      bool
      Mmu::lookup_data_addr (uint32 virt_addr, uint32& phys_addr, uint32& perms) const
      {

        if(kind_ == MmuArch::kMpu){
         phys_addr = virt_addr;
         uint32 region = lookup_mpu_perms( phys_addr, perms );
         perms |= (region << 24); //use the perms parameter to return the region hit
         return true;
        }
        ASSERT(kind_ == MmuArch::kMmu);
        
        const uint32 set = (virt_addr >> pd_fmt_->page_size_log2_) & jtlb_index_mask_;
        const uint32 vpn = get_virt_page_num(virt_addr);
        const uint32 pda = get_valid_asid_or_sasid_search_key(vpn, virt_addr, cpu_->state.auxs[AUX_SASID]);
        const uint32 pdg = get_valid_global_search_key(vpn);      
        
        // ---------------------------------------------------------------------------
        // 1. Check udtlb_
        //
        for (uint32 i = 0; i < kuDTLB_size; ++i) {
          if ( (pda == udtlb_[i].virt_pd0) || (pdg == udtlb_[i].virt_pd0) )
          { // uDTLB hit
            //
            phys_addr = get_phys_addr(udtlb_[i].phys_pd1,virt_addr);
            perms     = get_phys_addr_perm(udtlb_[i].phys_pd1);
            // Return immediately
            return true;
          }
        }
        // ---------------------------------------------------------------------------
        // 2. Check jtlb_
        //
        for (uint32 way = 0; way < jtlb_ways_; ++way) {
          if ( (pda == jtlb_[set][way].virt_pd0) || (pdg == jtlb_[set][way].virt_pd0)) {
            // TLB hit in way 0; get way 0 translation and perms
            phys_addr = get_phys_addr(jtlb_[set][way].phys_pd1,virt_addr);
            perms     = get_phys_addr_perm(jtlb_[set][way].phys_pd1);
            return true;
          }
        }
        // fall-through implies TLB miss
        return false;
      }

      // ---------------------------------------------------------------------------
      // Public instruction virtual address translation method
      //
      uint32
      Mmu::lookup_exec  (uint32 virt_addr, bool mode, uint32& phys_addr) const
      { // Early-out if address is in the unmapped region.
        // Note: unmapped_base_address_ == 0 when PID[T] == 0.
        if (virt_addr >= unmapped_base_address_) {
          phys_addr = virt_addr;
          return 0;
        }
        
        uint32 perms;
        uint32 ecause = 0;
        
        if (lookup_inst_addr (virt_addr, phys_addr, perms)) {
          if (perms & (mode ? (1<<pd_fmt_->eu_bit_) : (1<<pd_fmt_->ek_bit_))) { // jtlb_ hit, so check permissions
            return 0;
          } else {
            if(cpu_->sys_arch.isa_opts.is_isa_a6kv2())
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV,
                            IfetchProtV,
                            ((kind_==MmuArch::kMmu)? cpu_->sys_arch.isa_opts.PV_Mmu : cpu_->sys_arch.isa_opts.PV_Mpu)
                          );
            else
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, IfetchProtV, 0);
          }
        } else {
          ecause = ECR(cpu_->sys_arch.isa_opts.EV_ITLBMiss, IfetchTLBMiss, 0);
        }        
        // Return the exception code, if we get this far.
        return ecause;
      }
      
      // ---------------------------------------------------------------------------
      // Public data virtual address translation method
      //
      uint32
      Mmu::lookup_data(uint32 virt_addr, bool mode, uint32& phys_addr) const
      { // Early-out if address is in the unmapped region.
        // Note: unmapped_base_address_ == 0 when PID[T] == 0.
        if (virt_addr >= unmapped_base_address_) {
          phys_addr = virt_addr;
          return 0;
        }
        
        uint32 perms;
        uint32 ecause = 0;
        
        if (lookup_data_addr (virt_addr, phys_addr, perms)) {
          uint32 code_protect_fail = (cpu_->sys_arch.isa_opts.is_isa_a6kv2())
                         ? CODE_PROTECT_CHECK(phys_addr)
                         : 0;
          if ((!code_protect_fail) && (perms & (mode ? (1<<pd_fmt_->ru_bit_) : (1<<pd_fmt_->rk_bit_)) ) ) { // jtlb_ hit, so check permissions
            return 0;
          } else {
            if(cpu_->sys_arch.isa_opts.is_isa_a6kv2())
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV,
                            LoadTLBfault,
                            ((kind_==MmuArch::kMmu)? cpu_->sys_arch.isa_opts.PV_Mmu : cpu_->sys_arch.isa_opts.PV_Mpu)
                              | ((code_protect_fail)? cpu_->sys_arch.isa_opts.PV_CodeProtect : 0)
                          );
            else
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, LoadTLBfault, 0);
          }
        } else {
          ecause = ECR(cpu_->sys_arch.isa_opts.EV_DTLBMiss, LoadTLBfault, 0);
        }        
        // Return the exception code, if we get this far.
        return ecause;
      }

      
      // ---------------------------------------------------------------------------
      // Get the translation for INST 'addr' if one exists, but don't raise an
      // exception if it fails. Return physical address and permissions.
      //
      bool
      Mmu::translate_inst_addr(uint32 virt_addr, uint32& phys_addr, uint32& perms)
      {
        if(kind_ == MmuArch::kMpu){
          phys_addr = virt_addr;
          uint32 region = lookup_mpu_perms( phys_addr, perms );
          perms |= (region << 24); //use the perms parameter to return the region hit
          return true;
        }
        ASSERT(kind_ == MmuArch::kMmu);
        
        const uint32 set = (virt_addr >> pd_fmt_->page_size_log2_) & jtlb_index_mask_;
        const uint32 pd0 = get_virt_page_num(virt_addr);
        const uint32 pda = get_valid_asid_or_sasid_search_key(pd0, virt_addr, cpu_->state.auxs[AUX_SASID]);
        const uint32 pdg = pd0 | valid_global_;
        
        // ---------------------------------------------------------------------------
        // 1. Check uitlb_
        //
        for (uint32 i = 0; i < kuITLB_size; ++i) {
          if ( (pda == uitlb_[i].virt_pd0) || (pdg == uitlb_[i].virt_pd0) )
          { // uITLB hit
            phys_addr = get_phys_addr(uitlb_[i].phys_pd1,virt_addr);
            perms     = get_phys_addr_perm(uitlb_[i].phys_pd1);
            // Return immediately
            return true;
          }
        }
        // ---------------------------------------------------------------------------
        // 2. Check jtlb_
        //
        for (uint32 way = 0; way < jtlb_ways_; ++way) {
          if ( (pda == jtlb_[set][way].virt_pd0) || (pdg == jtlb_[set][way].virt_pd0)) {
            // TLB hit in way 0; get way 0 translation and perms
            phys_addr = get_phys_addr(jtlb_[set][way].phys_pd1,virt_addr);
            perms     = get_phys_addr_perm(jtlb_[set][way].phys_pd1);
            // Replace random entry in uITLB
            uitlb_[rnd_uitlb_idx_].virt_pd0 = jtlb_[set][way].virt_pd0;
            uitlb_[rnd_uitlb_idx_].phys_pd1 = jtlb_[set][way].phys_pd1;
            // compute next victim
            rnd_uitlb_idx_ = (rnd_uitlb_idx_ + 1) % kuITLB_size;
            return true;
          }
        }
        // Fall-through implies TLB miss
        return false;
      }
      
      // ---------------------------------------------------------------------------
      // Get the translation for DATA 'addr' if one exists, but don't raise an
      // exception if there is none. Return physical address and permissions.
      //
      bool
      Mmu::translate_data_addr(uint32 virt_addr, uint32& phys_addr, uint32& perms)
      {

        if(kind_ == MmuArch::kMpu){
          phys_addr = virt_addr;
          uint32 region = lookup_mpu_perms( phys_addr, perms );
          perms |= (region << 24); //use the perms parameter to return the region hit
          return true;
        }
        ASSERT(kind_ == MmuArch::kMmu);
        
        const uint32 set = (virt_addr >> pd_fmt_->page_size_log2_) & jtlb_index_mask_;
        const uint32 vpn = get_virt_page_num(virt_addr);
        const uint32 pda = get_valid_asid_or_sasid_search_key(vpn, virt_addr, cpu_->state.auxs[AUX_SASID]);
        const uint32 pdg = get_valid_global_search_key(vpn);
        
        // ---------------------------------------------------------------------------
        // 1. Check udtlb_
        //
        for (uint32 i = 0; i < kuDTLB_size; ++i) {
          if ( (pda == udtlb_[i].virt_pd0) || (pdg == udtlb_[i].virt_pd0) )
          { // uDTLB hit
            //
            phys_addr = get_phys_addr(udtlb_[i].phys_pd1,virt_addr);
            perms     = get_phys_addr_perm(udtlb_[i].phys_pd1);
            // Return immediately
            return true;
          }
        }
        // ---------------------------------------------------------------------------
        // 2. Check jtlb_
        //
        for (uint32 way = 0; way < jtlb_ways_; ++way) {
          if ( (pda == jtlb_[set][way].virt_pd0) || (pdg == jtlb_[set][way].virt_pd0)) {
            // TLB hit in way 0; get way 0 translation and perms
            phys_addr = get_phys_addr(jtlb_[set][way].phys_pd1,virt_addr);
            perms     = get_phys_addr_perm(jtlb_[set][way].phys_pd1);
            // Compute next victim
            rnd_udtlb_idx_ = (rnd_udtlb_idx_ + 1) % kuDTLB_size;
            return true;
          }
        }
        // Fall-through implies a TLB miss
        return false;
      }
      
      // ---------------------------------------------------------------------------
      // Translate each type of access in the given Kernel/User mode.
      // Return true if translation exists and permisions are not violated,
      // in which case 'phys_addr' contains the translated physical address.
      // Return false if jtlb miss or if protection violation would occur.
      //
      bool
      Mmu::translate_read(uint32 virt_addr, bool mode, uint32& phys_addr)
      { // Early-out if address is in the unmapped region.
        // Note: unmapped_base_address_ == 0 when PID[T] == 0.
        if (virt_addr >= unmapped_base_address_) {
          phys_addr = virt_addr;
          if (cpu_->sys_arch.isa_opts.is_isa_a6kv2() && CODE_PROTECT_CHECK(phys_addr)) {
            uint32 ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, LoadTLBfault, cpu_->sys_arch.isa_opts.PV_CodeProtect);
            cpu_->enter_exception (ecause, virt_addr, cpu_->state.pc);
            return false;
          }
          return true;
        }
        
        uint32 perms;
        uint32 ecause;
        
        if (translate_data_addr (virt_addr, phys_addr, perms)) {

          uint32 code_protect_fail = (cpu_->sys_arch.isa_opts.is_isa_a6kv2())
                                   ? CODE_PROTECT_CHECK(phys_addr)
                                   : 0;

          // TLB hit, so check permissions
          //
          if (perms & (mode ? (1<<pd_fmt_->ru_bit_) : (1<<pd_fmt_->rk_bit_))) {
            if (code_protect_fail) {
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, LoadTLBfault, cpu_->sys_arch.isa_opts.PV_CodeProtect);
              cpu_->enter_exception (ecause, virt_addr, cpu_->state.pc);
              return false;
            }
            return true;
          } else {
            if (cpu_->sys_arch.isa_opts.is_isa_a6kv2()){
              //Set MPU ECR
              if(kind_==MmuArch::kMpu)
                cpu_->state.auxs[AUX_MPU_ECR] = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, LoadTLBfault, ((perms>>24) & 0xFF) );
              
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV,
                            LoadTLBfault,
                            ((kind_==MmuArch::kMmu)? cpu_->sys_arch.isa_opts.PV_Mmu : cpu_->sys_arch.isa_opts.PV_Mpu)
                              | ((code_protect_fail)? cpu_->sys_arch.isa_opts.PV_CodeProtect : 0)
                          );
            }else
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, LoadTLBfault, 0);
          }
        } else {
          ecause   = ECR(cpu_->sys_arch.isa_opts.EV_DTLBMiss, LoadTLBfault, 0);
          rnd_jtlb_way_ = (rnd_jtlb_way_ + 1) & (jtlb_ways_ - 1);  // (rnd_jtlb_way_ + 1) % jtlb_ways_
        }
        // Set TLB_PD0 to VPN of faulting address, with V=1, G=0 and current ASID
        cpu_->state.auxs[AUX_TLB_PD0] = get_virt_page_num(virt_addr) | valid_asid_;
        // Raise the exception if we get this far. This assumes that state.pc is
        // correctly set to the address of the offending load or store instruction.
        cpu_->enter_exception (ecause, virt_addr, cpu_->state.pc);
        return false;
      }
      
      // ---------------------------------------------------------------------------
      //
      //
      bool
      Mmu::translate_write(uint32 virt_addr, bool mode, uint32& phys_addr)
      { // Early-out if address is in the unmapped region.
        // Note: unmapped_base_address_ == 0 when PID[T] == 0.
        if (virt_addr >= unmapped_base_address_) {
          phys_addr = virt_addr;

          if (cpu_->sys_arch.isa_opts.is_isa_a6kv2() && CODE_PROTECT_CHECK(phys_addr)) {
            uint32 ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, StoreTLBfault, cpu_->sys_arch.isa_opts.PV_CodeProtect);
            cpu_->enter_exception (ecause, virt_addr, cpu_->state.pc);
            return false;
          }
          return true;
        }
        
        uint32 perms;
        uint32 ecause;
        
        if (translate_data_addr (virt_addr, phys_addr, perms)) {

          uint32 code_protect_fail = (cpu_->sys_arch.isa_opts.is_isa_a6kv2())
                                   ? CODE_PROTECT_CHECK(phys_addr)
                                   : 0;

          // TLB hit, so check permissions
          if (perms & (mode ? (1<<pd_fmt_->wu_bit_) : (1<<pd_fmt_->wk_bit_))) {

            if (code_protect_fail) {
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, StoreTLBfault, 1);
              cpu_->enter_exception (ecause, virt_addr, cpu_->state.pc);
              return false;
            }
            return true;
          } else {
            if (cpu_->sys_arch.isa_opts.is_isa_a6kv2()){
              //Set MPU ECR
              if(kind_==MmuArch::kMpu)
                cpu_->state.auxs[AUX_MPU_ECR] = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, StoreTLBfault, ((perms>>24) & 0xFF) );
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, StoreTLBfault,
                            ((kind_==MmuArch::kMmu)? cpu_->sys_arch.isa_opts.PV_Mmu : cpu_->sys_arch.isa_opts.PV_Mpu)
                              | ((code_protect_fail)? cpu_->sys_arch.isa_opts.PV_CodeProtect : 0)
                          );
            }else
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, StoreTLBfault, 0);
          }
        } else {
          ecause = ECR(cpu_->sys_arch.isa_opts.EV_DTLBMiss, StoreTLBfault, 0);
          rnd_jtlb_way_ = (rnd_jtlb_way_ + 1) & (jtlb_ways_ - 1);  // (rnd_jtlb_way_ + 1) % jtlb_ways_
        }
        // Set TLB_PD0 to VPN of faulting address, with V=1, G=0 and current ASID
        cpu_->state.auxs[AUX_TLB_PD0] = get_virt_page_num(virt_addr) | valid_asid_;
        // Raise the exception if we get this far. This assumes that state.pc is
        // correctly set to the address of the offending load or store instruction.
        cpu_->enter_exception (ecause, virt_addr, cpu_->state.pc);
        return false;
      }
      
      // ---------------------------------------------------------------------------
      //
      //
      bool
      Mmu::translate_rmw(uint32 virt_addr, bool mode, uint32& phys_addr)
      { // Early-out if address is in the unmapped region.
        // Note: unmapped_base_address_ == 0 when PID[T] == 0.
        if (virt_addr >= unmapped_base_address_) {
          phys_addr = virt_addr;

          if (cpu_->sys_arch.isa_opts.is_isa_a6kv2() && CODE_PROTECT_CHECK(phys_addr)) {
            uint32 ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, ExTLBfault, cpu_->sys_arch.isa_opts.PV_CodeProtect);
            cpu_->enter_exception (ecause, virt_addr, cpu_->state.pc);
            return false;
          }
          return true;
        }
        
        uint32 perms;
        uint32 ecause;
        
        if (translate_data_addr (virt_addr, phys_addr, perms)) {

          uint32 code_protect_fail = (cpu_->sys_arch.isa_opts.is_isa_a6kv2())
                                   ? CODE_PROTECT_CHECK(phys_addr)
                                   : 0;

          // TLB hit, so check permissions
          if ( (perms & (mode ? (1<<pd_fmt_->ru_bit_) : (1<<pd_fmt_->rk_bit_)))
              &&
               (perms & (mode ? (1<<pd_fmt_->wu_bit_) : (1<<pd_fmt_->wk_bit_)))
              ){
            if (code_protect_fail) {
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, ExTLBfault, 1);
              cpu_->enter_exception (ecause, virt_addr, cpu_->state.pc);
              return false;
            }
            return true;
          } else {
            if (cpu_->sys_arch.isa_opts.is_isa_a6kv2()){
              //Set MPU ECR
              if(kind_==MmuArch::kMpu)
                cpu_->state.auxs[AUX_MPU_ECR] = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, ExTLBfault, ((perms>>24) & 0xFF) );
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV,
                            ExTLBfault,
                            ((kind_==MmuArch::kMmu)? cpu_->sys_arch.isa_opts.PV_Mmu : cpu_->sys_arch.isa_opts.PV_Mpu)
                              | ((code_protect_fail)? cpu_->sys_arch.isa_opts.PV_CodeProtect : 0)
                          );
            }else
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, ExTLBfault, 0);
          }
        } else {
          ecause = ECR(cpu_->sys_arch.isa_opts.EV_DTLBMiss, ExTLBfault, 0);
          rnd_jtlb_way_ = (rnd_jtlb_way_ + 1) & (jtlb_ways_ - 1);  // (rnd_jtlb_way_ + 1) % jtlb_ways_
        }
        // Set TLB_PD0 to VPN of faulting address, with V=1, G=0 and ASID current
        cpu_->state.auxs[AUX_TLB_PD0] = get_virt_page_num(virt_addr) | valid_asid_;
        // Raise the exception if we get this far. This assumes that state.pc is
        // correctly set to the address of the offending ex instruction.
        cpu_->enter_exception (ecause, virt_addr, cpu_->state.pc);
        return false;
      }
      
      // ---------------------------------------------------------------------------
      //
      //
      uint32
      Mmu::translate_exec(uint32 virt_addr, bool mode, uint32& phys_addr)
      { // Early-out if address is in the unmapped region.
        // Note: unmapped_base_address_ == 0 when PID[T] == 0.
        if (virt_addr >= unmapped_base_address_) {
          phys_addr = virt_addr;
          return 0;
        }
        
        uint32 perms;
        uint32 ecause = 0;
        
        if (translate_inst_addr (virt_addr, phys_addr, perms)) {
          // jtlb_ hit, so check permissions
          if (perms & (mode ? (1<<pd_fmt_->eu_bit_) : (1<<pd_fmt_->ek_bit_))) {
            return 0;
          } else {
            if (cpu_->sys_arch.isa_opts.is_isa_a6kv2()){
              //Set MPU ECR
              if(kind_==MmuArch::kMpu)
                cpu_->state.auxs[AUX_MPU_ECR] = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, IfetchProtV, ((perms>>24) & 0xFF) );
              
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV,
                            IfetchProtV,
                            ((kind_==MmuArch::kMmu)? cpu_->sys_arch.isa_opts.PV_Mmu : cpu_->sys_arch.isa_opts.PV_Mpu)
                          );
            }else
              ecause = ECR(cpu_->sys_arch.isa_opts.EV_ProtV, IfetchProtV, 0);
          }
        } else {
          ecause = ECR(cpu_->sys_arch.isa_opts.EV_ITLBMiss, IfetchTLBMiss, 0);
          rnd_jtlb_way_ = (rnd_jtlb_way_ + 1) & (jtlb_ways_ - 1);  // (rnd_jtlb_way_ + 1) % jtlb_ways_
        }
        // Set TLB_PD0 to VPN of faulting address, with V=1, G=0 and ASID current
        cpu_->state.auxs[AUX_TLB_PD0] = get_virt_page_num(virt_addr) | valid_asid_;
        return ecause; // return the exception code, if we get this far.
      }
      
      // -----------------------------------------------------------------------------
      // This method is triggered by a call to the AUX_TLB_Command register
      //
      void
      Mmu::command(uint32 command)
      {
        switch (command) {
          case kCmdTLBWrite: { // write TLB entry to index location specified in TLBIndex
            tlb_write(true);   // and invalidate uTLBs. Also used to remove entries.
            break;
          }
          case kCmdTLBRead: { // Read TLB entry into TLBPD0 and TLBPD1 from location
            tlb_read();       // specified in TLBIndex
            break;
          }
          case kCmdTLBGetIndex: { // set TLBIndex to contain a suitable index location for
            tlb_get_index();      // page descriptor in TLBPD0 or TLBPD1 or an error code
            break; 
          }            
          case kCmdTLBProbe: { // determine if TLB entry matching the virtual address
            tlb_probe ();      // supplied in PD0 and PD1 is present and return its
            break;             // index location
          }
            
          case kCmdTLBWriteNi: { 
            if (version_ > MmuArch::kMmuV1) {
              tlb_write(false); // write TLB entry to the index location specified in TLBIndex
              break;            // without invalidating uTLBs. Used for update on TLB miss.
            }
          }
          case kCmdIVUTLB: {
            if (version_ > MmuArch::kMmuV1) {
              utlb_clear(); // Invalidate uTLBs. Used when removing entries from 
              break;        // page table in memory.
            }
          }
          default: {
            LOG(LOG_ERROR) << "[MMU] CMD-Unknown";
            // Unrecognized command was issued, so set the TLBIndex[E] bit
            // FIXME: where is this documented?
            cpu_->state.auxs[AUX_TLB_Index] |= 0x80000000;
            break;
          }
        }
        return;
      }      
      // -----------------------------------------------------------------------------
      // Explicitly clear the uTLBs. This command is new for MMU versions > kMmuV1
      //
      void
      Mmu::utlb_clear()
      {
        for (uint32 i = 0; i < kuITLB_size; ++i) {
          uitlb_[i].virt_pd0 = uitlb_[i].phys_pd1 = 0;
          
        }
        for (uint32 i = 0; i < kuDTLB_size; ++i) {
          udtlb_[i].virt_pd0 = udtlb_[i].phys_pd1 = 0;
        }
        cpu_->purge_dcode_cache();
        cpu_->purge_translation_cache();
        cpu_->purge_page_cache(arcsim::sys::cpu::PageCache::ALL);
      }
      
      // -----------------------------------------------------------------------------
      // Write TLB entry to the index location specified in TLBIndex.
      //
      void
      Mmu::tlb_write(bool update_utlbs)
      { 
        // First we extract the index location to which the entry is to be loaded
        const uint32 idx =  cpu_->state.auxs[AUX_TLB_Index] & 0x7ff;
        const uint32 set = idx >> jtlb_ways_log2_;
        const uint32 way = idx &  (jtlb_ways_ - 1);
        
        ASSERT(way < jtlb_ways_ && set < jtlb_sets_);
        
        // ---------------------------------------------------------------------------
        // Check range
        //
        //   IDX_RANGE_1: 0x0000 - jtlb_index_max_ ...  jtlb
        //   IDX_RANGE_2: 0x0200 - 0x0203          ... uITLB
        //   IDX_RANGE_3: 0x0400 - 0x0407          ... uDTLB
        //  
        if (idx > jtlb_index_max_) {
          // Record an error in TLBIndex if index is out of range
          if (idx > 0x407)
            cpu_->state.auxs[AUX_TLB_Index] = 0x80000000UL;
          return;
        }
        
        // ---------------------------------------------------------------------------
        // WRITE TLB entry at the given index. The following code assumes a 2-way jtlb.
        //
        const uint32 aux_tlb_virt_pd0 = cpu_->state.auxs[AUX_TLB_PD0];
        const uint32 aux_tlb_phys_pd1 = cpu_->state.auxs[AUX_TLB_PD1];
        
        // store old translation
        const uint32 old_pd0 = jtlb_[set][way].virt_pd0;
        const uint32 old_pd1 = jtlb_[set][way].phys_pd1;
        // Write the new TLB entry to way 0
        jtlb_[set][way].virt_pd0 = aux_tlb_virt_pd0;
        jtlb_[set][way].phys_pd1 = aux_tlb_phys_pd1;

        // ---------------------------------------------------------------------------
        // If kCmdTlbWrite command is used we need to keep u(I|D)TLBs in sync
        //
        if (update_utlbs) {
          for (uint32 i = 0; i < kuITLB_size; ++i) {
            if ((uitlb_[i].virt_pd0 == old_pd0) && (uitlb_[i].phys_pd1 == old_pd1)) {
              uitlb_[i].virt_pd0 = aux_tlb_virt_pd0;
              uitlb_[i].phys_pd1 = aux_tlb_phys_pd1;
              break;
            }
          }
          for (uint32 i = 0; i < kuDTLB_size; ++i) { 
            if ((udtlb_[i].virt_pd0 == old_pd0) && (udtlb_[i].phys_pd1 == old_pd1)) {
              udtlb_[i].virt_pd0 = aux_tlb_virt_pd0;
              udtlb_[i].phys_pd1 = aux_tlb_phys_pd1;
              break;
            }
          } 
        }
        
        // ---------------------------------------------------------------------------
        // If we are trying to overwrite an entry with itself we can exit earlier as
        // there is nothing more we need to do
        //
        if ((old_pd0 == aux_tlb_virt_pd0) && (old_pd1 == aux_tlb_phys_pd1))
        { // If a page entry is replaced with itself exit early
          LOG(LOG_DEBUG2) << "[MMU] " << ((update_utlbs) ? "kCmdTlbWrite" : "kCmdTlbWriteNI")
                          << " - IDENTICAL REPLACEMENT PD0-0x" << HEX(old_pd0) << " PD1-0x" << HEX(old_pd1)
                          << " with PD0-0x" << HEX(aux_tlb_virt_pd0) << " PD1-0x" << HEX(aux_tlb_phys_pd1) ;
          return;
        }
        
        // ---------------------------------------------------------------------------
        // If an entry with the V bit set to false has been loaded, we need to remove
        // (i.e. shoot down) existing entries
        //
        if (! (aux_tlb_virt_pd0 & (1<<pd_fmt_->v_bit_)) ) {
          LOG(LOG_DEBUG2) << "[MMU] " << ((update_utlbs) ? "kCmdTlbWrite" : "kCmdTlbWriteNI")
                          << " - SHOOT DOWN PD0-0x" << HEX(old_pd0) << " PD1-0x" << HEX(old_pd1)
                          << " with PD0-0x" << HEX(aux_tlb_virt_pd0) << " PD1-0x" << HEX(aux_tlb_phys_pd1) ;
        }
        
        // ---------------------------------------------------------------------------
        // REPLACEMENT DEBUGGING
        //
        LOG(LOG_DEBUG2) << "[MMU] " << ((update_utlbs) ? "kCmdTlbWrite" : "kCmdTlbWriteNI")
                        << " - REPLACED PD0-0x" << HEX(old_pd0) << " PD1-0x" << HEX(old_pd1)
                        << " with PD0-0x" << HEX(aux_tlb_virt_pd0) << " PD1-0x" << HEX(aux_tlb_phys_pd1);
        
        // ---------------------------------------------------------------------------
        // Shootdown any hashed translations based on the replaced TLB entry if the
        // replaced TLB entry was valid. If the replaced TLB entry was invalid, there
        // is nothing we really have to do.
        //
        if (old_pd0 & (1 << pd_fmt_->v_bit_)) { 
          uint32 purge_virt_addr = get_virt_page_num(old_pd0);
          uint32 purge_phys_addr = get_phys_page_num(old_pd1);
          
          // Purge the page that was previously mapped by the TLB that has just been
          // overwritten from our page cache
          //
          uint32 count = cpu_->purge_page_cache_entry(arcsim::sys::cpu::PageCache::ALL,
                                                      purge_virt_addr);
          
          if (count) {
            LOG(LOG_DEBUG2) << "[MMU] PURGED '" << count << "' PAGE(S) @ VIRT_ADDR: '0x"
                            << HEX(purge_virt_addr) << "' PHYS_ADDR: '0x"
                            << HEX(purge_phys_addr) << "' from page cache.";
          } else {
            LOG(LOG_DEBUG2) << "[MMU] PURGED '0' entries from page cache.";
          }
        }
        
        // ---------------------------------------------------------------------------
        // Finally we need to shoot down all of our internal caching and state
        // data structures.
        //
        cpu_->purge_dcode_cache();    
        cpu_->purge_translation_cache();
        return;
      }
      
      // -----------------------------------------------------------------------------
      // Read TLB entry into TLBPD0 and TLBPD1 from the location specified in 
      // TLBIndex 
      //
      void
      Mmu::tlb_read()
      {
        const uint32 tlb_index = cpu_->state.auxs[AUX_TLB_Index] & 0x7ff;
        const uint32 set = tlb_index >> jtlb_ways_log2_;
        const uint32 way = tlb_index & (jtlb_ways_ - 1);
        
        ASSERT(way < jtlb_ways_ && set < jtlb_sets_);
        
        // ---------------------------------------------------------------------------
        // Reading uITLB
        // IDX_RANGE_2: 0x0200 - 0x0203 ... uITLB
        //
        if ((tlb_index >= 0x0200) && (tlb_index < 0x0204)) {
          uint32 uitlb_index = tlb_index & 0x0000F;
          
          // FIXME: check permissions
          cpu_->state.auxs[AUX_TLB_PD0] = uitlb_[uitlb_index].virt_pd0;
          cpu_->state.auxs[AUX_TLB_PD1] = uitlb_[uitlb_index].phys_pd1; 
        }
        
        // ---------------------------------------------------------------------------
        // Reading uDTLB
        // IDX_RANGE_3: 0x0400 - 0x0407 ... uDTLB
        //  
        if ((tlb_index >= 0x0400) && (tlb_index < 0x0408)) {
          const uint32 udtlb_index = tlb_index & 0x0000F;
          
          // FIXME: check permissions
          cpu_->state.auxs[AUX_TLB_PD0] = udtlb_[udtlb_index].virt_pd0;
          cpu_->state.auxs[AUX_TLB_PD1] = udtlb_[udtlb_index].phys_pd1;
          return;
        }
        
        // ---------------------------------------------------------------------------
        // Record an error in TLBIndex if index is out of range, and
        // return an entry with all bits set to zero.
        //
        if (tlb_index > jtlb_index_max_) {
          cpu_->state.auxs[AUX_TLB_Index] = 0x80000000UL;
          cpu_->state.auxs[AUX_TLB_PD0]   = 0;
          cpu_->state.auxs[AUX_TLB_PD1]   = 0;
          return;
        }
        
        // ---------------------------------------------------------------------------
        // Reading TLB - assumes 2 way jtlb
        // IDX_RANGE_1: 0x0000 - jtlb_index_max_ ...  jtlb
        //
        cpu_->state.auxs[AUX_TLB_PD0] = jtlb_[set][way].virt_pd0;
        cpu_->state.auxs[AUX_TLB_PD1] = jtlb_[set][way].phys_pd1;

        return;
      }
      
      // -----------------------------------------------------------------------------
      // Set TLBIndex to contain a suitable index location for the page descriptor
      // in TLBPD0 or TLBPD1 or an error code.
      //
      void
      Mmu::tlb_get_index()
      {
        const uint32 pd0 = cpu_->state.auxs[AUX_TLB_PD0];
        const uint32 set = (pd0 >> pd_fmt_->page_size_log2_) & jtlb_index_mask_;
        uint32       way = rnd_jtlb_way_;
        
        ASSERT(way < jtlb_ways_ && set < jtlb_sets_);
        
        // consider invalid ways for replacement first
        for (uint32 i = 0; i < jtlb_ways_; ++i) {
          if (! (jtlb_[set][i].virt_pd0 & (1<<pd_fmt_->v_bit_))) {
            way = i;
            break;
          }
        }
        LOG(LOG_DEBUG2) << "[MMU] kCmdTLBGetIndex - PD0:0x" << HEX(pd0)
                        << " - set:" << set << " way:" << way
                        << " idx:0x" << HEX(((set << jtlb_ways_log2_) | way)); 
        cpu_->state.auxs[AUX_TLB_Index] = (set << jtlb_ways_log2_) | way;
      }
      
      // -----------------------------------------------------------------------------
      // Determine if a TLB entry is present that matches the virtual address
      // supplied in TLBPD0 or TLBPD1, and return its index location or error
      // code in TLBIndex.
      // 
      // NOTE that tlb_probe() only searches the jtlb and NOT the u(I|D)TLBs
      //
      void
      Mmu::tlb_probe ()
      {
        uint32 num_matches = 0;
        uint32 set         = 0;
        uint32 way         = 0;

        const uint32 pd0 = cpu_->state.auxs[AUX_TLB_PD0];
        const uint32 pdg = (pd0 & pd_fmt_->vpn_mask_) | (1<<pd_fmt_->v_bit_);
        uint32 pda = (pd0 & (pd_fmt_->vpn_mask_ | pd_fmt_->asid_mask_)) | (1<<pd_fmt_->v_bit_);
        
        if (pd0 & (1<<pd_fmt_->s_bit_)) { // is SASID matching enabled
          const uint32 shift = (pd0 & pd_fmt_->sasid_mask_);
          if (cpu_->state.auxs[AUX_SASID] & (1 << shift)) { // can ASID use shared library
            pda = (pd0 & (pd_fmt_->vpn_mask_ | pd_fmt_->sasid_mask_)) | (1<<pd_fmt_->v_bit_); // match SASID
          } else {
            pda = 0; // Fail - ASID can not use shared library
          }
        }
                
        // probe JTLB
        for (set = 0; set < jtlb_sets_; ++set) {
          for (way = 0; way < jtlb_ways_; ++way) {
            const uint32 jtlb_pd0 = jtlb_[set][way].virt_pd0;
            if ((pda == jtlb_pd0) || (pdg == jtlb_pd0)) {
              cpu_->state.auxs[AUX_TLB_Index] = (set << jtlb_ways_log2_) | way;
              ++num_matches;
            }
          }
        }
        
        if (num_matches > 1) {
          // If more than one matching entry is found in the TLB, the TLBIndex register
          // will be loaded with the error flag E set and the Index filed containing
          // error code 0x1.
          //
          cpu_->state.auxs[AUX_TLB_Index] = 0x80000001UL;
        } else if (num_matches == 0) {
          // If no matching entry is found in the TLB, the TLBIndex will be loaded with
          // error flag E set and the Index filed containing error code 0x0.
          //
          cpu_->state.auxs[AUX_TLB_Index] = 0x80000000UL;
        }
        LOG(LOG_DEBUG2) << "[MMU] kCmdTLBProbe - # of matches:" << num_matches
                        << " - set:" << set << " way:" << way
                        << " - TLBIndex:0x"    << HEX(cpu_->state.auxs[AUX_TLB_Index])
                        << " - AUX_TLB_PD0:0x" << HEX(cpu_->state.auxs[AUX_TLB_PD0]); 
      }
      
    } } } // arcsim::sys::cpu

