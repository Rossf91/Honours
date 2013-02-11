//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//
//
// =====================================================================
//
// Description: TechPubs/UserDocs/Output/5126_ARC700_Bookshelf/trunk/pdf/ARC700_MemoryManagementUnit_Reference.pdf
//
// TODO(iboehm): Finish and enable SASID implementation. NOTE, SASID AUX
//     register has to be enabled.
//
// =====================================================================

#ifndef INC_SYS_MEM_MMU_H_
#define INC_SYS_MEM_MMU_H_

#include "api/types.h"

// List of supported MMU commands:
//  name - TLB command name (used to generate enum name)
//  number - TLB command number
//
#define MMU_COMMAND_LIST(V)    \
  V(TLBWrite,0x1)              \
  V(TLBRead,0x2)               \
  V(TLBGetIndex,0x3)           \
  V(TLBProbe,0x4)              \
  V(TLBWriteNi,0x5)            \
  V(IVUTLB,0x6)

// List of page descriptor configurations (NOTE: Order matters):
//   name - String name of page descriptor mode
//   mode - Page descriptor mode enum
//   page size - Page size in bytes
//   page size log2 - log2 of page size
//   vpn-mask - Virtual Page Number (VPN)
//   ppn-mask - Physical Page Number (PPN)
//   asid-mask - Mask to extract ASID part of VPN
//   sasid-mask - Mask to extract SASID of VPN
//   perm-mask - Mask to extract permissions
//   valid-bit - Valid bit position
//   global-bit - Global bit position
//   sasid-bit - SASID bit position
//   rk-bit,wk-bit,ek-bit - Kernel R/W/E bit positions
//   ru-bit,wu-bit,eu-bit - User R/W/W bit positions
//   fc-bit - Flush cache bit position
//
#define PAGE_DESCR_CONFIG_LIST(V) \
  V(Compat8K ,Mmu::kPageDescriptorCompatFormatMode,\
    8192,13,0x7fffe000,0xffffe000,0xff,0x1f,0x1fc,10,8,31,8,7,6,5,4,3,2) \
  V(Normal1K ,Mmu::kPageDescriptorNormalFormatMode,\
    1024,10,0x7ffffc00,0xfffffc00,0xff,0x1f,0x07f, 9,8,31,6,5,4,3,2,1,0) \
  V(Normal2K ,Mmu::kPageDescriptorNormalFormatMode,\
    2048,11,0x7ffff800,0xfffff800,0xff,0x1f,0x07f, 9,8,31,6,5,4,3,2,1,0) \
  V(Normal4K ,Mmu::kPageDescriptorNormalFormatMode,\
    4096,12,0x7ffff000,0xfffff000,0xff,0x1f,0x07f, 9,8,31,6,5,4,3,2,1,0) \
  V(Normal8K ,Mmu::kPageDescriptorNormalFormatMode,\
    8192,13,0x7fffe000,0xffffe000,0xff,0x1f,0x07f, 9,8,31,6,5,4,3,2,1,0) \
  V(Normal16K,Mmu::kPageDescriptorNormalFormatMode,\
    16384,14,0x7fffc000,0xffffc000,0xff,0x1f,0x07f, 9,8,31,6,5,4,3,2,1,0)


class MmuArch;

namespace arcsim {
  namespace sys {
    namespace cpu {

      // ---------------------------------------------------------------------------
      // Forward declaration
      //
      class  Processor;

      // ---------------------------------------------------------------------------
      // MMU Class
      //
      class Mmu
      {
      public:
        
        // MMU Command enumeration
        enum Command {
#define MMU_COMMAND_DECL(cmd_,cmd_num_) kCmd##cmd_ = cmd_num_,
          MMU_COMMAND_LIST(MMU_COMMAND_DECL)
#undef MMU_COMMAND_DECL
        };

        // Page descriptor format modes
        enum PageDescriptorFormatMode {
          kPageDescriptorCompatFormatMode = 0x1,
          kPageDescriptorNormalFormatMode = 0x2,
        };
                
        // Page descriptor format structure
        struct PageDescriptorFormatEntry {
          const char*  name_;
          const Mmu::PageDescriptorFormatMode mode_; // active page descriptor format
          const uint32 page_size_;                   // page size in bytes
          const uint32 page_size_log2_;
          const uint32 vpn_mask_;                    // virtual page number mask
          const uint32 ppn_mask_;                    // physical page number mask
          const uint32 asid_mask_;                   // asid mask
          const uint32 sasid_mask_;                  // sasid mask
          const uint32 perm_mask_;                   // permission mask
          const uint8  v_bit_;                       // valid bit
          const uint8  g_bit_;                       // global bit
          const uint8  s_bit_;                       // sasid bit
          const uint8  rk_bit_;
          const uint8  wk_bit_;
          const uint8  ek_bit_;
          const uint8  ru_bit_;
          const uint8  wu_bit_;
          const uint8  eu_bit_;
          const uint8  fc_bit_;
        };

        // Enum capturing valid indices into PageDescriptorFormatEntry table
        enum PageDescriptorFormatEntryIndex {
#define DECL_ENUM(str_,...) kIndex##str_,
          PAGE_DESCR_CONFIG_LIST(DECL_ENUM)
#undef DECL_ENUM
          kPageDescriptorFormatEntryIndexCount,
        };

        // ---------------------------------------------------------------------
        // Constructor/Destructor
        //
        explicit Mmu();
        ~Mmu();

        // Construct MMU instance given a MMU configuration
        //
        void construct(arcsim::sys::cpu::Processor* cpu, MmuArch& mmu_arch);

        // ---------------------------------------------------------------------
        // MMU operations
        //
        void    command         (uint32 command);
        void    write_pid       (uint32 pid);

        // Lookup virtual to physical instruction address mapping
        uint32  lookup_exec(uint32 addr, bool mode, uint32& phys_addr) const;


        uint32  lookup_mpu_perms(uint32 addr, uint32 &perms) const;
        
        // Lookup virtual to physcial data address mapping
        uint32  lookup_data(uint32 addr, bool mode, uint32& phys_addr) const;

        // Translate virtual to physical addresses
        // NOTE: these methods have side-effects
        bool    translate_read  (uint32 addr, bool mode, uint32& phys_addr);
        bool    translate_write (uint32 addr, bool mode, uint32& phys_addr);
        bool    translate_rmw   (uint32 addr, bool mode, uint32& phys_addr);
        uint32  translate_exec  (uint32 addr, bool mode, uint32& phys_addr);

      private:

        struct EntryTLB {  // TLB entry structure
          uint32 virt_pd0; // PD0 ... virtual  half of TLB entry
          uint32 phys_pd1; // PD1 ... physical half of TLB entry
        };

        const uint32 kGlobalTlbEnableMask;
        const uint32 kSharedLibraryAsidEnableMask;
        
        arcsim::sys::cpu::Processor* cpu_;    // reference to processor
        PageDescriptorFormatEntry*   pd_fmt_; // pointer to active PD format entry

        uint32      version_;                 // version 1,2,3 are supported
        uint32      kind_;

        uint32      mpu_num_regions;

        // uITLB/uDTLB data structures
        static const uint32 kuITLB_size = 4;
        static const uint32 kuDTLB_size = 8;
        EntryTLB    uitlb_[kuITLB_size];
        EntryTLB    udtlb_[kuDTLB_size];

        // JTLB dynamically allocated and indexed as jTLB[SET][WAY]
        EntryTLB**  jtlb_;
        uint32      jtlb_sets_;
        uint32      jtlb_ways_;
        uint32      jtlb_index_mask_;
        uint32      jtlb_index_max_;
        uint32      jtlb_sets_log2_;
        uint32      jtlb_ways_log2_;

        bool        is_global_tlb_enabled_;           // is MMU enabled or disabled
        bool        is_shared_library_asid_enabled_;  // is SASID matching enabled or disabled 
        
        uint32      unmapped_base_address_; // addresses above this value will not be translated

        uint32      valid_asid_;            // ASID ...Address Space Identifier Mask
        uint32      valid_sasid_;           // SASID...Shared Address Space Identifier Mask
        uint32      valid_global_;          

        // Ways and indices marked for replacement
        uint32      rnd_jtlb_way_;
        uint32      rnd_uitlb_idx_;
        uint32      rnd_udtlb_idx_;

        // ---------------------------------------------------------------------

        // Extract various information from virtual and physical addresses
        inline uint32 get_virt_page_num(uint32 pd0)               const;
        inline uint32 get_phys_page_num(uint32 pd1)               const;
        inline uint32 get_phys_addr(uint32 pd1, uint32 virt_addr) const;
        inline uint32 get_phys_addr_perm(uint32 pd1)              const;
        
        inline uint32 get_valid_asid_or_sasid_search_key(uint32 vpn, uint32 vaddr, uint32 sasid) const;
        inline uint32 get_valid_global_search_key(uint32 vpn) const;
        
        // Destroy instance and free heap allocated re-sources
        void    destroy();

        // Methods used for instruction/data address translation
        //
        bool    translate_data_addr (uint32 addr, uint32& phys_addr, uint32& perms);
        bool    translate_inst_addr (uint32 addr, uint32& phys_addr, uint32& perms);

        // Translate instruction/data address without causing any side-effects
        bool    lookup_inst_addr(uint32 addr, uint32& phys_addr, uint32& perms) const;
        bool    lookup_data_addr(uint32 addr, uint32& phys_addr, uint32& perms) const;

        // Methods implementing MMU commands
        void    tlb_write (bool invalidate_utlbs);
        void    tlb_read ();
        void    tlb_get_index ();
        void    tlb_probe ();
        void    utlb_clear ();
      };

} } } // arcsim::sys::cpu


#endif  // INC_SYS_MEM_MMU_H_

