//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================

#include "ise/eia/EiaInstruction.h"
#include "ise/eia/EiaExtension.h"

#include "Assertion.h"

namespace arcsim {
  namespace ise {
    namespace eia {
      
      EiaInstruction::EiaInstruction(EiaExtension* _parent,
                                     std::string   _name,
                                     EiaInstructionInterface::Kind _kind,
                                     uint32  _opcode_major,
                                     uint32  _opcode,
                                     uint32  _cycles,
                                     bool    _has_dst,
                                     bool    _is_blocking,
                                     bool    _is_flag_setting)
      : parent(*_parent),
        name_(_name),
        id_(_parent->get_id()),
        kind_(_kind),
        opcode_major_(_opcode_major),
        opcode_(_opcode),
        cycles_(_cycles),
        has_dst_(_has_dst),
        is_blocking_(_is_blocking),
        is_flag_setting_(_is_flag_setting)
      {
        ASSERT((opcode_major_ < 32) && "EIA INSTRUCTION ERROR: Major Opcode larger than 31.`");
        ASSERT((opcode_ < 64)       && "EIA INSTRUCTION ERROR: Opcode larger than 63.");
      }

      EiaInstruction::~EiaInstruction()
      { /* EMPTY */ }
      
      // Interface methods ----------------------------------------------------
      // 
      
      uint32
      EiaInstruction::eval_zero_opd (EiaBflags bflags_in,
                                     EiaXflags xflags_in)
      {
        return 0;
      }
      
      uint32
      EiaInstruction::eval_zero_opd (EiaBflags  blags_in,                              
                                     EiaXflags  xflags_in,
                                     EiaBflags* bflags_out,
                                     EiaXflags* xflags_out)
      {
        return 0;
      }
      
      uint32
      EiaInstruction::eval_single_opd (uint32    src1,
                                       EiaBflags bflags_in,
                                       EiaXflags xflags_in)
      {
        return 0;
      }
      
      uint32
      EiaInstruction::eval_single_opd (uint32     src1,
                                       EiaBflags  bflags_in,
                                       EiaXflags  xflags_in,
                                       EiaBflags* bflags_out,
                                       EiaXflags* xflags_out)
      {
        return 0;
      }
      
      uint32
      EiaInstruction::eval_dual_opd (uint32    src1,
                                     uint32    src2,
                                     EiaBflags bflags_in,
                                     EiaXflags xflags_in)
      {
        return 0;
      }
      
      uint32
      EiaInstruction::eval_dual_opd (uint32     src1,
                                     uint32     src2,
                                     EiaBflags  bflags_in,
                                     EiaXflags  xflags_in,
                                     EiaBflags* bflags_out,
                                     EiaXflags* xflags_out)
      {
        return 0;
      }
      
      
} } } //  arcsim::ise::eia
