//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#include "EiaWhizzBangExtension.h"

/**
 * simLoadEiaExtension() function must be implemented by each shared library
 * that implementes one or more EiaExtensions. It is called by an external agent
 * (i.e. simulator) and allows the library to register all EiaExtensions it
 * implements by using the @see simRegisterEiaExtension(simContext, cpu_id, EiaExtensionInterfacePtr)
 * API function.
 *
 */
void
simLoadEiaExtension (simContext ctx)
{
  // Register EIA extension for appropriate context and cpu id
  //
  simRegisterEiaExtension(ctx, 0, arcsim::ise::eia::EiaWhizzBangExtension::create());
}

/**
 * The simCreateEiaExtension() function CAN be implemented by a shared library
 * that contains one or more EiaExtensions. It is called by a test harness at the
 * appropriate time in order to retrieve a fully instantiated EiaExtension for
 * testing. If a shared library contains several EiaExtensions the test harness
 * will call this method incrementing the parameter id by 1 and starting from 0,
 * until a null pointer is returned. So if a shared library implements 10 EiaExtensions,
 * it should return an instance to the first EiaExtension for id 0, to the second
 * for id 1, etc. until it is called with id '10' when it should return NULL to
 * signal the test harness that all EiaExtensions have been instantiated.
 *
 * @param the id of the EIA instruction that should be returned
 * @return EiaExtensionInterfacePtr to fully instantiated EiaExtension
 *
 */
EiaExtensionInterfacePtr
simCreateEiaExtension(uint32 eia_id)
{
  if (eia_id == 0) { return arcsim::ise::eia::EiaWhizzBangExtension::create(); }
  else             { return NULL; };
}

// -----------------------------------------------------------------------------

namespace arcsim {
  namespace ise {
    namespace eia {
      
      /**
       * EiaWhizzBangExtensionId number
       *
       * NOTE: Each extension has a unique integer identifier in the range
       *       0 to 31. User extensions must be assigned numbers from 16 to 31,
       *       whereas manufacturer's proprietary extensions are numbered
       *       from 0 to 15. It is the responsibility of the User and/or the
       *       system integration tools to ensure that each extension in the
       *       same system has a unique identifier that meets these requirements.
       */
      const uint32 EiaWhizzBangExtensionId = 16;
      
      // -----------------------------------------------------------------------
      
      /**
       * EiaWhizzBangConditionCode constructor.
       *
       * NOTE: The constructor of the base class is called in the initialisation
       *       list re-using the common init functionality of the base class.
       *
       * @param _eia_extension pointer to the enclosing EiaExtension.
       */
      EiaWhizzBangConditionCode::EiaWhizzBangConditionCode(EiaExtension* _eia_extension)
      : EiaConditionCode(_eia_extension,           /* EIA extension */
                         "whizz_cc",               /* name */
                         17),                      /* number */
        whizz_aux_reg (*_eia_extension->get_aux_register("whizz_aux_reg")->get_value_ptr()),
        whizz_core_reg(*_eia_extension->get_core_register("whizz_core_reg")->get_value_ptr())
      { /* EMPTY */ }
      
      /**
       * This method overrides the default implementation of EiaConditionCode.
       * It implements the behaviour of this Extension Condition Code.
       * @param cc condition code.
       * @return Outcome of condition code evaluation.
       */
      bool
      EiaWhizzBangConditionCode::eval_condition_code(uint8 _cc)
      {
        // Whizzy condition code evaluation
        //
        if (_cc && whizz_aux_reg || whizz_core_reg) {          
          ++whizz_aux_reg;
          ++whizz_core_reg;        
          return true;
        } else {
          ++whizz_aux_reg;
          ++whizz_core_reg;        
          return false;
        } 
      }
      
      // -----------------------------------------------------------------------
      
      /**
       * EiaWhizzBangInstruction constructor.
       *
       * NOTE: The constructor of the base class is called in the initialisation
       *       list re-using the common init functionality of the base class.
       *
       * @param _eia_extension pointer to the enclosing EiaExtension.
       */
      EiaWhizzBangInstruction::EiaWhizzBangInstruction(EiaExtension* _eia_extension)
      : EiaInstruction(_eia_extension,                        /* EIA extension */
                       "whizz",                               /* name */
                       EiaInstructionInterface::DUAL_OPD,     /* kind */
                       0x7,                                   /* major opcode */
                       0x2,                                   /* opcode */
                       3,                                     /* cycles */
                       true,                                  /* has_dst */
                       false,                                 /* is_blocking */
                       true),                                 /* is_flag_setting */
        whizz_aux_reg (*_eia_extension->get_aux_register("whizz_aux_reg")->get_value_ptr()),
        whizz_core_reg(*_eia_extension->get_core_register("whizz_core_reg")->get_value_ptr())
      { /* EMPTY */ }
      
      
      /**
       * This method overrides the default implementation of EiaInstruction.
       * It implements the behaviour of this Extension Instruction. Exactly
       * what method one should override from the parent EiaInstruction class
       * depends on the instruction kind (@see arcsim::ise::eia::EiaInstructionInterface::Kind)
       * and if this instruction is flag setting.
       *
       * Because EiaWhizzBangInstruction is a DUAL_OP flag_setting instruction
       * it needs to override the eval_dual_opd() method from the parent.
       *
       * @param src1 first operand.
       * @param src2 second operand.
       * @param bflags_in input bflags.
       * @param xflags_in input xflags.
       * @param bflags_out because this is a flag setting instruction it receives a pointer to bflags it can modify.
       * @param xflags_out because this is a flag setting instruction it receives a pointer to xflags it can modify.
       * @return The result of this instruction.
       */
      uint32
      EiaWhizzBangInstruction::eval_dual_opd (uint32     src1,
                                              uint32     src2,
                                              EiaBflags  bflags_in,
                                              EiaXflags  xflags_in,
                                              EiaBflags* bflags_out,
                                              EiaXflags* xflags_out)
      {
        uint32 ret_val = 0x0;
        
        // If the 'blfags_in.V' flag is set we add 'src1' and 'src2' together and set
        // xflags_out->X0 to '0'
        //
        if (bflags_in.V) {
          ret_val = src1 + src2 + whizz_core_reg;
          xflags_out->X0 = 0;
        } else {
          // Otherwise we do the following WhizzBang magic calculation
          //
          ret_val = src1 - src2 + 0x77ff - whizz_aux_reg;
          xflags_out->X0 = 0;
        }
        
        // If xflags_in.X3 is set we increment the result by one
        //
        if (xflags_in.X3) {
          ++ret_val;
        }
        ++whizz_aux_reg;
        ++whizz_core_reg;        
        return ret_val;
      }


      // -----------------------------------------------------------------------
      
      
      /**
       * EiaWhizzBangExtension constructor.
       * @param name extension name.
       * @param comment extension comment.
       */
      EiaWhizzBangExtension::EiaWhizzBangExtension(uint32 _id,
                                                   std::string _name,
                                                   std::string _comment)
      : EiaExtension(_id,
                     _name,
                     _comment)
      { /* EMPTY */ }
      
      
      /**
       * EiaExtension factory method that is called in order to construct a
       * EiaWhizzBangExtension.
       *
       * NOTE: Initialisation depends on a certain very simple order:
       *       1. Creation of EiaExtension
       *       2. Creation of one or more EiaCoreRegisters
       *       3. Creation of one or more EiaAuxRegisters
       *       4. Creation of one or more EiaConditionCodes
       *       5. Creation of one or more EiaInstructions
       *
       * After creating each extension element such as a core/aux register, cond
       * codes or instructions, they must be added to the EiaExtension by using
       * the appropriate 'add_*()' methods.
       *
       */
      EiaExtensionInterface*
      EiaWhizzBangExtension::create_internal()
      { // 1. Create HEAP allocated EiaWhizzBangExtension, with id of 16 supplied
        //    either by the author of the extension, or by an integration tool.
        //
        EiaWhizzBangExtension* eia = new EiaWhizzBangExtension(EiaWhizzBangExtensionId,
                                                               "WhizzBangExtension",
                                                               "WhizzBangExtension comment");
        // 2. Add EiaCoreRegister to EiaWhizzBangExtension
        //
        eia->add_core_register(new EiaCoreRegister(eia,               /* parent */
                                                   "whizz_core_reg",  /* name   */
                                                   56,                /* number */
                                                   0,                 /* initial value */
                                                   true,              /* w_direct */
                                                   false,             /* w_prot */
                                                   false,             /* w_only */
                                                   false));           /* r_only */
        // 3. Add EiaAuxRegister to EiaWhizzBangExtension
        //
        eia->add_aux_register(new EiaAuxRegister(eia,             /* parent */
                                                 "whizz_aux_reg", /* name   */
                                                 0x4243,          /* number */
                                                 0,               /* initial value */
                                                 true,            /* w_direct */
                                                 false,           /* w_prot */
                                                 false,           /* w_only */
                                                 false));         /* r_only */
        // 4. Add EiaWhizzBangConditionCode to EiaWhizzBangExtension
        //
        eia->add_cond_code(new EiaWhizzBangConditionCode(eia));
        // 5. Add EiaWhizzBangInstruction to EiaWhizzBangExtension
        //
        eia->add_eia_instruction(new EiaWhizzBangInstruction(eia));
        
        return eia;
      }
      
      
} } } //  arcsim::ise::eia
