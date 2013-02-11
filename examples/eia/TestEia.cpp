//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
//  Description:
//
//  Test program that loads and inspects an EIA extension library that
//  adheres to the EIA API. Furthermore extension instruction methods
//  and extension codition code evaluation methods are executed for test
//  purposes.
//
// =============================================================================

#include <cstdlib>

#include <map>
#include <string>

// EIA API headers
//
#include "ise/eia/EiaExtensionInterface.h"
#include "ise/eia/EiaInstructionInterface.h"
#include "ise/eia/EiaConditionCodeInterface.h"
#include "ise/eia/EiaCoreRegisterInterface.h"
#include "ise/eia/EiaAuxRegisterInterface.h"

// Simulator headers for loading of shared libraries and logging
//
#include "util/system/SharedLibrary.h"
#include "util/Log.h"
#include "util/OutputStream.h"

// Import EIA namespaces in order to do less typing when specifying class
// names and types provided by the EIA API.
//
using namespace arcsim::ise::eia;
using namespace arcsim::util::system;

int
main(int argc, char** argv)
{
  if (argc != 2) {
    LOG(LOG_ERROR) << " Supply exactly ONE argument which is the path to an EIA shared object.";
    return EXIT_FAILURE;
  }
 
  void* plugin_handle   = NULL;
  void* factory_handle = NULL;

  // Open shared library
  //
  SharedLibrary::open(&plugin_handle, argv[1]);
  
  // Lookup EIA 'factory' function
  //
  SharedLibrary::lookup_symbol(&factory_handle, plugin_handle, "simCreateEiaExtension");

  // Cast to proper function type
  //
  EiaExtensionInterfacePtr (*eia_create)(uint32) = (EiaExtensionInterfacePtr (*)(uint32))factory_handle;
  
  // Execute init function
  //
  EiaExtensionInterface* eia_ext = (EiaExtensionInterface*)(*eia_create)(0);    
  
  
  PRINTF() << "\n\n-- Loaded EIA extension: '"
           << eia_ext->get_name()
           << "' (Version: "
           << eia_ext->get_version()
           << ", Id: "
           << eia_ext->get_id()
           << ")\n";
  
  // Retrieve all instructions, condition codes, core and auxiliary registers
  //
  
  EiaInstructionInterface** inst_array      = eia_ext->get_eia_instructions();
  uint32 inst_array_size                    = eia_ext->get_eia_instructions_count();
  
  EiaConditionCodeInterface** cc_array      = eia_ext->get_cond_codes();
  uint32 cc_array_size                      = eia_ext->get_cond_codes_count();
  
  EiaCoreRegisterInterface** core_reg_array = eia_ext->get_core_registers();
  uint32 core_reg_array_size                = eia_ext->get_core_registers_count();

  EiaAuxRegisterInterface** aux_reg_array   = eia_ext->get_aux_registers();
  uint32 aux_reg_array_size                 = eia_ext->get_aux_registers_count();

  // Iterate over all defined core registers
  //
  for (uint32 i = 0; i < core_reg_array_size; ++i)
  {
    EiaCoreRegisterInterface& reg = *core_reg_array[i];
    PRINTF() << "   |\n"
             << "   +-- EIA CORE REG: '"  << reg.get_name()
             << "' NUM: '"                << reg.get_number()
             << "' VALUE: '"              << reg.get_value() << "'\n";
  }
  
  // Iterate over all aux registers
  //
  for (uint32 i = 0; i < aux_reg_array_size; ++i)
  {
    EiaAuxRegisterInterface& reg = *aux_reg_array[i];
    PRINTF() << "   |\n"
             << "   +-- EIA AUX REG: '" << reg.get_name()
             << "' NUM: '"              << reg.get_number()
             << "' VALUE: '"            << reg.get_value() << "'\n";
  }

  
  // Iterate over all extension instructions
  //
  for (uint32 i = 0; i < inst_array_size; ++i)
  {
    EiaInstructionInterface& inst = *inst_array[i];
    
    PRINTF() << "   |\n"
             << "   +-- EIA INSTRUCTION: '" << inst.get_name() << "'\n";
    
    EiaBflags bflags = {0,0,0,0};
    EiaXflags xflags = {0,0,0,0};

    EiaBflags bflags_out = {0,0,0,0};
    EiaXflags xflags_out = {0,0,0,0};

    switch (inst.get_kind()) {
      case EiaInstructionInterface::DUAL_OPD:
      {
        // Call eval method
        //
        inst.eval_dual_opd(10, 10, bflags, xflags);
        inst.eval_dual_opd(10, 10, bflags, xflags, &bflags_out, &xflags_out);
        break;
      }
      case EiaInstructionInterface::SINGLE_OPD:
      {
        // Call eval method
        //
        inst.eval_single_opd(10, bflags, xflags);
        inst.eval_single_opd(10, bflags, xflags, &bflags_out, &xflags_out);
        break;
      }
      case EiaInstructionInterface::ZERO_OPD:
      {
        // Call eval method
        //
        inst.eval_zero_opd(bflags, xflags);
        inst.eval_zero_opd(bflags, xflags, &bflags_out, &xflags_out);
        break;
      }

    }
  }
  
  // Iterate over all defined core registers
  //
  for (uint32 i = 0; i < core_reg_array_size; ++i)
  {
    EiaCoreRegisterInterface& reg = *core_reg_array[i];
    PRINTF() << "   |\n"
             << "   +-- EIA CORE REG: '"  << reg.get_name()
             << "' NUM: '"                << reg.get_number()
             << "' VALUE: '"              << reg.get_value() << "'\n";
  }
  
  // Iterate over all aux registers
  //
  for (uint32 i = 0; i < aux_reg_array_size; ++i)
  {
    EiaAuxRegisterInterface& reg = *aux_reg_array[i];
    PRINTF() << "   |\n"
             << "   +-- EIA AUX REG: '" << reg.get_name()
             << "' NUM: '"              << reg.get_number()
             << "' VALUE: '"            << reg.get_value() << "'\n";
  }

  // Iterate over all condition codes
  //

  for (uint32 i = 0; i < cc_array_size; ++i)
  {
    EiaConditionCodeInterface& cond_code = *cc_array[i];
    PRINTF() << "   |\n"
             << "   +-- EIA COND CODE: '" << cond_code.get_name() << "'\n";
    // Call eval CC method
    //
    cond_code.eval_condition_code(0);
    
  }
  
  // Iterate over all defined core registers
  //
  for (uint32 i = 0; i < core_reg_array_size; ++i)
  {
    EiaCoreRegisterInterface& reg = *core_reg_array[i];
    PRINTF() << "   |\n"
             << "   +-- EIA CORE REG: '"  << reg.get_name()
             << "' NUM: '"                << reg.get_number()
             << "' VALUE: '"              << reg.get_value() << "'\n";
  }
  
  // Iterate over all aux registers
  //
  for (uint32 i = 0; i < aux_reg_array_size; ++i)
  {
    EiaAuxRegisterInterface& reg = *aux_reg_array[i];
    PRINTF() << "   |\n"
             << "   +-- EIA AUX REG: '" << reg.get_name()
             << "' NUM: '"              << reg.get_number()
             << "' VALUE: '"            << reg.get_value() << "'\n";
  }
    
  PRINTF() << "\n\n";
  
  // Destroy EIA extension
  //
  delete eia_ext;
  
  return EXIT_SUCCESS;
}

