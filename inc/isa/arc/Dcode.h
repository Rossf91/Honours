//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2004 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// The Dcode class represents a pre-decoded microcode instruction and provides
// a decode method that takes a microcode word and initiliases a Dcode object.
//
// Note that we want to keep this structure as small as possible.
//
// =====================================================================

#ifndef INC_ISA_ARC_DCODE_H_
#define INC_ISA_ARC_DCODE_H_

// ---------------------------------------------------------------------------
// HEADERS
//

#include "define.h"
#include "api/types.h"

#include "isa/arc/Opcode.h"

// Decode constants for enter_s and leave_s instructions only
const unsigned int ENTER_LEAVE_JMP_BIT  = 26;
const unsigned int ENTER_LEAVE_LINK_BIT = 25;
const unsigned int ENTER_LEAVE_FP_BIT   = 24;

// -----------------------------------------------------------------------------
// Forward declarations
//
class IsaOptions;
struct cpuState;
struct regStats;

namespace arcsim {

  // ---------------------------------------------------------------------------  
  // Forward declarations
  //
  namespace sys {
    namespace cpu {
      class EiaExtensionManager;
  } }

  namespace ise {
    namespace eia {
      class EiaInstructionInterface;
      class EiaConditionCodeInterface;
  } }
  
  namespace isa {
    namespace arc {
      
      class Dcode
      {
      public:
        // ---------------------------------------------------------------------
        // Instruction Kind type
        //
        enum Kind
        { // Arithmetic, Logical, Move instruction kinds
          //
          kArithmetic             = 0x00000001,
          kLogical                = 0x00000002,
          kMove                   = 0x00000004,
          kExtension              = 0x00000008,
          // Memory instruction kinds
          //
          kMemLoad                = 0x00000010,
          kMemStore               = 0x00000020,
          kMemExchg               = 0x00000040,
          kMemEnterLeave          = 0x00000080,
          // Control flow instruction kinds
          //
          kControlFlowBranch      = 0x00000100,
          kControlFlowJump        = 0x00000200,
          kControlFlowFlag        = 0x00000400,
          kControlFlowTrap        = 0x00000800,
          // 'Hint' instruction kinds
          //
          kHintNop                = 0x00001000,
          kHintSync               = 0x00002000,
          kHintSleep              = 0x00004000,
          // Illegal instruction
          //
          kException              = 0x80000000,
        };
        
        enum KindMask
        {
          kArithmLogicalMovKindMask= 0x0000000F,
          kMemoryKindMask          = 0x000000F0,
          kControlFlowKindMask     = 0x00000F00,
          kHintKindMask            = 0x0000F000,
        };

        // ---------------------------------------------------------------------
        // Struct encoding information about src, dst register numbers, instruction
        // bit pattern, and flags indicating which registers are used.
        //
        // SIZE: 4b + 9 * 1b = 13b
        //
        struct Info
        {
          uint32  ir;           // instruction bit pattern
          uint8   rf_wa0;       // dst register number
          uint8   rf_wa1;       // dst register number
          uint8   rf_ra0;       // src register number
          uint8   rf_ra1;       // src register number
          bool    rf_wenb0;     // flag indicating which dst regs are written
          bool    rf_wenb1;     // flag indicating which dst regs are written
          bool    rf_renb0;     // flag indicating which src regs are read
          bool    rf_renb1;     // flag indicating which src regs are read
          bool    isReturn;     // true if jumping through BLINK
          
          Info()  // Initialise info struct
          : ir(0),
            rf_wa0(0), rf_wenb0(false), rf_wa1(0), rf_wenb1(false),
            rf_ra0(0), rf_renb0(false), rf_ra1(0), rf_renb1(false),
            isReturn(false)
          { /* EMPTY */ }
        };
        
        // ---------------------------------------------------------------------
        // Member variables
        // 
        // 32-bit SIZE: 4b(Kind) + 13b(Info) + 4*8b + 6*sizeof(ptr) + 3*4b + 2b + 3*4b + 17*1b = 116b
        // 64-bit SIZE: 4b(Kind) + 13b(Info) + 4*8b + 6*sizeof(ptr) + 3*4b + 2b + 3*4b + 17*1b = 140b
        //
        Kind       kind;               // instruction kind
        Info       info;               // instruction info structure

        uint8      code;               // load, store, add, sub, bcc, jcc etc
        uint8      size;               // size of inst + limm if present
        uint8      link_offset;        // offset to return location
        uint8      q_field;            // condition field
        
        uint32     *src1;              // ptr to 1st source
        uint32     *src2;              // ptr to 2nd source
        uint32     *dst1;              // ptr to 1st destination
        uint32     *dst2;              // ptr to 2nd destination
        ise::eia::EiaInstructionInterface*   eia_inst; // ptr to object that implements instruction        
        ise::eia::EiaConditionCodeInterface* eia_cond; // ptr to object that implements condition
        
        uint32     limm;               // long immediate data

        // FIXME(iboehm): remove overloading of 'shimm' for ENTER and LEAVE as it
        //                causes only problems.
        uint32     shimm;              // short immediate data
        uint32     jmp_target;         // target of jump or branch
        uint16     addr_shift;         // shift by 1, 2 or 3 bits
        
        uint32     aps_inst_matches;   // actionpoint instruction matches        
        uint32     xpu_required;       // EIA permissions required in XPU register

        uint32     fetch_addr[3];      // fetch buffer address/es
        uint8      fetches;            // number of inst fetches required
        
        bool       flag_enable;        // F bit from the instruction
        bool       z_wen;              // enables write to Z flag
        bool       n_wen;              // enables write to N flag
        bool       c_wen;              // enables write to C flag
        bool       v_wen;              // enables write to V flag
        
        bool       pre_addr;           // source of address
        bool       link;               // controls write to BLINK
        
        // FIXME(iboehm): remove overloading of dslot for ENTER and LEAVE as it
        //                causes only problems.
        bool       dslot;              // true if inst HAS a delay-slot
        bool       in_dslot;           // true if inst IS in a delay slot
        bool       taken_branch;       // true if inst is branch and branch is taken
        bool       has_limm;           // true if inst has limm data
        bool       cache_byp;          // true if load/store bypasses cache
        
        bool       illegal_operand;
        bool       illegal_inst_format;
        bool       illegal_inst_subcode;
        bool       illegal_in_dslot;
                
        // ---------------------------------------------------------------------------
        // Additional fields for special simulation modes and options
        //
#ifdef REGTRACK_SIM
        // FIXME: define the following as a struct
        //

        // NOTE: During JIT code generation, these pointers will not point to
        //       the global cpuState structure but to a dummy cpuState.
        //
        regStats*   dst1_stats;           // ptr to 1st destination statistics
        regStats*   dst2_stats;           // ptr to 2nd destination statistics
        regStats*   src1_stats;           // ptr to 1st source statistics
        regStats*   src2_stats;           // ptr to 2nd source statistics
#endif /* REGTRACK_SIM */

#ifdef CYCLE_ACC_SIM
        // FIXME: define the following as a struct
        //
        uint32    fet_cycles;             // inst fetch cycles
        uint32    mem_cycles;             // inst memory cycles
        uint32    exe_cycles;             // inst execution cycles
        uint32    br_cycles;              // fixed extra cost of a branch
        uint32    extra_cycles;           // fixed extra stall cycles
        uint64*   src1_avail;             // ptr to src1 reg cycle
        uint64*   src2_avail;             // ptr to src2 reg cycle
        uint64*   dst1_avail;             // ptr to dst1 reg cycle
        uint64*   dst2_avail;             // ptr to dst2 reg cycle
        bool      pipe_flush;             // true => instruction flushes pipeline
#endif /* CYCLE_ACC_SIM */
                
        // ---------------------------------------------------------------------
        // Constructor
        //
        Dcode();
        
        // Destructor
        //
        ~Dcode();
        
        // Decode instruction
        //
        void decode (const IsaOptions&  isa_opts,
                     uint32             inst,
                     uint32             pc,
                     cpuState&          state,
                     const arcsim::sys::cpu::EiaExtensionManager& eia_mgr,
                     bool               from_dslot);
        
        // Decode to instruction error
        //
        void set_instruction_error(const IsaOptions& isa_opts,
                                   const arcsim::sys::cpu::EiaExtensionManager& eia_mgr);
        
        // Clear Dcode object
        //
        void clear (cpuState& state);

        // Determine if instruction has a delay slot instruction. NOTE dslot
        // is set dynamically.
        //
        // FIXME(iboehm): remove overloading of dslot for ENTER and LEAVE as it
        //                causes only problems.
        //
        inline bool has_dslot_inst() const {
          return dslot && (code != OpCode::ENTER) && (code != OpCode::LEAVE);
        }
        
        // ---------------------------------------------------------------------
        // Efficient instruction kind query methods
        //
        inline bool is_memory_kind_inst() const {
          return kind & arcsim::isa::arc::Dcode::kMemoryKindMask;
        }
        
        inline bool is_arithm_logical_mov_kind_inst() const {
          return kind & arcsim::isa::arc::Dcode::kArithmLogicalMovKindMask;
        }

        inline bool is_control_flow_kind_inst() const {
          return kind & kControlFlowKindMask;
        }

        inline bool is_hint_kind_inst() const {
          return kind & kHintKindMask;
        }

      private:
        // Disallow copy constructor and assignment for Dcode objects. If you really
        // need to copy an object of this class write a clone(const Dcode&) method.
        //
        DISALLOW_COPY_AND_ASSIGN(Dcode);
      };

} } }  // namespace arcsim::isa::arc

      
#endif  // INC_ISA_ARC_DCODE_H_
