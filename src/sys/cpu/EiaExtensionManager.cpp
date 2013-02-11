//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// EIA Extension management class implementation
//
// =====================================================================

#include <iomanip>

#include "sys/cpu/EiaExtensionManager.h"

#include "ise/eia/EiaExtensionInterface.h"
#include "ise/eia/EiaInstructionInterface.h"
#include "ise/eia/EiaCoreRegisterInterface.h"
#include "ise/eia/EiaAuxRegisterInterface.h"
#include "ise/eia/EiaConditionCodeInterface.h"

#include "util/Log.h"

namespace arcsim {
  namespace sys {
    namespace cpu {
      
      
      // Maximum number of possible extension condition codes
      //
      static const uint8 kMaxExtCodes = 16;
      
      EiaExtensionManager::EiaExtensionManager()
      : are_eia_core_regs_defined(false),
        are_eia_instructions_defined(false),
        are_eia_aux_regs_defined(false),
        are_eia_cond_codes_defined(false),
        any_eia_extensions_defined(false)
      { 
        for (int i = 0; i < kMaxExtCodes; i++)
          eia_pred_names[i] = 0;
      }
      
      EiaExtensionManager::~EiaExtensionManager()
      {
        // Clean-up potential EIA extensions that have been registered with this EIA Manager
        //
        for (std::map<std::string,arcsim::ise::eia::EiaExtensionInterface*>::const_iterator
             I = eia_extension_map.begin(),
             E = eia_extension_map.end();
             I != E; ++I)
        {
          if (I->second) { delete I->second; }
        }
        
        // Remove predicate strings for extensions that were registered
        //
        for (int i = 0; i < kMaxExtCodes; i++)
          if (eia_pred_names[i])
            delete eia_pred_names[i];
      }
      
      bool
      EiaExtensionManager::add_eia_extension(uint32 cpu_id,
                                             arcsim::ise::eia::EiaExtensionInterface* eia_ext)
      {
        bool success = true;
                
        if (eia_ext != NULL)
        {
          // 1. Keep EIA Extensions in 'eia_extension_map'
          //
          eia_extension_map[eia_ext->get_name()] = eia_ext;
          
          LOG(LOG_INFO) << "[CPU" << cpu_id << "] Registered EIA Extension '"
                        << eia_ext->get_name() << "'";
          
          // -------------------------------------------------------------------
          // 2. Efficiently encode EIA instructions into the 'opcode_eia_instruction_map'
          //
          uint32 inst_array_size  = eia_ext->get_eia_instructions_count();
          if ( inst_array_size > 0 )
          { // Extension instructions have been defined
            //
            are_eia_instructions_defined = true;
            any_eia_extensions_defined   = true;

            // Get HEAP allocated array of EiaInstructionInterface pointers
            //
            arcsim::ise::eia::EiaInstructionInterface** inst_array = eia_ext->get_eia_instructions();
            // Iterate over all extension instructions
            //
            for (uint32 i = 0; i < inst_array_size; ++i)
            {
              arcsim::ise::eia::EiaInstructionInterface& inst = *inst_array[i];
              // Create key for 'opcode_eia_instruction_map'
              //
              uint32 opcode_major = inst.get_opcode(arcsim::ise::eia::EiaInstructionInterface::OPCODE_MAJOR);
              uint32 opcode       = inst.get_opcode(arcsim::ise::eia::EiaInstructionInterface::OPCODE);
              uint32 key          = ( ( (opcode_major & 0x1F) << 6) | (opcode & 0x3F) ); 
              
              // Register availability of EIA instruction for major opcode
              //
              eia_major_opcode_enabled_bitset[opcode_major] = 1;
              
              // Now we can insert a pointer to an EiaInstructionInterface into the
              // 'opcode_eia_instruction_map' using our key
              //
              opcode_eia_instruction_map[key] = inst_array[i];
              
              LOG(LOG_DEBUG2) << "[CPU" << cpu_id << "] Registered EIA Instruction '"
                              << inst.get_name() << "' with major opcode'0x"
                              << std::hex << std::setw(8) << std::setfill('0')
                              << opcode_major
                              << "' in fast lookup map with key: '0x"
                              << std::hex << std::setw(8) << std::setfill('0')
                              << key << "'.";
            }
            // Remove heap allocated array of EiaInstructionInterface pointers
            //
            delete [] inst_array;
          }
          // -------------------------------------------------------------------
          // 3. Efficiently encode EIA core registers into the 'eia_core_reg_map'
          //
          uint32 core_reg_array_size  = eia_ext->get_core_registers_count();
          if ( core_reg_array_size > 0 )
          { // Extension core registers have been defined
            //
            are_eia_core_regs_defined    = true;
            any_eia_extensions_defined   = true;
            
            // Get HEAP allocated array of EiaCoreRegisterInterface pointers
            //
            arcsim::ise::eia::EiaCoreRegisterInterface** core_reg_array = eia_ext->get_core_registers();
            // Iterate over all extension core registers
            //
            for (uint32 i = 0; i < core_reg_array_size; ++i)
            {
              arcsim::ise::eia::EiaCoreRegisterInterface& reg = *core_reg_array[i];
              // Insert extension core register into 'eia_core_reg_map'
              //
              eia_core_reg_map[reg.get_number()] = core_reg_array[i];
              
              LOG(LOG_DEBUG2) << "[CPU" << cpu_id << "] Registered EIA Core Register '"
                              << reg.get_name()   << "' number '"
                              << reg.get_number() << "' with initial value '0x"
                              << std::hex << std::setw(8) << std::setfill('0')
                              << reg.get_value()  << "'.";
            }
            // Remove heap allocated array of EiaCoreRegisterInterface pointers
            //
            delete [] core_reg_array;
          }
          
          // -------------------------------------------------------------------
          // 4. Efficiently encode EIA aux registers into the 'eia_aux_reg_map'
          //
          uint32 aux_reg_array_size  = eia_ext->get_aux_registers_count();
          if ( aux_reg_array_size > 0 )
          { // Extension core registers have been defined
            //
            are_eia_aux_regs_defined     = true;
            any_eia_extensions_defined   = true;
            
            // Get HEAP allocated array of EiaCoreRegisterInterface pointers
            //
            arcsim::ise::eia::EiaAuxRegisterInterface** aux_reg_array = eia_ext->get_aux_registers();
            // Iterate over all extension aux registers
            //
            for (uint32 i = 0; i < aux_reg_array_size; ++i)
            {
              arcsim::ise::eia::EiaAuxRegisterInterface& reg = *aux_reg_array[i];
              // Insert extension aux register into 'eia_aux_reg_map'
              //
              eia_aux_reg_map[reg.get_number()] = aux_reg_array[i];
              
              LOG(LOG_DEBUG2) << "[CPU" << cpu_id << "] Registered EIA AUX Register '"
                              << reg.get_name()   << "' number '0x"
                              << std::hex << std::setw(8) << std::setfill('0')
                              << reg.get_number() << "' with initial value '0x"
                              << std::hex << std::setw(8) << std::setfill('0')
                              << reg.get_value()  << "'.";
            }
            // Remove heap allocated array of EiaCoreRegisterInterface pointers
            //
            delete [] aux_reg_array;
          }
          
          // -------------------------------------------------------------------
          // 5. Efficiently encode EIA condition codes into the 'eia_conditions_map'
          //
          uint32 cond_code_array_size  = eia_ext->get_cond_codes_count();
          if ( cond_code_array_size > 0 )
          { // Extension core registers have been defined
            //
            are_eia_cond_codes_defined   = true;
            any_eia_extensions_defined   = true;
            
            // Get HEAP allocated array of EiaConditionCodeInterface pointers
            //
            arcsim::ise::eia::EiaConditionCodeInterface** cond_code_array = eia_ext->get_cond_codes();
            // Iterate over all extension condition codes
            //
            for (uint32 i = 0; i < cond_code_array_size; ++i)
            {
              arcsim::ise::eia::EiaConditionCodeInterface& cond = *cond_code_array[i];
              
              // Check to see if this condition code number is already assigned
              //
              uint32 cc_num = cond.get_number();
              
              if ((cc_num < 16) || (cc_num > 31)) {
                LOG(LOG_ERROR) <<  "[CPU" << cpu_id  << "] EIA Condition '"
                               << cond.get_name()   << "' uses number '0x"
                               << std::hex << std::setw(8) << std::setfill('0')
                               << cc_num
                               << ", which is not in the extension range (0x10-0x1f)";
                success = false;
              }
              
              if (eia_cc_names[cc_num]) {
                LOG(LOG_ERROR) <<  "[CPU" << cpu_id  << "] EIA Condition '"
                               << cond.get_name()   << "' redefines number '0x"
                               << std::hex << std::setw(8) << std::setfill('0')
                               << cc_num
                               << ", which was previously '"
                               << eia_cc_names[cc_num]
                               << "'";
                success = false;
              }
              
              if (success) {
                // Insert extension condition code into 'eia_cond_code_map'
                //
                eia_cond_code_map[cond.get_number()] = cond_code_array[i];

                LOG(LOG_DEBUG2) << "[CPU" << cpu_id  << "] Registered EIA Condition '"
                                << cond.get_name()   << "' number '0x"
                                << std::hex << std::setw(8) << std::setfill('0')
                                << cc_num;
              
                // Set up the table entry for this condition, for fast lookup
                // during disassembly.
                //
                eia_cc_names[cc_num] = new std::string(cond_code_array[i]->get_name());

                // Create predicate for this condition, based on its name.
                // Also used during disassembly.
                //
                eia_pred_names[cc_num-16] = new std::string(".");
                eia_pred_names[cc_num-16]->append(cond_code_array[i]->get_name());
              }
            }
            // Remove heap allocated array of EiaConditionCodeInterface pointers
            //
            delete [] cond_code_array;
          }

          
        } else {
          success = false;
        }
        return success;
      }
      
      
      
} } } //  arcsim::sys::cpu

