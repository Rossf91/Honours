//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// EIA Extension management class.
//
// =====================================================================


#ifndef INC_PROCESSOR_EIAEXTENSIONMANAGER_H_
#define INC_PROCESSOR_EIAEXTENSIONMANAGER_H_

#include "api/types.h"

#include <map>
#include <string>
#include <bitset>

namespace arcsim {

  // ---------------------------------------------------------------------------
  // Forward declarations
  //
  namespace ise {
    namespace eia {
      class EiaExtensionInterface;
      class EiaInstructionInterface;
      class EiaCoreRegisterInterface;
      class EiaAuxRegisterInterface;
      class EiaConditionCodeInterface;
    } }

  namespace sys {
    namespace cpu {

      class EiaExtensionManager
      {
      public:
        
        // Map of EIA extension interfaces registered for this processor indexed by their name
        //
        std::map<std::string,arcsim::ise::eia::EiaExtensionInterface*>  eia_extension_map;
        
        // Indicates whether or not EIA extension instructions have been defined
        //
        bool any_eia_extensions_defined;

        // ---------------------------------------------------------------------
        // EIA extension instructions
        //
        
        // Indicates whether or not EIA extension instructions have been defined
        //
        bool are_eia_instructions_defined;
        
        // Map of EIA instructions registered for this processor indexed by the following
        //  key : ( ( (opcode_major & 0x1F) << 6) | (opcode & 0x3F) );
        //
        std::map<uint32,arcsim::ise::eia::EiaInstructionInterface*>     opcode_eia_instruction_map;
        
        // Bitfield encoding whether one or more EIA instructions are defined for
        // one of the 32 major opcodes
        //
        std::bitset<32> eia_major_opcode_enabled_bitset;
        
        // ---------------------------------------------------------------------
        // EIA extension core registers
        //

        // Indicates whether or not EIA extension core registers have been defined
        //
        bool are_eia_core_regs_defined;
        
        // Map of EIA extension core regs registered for this processor indexed by their number
        //
        std::map<uint32,arcsim::ise::eia::EiaCoreRegisterInterface*>    eia_core_reg_map;
        
        // ---------------------------------------------------------------------
        // EIA extension aux registers
        //
        
        // Indicates whether or not EIA extension aux registers have been defined
        //
        bool are_eia_aux_regs_defined;

        // Map of EIA extension aux regs registered for this processor indexed by their number
        //
        std::map<uint32,arcsim::ise::eia::EiaAuxRegisterInterface*>    eia_aux_reg_map;

        
        // ---------------------------------------------------------------------
        // EIA extension condition codes
        //
        
        // Indicates whether or not EIA extension condition codes have been defined
        //
        bool are_eia_cond_codes_defined;

        // Map of EIA extension conditions registered for this processor indexed by number
        //
        std::map<uint32,arcsim::ise::eia::EiaConditionCodeInterface*>    eia_cond_code_map;

        // Array of condition names, used during disassembly. These are appended
        // to branch or jump mnemonics, so they have no preceding '.'
        // Null elements indicate undefined extension condition codes.
        //
        std::string* eia_cc_names[16];
        
        // Array of predicate names, used during disassembly. These require '.'
        // before the condition name. Null elements indicate undefined extension
        // condition codes.
        //
        std::string* eia_pred_names[16];
        
        // ---------------------------------------------------------------------

        
        // Constructor
        //
        EiaExtensionManager();
        
        // Destructor
        //
        ~EiaExtensionManager();
        
        // Add EIA Extension to manager
        //
        bool add_eia_extension(uint32 cpu_id, arcsim::ise::eia::EiaExtensionInterface* eia_ext);
        
      };
      
} } } //  arcsim::sys::cpu


#endif  // INC_PROCESSOR_EIAEXTENSIONMANAGER_H_

