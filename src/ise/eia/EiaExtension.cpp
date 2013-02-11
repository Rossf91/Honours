//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================

#include "ise/eia/EiaExtension.h"
#include "ise/eia/EiaInstructionInterface.h"
#include "ise/eia/EiaConditionCodeInterface.h"
#include "ise/eia/EiaAuxRegisterInterface.h"
#include "ise/eia/EiaCoreRegisterInterface.h"

#include "system.h"
#include "sys/cpu/processor.h"

#include "util/Log.h"

// -----------------------------------------------------------------------------
// Register EIA Extensions with appropriate processor
//
void
simRegisterEiaExtension (simContext               sim_ctx,
                         uint32                   cpu_id,
                         EiaExtensionInterfacePtr eia_ext_ptr)
{
  if (sim_ctx == NULL || eia_ext_ptr == NULL) return;
  
  // Cast EiaExtensionInterfacePtr to arcsim::ise::eia::EiaExtensionInterface*
  //
  arcsim::ise::eia::EiaExtensionInterface* eia_ext = reinterpret_cast<arcsim::ise::eia::EiaExtensionInterface*>(eia_ext_ptr);

  // Cast sim_ctx to System*
  //
  System& sys = *reinterpret_cast<System*>(sim_ctx);

  // Check if processor with given ID exists
  //
  if (cpu_id < sys.total_cores) {  
    // Register EIA extension with processor
    //
    sys.cpu[cpu_id]->register_eia_extension(eia_ext);
  }
}

namespace arcsim {
  namespace ise {
    namespace eia {
      
      EiaExtension::EiaExtension(uint32 _id, std::string _name, std::string _comment)
      : id_(_id),
        name_(_name),
        comment_(_comment)
      { /* EMPTY */ }

      EiaExtension::~EiaExtension()
      {
        // ---------------------------------------------------------------------
        // Destroy heap-allocated structures
        //
        
        // EiaInstructions
        //
        for (std::map<std::string,EiaInstructionInterface*>::const_iterator
                I = inst_map_.begin(),
                E = inst_map_.end();
             I != E; ++I)
        { delete I->second; }

        // EiaConditionCodes
        //
        for (std::map<std::string,EiaConditionCodeInterface*>::const_iterator
                I = cond_code_map_.begin(),
                E = cond_code_map_.end();
             I != E; ++I)
        { delete I->second; }

        // EiaCoreRegisters
        //
        for (std::map<std::string,EiaCoreRegisterInterface*>::const_iterator
                I = core_reg_map_.begin(),
                E = core_reg_map_.end();
             I != E; ++I)
        { delete I->second; }
        
        // EiaAuxRegisters
        //
        for (std::map<std::string,EiaAuxRegisterInterface*>::const_iterator
                I = aux_reg_map_.begin(),
                E = aux_reg_map_.end();
             I != E; ++I)
        { delete I->second; }
      }
      
      // Return EIA instruction version number -------------------------------
      //
      uint32
      EiaExtension::get_version() const
      {
        // Every EIA extension that is built using this API will have this version
        // number compiled in. If the API changes the base class will be changed to
        // include the new version number. Thus any EIA Extensions that are built
        // using this base class will have the new version number compiled in.
        //
        return EiaExtensionInterface::EIA_API_VERSION_1;
      }
      
      // Modify available insns, cond codes Method ---------------------------
      //
      
      // Derived classes and friends can modify and add things to an extension
      //
      void EiaExtension::add_eia_instruction(EiaInstructionInterface* _inst)
      {
        inst_map_[ _inst->get_name() ] = _inst;
      }
      
      void EiaExtension::add_cond_code(EiaConditionCodeInterface* _cond_code)
      {
        cond_code_map_[ _cond_code->get_name() ] = _cond_code;
      }
      
      void EiaExtension::add_core_register(EiaCoreRegisterInterface* _core_reg)
      {
        core_reg_map_[ _core_reg->get_name() ] = _core_reg;
      }
      
      void EiaExtension::add_aux_register(EiaAuxRegisterInterface* _aux_reg)
      {
        aux_reg_map_[ _aux_reg->get_name() ] = _aux_reg;
      }      
      
} } } //  arcsim::ise::eia
