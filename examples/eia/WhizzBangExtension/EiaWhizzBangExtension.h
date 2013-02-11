//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#ifndef INC_ISE_EIA_SAMPLE_EIAWHIZZBANGEXTENSION_H_
#define INC_ISE_EIA_SAMPLE_EIAWHIZZBANGEXTENSION_H_

// Header including base class and interface definition for EiaExtension
//
#include "ise/eia/EiaExtension.h"
// Header including EiaAuxRegister class
//
#include "ise/eia/EiaAuxRegister.h"
// Header including EiaCoreRegister class
//
#include "ise/eia/EiaCoreRegister.h"
// Header including base class and interface definition for EiaConditionCode
//
#include "ise/eia/EiaConditionCode.h"
// Header including base class and interface definition for EiaInstruction
//
#include "ise/eia/EiaInstruction.h"


namespace arcsim {
  namespace ise  {
    namespace eia {
      
      /**
       * EiaWhizzBangConditionCode class implementing an Extension Condition Code.
       */
      class EiaWhizzBangConditionCode:
        public EiaConditionCode
      {
        private:
          /**
           * Reference to Extension Auxiliary Register.
           * This reference is setup in the EiaWhizzBangConditionCode constructor.
           * If an EiaExtension defines several Extension Auxiliary Registers there
           * should be a reference to each one of them in this class, and all of them
           * should be setup in the constructor of EiaWhizzBangConditionCode.
           */
          uint32&   whizz_aux_reg;
        
          
          /**
           * Reference to Extension Core Register.
           * This reference is setup in the EiaWhizzBangConditionCode constructor.
           * If an EiaExtension defines several Core Auxiliary Registers there
           * should be a reference to each one of them in this class, and all of them
           * should be setup in the constructor of EiaWhizzBangConditionCode.
           */
          uint32&  whizz_core_reg;

        public:
        /**
         * EiaWhizzBangConditionCode constructor.
         * @param _eia_extension pointer to the enclosing EiaExtension.
         */
        explicit EiaWhizzBangConditionCode(EiaExtension* _eia_extension);

        /**
         * This method overrides the default implementation of EiaConditionCode.
         * It implements the behaviour of this Extension Condition Code.
         * @param cc condition code.
         * @return Outcome of condition code evaluation.
         */
        bool eval_condition_code (uint8 cc);
        
      };

      
      // -----------------------------------------------------------------------

      /**
       * EiaWhizzBangInstruction class implementing an Extension Instruction.
       */
      class EiaWhizzBangInstruction:
        public EiaInstruction
      {
      private:
        /**
         * Reference to Extension Auxiliary Register.
         * This reference is setup in the EiaWhizzBangInstruction constructor.
         * If an EiaExtension defines several Extension Auxiliary Registers there
         * should be a reference to each one of them in this class, and all of them
         * should be setup in the constructor of EiaWhizzBangConditionCode.
         */
        uint32&   whizz_aux_reg;
        
        /**
         * Reference to Extension Core Register.
         * This reference is setup in the EiaWhizzBangInstruction constructor.
         * If an EiaExtension defines several Core Auxiliary Registers there
         * should be a reference to each one of them in this class, and all of them
         * should be setup in the constructor of EiaWhizzBangConditionCode.
         */
        uint32&  whizz_core_reg;
        
      public:
        /**
         * EiaWhizzBangInstruction constructor.
         * @param _eia_extension pointer to the enclosing EiaExtension.
         */
        explicit EiaWhizzBangInstruction(EiaExtension* _eia_extension);
        
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
        uint32 eval_dual_opd (uint32     src1,
                              uint32     src2,
                              EiaBflags  bflags_in,
                              EiaXflags  xflags_in,
                              EiaBflags* bflags_out,
                              EiaXflags* xflags_out);

      };
      
      // -----------------------------------------------------------------------

      
      /**
       * EiaWhizzBangExtension class implementing an Eia Extension.
       * This class also implements EiaExtensionFactory class in order to provide
       * a well defined mathod that can be called upon creating/instantiating an
       * EiaExtension (i.e. Factory software design pattern).
       */
      class EiaWhizzBangExtension :
        public EiaExtension,
        public EiaExtensionFactory<EiaWhizzBangExtension>
      {
      public:
        
        /**
         * EiaWhizzBangExtension constructor.
         * @param name extension name.
         * @param comment extension comment.
         */
        explicit EiaWhizzBangExtension(uint32 id, std::string name, std::string comment);
        
        
        // EiaExtensionFactory Method ------------------------------------------
        //
        
        /**
         * EiaExtension factory method that is called in order to construct the
         * EiaWhizzBangExtension.
         */
        static EiaExtensionInterface* create_internal(void);

      };

      
} } } //  arcsim::ise::eia


#endif  // INC_ISE_EIA_SAMPLE_EIAWHIZZBANGEXTENSION_H_
