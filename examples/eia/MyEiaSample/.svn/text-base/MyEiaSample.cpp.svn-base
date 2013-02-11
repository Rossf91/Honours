//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

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
  : EiaConditionCode(ext,       /* EIA extension */
                     "mycc",    /* name   - USER INPUT */
                     16),       /* number - USER INPUT  */
    auxr (*ext->get_aux_register ("auxr")->get_value_ptr()),
    r55  (*ext->get_core_register("r55" )->get_value_ptr())
  { /* EMPTY */ }
  
  bool eval_condition_code (uint8 cc)
  {
/* START: USER INPUT */
    return (cc | auxr | r55);
/* END: USER INPUT */
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
  : EiaInstruction(ext,                                   /* EIA extension */
                   "myadd",                               /* name - USER INPUT */
                   arcsim::ise::eia::EiaInstructionInterface::DUAL_OPD, /* kind  - USER INPUT */
                   0x7,                                   /* major opcode  - USER INPUT */
                   0x1,                                   /* opcode - USER INPUT */
                   3,                                     /* cycles - USER INPUT */
                   true,                                  /* has_dst - USER INPUT */
                   false,                                 /* is_blocking - USER INPUT */
                   true),                                 /* is_flag_setting - USER INPUT */
    auxr (*ext->get_aux_register ("auxr")->get_value_ptr()),
    r55  (*ext->get_core_register("r55" )->get_value_ptr())
  { /* EMPTY */ }
  
  uint32 eval_dual_opd (uint32     src1,
                        uint32     src2,
                        arcsim::ise::eia::EiaBflags  bflags_in,
                        arcsim::ise::eia::EiaXflags  xflags_in,
                        arcsim::ise::eia::EiaBflags* bflags_out,
                        arcsim::ise::eia::EiaXflags* xflags_out)
  {
/* START: USER INPUT */
    uint32 dst = (src1 + src2) + r55 + 0x10;
    if (bflags_in.V)
    {
      if (dst == 0)
        xflags_out->X1 = 1;
      else
        xflags_out->X2 = 1;
    }
    auxr = dst;
    return dst;
/* END: USER INPUT */
  }
};

// -----------------------------------------------------------------------------
// EIA extension
class MyExtension : public arcsim::ise::eia::EiaExtension,
                              public arcsim::ise::eia::EiaExtensionFactory<MyExtension> {
public:
  static const uint32 MyExtensionId = 17;                 /* id - USER OR TOOL INPUT */
  
  explicit MyExtension(uint32 id, std::string name, std::string comment)
  : arcsim::ise::eia::EiaExtension(id, name, comment)
  { /* EMPTY */ }
  
  static  arcsim::ise::eia::EiaExtensionInterface* create_internal(void)
  { // 1. Create MyExtension
    MyExtension* eia = new MyExtension(MyExtensionId,
                                       "myext",           /* name - USER INPUT */
                                       "myext comment");  /* comment - USER INPUT */
    // 2. Add EiaCoreRegister to MyExtension
    eia->add_core_register(new arcsim::ise::eia::EiaCoreRegister(
                           eia,     /* parent */
                            "r55",  /* name - USER INPUT */
                            55,     /* number - USER INPUT */
                            0xf,    /* initial value - USER INPUT */
                            true,   /* w_direct - USER INPUT */
                            false,  /* w_prot - USER INPUT */
                            false,  /* w_only - USER INPUT */
                            false));/* r_only - USER INPUT */
    // 3. Add EiaAuxRegister to MyExtension
    eia->add_aux_register(new arcsim::ise::eia::EiaAuxRegister(
                           eia,     /* parent */
                           "auxr",  /* name - USER INPUT */
                           0x4242,  /* number - USER INPUT */
                           0xf,     /* initial value - USER INPUT */
                           true,    /* w_direct - USER INPUT */
                           false,   /* w_prot - USER INPUT */
                           false,   /* w_only - USER INPUT */
                           false)); /* r_only - USER INPUT */
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

