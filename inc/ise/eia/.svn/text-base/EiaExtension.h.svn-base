//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#ifndef _INC_ISE_EIA_EIAEXTENSION_H_
#define _INC_ISE_EIA_EIAEXTENSION_H_

#include "ise/eia/EiaExtensionInterface.h"

#include "api/types.h"
#include <string>
#include <map>

namespace arcsim {
  namespace ise  {
    namespace eia {
      
      // -----------------------------------------------------------------------
      // Forward decls.
      //
      class EiaInstructionInterface;
      class EiaConditionCodeInterface;
      class EiaCoreRegister;
      class EiaAuxRegister;
      
      /**
       * EiaExtension base class.
       * When defining new EIA extensions one should derive the new EIA
       * extension from this class.
       */
      class EiaExtension:
        public EiaExtensionInterface
      {
      private:
        uint32                                            id_;
        std::string                                       name_;
        std::string                                       comment_;
        
        std::map<std::string,EiaInstructionInterface*>    inst_map_;
        std::map<std::string,EiaConditionCodeInterface*>  cond_code_map_;
        std::map<std::string,EiaCoreRegisterInterface*>   core_reg_map_;
        std::map<std::string,EiaAuxRegisterInterface*>    aux_reg_map_;
      
      protected:
        // Constructor is protected as an EiaExtension MUST be instantiated via
        // factory methods in classes deriving EiaExtension
        //
        explicit EiaExtension(uint32 _id, std::string _name, std::string _comment);

        // Derived classes and friends can modify and add things to an extension
        //
        void add_eia_instruction(EiaInstructionInterface* _inst);
        void add_cond_code(EiaConditionCodeInterface* _cond_code);
        void add_core_register(EiaCoreRegisterInterface* _core_reg);
        void add_aux_register(EiaAuxRegisterInterface* _aux_reg);

      public:
                
        // Destructor
        //
        ~EiaExtension();
        
        // EiaExtensionInterface Methods ---------------------------------------
        //
        uint32        get_version() const;
        const char*   get_name()    { return name_.c_str();    }
        const char*   get_comment() { return comment_.c_str(); }
        const uint32  get_id()      { return id_;              }
        
        // Get either all or specific EIA instructions -------------------------
        //
        uint32 get_eia_instructions_count()
        {
          return inst_map_.size();
        }
        
        EiaInstructionInterface**   get_eia_instructions()
        {
          uint32                    index = 0;
          EiaInstructionInterface** inst_array = new EiaInstructionInterface*[inst_map_.size()];
          
          for (std::map<std::string,EiaInstructionInterface*>::const_iterator
               I = inst_map_.begin(),
               E = inst_map_.end();
               I != E; ++I)
          { inst_array[index++] = I->second; }
          
          // return HEAP allocated array of EiaInstructionInterface pointers
          //
          return inst_array;
        }
        
        EiaInstructionInterface*
        get_eia_instruction(const char* _name)
        {
          if (inst_map_.find(_name) != inst_map_.end()) {
            return inst_map_[_name];
          } else {
            return 0;
          }
        }
        
        // Get either all or specific condition codes --------------------------
        //
        uint32                      get_cond_codes_count()
        {
          return cond_code_map_.size();
        }

        EiaConditionCodeInterface** get_cond_codes()        
        {
          uint32                      index      = 0;
          EiaConditionCodeInterface** cond_array = new EiaConditionCodeInterface*[cond_code_map_.size()];

          for (std::map<std::string,EiaConditionCodeInterface*>::const_iterator
               I = cond_code_map_.begin(),
               E = cond_code_map_.end();
               I != E; ++I)
          { cond_array[index++] = I->second; }
          
          // return HEAP allocated array of EiaConditionCodeInterface pointers
          //
          return cond_array;
        }
        
        EiaConditionCodeInterface*
        get_cond_code(const char* _name)
        {
          if (cond_code_map_.find(_name) != cond_code_map_.end()) {
            return cond_code_map_[_name];
          } else {
            return 0;
          }
        }
        
        // Get either all or specific core registers ---------------------------
        //
        uint32
        get_core_registers_count()
        {
          return core_reg_map_.size();
        }
        
        EiaCoreRegisterInterface**
        get_core_registers()    
        {
          uint32            index          = 0;
          EiaCoreRegisterInterface** core_reg_array = new EiaCoreRegisterInterface*[core_reg_map_.size()];
          
          for (std::map<std::string,EiaCoreRegisterInterface*>::const_iterator
               I = core_reg_map_.begin(),
               E = core_reg_map_.end();
               I != E; ++I)
          { core_reg_array[index++] = I->second; }
          
          // return HEAP allocated array of EiaCoreRegister pointers
          //
          return core_reg_array;
        }
        
        EiaCoreRegisterInterface*
        get_core_register(const char* _name)
        {
          if (core_reg_map_.find(_name) != core_reg_map_.end()) {
            return core_reg_map_[_name];
          } else {
            return 0;
          }
        }
        
        // Get either all or specific aux registers ---------------------------
        //
        uint32
        get_aux_registers_count()
        {
          return aux_reg_map_.size();
        }
        
        EiaAuxRegisterInterface**
        get_aux_registers()    
        {
          uint32           index         = 0;
          EiaAuxRegisterInterface** aux_reg_array = new EiaAuxRegisterInterface*[aux_reg_map_.size()];
          
          for (std::map<std::string,EiaAuxRegisterInterface*>::const_iterator
               I = aux_reg_map_.begin(),
               E = aux_reg_map_.end();
               I != E; ++I)
          { aux_reg_array[index++] = I->second; }
          
          // return HEAP allocated array of EiaAuxRegister pointers
          //
          return aux_reg_array;

        }
        
        EiaAuxRegisterInterface*
        get_aux_register(const char* _name)
        {
          if (aux_reg_map_.find(_name) != aux_reg_map_.end()) {
            return aux_reg_map_[_name];
          } else {
            return 0;
          }
        }
        
      }; 
      
      
} } } //  arcsim::ise::eia

/** @mainpage API Documentation
 *
 *  The ARCompact ISA is designed to be extendable according to the
 *  requirements of the system in which it is used. These extensions may include
 *  more core and auxiliary registers, new instructions, and additional condition
 *  code tests.
 *
 *  The following API defines how to implement such extensions in software to
 *  enable simulation and testing of such extensions.
 *
 * @section example_eia_extension Example EIA Extension
 *
 * The following code demonstrates how to define a sample EIA Extension called
 * with an extension condition code, extension instruction, extension core register,
 * and an extension auxiliary register.
 *
 * @code

 #include "api/api_funs.h"
 #include "ise/eia/EiaExtension.h"
 #include "ise/eia/EiaAuxRegister.h"
 #include "ise/eia/EiaCoreRegister.h"
 #include "ise/eia/EiaConditionCode.h"
 #include "ise/eia/EiaInstruction.h"
 
 // -----------------------------------------------------------------------------
 // Extension condition code
 class MyConditionCode: public arcsim::ise::eia::EiaConditionCode
 {
 private:
   uint32&  auxr;
   uint32&  r55;
 public:
 explicit MyConditionCode(arcsim::ise::eia::EiaExtension* ext)
  : EiaConditionCode(ext,       // EIA extension
                    "mycc",    // name  
                    42),       // number 
    auxr (*ext->get_aux_register ("auxr")->get_value_ptr()),
    r55  (*ext->get_core_register("r55" )->get_value_ptr())
    { }

  bool eval_condition_code (uint8 cc)
  {
    return (cc | auxr | r55);
  }
};

// -----------------------------------------------------------------------------
// Extension instruction
class MyInstruction: public arcsim::ise::eia::EiaInstruction
{
private:
  uint32&  auxr;
  uint32&  r55;
public:
  explicit MyInstruction(arcsim::ise::eia::EiaExtension* ext)
  : EiaInstruction(ext,                                   // EIA extension
                   "myadd",                               // name
                   arcsim::ise::eia::EiaInstructionInterface::DUAL_OPD, // kind 
                   0xFF,                                  // major opcode 
                   0xFFFF,                                // opcode
                   3,                                     // cycles
                   true,                                  // has_dst
                   false,                                 // is_blocking
                   true),                                 // is_flag_setting
  auxr (*ext->get_aux_register ("auxr")->get_value_ptr()),
  r55  (*ext->get_core_register("r55" )->get_value_ptr())
  { }
  
  uint32 eval_dual_opd (uint32     src1,
                        uint32     src2,
                        arcsim::ise::eia::EiaBflags  bflags_in,
                        arcsim::ise::eia::EiaXflags  xflags_in,
                        arcsim::ise::eia::EiaBflags* bflags_out,
                        arcsim::ise::eia::EiaXflags* xflags_out)
  {
    uint32 dst = (src1 + src2) * r55 + 0x10;
    if (bflags_in.V)
    {
      if (dst == 0)
        xflags_out->flag_1 = 1;
      else
        xflags_out->flag_2 = 1;
    }
    auxr = dst;
    return dst;
  }
};

// -----------------------------------------------------------------------------
// EIA extension
class MyExtension : public arcsim::ise::eia::EiaExtension,
public arcsim::ise::eia::EiaExtensionFactory<MyExtension> {
public:
  explicit MyExtension(std::string name, std::string comment)
  : arcsim::ise::eia::EiaExtension(name, comment)
  { }
  
  static  arcsim::ise::eia::EiaExtensionInterface* create_internal(void)
  { // 1. Create MyExtension
    MyExtension* eia = new MyExtension("myext",           // name
                                       "myext comment");  // comment
    // 2. Add EiaCoreRegister to MyExtension
    eia->add_core_register(new arcsim::ise::eia::EiaCoreRegister("r55",  // name
                                                                 32,     // number
                                                                 0,      // initial value
                                                                 true,   // w_direct
                                                                 false,  // w_prot
                                                                 false,  // w_only
                                                                 false));// r_only
    // 3. Add EiaAuxRegister to MyExtension
    eia->add_aux_register(new arcsim::ise::eia::EiaAuxRegister("auxr",   // name
                                                               0x4242,   // number
                                                               0,        // initial value
                                                               true,     // w_direct
                                                               false,    // w_prot
                                                               false,    // w_only
                                                               false));  // r_only
    // 4. Add MyConditionCode to MyExtension
    eia->add_cond_code(new MyConditionCode(eia));
    // 5. Add MyInstruction to MyExtension
    eia->add_eia_instruction(new MyInstruction(eia));
    return eia;
  }
};
// -----------------------------------------------------------------------------
// Function called by simulator giving control to EIA extension library so
// that it can register as many EIA extensions as it desires.
//
void simLoadEiaExtension (simContext ctx)
{
  simRegisterEiaExtension(ctx, 0, MyExtension::create());
}
// Function called by test harness
//
EiaExtensionInterfacePtr simCreateEiaExtension(uint32 eia_id)
{
  if (eia_id == 0) { return MyExtension::create(); }
  else             { return NULL;                  }
}

 * @endcode
 */



#endif  // _INC_ISE_EIA_EIAEXTENSION_H_
