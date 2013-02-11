//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#ifndef _INC_ISE_EIA_EIAINSTRUCTIONINTERFACE_H_
#define _INC_ISE_EIA_EIAINSTRUCTIONINTERFACE_H_

#include "api/types.h"

namespace arcsim {
  namespace ise {
    namespace eia {
      
      /**
       * EiaBflags structure.
       */
      typedef struct EiaBflags {
        char Z; /**< struct field Z. */
        char N; /**< struct field N. */
        char C; /**< struct field C. */
        char V; /**< struct field V. */
      } EiaBflags; /**< EiaBflags type. */
      
      /**
       * EiaXflags structure.
       */
      typedef struct EiaXflags {
        char X0; /**< struct field X0. */
        char X1; /**< struct field X1. */
        char X2; /**< struct field X3. */
        char X3; /**< struct field X4. */
      } EiaXflags; /**< EiaXflags type. */

      // -----------------------------------------------------------------------
      
      /**
       * EiaInstructionInterface.
       * The EiaInstructionInterface is an interface defining the methods/behaviour
       * of extension instructions. Each extension instruction MUST implement this
       * interface.
       */
      class EiaInstructionInterface
      {
      protected:
        // Constructor MUST be protected and empty!
        //
        EiaInstructionInterface()
        { /* EMPTY */ }
        
      public:
        /** 
         * ExtensionInstruction Kinds.
         */
        enum Kind {
          DUAL_OPD,   /**< DUAL_OPD - dual operand instruction kind.      */
          SINGLE_OPD, /**< SINGLE_OPD - single operand instruction kind.  */
          ZERO_OPD    /**< ZERO_OPD - zero operand instruction kind.      */
        };
        
        /** 
         * OpcodeType Kinds.
         */
        enum OpcodeType {
          OPCODE,       /**< OPCODE - instruction opcode.      */
          OPCODE_MAJOR  /**< OPCODE_MAJOR - major instruction opcode.      */
        };
        
        /**
         * Destructor of interface must be declared AND virtual so all implementations
         * of this interface can be destroyed correctly.
         */
        virtual ~EiaInstructionInterface()
        { /* EMPTY */ }

        /**
         * Get name of EiaInstruction.
         * @return instruction name.
         */
        virtual const char*   get_name() const = 0;
        
        /**
         * Get kind of EiaInstruction.
         * @return instruction kind.
         */
        virtual Kind          get_kind() const = 0;

        /**
         * Get identity of the owning EiaExtension.
         * @return extension identity.
         */
        virtual const uint32  get_id()         = 0;
        
        /**
         * Get requested instruction opcode.
         * @param opc_type the requested opcde type (@see OpcodeType).
         * @return requested instruction opcode.
         */
        virtual uint32        get_opcode(OpcodeType opc_type) const = 0;
        
        /**
         * Get instruction latency in cycles.
         * @return latency in cycles.
         */
        virtual uint32        get_cycles()      = 0;

        /**
         * Does the instruction have a destination.
         * @return true/false.
         */
        virtual bool          has_dest()        = 0;
        
        /**
         * Is the instruction blocking.
         * @return true/false.
         */
        virtual bool          is_blocking()     = 0;
        
        /**
         * Is the instruction flag setting.
         * @return true/false.
         */
        virtual bool          is_flag_setting() = 0;
        
        // Evaluate methods ----------------------------------------------------
        //
        
        /**
         * This method is called for NON flag setting ZERO_OPD instruction kinds.
         *
         * @param bflags_in input bflags.
         * @param xflags_in input xflags.
         * @return The result of this instruction.
         */
        virtual uint32 eval_zero_opd (EiaBflags bflags_in,
                                      EiaXflags xflags_in) = 0;
        
        /**
         * This method is called for flag setting ZERO_OPD instruction kinds.
         *
         * @param bflags_in input bflags.
         * @param xflags_in input xflags.
         * @param bflags_out because this is a flag setting instruction it receives a pointer to bflags it can modify.
         * @param xflags_out because this is a flag setting instruction it receives a pointer to xflags it can modify.
         * @return The result of this instruction.
         */
        virtual uint32 eval_zero_opd (EiaBflags  bflags_in,
                                      EiaXflags  xflags_in,
                                      EiaBflags* bflags_out,
                                      EiaXflags* xflags_out) = 0;

        /**
         * This method is called for NON flag setting SINGLE_OPD instruction kinds.
         *
         * @param src1 input operand.
         * @param bflags_in input bflags.
         * @param xflags_in input xflags.
         * @return The result of this instruction.
         */
        virtual uint32 eval_single_opd (uint32    src1,
                                        EiaBflags bflags_in,
                                        EiaXflags xflags_in) = 0;
        
        /**
         * This method is called for flag setting SINGLE_OPD instruction kinds.
         *
         * @param src1 input operand.
         * @param bflags_in input bflags.
         * @param xflags_in input xflags.
         * @param bflags_out because this is a flag setting instruction it receives a pointer to bflags it can modify.
         * @param xflags_out because this is a flag setting instruction it receives a pointer to xflags it can modify.
         * @return The result of this instruction.
         */
        virtual uint32 eval_single_opd (uint32     src1 ,
                                        EiaBflags  bflags_in,
                                        EiaXflags  xflags_in,
                                        EiaBflags* bflags_out,
                                        EiaXflags* xflags_out) = 0;
        
        /**
         * This method is called for NON flag setting DUAL_OPD instruction kinds.
         *
         * @param src1 first input operand.
         * @param src2 second input operand.
         * @param bflags_in input bflags.
         * @param xflags_in input xflags.
         * @return The result of this instruction.
         */
        virtual uint32 eval_dual_opd (uint32    src1,
                                      uint32    src2,
                                      EiaBflags bflags_in,
                                      EiaXflags xflags_in) = 0;
        
        /**
         * This method is called for NON flag setting DUAL_OPD instruction kinds.
         *
         * @param src1 first input operand.
         * @param src2 second input operand.
         * @param bflags_in input bflags.
         * @param xflags_in input xflags.
         * @param bflags_out because this is a flag setting instruction it receives a pointer to bflags it can modify.
         * @param xflags_out because this is a flag setting instruction it receives a pointer to xflags it can modify.
         * @return The result of this instruction.
         */
        virtual uint32 eval_dual_opd (uint32     src1,
                                      uint32     src2,
                                      EiaBflags  bflags_in,
                                      EiaXflags  xflags_in,
                                      EiaBflags* bflags_out,
                                      EiaXflags* xflags_out) = 0;
        
      };

} } } //  arcsim::ise::eia

#endif  // _INC_ISE_EIA_EIAINSTRUCTIONINTERFACE_H_
