//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#ifndef _INC_ISE_EIA_EIAINSTRUCTION_H_
#define _INC_ISE_EIA_EIAINSTRUCTION_H_

#include "ise/eia/EiaInstructionInterface.h"

#include "api/types.h"
#include <string>


namespace arcsim {
  namespace ise {
    namespace eia {

      // Forward declare EiaExtension
      //
      class EiaExtension;
      
      
      /**
       * EiaInstruction base class.
       * When defining new extension instructions one should derive the new
       * extension instruction from this class.
       */
      class EiaInstruction :
        public EiaInstructionInterface
      {
      private:
        std::string                   name_;
        uint32                        id_;
        
        EiaInstructionInterface::Kind kind_;
        
        uint32                        opcode_major_;
        uint32                        opcode_;
        uint32                        cycles_;

        bool                          has_dst_;
        bool                          is_blocking_;
        bool                          is_flag_setting_;

      protected:
        // extension instructions may have direct access to state defined by their
        // enclosing extension.
        //
        EiaExtension& parent;
        
      public:        

        // Constructor/Destructor
        //
        explicit EiaInstruction(EiaExtension* _parent,
                                std::string   _name,
                                EiaInstructionInterface::Kind _kind,
                                uint32  _opcode_major,
                                uint32  _opcode,
                                uint32  _cycles,
                                bool    _has_dst,
                                bool    _is_blocking,
                                bool    _is_flag_setting);
        ~EiaInstruction();
        
        
        // Interface methods ---------------------------------------------------
        //
        
        const char*   get_name() const  { return name_.c_str(); }
        const uint32  get_id()          { return id_;           }
        Kind          get_kind() const  { return kind_;         }
        uint32        get_opcode(OpcodeType opc_type) const {
          switch (opc_type) {
            case EiaInstructionInterface::OPCODE_MAJOR: return opcode_major_;
            case EiaInstructionInterface::OPCODE:       return opcode_;
          }
        }
        
        uint32        get_cycles()      { return cycles_;           }
        bool          has_dest()        { return has_dst_;          }
        bool          is_blocking()     { return is_blocking_;      }
        bool          is_flag_setting() { return is_flag_setting_;  }

        
        uint32 eval_zero_opd (EiaBflags bflags_in,
                              EiaXflags xflags_in);
        
        uint32 eval_zero_opd (EiaBflags  blags_in,                              
                              EiaXflags  xflags_in,
                              EiaBflags* bflags_out,
                              EiaXflags* xflags_out);
        
        uint32 eval_single_opd (uint32    src1,
                                EiaBflags bflags_in,
                                EiaXflags xflags_in);
        
        uint32 eval_single_opd (uint32     src1 ,
                                EiaBflags  bflags_in,
                                EiaXflags  xflags_in,
                                EiaBflags* bflags_out,
                                EiaXflags* xflags_out);
        
        uint32 eval_dual_opd (uint32    src1,
                              uint32    src2,
                              EiaBflags bflags_in,
                              EiaXflags xflags_in);
        
        uint32 eval_dual_opd (uint32     src1,
                              uint32     src2,
                              EiaBflags  bflags_in,
                              EiaXflags  xflags_in,
                              EiaBflags* bflags_out,
                              EiaXflags* xflags_out);
        
      };
      
} } } //  arcsim::ise::eia

#endif  // _INC_ISE_EIA_EIAINSTRUCTION_H_
