//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2004 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  This file contains the method Dcode::decode which decodes an ARC
//  instruction and sets up the Dcode object data members to permit
//  subsequent interpretation or just-in-time translation of that
//  instruction.
//
//  The Dcode::decode method relies heavily on instruction set constants
//  that are defined in decode_const.h
//
// =====================================================================

#include "isa/arc/Dcode.h"
#include "isa/arc/DcodeConst.h"

#include "arch/Configuration.h"

#include "sys/cpu/state.h"
#include "sys/cpu/EiaExtensionManager.h"

#include "ise/eia/EiaInstructionInterface.h"
#include "ise/eia/EiaCoreRegisterInterface.h"
#include "ise/eia/EiaConditionCodeInterface.h"

#include "exceptions.h"

// ---------------------------------------------------------------------------
// DECODE TASK MACROS
//

#define DECODE_TASK0(_t_) \
  static inline void _t_(Dcode& d, const IsaOptions& isa_opts, const arcsim::sys::cpu::EiaExtensionManager& eia_mgr)

#define DECODE_TASK(_t_) \
  static inline void _t_(Dcode& d, const IsaOptions& isa_opts, const arcsim::sys::cpu::EiaExtensionManager& eia_mgr, cpuState& state, uint32 pc, uint32 inst)

#define DECODE_TASK1(_t_,_a_) \
  static  inline void _t_(Dcode& d, const IsaOptions& isa_opts, const arcsim::sys::cpu::EiaExtensionManager& eia_mgr, cpuState& state, uint32 pc, uint32 inst, _a_)

#define DECODE_TASK2(_t_,_a_,_b_) \
  static inline void _t_(Dcode& d, const IsaOptions& isa_opts, const arcsim::sys::cpu::EiaExtensionManager& eia_mgr, cpuState& state, uint32 pc, uint32 inst, _a_, _b_)

#define DECODE_TASK3(_t_,_a_,_b_,_c_) \
  static inline void _t_(Dcode& d, const IsaOptions& isa_opts, const arcsim::sys::cpu::EiaExtensionManager& eia_mgr, cpuState& state, uint32 pc, uint32 inst, _a_, _b_, _c_)

#define DECODE_TASK4(_t_,_a_,_b_,_c_,_d_) \
  static inline void _t_(Dcode& d, const IsaOptions& isa_opts, const arcsim::sys::cpu::EiaExtensionManager& eia_mgr, cpuState& state, uint32 pc, uint32 inst, _a_, _b_, _c_, _d_)

#define DECODE_TASK5(_t_,_a_,_b_,_c_,_d_,_e_) \
  static inline void _t_(Dcode& d, const IsaOptions& isa_opts, const arcsim::sys::cpu::EiaExtensionManager& eia_mgr, cpuState& state, uint32 pc, uint32 inst, _a_, _b_, _c_, _d_, _e_)

#define DECODE_TASK6(_t_,_a_,_b_,_c_,_d_,_e_,_f_) \
  static inline void _t_(Dcode& d, const IsaOptions& isa_opts, const arcsim::sys::cpu::EiaExtensionManager& eia_mgr, cpuState& state, uint32 pc, uint32 inst, _a_, _b_, _c_, _d_, _e_,_f_)

// ---------------------------------------------------------------------------
// REGISTER READ/WRITE TASK MACROS
//

#define READ_TASK(_t_) \
  static inline void _t_(Dcode&                                       d,          \
                         const IsaOptions&                            isa_opts,   \
                         const arcsim::sys::cpu::EiaExtensionManager& eia_mgr,\
                         cpuState&                                    state,      \
                         const uint8                                  r)

#define WRITE_TASK(_t_) \
  static inline void _t_(Dcode&                                       d,          \
                         const IsaOptions&                            isa_opts,   \
                         const arcsim::sys::cpu::EiaExtensionManager& eia_mgr,\
                         cpuState&                                    state,      \
                         const uint8                                  r)


// ---------------------------------------------------------------------------
// MACROS that expand to something sensible for cycle accurate simulation and
// register tracking simulation
//

#ifdef CYCLE_ACC_SIM
#define OPD_AVAILABILITY(_opd_ptr_,_opd_) _opd_ptr_ = &(state.gprs_avail[_opd_]); 
#else
#define OPD_AVAILABILITY(_opd_ptr_,_opd_)
#endif

#ifdef REGTRACK_SIM
#define OPD_STATS(_opt_ptr_,_opd_) _opt_ptr_ = &(state.gprs_stats[_opd_]);
#else
#define OPD_STATS(_opt_ptr_,_opd_)
#endif

// ---------------------------------------------------------------------------
// MACROS to support decoding of EIA extensions and their permissions
//
#define REQUIRE_EIA_EXTENSION(_id_) d.xpu_required |= (1 << (_id_));
#define IN_EIA_REG_RANGE(_r_) (((_r_) > 0x1f /* 31 */) && ((_r_) < 0x3c /* 60 */ ))
#define IS_RESERVED_REG(_r_) ((_r_) == RESERVED_REG)
#define IS_UNIMP_REG(_r_) \
  (isa_opts.only_16_regs && \
   ( (((_r_)>3) && ((_r_)<10)) || (((_r_)>15) && ((_r_)<26))))

/* A short immediate operand, such as s12 or u6, is stored in the shimm field
 * and the src2 pointer is set to point at the shimm location.
 * A relative branch offset, such as the s12 in an lpcc or brcc instruction,
 * is stored in the shimm field, but src2 is not automatically set to point
 * at it. This is because there may be another src2 operand for the comparison
 * part of a BRcc instruction, which may be a limm for example.
 * The presence of a limm value is noted during decode, but the actual value
 * is not loaded into the limm field until after decoding is complete.
 *
 * N.B. A STORE instruction MUST use the OFFSET macro to extract the literal
 * address displacement. A LOAD instruction MUST use the SHIMM macro to
 * extract the second operand of the effective-address calculation, as this
 * may be a register, and is therefore passed through the inst->src2 pointer
 * rather than the inst->shimm value. If a STORE instruction is decoded with
 * the SHIMM macro, this will incorrectly set the inst->src2 pointer to
 * point at the shimm value. This will then be used as the store value, giving
 * incorrect behaviour (you have been warned!).
 */
#define SHIMM(_v_)    d.shimm = (_v_); d.src2 = &d.shimm;
#define OFFSET(_v_)   d.shimm = (_v_);
#define REL_JUMP(_v_) d.jmp_target = (((pc & 0xfffffffc)+(sint32)(_v_)) & state.pc_mask); 
#define ABS_JUMP(_v_) d.jmp_target = (_v_);


#define SET(_v_) _v_ = true
#define CLEAR(_v_) _v_ = false


// ---------------------------------------------------------------------------
// STATIC VARIABLES
//


namespace arcsim {
  namespace isa {
    namespace arc {
      
      static const unsigned char limm_reg = LIMM_REG;
      
      static int minor6_map[] =
      {
        OpCode::FMUL,      OpCode::FADD,       OpCode::FSUB,       OpCode::EXCEPTION,
        OpCode::EXCEPTION, OpCode::EXCEPTION,  OpCode::EXCEPTION,  OpCode::EXCEPTION,
        OpCode::DMULH11,   OpCode::DMULH12,    OpCode::DMULH21,    OpCode::DMULH22,
        OpCode::DADDH11,   OpCode::DADDH12,    OpCode::DADDH21,    OpCode::DADDH22,
        OpCode::DSUBH11,   OpCode::DSUBH12,    OpCode::DSUBH21,    OpCode::DSUBH22,
        OpCode::DRSUBH11,  OpCode::DRSUBH12,   OpCode::DRSUBH21,   OpCode::DRSUBH22,
        OpCode::DEXCL1,    OpCode::DEXCL2
      };

      
      // -----------------------------------------------------------------------
      // TASKS FOR READING AND WRITING OPERANDS
      //
      
      // -----------------------------------------------------------------------
      // Wire up 'src1'
      //
      READ_TASK(READ_0)
      {
        // 1. Is this the LIMM register
        //
        if (r == limm_reg) {
          d.has_limm = true;
          d.src1     = &(d.limm);
        }
        // 2. Are EIA core registers defined and is this an EIA core register?
        //
        else if (eia_mgr.are_eia_core_regs_defined && IN_EIA_REG_RANGE(r))
        {
          std::map<uint32, ise::eia::EiaCoreRegisterInterface*>::const_iterator R;
          if (( R = eia_mgr.eia_core_reg_map.find(r) ) != eia_mgr.eia_core_reg_map.end())
          {
            d.src1 = R->second->get_value_ptr();
            REQUIRE_EIA_EXTENSION(R->second->get_id())
          } else {
            d.illegal_operand = true;
          }
        }
        // 3. If the register is not a defined extension register, but falls into
        // the range of extension registers we have the following possible outcomes:
        //
        //  - For the ARC 600 configured with an optional multiply we need to allow
        //    reading of multiply result registers r57,r58, and r59
        //  - For the ARC 700 we need to allow reading of register r58 (TSCH)
        //  - If this register falls into the range for an extension register,
        //    but such an extension register is not defined, this read is illegal
        //  - If the register is RESERVED_REG the read is also illegal
        //
        else if (/* 1. Check for EIA register range and RESERVED_REG */
                 (IN_EIA_REG_RANGE(r) || IS_RESERVED_REG(r) || IS_UNIMP_REG(r) )
                 /* 2. CHECK for A600 with optional multiply registers */
                 && !(   isa_opts.is_isa_a600()
                      && isa_opts.mul64_option
                      && ((r == MLO_REG) || (r == MMID_REG) || (r == MHI_REG)) )
                 /* 3. CHECK for A700 with RTSC registers */
                 && !(   isa_opts.is_isa_a700()
                      && (r == TSCH_REG)) )
        {
          d.illegal_operand = true;
        }
        // 4. FINALLY, this IS a LEGAL baseline register.
        //
        else {
          d.src1 = &(state.gprs[r]);
        }
        OPD_AVAILABILITY(d.src1_avail, r);
        OPD_STATS(d.src1_stats, r);
        d.info.rf_ra0   = r;
        d.info.rf_renb0 = (r != limm_reg);
      }
      
      // -----------------------------------------------------------------------
      // Wire up 'src1' : 5-bit source register address for format 14 'h' field
      //                : Cannot be an extension register or the RESERVED_REG
      //
      READ_TASK(READ_0_H)      
      {
        // 1. Is this the LIMM register
        //
        if (r == 0x1e /* 30 */ ) {
          d.has_limm  = true;
          d.src1      = &(d.limm);
        }
        // 2. It this an unimplemented baseline register?
        //
        else if (IS_UNIMP_REG(r)) {
          d.illegal_operand = true;
        }
        // 3. FINALLY, this IS a LEGAL baseline register.
        //
        else {
          d.src1 = &(state.gprs[r]);
        }
        OPD_AVAILABILITY(d.src1_avail, r);
        OPD_STATS(d.src1_stats, r);
        d.info.rf_ra0   = r;
        d.info.rf_renb0 = (r != 0x1e  /* 30 */ );
      }
      
      // -----------------------------------------------------------------------
      // Wire up 'src2'
      //
      READ_TASK(READ_1)
      {
        // 1. Is this the LIMM register?
        //
        if (r == limm_reg) {
          d.has_limm = true;
          d.src2     = &(d.limm);
        }
        // 2. Are EIA core registers defined and is this an EIA core register?
        //
        else if (eia_mgr.are_eia_core_regs_defined && IN_EIA_REG_RANGE(r))
        {
          std::map<uint32, ise::eia::EiaCoreRegisterInterface*>::const_iterator R;
          if (( R = eia_mgr.eia_core_reg_map.find(r) ) != eia_mgr.eia_core_reg_map.end())
          {
            d.src2 = R->second->get_value_ptr();
            REQUIRE_EIA_EXTENSION(R->second->get_id())
          } else {
            d.illegal_operand = true;
          }
        }
        // 3. If the register is not a defined extension register, but falls into
        // the range of extension registers we have the following possible outcomes:
        //
        //  - For the ARC 600 configured with an optional multiply we need to allow
        //    reading of multiply result registers r57,r58, and r59
        //  - For the ARC 700 we need to allow reading of register r58 (TSCH)
        //  - If this register falls into the range for an extension register,
        //    but such an extension register is not defined, this read is illegal
        //  - If the register is RESERVED_REG the read is also illegal
        //
        else if (/* 1. Check for EIA register range and RESERVED_REG */
                    (IN_EIA_REG_RANGE(r) || IS_RESERVED_REG(r) || IS_UNIMP_REG(r) )
                 /* 2. CHECK for A600 with optional multiply registers */
                 && !(   isa_opts.is_isa_a600()
                      && isa_opts.mul64_option
                      && ((r == MLO_REG) || (r == MMID_REG)    || (r == MHI_REG)) )
                 /* 3. CHECK for A700 with RTSC registers */
                 && !(   isa_opts.is_isa_a700()
                      && (r == TSCH_REG)) )
        {
          d.illegal_operand = true;
        }
        // 4. FINALLY, this IS a LEGAL baseline register.
        //
        else {
          d.src2 = &(state.gprs[r]);
        }
        OPD_AVAILABILITY(d.src2_avail, r);
        OPD_STATS(d.src2_stats, r);
        d.info.rf_ra1 = r;
        d.info.rf_renb1 = (r != limm_reg);
      }
      
      // -----------------------------------------------------------------------
      // Wire up 'src2' : 5-bit source register address for format 14 'h' field
      //                : Cannot be an extension register or the RESERVED_REG
      //
      READ_TASK(READ_1_H)      
      {
        // 1. Is this the LIMM register
        //
        if (r == 0x1e /* 30 */ ) {
          d.has_limm  = true;
          d.src2      = &(d.limm);
        }
        // 2. Is this an unimplemented baseline register
        //           
        else if (IS_UNIMP_REG(r)) {
          d.illegal_operand = true;
        }
        // 3. FINALLY, this IS a LEGAL baseline register.
        //
        else {
          d.src2 = &(state.gprs[r]);
        }
        OPD_AVAILABILITY(d.src2_avail, r);
        OPD_STATS(d.src2_stats, r);
        d.info.rf_ra1   = r;
        d.info.rf_renb1 = (r != 0x1e  /* 30 */ );
      }

      // -----------------------------------------------------------------------
      // Wire up 'dst1'
      //
      WRITE_TASK(WRITE_0)      
      {
        // Is this an EIA core register?
        //
        if (eia_mgr.are_eia_core_regs_defined && IN_EIA_REG_RANGE(r))
        {
          std::map<uint32, ise::eia::EiaCoreRegisterInterface*>::const_iterator R;
          if (( R = eia_mgr.eia_core_reg_map.find(r) ) != eia_mgr.eia_core_reg_map.end())
          {
            d.dst1 = R->second->get_value_ptr();
            REQUIRE_EIA_EXTENSION(R->second->get_id())
          } else {
            d.illegal_operand = true;
          }
        }
        // This is not a defined extension register, but may be in the
        // range for an extension register (which would be illegal),
        // or may be the RESERVED_REG (also illegal).
        //
        else if (IN_EIA_REG_RANGE(r) || IS_RESERVED_REG(r) || IS_UNIMP_REG(r)) {
          d.illegal_operand = true;
        }
        // This is a legal baseline register.
        //           
        else {
          if (r == LP_COUNT) { 
            d.dst1 = &(state.next_lpc);
          } else {
            d.dst1 = &(state.gprs[r]);
          }
        }
        OPD_AVAILABILITY(d.dst1_avail, r);
        OPD_STATS(d.dst1_stats, r);
        d.info.rf_wa0   = r;
        d.info.rf_wenb0 = (r != limm_reg);
      }
      
      // -----------------------------------------------------------------------
      // Wire up 'dst1' : 5-bit destination address for format 14 'h' field
      //                : Cannot be an extension register or the RESERVED_REG
      //                  or LP_COUNT.
      //
      WRITE_TASK(WRITE_0_H)      
      {
        // Is this the LIMM register
        //
        if (r == 0x1e /* 30 */ ) {
          d.info.rf_wa0   = limm_reg;
          d.info.rf_wenb0 = false;
          d.dst1          = &(state.gprs[limm_reg]);
        }
        else if (IS_UNIMP_REG(r)) {
          // It is an unimplemented baseline register
          //           
          d.illegal_operand = true;
        }
        else {
          d.info.rf_wa0   = r;
          d.info.rf_wenb0 = true;
          d.dst1          = &(state.gprs[r]);
        }
        OPD_AVAILABILITY(d.dst1_avail, d.info.rf_wa0);
        OPD_STATS       (d.dst1_stats, d.info.rf_wa0);
      }
      
      // -----------------------------------------------------------------------
      // Wire up 'dst2'
      //
      WRITE_TASK(WRITE_1)      
      {
        // Is this an EIA core register?
        //
        if (eia_mgr.are_eia_core_regs_defined && IN_EIA_REG_RANGE(r))
        {
          std::map<uint32, ise::eia::EiaCoreRegisterInterface*>::const_iterator R;
          if (( R = eia_mgr.eia_core_reg_map.find(r) ) != eia_mgr.eia_core_reg_map.end())
          {
            d.dst2 = R->second->get_value_ptr();
            REQUIRE_EIA_EXTENSION(R->second->get_id())
          } else {
            d.illegal_operand = true;
          }
        }
        // This is not a defined extension register, but may be in the
        // range for an extension register (which would be illegal),
        // or may be the RESERVED_REG (also illegal).
        //
        else if (IN_EIA_REG_RANGE(r) || IS_RESERVED_REG(r) || IS_UNIMP_REG(r)) {
          d.illegal_operand = true;
        }
        // This is a legal baseline register.
        //           
        else {
          if (r == LP_COUNT) { 
            d.dst2 = &(state.next_lpc);
          } else {
            d.dst2 = &(state.gprs[r]);
          }
        }
        OPD_AVAILABILITY(d.dst2_avail, r);
        OPD_STATS(d.dst2_stats, r);
        d.info.rf_wa1   = r;
        d.info.rf_wenb1 = (r != limm_reg);
      }


// --------------------------------------------------------------------------- 
// DECODE TASK DEFINITIONS
//

DECODE_TASK0 (inst_error_task)
{
  d.code = OpCode::EXCEPTION;
  d.kind = isa::arc::Dcode::kException;
  SHIMM (ECR(isa_opts.EV_InstructionError, IllegalInstruction, 0))
}

DECODE_TASK (inst_sequence_error_task)
{
  d.code = OpCode::EXCEPTION;
  d.kind = isa::arc::Dcode::kException;
  SHIMM (ECR(isa_opts.EV_InstructionError, IllegalSequence, 0))
}


DECODE_TASK (privilege_violation_task)
{
  d.code = OpCode::EXCEPTION;
  d.kind = isa::arc::Dcode::kException;
  SHIMM (ECR(isa_opts.EV_PrivilegeV, PrivilegeViolation, 0))
}

DECODE_TASK0 (flag_enable_task)
{
  d.z_wen &= d.flag_enable;
  d.n_wen &= d.flag_enable;
  d.c_wen &= d.flag_enable;
  d.v_wen &= d.flag_enable;
}

DECODE_TASK (disallowed_in_dslot)
{
  // Set the illegal_in_dslot flag, which will be checked dynamically
  // when resolving priority between Illegal Instruction Sequence
  // and Privilege Violation exceptions.
  //
  d.illegal_in_dslot = true;
}

DECODE_TASK (privileged_instruction)
{
  if (state.U)
    privilege_violation_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (disallow_link_reg_destination)
{
  if (   (d.info.rf_wenb0 && ((d.info.rf_wa0 == ILINK1) || (d.info.rf_wa0 == ILINK2)))
      || (d.info.rf_wenb1 && ((d.info.rf_wa1 == ILINK1) || (d.info.rf_wa1 == ILINK2)))
      )
    d.illegal_inst_format = true;
}

DECODE_TASK (disallow_lp_count_destination)
{
  if (   (d.info.rf_wenb0 && (d.info.rf_wa0 == LP_COUNT))
      || (d.info.rf_wenb1 && (d.info.rf_wa1 == LP_COUNT)))
    d.illegal_inst_format = true;
}

DECODE_TASK (disallow_lp_count_destination1)
{
  if (   (d.info.rf_wenb1 && (d.info.rf_wa1 == LP_COUNT)))
    d.illegal_inst_format = true;
}

DECODE_TASK (disallow_r63_destination)
{
  if (   (d.info.rf_wenb0 && (d.info.rf_wa0 == 63))
      || (d.info.rf_wenb1 && (d.info.rf_wa1 == 63))
      )
    d.illegal_inst_format = true;
}

DECODE_TASK (disallow_r62_destination)
{
  if (   (d.info.rf_wenb0 && (d.info.rf_wa0 == 62))
      || (d.info.rf_wenb1 && (d.info.rf_wa1 == 62))
      )
    d.illegal_inst_format = true;
}

DECODE_TASK (disallow_r61_access)
{
  if (   (d.info.rf_wenb0 && (d.info.rf_wa0 == 61))
      || (d.info.rf_wenb1 && (d.info.rf_wa1 == 61))
      || (d.info.rf_renb0 && (d.info.rf_ra0 == 61))
      || (d.info.rf_renb1 && (d.info.rf_ra1 == 61))
      )
    d.illegal_inst_format = true;
}

DECODE_TASK (disallow_lpc_a6k_writes)
{
  if (isa_opts.is_isa_a6k())
  {
    bool multi_cycle;
    
    multi_cycle =  (d.code == OpCode::MPY)
                || (d.code == OpCode::MPYH)
                || (d.code == OpCode::MPYHU)
                || (d.code == OpCode::MPYU)
                || (d.code == OpCode::DIV)
                || (d.code == OpCode::DIVU)
                || (d.code == OpCode::REM)
                || (d.code == OpCode::REMU)
                || (d.code == OpCode::LR);
    
    if (   multi_cycle
        && (   (d.info.rf_wenb0 && (d.info.rf_wa0 == LP_COUNT))
            || (d.info.rf_wenb1 && (d.info.rf_wa1 == LP_COUNT))
           )
       )  
      d.illegal_inst_format = true;
  }
}

DECODE_TASK (disallow_upper32_destination)
{
  if (   (d.info.rf_wenb0 && (d.info.rf_wa0 > 31) && (d.info.rf_wa0 != 60))
      || (d.info.rf_wenb1 && (d.info.rf_wa1 > 31))
      )
    d.illegal_inst_format = true;
}

/////////////////////////////////////////////////////////////
//
// Register operand and general format decode tasks
//
/////////////////////////////////////////////////////////////

DECODE_TASK (f_bit_task)
{
  d.flag_enable = BITSEL(inst,15);
}

DECODE_TASK (regs_a_32_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, UNSIGNED_BITS(inst,5,0));
}

DECODE_TASK (regs_a1_32_task)
{
  WRITE_1 (d, isa_opts, eia_mgr, state, UNSIGNED_BITS(inst,5,0));
}

DECODE_TASK (regs_c_32_task)
{
  READ_1 (d, isa_opts, eia_mgr, state, UNSIGNED_BITS(inst,11,6));
}

DECODE_TASK (regs_q_32_task)
{
  d.q_field = UNSIGNED_BITS(inst,4,0);
  
  if (d.q_field > 15) {
    if (eia_mgr.are_eia_cond_codes_defined) {
      // find extension condition evaluator and install in this object
      std::map<uint32,ise::eia::EiaConditionCodeInterface*>::const_iterator   I;
      std::map<uint32,ise::eia::EiaConditionCodeInterface*>::const_iterator   E = eia_mgr.eia_cond_code_map.end();
      
      // Is this condition defined by EIA?
      //
      if ((I = eia_mgr.eia_cond_code_map.find(d.q_field)) != E)
      { // Found extension condition, now register pointer to its evaluation method
        //
        d.eia_cond = I->second;
        REQUIRE_EIA_EXTENSION(I->second->get_id())
      }
    } else if (!isa_opts.sat_option || (d.q_field > 17)) {
      d.illegal_operand = true;
    }
  }
}

DECODE_TASK (regs_cq_32_task)
{
  regs_c_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_q_32_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_u6w_32_task)
{
  SHIMM (UNSIGNED_BITS(inst,11,6) << 1)
}

DECODE_TASK (jmp_s12_32_task)
{
  ABS_JUMP ((SIGNED_BITS(inst,5,0)<<6) | UNSIGNED_BITS(inst,11,6))
  d.src2 = &d.jmp_target;
}

DECODE_TASK (regs_u6_32_task)
{
  SHIMM (UNSIGNED_BITS(inst,11,6))
}

DECODE_TASK (regs_bu6_32_task)
{
  READ_0 (d, isa_opts, eia_mgr, state,
          (UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24));
  regs_u6_32_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_b_32_task)
{
  READ_0 (d, isa_opts, eia_mgr, state,
          (UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24));
}

DECODE_TASK (regs_bq_32_task)
{
  regs_b_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_q_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  f_bit_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_bbq_32_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state,
           (UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24));
  regs_bq_32_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_bu6q_32_task)
{
  regs_bu6_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_q_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  f_bit_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_bc_32_task)
{
  regs_b_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  READ_1 (d, isa_opts, eia_mgr, state, UNSIGNED_BITS(inst,11,6));
}

DECODE_TASK (regs_abc_32_task)
{
  regs_a_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_bc_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  f_bit_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_a1bc_32_task)
{
  regs_a1_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_bc_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  f_bit_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_mov_bs12_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24));
  SHIMM ((SIGNED_BITS(inst,5,0)<<6) | UNSIGNED_BITS(inst,11,6))
}

DECODE_TASK (regs_bs12_32_task)
{
  regs_b_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  SHIMM ((SIGNED_BITS(inst,5,0)<<6) | UNSIGNED_BITS(inst,11,6))
  f_bit_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (flag_s12_32_task)
{
  SHIMM ((SIGNED_BITS(inst,5,0)<<6) | UNSIGNED_BITS(inst,11,6))
  f_bit_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_bbs12_32_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24));
  regs_bs12_32_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_sop_bc_32_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24));
  READ_1 (d, isa_opts, eia_mgr, state, UNSIGNED_BITS(inst,11,6));
  f_bit_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_sop_bu6_32_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24));
  SHIMM (UNSIGNED_BITS(inst,11,6))
  f_bit_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_bcq_32_task)
{
  uint8 reg = (UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24);
  WRITE_0 (d, isa_opts, eia_mgr, state, reg);
  READ_0 (d, isa_opts, eia_mgr, state, reg);
  READ_1 (d, isa_opts, eia_mgr, state, UNSIGNED_BITS(inst,11,6));  
  regs_q_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  f_bit_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_abu6_32_task)
{
  regs_a_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_bu6_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  f_bit_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_mov_bc_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24));
  READ_1 (d, isa_opts, eia_mgr, state, UNSIGNED_BITS(inst,11,6));
}

DECODE_TASK (regs_mov_bu6_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24));
  SHIMM (UNSIGNED_BITS(inst,11,6))
}

DECODE_TASK (jmp_lpcc_s12_task)
{
  REL_JUMP ((SIGNED_BITS(inst,5,0)<<7) | (UNSIGNED_BITS(inst,11,6)<<1))
}

DECODE_TASK (regs_lpcc_u6_task)
{
  REL_JUMP (UNSIGNED_BITS(inst,11,6)<<1)
}

DECODE_TASK (regs_lpcc_u6q_task)
{
  REL_JUMP (UNSIGNED_BITS(inst,11,6)<<1)
  regs_q_32_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_b_dest_16_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
}

DECODE_TASK (regs_b_ld_dest_16_task)
{
  WRITE_1 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
}

// Used only by load instructions
//
DECODE_TASK (regs_c_dest_16_task)
{
  WRITE_1 (d, isa_opts, eia_mgr, state, (BITSEL(inst,23)<<3) | UNSIGNED_BITS(inst,23,21));
}

DECODE_TASK (regs_b_src1_16_task)
{
  READ_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
}

DECODE_TASK (regs_b_src2_16_task)
{
  READ_1 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
}

DECODE_TASK (regs_c_src2_16_task)
{
  READ_1 (d, isa_opts, eia_mgr, state, (BITSEL(inst,23)<<3) | UNSIGNED_BITS(inst,23,21));
}

DECODE_TASK (regs_bbc_16_task)
{
  regs_b_dest_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_b_src1_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_c_src2_16_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_a_16_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,18)<<3) | UNSIGNED_BITS(inst,18,16));
}

DECODE_TASK (regs_a1_16_task)
{
  WRITE_1 (d, isa_opts, eia_mgr, state, (BITSEL(inst,18)<<3) | UNSIGNED_BITS(inst,18,16));
}

DECODE_TASK (regs_bc_16_task)
{
  READ_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  READ_1 (d, isa_opts, eia_mgr, state, (BITSEL(inst,23)<<3) | UNSIGNED_BITS(inst,23,21));
}

DECODE_TASK (regs_cbu3_16_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,23)<<3) | UNSIGNED_BITS(inst,23,21));
  READ_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  SHIMM (UNSIGNED_BITS(inst,18,16))
}

DECODE_TASK (regs_bbh_16_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  READ_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  
  if (isa_opts.is_isa_a6k() && isa_opts.new_fmt_14) {
    READ_1_H (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,17,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
  else {
    READ_1 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,18,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
}

DECODE_TASK (regs_mov_hb_16_task)
{
  if (isa_opts.is_isa_a6k() && isa_opts.new_fmt_14) {
    WRITE_0_H (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,17,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
  else {
    WRITE_0 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,18,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
  
  READ_1 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
}

DECODE_TASK (regs_mov_bh_16_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  
  if (isa_opts.is_isa_a6k() && isa_opts.new_fmt_14) {
    READ_1_H (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,17,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
  else {      
    READ_1 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,18,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
}

DECODE_TASK (regs_bh_16_task)
{
  READ_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  
  if (isa_opts.is_isa_a6k() && isa_opts.new_fmt_14) {
    READ_1_H (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,17,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
  else {      
    READ_1 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,18,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
}

DECODE_TASK (shimm_s3_16_task)
{
  sint32 s = UNSIGNED_BITS(inst,26,24);
  if (s == 7)
    s = -1;
  SHIMM (s) 
}

DECODE_TASK (regs_h_dst_16_task)
{
  if (isa_opts.is_isa_a6k() && isa_opts.new_fmt_14) {
    WRITE_0_H (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,17,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
  else {      
    WRITE_0 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,18,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
}

DECODE_TASK (regs_h_src1_16_task)
{
  if (isa_opts.is_isa_a6k() && isa_opts.new_fmt_14) {
    READ_0_H (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,17,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
  else {      
    READ_0 (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,18,16)<<3) | UNSIGNED_BITS(inst,23,21));
  }
}

DECODE_TASK (regs_h_src2_16_task)
{
  READ_1_H (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,17,16)<<3) | UNSIGNED_BITS(inst,23,21));
}

DECODE_TASK (regs_g_dst_16_task)
{
  WRITE_0_H (d, isa_opts, eia_mgr, state, (UNSIGNED_BITS(inst,20,19)<<3) | UNSIGNED_BITS(inst,26,24));
}

DECODE_TASK (regs_a_dst_16_task)
{
  WRITE_0_H (d, isa_opts, eia_mgr, state, (BITSEL(inst,18)<<3) | UNSIGNED_BITS(inst,18,16));
}

DECODE_TASK (regs_hhs3_16_task)
{
  regs_h_dst_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_h_src1_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  shimm_s3_16_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_mov_hs3_16_task)
{
  regs_h_dst_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  shimm_s3_16_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_hs3_16_task)
{
  regs_h_src1_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  shimm_s3_16_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_bspu7_16_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  READ_0 (d, isa_opts, eia_mgr, state, 28);
  SHIMM (UNSIGNED_BITS(inst,20,16)<<2)
}

DECODE_TASK (regs_r0gps9_16_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, 0);
  READ_0 (d, isa_opts, eia_mgr, state, 26);
  SHIMM (UNSIGNED_BITS(inst,24,16))
}

DECODE_TASK (regs_bu5_16_task)
{
  READ_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  SHIMM (UNSIGNED_BITS(inst,20,16))
}

DECODE_TASK (regs_bbu5_16_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  READ_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  SHIMM (UNSIGNED_BITS(inst,20,16))
}

DECODE_TASK (regs_bu7_16_task)
{
  READ_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  SHIMM (UNSIGNED_BITS(inst,22,16))
}

DECODE_TASK (regs_bbu7_16_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  regs_bu7_16_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (regs_mov_bu7_16_task)
{
  WRITE_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  SHIMM (UNSIGNED_BITS(inst,22,16))
}

DECODE_TASK (regs_b_16_task)
{
  READ_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
}

DECODE_TASK (pop_16_task)
{
  d.code = OpCode::LW_PRE_A;
  d.kind = isa::arc::Dcode::kMemLoad;
  WRITE_0 (d, isa_opts, eia_mgr, state, SP_REG);
  READ_0 (d, isa_opts, eia_mgr, state, SP_REG);
  SHIMM (4)
  d.pre_addr = true; // use ra0 rather than ra0+offset
  
  if (UNSIGNED_BITS(inst,20,16) == 1)
    WRITE_1 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  else if (UNSIGNED_BITS(inst,20,16) == 17)
    WRITE_1 (d, isa_opts, eia_mgr, state, 31);
  else
    d.illegal_inst_format = true;
  
  // disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_upper32_destination (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (push_16_task)
{
  d.code = OpCode::ST_WORD;
  d.kind = isa::arc::Dcode::kMemStore;
  WRITE_0 (d, isa_opts, eia_mgr, state, SP_REG);
  READ_0 (d, isa_opts, eia_mgr, state, SP_REG);
  OFFSET (static_cast<uint32>(-4))
  
  if (UNSIGNED_BITS(inst,20,16) == 1)
    READ_1 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  else if (UNSIGNED_BITS(inst,20,16) == 17)
    READ_1 (d, isa_opts, eia_mgr, state, 31);
  else
    d.illegal_inst_format = true;
}

DECODE_TASK (regs_push_16_task)
{
  READ_0 (d, isa_opts, eia_mgr, state, SP_REG);
  OFFSET (static_cast<uint32>(-4));
  WRITE_1 (d, isa_opts, eia_mgr, state, SP_REG);
  // dcode.cpp determines WRITE_0() register
}

DECODE_TASK (enter_s_task)
{
  if (isa_opts.density_option) {
    d.code   = OpCode::ENTER;
    d.kind   = arcsim::isa::arc::Dcode::kMemEnterLeave;
    // Overload the following fields
    //   shimm   - number of general regs starting r13 to save/restore
    //   link    - 1 => save BLINK,  0 => don't save BLINK
    //   dslot   - 1 => save FP,     0 => don't save FP
    //
    d.shimm        = UNSIGNED_BITS(inst, 20, 17);
    d.link         = BITSEL(inst, ENTER_LEAVE_LINK_BIT);
    d.dslot        = BITSEL(inst, ENTER_LEAVE_FP_BIT);
    //
    disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
    //
    // max saved register is r26, so shimm value of 15 is illegal
    //
    if (d.shimm == 15)
      inst_error_task (d, isa_opts, eia_mgr);
  } else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK (leave_s_task)
{
  if (isa_opts.density_option) {
    // Overload the following fields
    //   shimm         - number of general regs starting r13 to save/restore
    //   link          - 1 => restore BLINK, 0 => don't restore BLINK
    //   dslot         - 1 => restore FP,    0 => don't restore FP
    //   info.isReturn - 1 => jump to blink after restoring context
    //
    d.code         = OpCode::LEAVE;
    d.kind         = arcsim::isa::arc::Dcode::kMemEnterLeave;
    d.shimm        = UNSIGNED_BITS(inst, 20, 17);
    d.link         = BITSEL(inst, ENTER_LEAVE_LINK_BIT);
    d.dslot        = BITSEL(inst, ENTER_LEAVE_FP_BIT);
    d.info.isReturn= BITSEL(inst, ENTER_LEAVE_JMP_BIT);
    //
    disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
    //
    // max saved register is r26, so shimm value of 15 is illegal
    //
    if (d.shimm == 15)
      inst_error_task (d, isa_opts, eia_mgr);
  } else
    inst_error_task (d, isa_opts, eia_mgr);
};

DECODE_TASK (zero_operand_task)
{
  d.info.rf_ra0 = 0;
  d.info.rf_ra1 = 0;
  d.info.rf_wa0 = 0;
  d.info.rf_wa1 = 0;
  d.illegal_inst_format = false;
  d.illegal_inst_subcode = false;
  d.illegal_operand = false;
  d.illegal_in_dslot = false;
  CLEAR (d.info.rf_renb0);
  CLEAR (d.info.rf_renb1);
  CLEAR (d.info.rf_wenb0);
  CLEAR (d.has_limm);
}

/////////////////////////////////////////////////////////////
//
// Flag update decode tasks
//
/////////////////////////////////////////////////////////////

DECODE_TASK0 (enable_all_flag_writes)
{
  SET (d.z_wen);
  SET (d.n_wen);
  SET (d.c_wen);
  SET (d.v_wen);
}

DECODE_TASK0 (enable_znc_flag_writes)
{
  SET (d.z_wen);
  SET (d.n_wen);
  SET (d.c_wen);
}

DECODE_TASK0 (enable_zn_flag_writes)
{
  SET (d.z_wen);
  SET (d.n_wen);
}

DECODE_TASK0 (enable_znv_flag_writes)
{
  SET (d.z_wen);
  SET (d.n_wen);
  SET (d.v_wen);
}

/////////////////////////////////////////////////////////////
//
// Arithmetic instruction decode tasks
//
/////////////////////////////////////////////////////////////

DECODE_TASK0 (add_task)
{
  if (d.flag_enable || d.q_field)
    d.code = OpCode::ADD_F;
  else
    d.code = OpCode::ADD;
  d.kind   = arcsim::isa::arc::Dcode::kArithmetic;
  enable_all_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (add_16_task)
{
  d.code = OpCode::ADD;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
}

DECODE_TASK0 (adc_task)
{
  d.code = OpCode::ADC;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  enable_all_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (sub_task)
{
  if (d.flag_enable || d.q_field)
    d.code = OpCode::SUB_F;
  else
    d.code = OpCode::SUB;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  enable_all_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (sub_16_task)
{
  d.code = OpCode::SUB;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
}

DECODE_TASK0 (sbc_task)
{
  d.code = OpCode::SBC;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  enable_all_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (and_task)
{
  if (d.flag_enable || d.q_field)
    d.code = OpCode::AND_F;
  else
    d.code = OpCode::AND;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_zn_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (or_task)
{
  if (d.flag_enable || d.q_field)
    d.code = OpCode::OR_F;
  else
    d.code = OpCode::OR;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_zn_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (xor_task)
{
  d.code = OpCode::XOR;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_zn_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (abs_task)
{
  d.code = OpCode::ABS;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  enable_all_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (min_task)
{
  d.code = OpCode::MIN;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  enable_all_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (max_task)
{
  d.code = OpCode::MAX;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  enable_all_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (bic_task)
{
  d.code = OpCode::BIC;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
}

DECODE_TASK (mov_task)
{
  f_bit_task (d, isa_opts, eia_mgr, state, pc, inst);
  CLEAR (d.info.rf_renb0);
  if (d.flag_enable || d.q_field)
  {
    d.code = OpCode::MOV_F;
    enable_zn_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    d.code = OpCode::MOV;
  d.kind = arcsim::isa::arc::Dcode::kMove;
}

DECODE_TASK0 (mov_s_task)
{
  CLEAR (d.info.rf_renb0);
  d.code = OpCode::MOV;
  d.kind = arcsim::isa::arc::Dcode::kMove;
}

DECODE_TASK0 (mov_s_ne_task)
{
  CLEAR (d.info.rf_renb0);
  d.code = OpCode::MOV_F;
  d.kind = arcsim::isa::arc::Dcode::kMove;
  d.q_field = 2;          // NE condition
}

DECODE_TASK0 (tst_task)
{
  d.code = OpCode::TST;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_zn_flag_writes (d, isa_opts, eia_mgr);
  d.flag_enable = true;
}

DECODE_TASK0 (cmp_task)
{
  d.code = OpCode::CMP;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  d.flag_enable = true;
  enable_all_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (rcmp_task)
{
  d.code = OpCode::RCMP;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  d.flag_enable = true;
  enable_all_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (rsub_task)
{
  d.code = OpCode::RSUB;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  enable_all_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (bset_task)
{
  d.code = OpCode::BSET;      // logic is: dst = src1 | src2
  d.kind = arcsim::isa::arc::Dcode::kLogical;
}

DECODE_TASK0 (bclr_task)
{
  d.code = OpCode::BCLR;       // logic is: dst = src1 & src2
  d.kind = arcsim::isa::arc::Dcode::kLogical;
}

DECODE_TASK0 (btst_32_task)
{
  d.code = OpCode::BTST;      // logic is: test(src1 & src2)
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_zn_flag_writes (d, isa_opts, eia_mgr);
  d.flag_enable = true;
}

DECODE_TASK0 (btst_16_task)
{
  d.code = OpCode::BTST;      // logic is: test(src1 & src2)
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_zn_flag_writes (d, isa_opts, eia_mgr);
  d.flag_enable = true;
}

DECODE_TASK0 (bxor_task)
{
  d.code = OpCode::BXOR;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
}

DECODE_TASK0 (bmsk_task)
{
  d.code = OpCode::BMSK;    // logic is: dst = src1 & src2
  d.kind = arcsim::isa::arc::Dcode::kLogical;
}

DECODE_TASK0 (bmskn_task)
{
  if (isa_opts.is_isa_a6k()) {
    d.code = OpCode::BMSKN;    // logic is: dst = src1 & ~src2
    d.kind = arcsim::isa::arc::Dcode::kLogical;
  } else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (add1_task)
{
  d.code = OpCode::ADD1;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
}

DECODE_TASK0 (add2_task)
{
  d.code = OpCode::ADD2;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
}

DECODE_TASK0 (add3_task)
{
  d.code = OpCode::ADD3;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
}

DECODE_TASK0 (sub1_task)
{
  d.code = OpCode::SUB1;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
}

DECODE_TASK0 (sub2_task)
{
  d.code = OpCode::SUB2;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
}

DECODE_TASK0 (sub3_task)
{
  d.code = OpCode::SUB3;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
}

DECODE_TASK (sub_s_ne_task)
{
  d.code = OpCode::SUB_F;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  WRITE_0 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  READ_1 (d, isa_opts, eia_mgr, state, (BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24));
  d.q_field = 2;          // NE condition
}

DECODE_TASK1 (mpy32_task, unsigned char subop)
{
  if (isa_opts.mpy32_option)
  {
    d.code = subop;
    d.kind = arcsim::isa::arc::Dcode::kArithmetic;
    enable_znv_flag_writes (d, isa_opts, eia_mgr);
    disallow_lp_count_destination (d, isa_opts, eia_mgr, state, pc, inst);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK1 (mpy16_task, unsigned char subop)
{
  if (isa_opts.mpy16_option)
  {
    d.code = subop;
    d.kind = arcsim::isa::arc::Dcode::kArithmetic;
    enable_znv_flag_writes (d, isa_opts, eia_mgr);
    disallow_lp_count_destination (d, isa_opts, eia_mgr, state, pc, inst);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK1 (sat_alu_task, unsigned char subop)
{
  if (isa_opts.sat_option)
  {
    d.code = subop;
    d.kind = arcsim::isa::arc::Dcode::kArithmetic;
    enable_all_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK1 (sat_znv_task, unsigned char subop)
{
  if (isa_opts.sat_option)
  {
    d.code = subop;
    d.kind = arcsim::isa::arc::Dcode::kArithmetic;
    enable_znv_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (divaw_task)
{
  d.code = OpCode::DIVAW;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
}

DECODE_TASK0 (div_task)
{
  if (isa_opts.div_rem_option)
  {
    d.code = OpCode::DIV;
    d.kind = arcsim::isa::arc::Dcode::kArithmetic;
    enable_znv_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (divu_task)
{
  if (isa_opts.div_rem_option)
  {
    d.code = OpCode::DIVU;
    d.kind = arcsim::isa::arc::Dcode::kArithmetic;
    enable_znv_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (rem_task)
{
  if (isa_opts.div_rem_option)
  {
    d.code = OpCode::REM;
    d.kind = arcsim::isa::arc::Dcode::kArithmetic;
    enable_znv_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (remu_task)
{
  if (isa_opts.div_rem_option)
  {
    d.code = OpCode::REMU;
    d.kind = arcsim::isa::arc::Dcode::kArithmetic;
    enable_znv_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (mul64_task)
{
  if (isa_opts.mul64_option)
  {
    d.code = OpCode::MUL64;
    d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (mulu64_task)
{
  if (isa_opts.mul64_option)
  {
    d.code = OpCode::MULU64;
    d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK1 (swap_task, unsigned char subop)
{
  if (isa_opts.swap_option)
  {
    d.code = subop;
    d.kind = arcsim::isa::arc::Dcode::kLogical;
    enable_zn_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK1 (swap_a6k_task, unsigned char subop)
{
  if (isa_opts.swap_option && isa_opts.is_isa_a6k())
  {
    d.code = subop;
    d.kind = arcsim::isa::arc::Dcode::kLogical;
    enable_zn_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK1 (shas_a6k_task, unsigned char subop)
{
  if (isa_opts.shas_option && isa_opts.is_isa_a6k())
  {
    d.code = subop;
    d.kind = arcsim::isa::arc::Dcode::kLogical;
    enable_zn_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK1 (norm_task, unsigned char subop)
{
  if (isa_opts.norm_option)
  {
    d.code = subop;
    d.kind = arcsim::isa::arc::Dcode::kArithmetic;
    enable_zn_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK1 (norm_a6k_task, unsigned char subop)
{
  if (isa_opts.norm_option && isa_opts.is_isa_a6k())
  {
    d.code = subop;
    d.kind = arcsim::isa::arc::Dcode::kLogical;
    enable_zn_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK1 (setcc_task, unsigned char subop)
{
  if (isa_opts.density_option && isa_opts.is_isa_a6k())
  {
    d.code = subop;
    d.kind = arcsim::isa::arc::Dcode::kLogical;
    enable_all_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);      
}


/////////////////////////////////////////////////////////////
//
// Memory load / store instruction decode tasks
//
/////////////////////////////////////////////////////////////

DECODE_TASK (ld_s_rr_task)
{
  d.code = OpCode::LW;
  d.kind = arcsim::isa::arc::Dcode::kMemLoad;
  //disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (ldb_s_rr_task)
{
  d.code = OpCode::LD_BYTE_U;
  d.kind = arcsim::isa::arc::Dcode::kMemLoad;
  disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (ldw_s_rr_task)
{
  d.code = OpCode::LD_HALF_U;
  d.kind = arcsim::isa::arc::Dcode::kMemLoad;
  //disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (load_16_task)
{
  regs_c_dest_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_b_src1_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  SHIMM (UNSIGNED_BITS(inst,20,16))
  //disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (store_16_task)
{
  regs_b_src1_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_c_src2_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  OFFSET (UNSIGNED_BITS(inst,20,16))
}

DECODE_TASK (ex_task)
{
  if (!isa_opts.is_isa_a600())
  {
    d.code = OpCode::EX;
    d.kind = arcsim::isa::arc::Dcode::kMemExchg;
    d.cache_byp = BITSEL(inst,15);
    READ_0 (d, isa_opts, eia_mgr, state, d.info.rf_wa0);
    //disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
    disallow_lp_count_destination (d, isa_opts, eia_mgr, state, pc, inst);
    disallow_upper32_destination (d, isa_opts, eia_mgr, state, pc, inst);
    if (d.info.rf_ra0 == limm_reg)
      d.illegal_inst_format = true;
    if (d.info.rf_renb0 && d.info.rf_ra0 > 31)
      d.illegal_inst_format = true;
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK (llock_task)
{
  if ((isa_opts.atomic_option < 1) || isa_opts.is_isa_a600()) // A700 and AV2 support LLOCK
  {
    d.illegal_inst_format = true;
  }
  else
  {
    d.code = OpCode::LLOCK;
    d.kind = arcsim::isa::arc::Dcode::kMemLoad;
    d.cache_byp = BITSEL(inst,15);
    disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
    disallow_upper32_destination (d, isa_opts, eia_mgr, state, pc, inst);
  }
}

DECODE_TASK (scond_task)
{
  if ((isa_opts.atomic_option < 1) || isa_opts.is_isa_a600()) // A700 and AV2 support SCOND
  {
    d.illegal_inst_format = true;
  }
  else
  {
    d.code = OpCode::SCOND;
    d.kind = arcsim::isa::arc::Dcode::kMemStore;
    d.cache_byp = BITSEL(inst,15);
    disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
  }
}

DECODE_TASK (load_rr_32_task)
{
  regs_a1bc_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  
  unsigned int opd_size = UNSIGNED_BITS(inst,18,17);
  
  if (opd_size == 0)
  {
    //  Load-word instructions are sub-coded for speed
    //
    switch ( UNSIGNED_BITS(inst,23,22) )
    {
      case 0: // No field syntax
        d.code = OpCode::LW;
        d.kind = arcsim::isa::arc::Dcode::kMemLoad;
        break;
        
      case 1: // .A or .AW mode
      {
        if (d.info.rf_ra0 == limm_reg)
        {
          SET (d.illegal_inst_format);
        }
        else if (d.info.rf_ra0 != d.info.rf_wa1)
          WRITE_0 (d, isa_opts, eia_mgr, state, d.info.rf_ra0);
        d.code = OpCode::LW_A;
        d.kind = arcsim::isa::arc::Dcode::kMemLoad;
        break;
      }
      case 2: // .AB mode
      {
        if (d.info.rf_ra0 == limm_reg)
        {
          SET (d.illegal_inst_format);
        }
        else if (d.info.rf_ra0 != d.info.rf_wa1)
          WRITE_0 (d, isa_opts, eia_mgr, state, d.info.rf_ra0);
        d.pre_addr = true;
        d.code = OpCode::LW_PRE_A;
        d.kind = arcsim::isa::arc::Dcode::kMemLoad;
        break;
      }
      case 3: // .AS mode
        d.addr_shift = 2;
        d.code = OpCode::LW_SH2;
        d.kind = arcsim::isa::arc::Dcode::kMemLoad;
        break;
    }
    //.X is illegal for ZZ=00
    if (BITSEL(inst,16))
      SET (d.illegal_inst_format);
  }
  else
  {
    // Load byte and Load half-word instructions handled here.
    //
    switch (opd_size)
    {
      case 1:  {
        d.code = BITSEL(inst,16) ? OpCode::LD_BYTE_S : OpCode::LD_BYTE_U;
        d.kind = arcsim::isa::arc::Dcode::kMemLoad;
        break;
      }
      case 2:  {
        d.code = BITSEL(inst,16) ? OpCode::LD_HALF_S : OpCode::LD_HALF_U;
        d.kind = arcsim::isa::arc::Dcode::kMemLoad;
        break; }
      default: SET (d.illegal_inst_format);                              break;
    }
    
    switch ( UNSIGNED_BITS(inst,23,22) )
    {
      case 0: // No modifier
        break;
        
      case 1: // .A or .AW mode
      case 2: // .AB mode
      {
        if (d.info.rf_ra0 == limm_reg)
        {
          SET (d.illegal_inst_format);
        }
        else if (d.info.rf_ra0 != d.info.rf_wa1)
          WRITE_0 (d, isa_opts, eia_mgr, state, d.info.rf_ra0);
        d.pre_addr = (BITSEL(inst,22) == 0);
        break;
      }
      case 3: // .AS mode
        if ( UNSIGNED_BITS(inst,18,17) == 2)
          d.addr_shift = 1;
        else //.AS is illegal for LDB
          SET (d.illegal_inst_format);
        break;
    }
  }
  d.cache_byp = BITSEL(inst,15);
  //disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (load_32_task)
{
  regs_a1_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  regs_b_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  
  SHIMM ((SIGNED_BITS(inst,15,15)<<8) | UNSIGNED_BITS(inst,23,16))
  
  unsigned int opd_size = UNSIGNED_BITS(inst,8,7);
  
  //check for the special case prefetch format
  switch (opd_size)
  {
    case (0): //  Load-word instructions are sub-coded for speed
    {
      switch ( UNSIGNED_BITS(inst,10,9) )
      {
        case 0: // No field syntax
          d.code = OpCode::LW;
          d.kind = arcsim::isa::arc::Dcode::kMemLoad;
          break;
          
        case 1: // .A or .AW mode
        {
          if (d.info.rf_ra0 == limm_reg)
          {
            SET (d.illegal_inst_format);
          }
          else if (d.info.rf_ra0 != d.info.rf_wa1)
            WRITE_0 (d, isa_opts, eia_mgr, state, d.info.rf_ra0);
          d.code = OpCode::LW_A;
          d.kind = arcsim::isa::arc::Dcode::kMemLoad;
          break;
        }
        case 2: // .AB mode
        {
          if (d.info.rf_ra0 == limm_reg)
          {
            SET (d.illegal_inst_format);
          }
          else if (d.info.rf_ra0 != d.info.rf_wa1)
            WRITE_0 (d, isa_opts, eia_mgr, state, d.info.rf_ra0);
          SET (d.pre_addr);
          d.code = OpCode::LW_PRE_A;
          d.kind = arcsim::isa::arc::Dcode::kMemLoad;
          break;
        }
        case 3: // .AS mode
          d.addr_shift = 2;
          d.code = OpCode::LW_SH2;
          d.kind = arcsim::isa::arc::Dcode::kMemLoad;
          break;
      }
      //.X is illegal for ZZ=00
      if (BITSEL(inst,6))
        SET (d.illegal_inst_format);
      break;
    }
    case (1): //load byte
    {
      d.code = BITSEL(inst,6) ? OpCode::LD_BYTE_S : OpCode::LD_BYTE_U;
      d.kind = arcsim::isa::arc::Dcode::kMemLoad;
      switch ( UNSIGNED_BITS(inst,10,9) )
      {
        case 0: // No modifier
          break;
        case 1: // .A or .AW mode
        case 2: // .AB mode
        {
          if (d.info.rf_ra0 == limm_reg)
          {
            SET (d.illegal_inst_format);
          }
          else if (d.info.rf_ra0 != d.info.rf_wa1)
            WRITE_0 (d, isa_opts, eia_mgr, state, d.info.rf_ra0);

          d.pre_addr = (BITSEL(inst,9) == 0);
          break;
        }
        case 3: // .AS mode
          SET (d.illegal_inst_format);
          break;
      }
      break;
    }
    case (2): //load half
    {
      d.code = BITSEL(inst,6) ? OpCode::LD_HALF_S : OpCode::LD_HALF_U;
      d.kind = arcsim::isa::arc::Dcode::kMemLoad;
      switch ( UNSIGNED_BITS(inst,10,9) )
      {
        case 0: // No modifier
          break;
        case 1: // .A or .AW mode
        case 2: // .AB mode
        {
          if (d.info.rf_ra0 == limm_reg)
          {
            d.illegal_inst_format = true;
          }
          else if (d.info.rf_ra0 != d.info.rf_wa1)
            WRITE_0 (d, isa_opts, eia_mgr, state, d.info.rf_ra0);
          d.pre_addr = (BITSEL(inst,9) == 0);
          break;
        }
        case 3: // .AS mode
          d.addr_shift = 1;
          break;
      }
      break;
    }
    default: //invalid
    {
      d.illegal_inst_format = true;
    }
  }
  
  d.cache_byp = BITSEL(inst,11);
  //disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (store_32_task)
{
  regs_b_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  
  if (BITSEL(inst,0) == 1) {
    // Overload jmp_target field, which is not otherwise used by STORE instruction
    // with 'shimm'.
    //
    d.jmp_target = SIGNED_BITS(inst,11,6);
    d.src2 = &d.jmp_target;
  }
  else
    regs_c_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  
  OFFSET ((SIGNED_BITS(inst,15,15)<<8) | UNSIGNED_BITS(inst,23,16))
  
  switch ( UNSIGNED_BITS(inst,2,1) )
  {
    case 0:  d.code = OpCode::ST_WORD; d.kind = arcsim::isa::arc::Dcode::kMemStore; break;
    case 1:  d.code = OpCode::ST_BYTE; d.kind = arcsim::isa::arc::Dcode::kMemStore; break;
    case 2:  d.code = OpCode::ST_HALF; d.kind = arcsim::isa::arc::Dcode::kMemStore; break;
    default: SET (d.illegal_operand);  break;
  }
  
  switch ( UNSIGNED_BITS(inst,4,3) )
  {
    case 0: // No modifier
      break;
      
    case 1: // .A or .AW mode
    case 2: // .AB mode
    {
      if (d.info.rf_ra0 == limm_reg)
          SET (d.illegal_inst_format);
      else
        WRITE_0 (d, isa_opts, eia_mgr, state, d.info.rf_ra0);
        
        d.pre_addr = (BITSEL(inst,3) == 0);
      break;
    }
    case 3: // .AS mode
      switch ( UNSIGNED_BITS(inst,2,1) )
    {
      case 0: d.addr_shift = 2;             break;
      case 1: SET (d.illegal_inst_format);  break;
      case 2: d.addr_shift = 1;             break;
      default:                              break;
    }
      break;
  }
  d.cache_byp = BITSEL(inst,5);
}

DECODE_TASK (mem_sp_16_task)
{
  READ_0 (d, isa_opts, eia_mgr, state, SP_REG);
  SHIMM (UNSIGNED_BITS(inst,20,16))
}

DECODE_TASK (r0_gp_16_task)
{
  WRITE_1 (d, isa_opts, eia_mgr, state, 0);      // always writes r0
  READ_0 (d, isa_opts, eia_mgr, state, GP_REG);  // always uses gp as base address
  SHIMM ((SIGNED_BITS(inst,24,24)<<9) | UNSIGNED_BITS(inst,24,16))
  //disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_upper32_destination (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (load_pcl_16_task)
{
  d.code = OpCode::LW_SH2;
  d.kind = arcsim::isa::arc::Dcode::kMemLoad;
  regs_b_ld_dest_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  READ_0 (d, isa_opts, eia_mgr, state, PCL_REG);
  SHIMM (UNSIGNED_BITS(inst,23,16))
  d.addr_shift = 2;
  //disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_upper32_destination (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (mv_imm_16_task)
{
  regs_b_dest_16_task (d, isa_opts, eia_mgr, state, pc, inst);
  SHIMM (UNSIGNED_BITS(inst,23,16))
  d.code = OpCode::MOV;
  d.kind = arcsim::isa::arc::Dcode::kMove;
}

DECODE_TASK (arith_sp_sp_task)
{
  READ_0 (d, isa_opts, eia_mgr, state, SP_REG);
  WRITE_0 (d, isa_opts, eia_mgr, state, SP_REG);
  SHIMM (UNSIGNED_BITS(inst,20,16)<<2)
}

/////////////////////////////////////////////////////////////
//
// Shift and logical operation tasks
//
/////////////////////////////////////////////////////////////

DECODE_TASK0 (asl_task)
{
  d.code = OpCode::ASL;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  enable_znc_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (asr_task)
{
  d.code = OpCode::ASR;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  enable_znc_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (lsr_task)
{
  d.code = OpCode::LSR;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_znc_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (rlc_task)
{
  d.code = OpCode::RLC;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_znc_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (ror_task)
{
  d.code = OpCode::ROR;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_znc_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (rol_task)
{
  if (isa_opts.is_isa_a6k())
  {
    d.code = OpCode::ROL;
    d.kind = arcsim::isa::arc::Dcode::kLogical;
    enable_znc_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (rrc_task)
{
  d.code = OpCode::RRC;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_znc_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK1 (shift_task, unsigned char subop)
{
  if (isa_opts.shift_option)
  {
    d.code = subop;
    d.kind = arcsim::isa::arc::Dcode::kLogical;
    enable_znc_flag_writes (d, isa_opts, eia_mgr);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}   

DECODE_TASK0 (sexb_task)
{
  d.code = OpCode::SEXBYTE;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_zn_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (sexw_task)
{
  d.code = OpCode::SEXWORD;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_zn_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (extb_task)
{
  d.code = OpCode::EXTBYTE;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_zn_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (extw_task)
{
  d.code = OpCode::EXTWORD;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_zn_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (not_task)
{
  d.code = OpCode::NOT;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
  enable_zn_flag_writes (d, isa_opts, eia_mgr);
}

DECODE_TASK0 (neg_task)
{
  d.code = OpCode::NEG;
  d.kind = arcsim::isa::arc::Dcode::kLogical;
}

/////////////////////////////////////////////////////////////
//
// Load and Store Aux Regs, and related tasks
//
/////////////////////////////////////////////////////////////

DECODE_TASK (lr_task)
{
  d.code = OpCode::LR;
  d.kind = arcsim::isa::arc::Dcode::kMove;
  if ((BITSEL(inst,15) == 1) || (UNSIGNED_BITS(inst,23,22) == 3))
    d.illegal_inst_subcode = true;
}

DECODE_TASK (sr_task)
{
  d.code = OpCode::SR;
  d.kind = arcsim::isa::arc::Dcode::kMove;
  if ((BITSEL(inst,15) == 1) || (UNSIGNED_BITS(inst,23,22) == 3))
    d.illegal_inst_subcode = true;
}

DECODE_TASK (aex_task)
{
  d.code = OpCode::AEX;
  d.kind = arcsim::isa::arc::Dcode::kMove;
  //AEX also writes to the register it reads from
  d.info.rf_wa0 = d.info.rf_ra0;
  d.info.rf_wenb0 = true;
  d.dst1 = d.src1;
}

DECODE_TASK (flag_task)
{
  d.code = OpCode::FLAG;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowFlag;
  if (isa_opts.is_isa_a6k())
  {
    d.flag_enable = BITSEL(inst,15);
  }
  else
  {  
    if (BITSEL(inst,15))
      d.illegal_inst_subcode = true;
  }
}

/////////////////////////////////////////////////////////////
//
// Jump and branch micro-code tasks
//
/////////////////////////////////////////////////////////////

DECODE_TASK (link_task)
{
  SET (d.link);       // enable linkage action
  WRITE_0 (d, isa_opts, eia_mgr, state, BLINK);   // use BLINK reg as destination
}

DECODE_TASK (jcc_task)
{
  d.dslot = BITSEL(inst,16);
  d.flag_enable = BITSEL(inst,15);
  if (    d.info.rf_renb1 
       && ((d.info.rf_ra1 == ILINK1) || (d.info.rf_ra1 == ILINK2))
       && !isa_opts.is_isa_a6k()
     )
  {
    //the .D and .F forms are mutually exclusive
    if (d.dslot || !d.flag_enable)
      d.illegal_inst_format = true;
    else if (d.info.rf_ra1 == ILINK1)
      d.code = OpCode::J_F_ILINK1;
    else
      d.code = OpCode::J_F_ILINK2;
  } else {
    d.code = OpCode::JCC_SRC2;
  }
  d.kind = arcsim::isa::arc::Dcode::kControlFlowJump;
  if (d.dslot)
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA if delayed branch
  else
    d.dst2 = &(state.next_pc);       // set next_PC if not delayed
  d.info.isReturn = d.info.rf_renb1 && (d.info.rf_ra1 == BLINK);
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (j_s_task)
{
  d.code = OpCode::JCC_SRC1;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowJump;
  d.dslot = BITSEL(inst,21);
  if (d.dslot)
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA if delayed branch
  else
    d.dst2 = &(state.next_pc);       // set next_PC if not delayed
  d.info.isReturn = d.info.rf_renb0 && (d.info.rf_ra0 == BLINK);
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (jeq_s_task)
{
  d.code = OpCode::JCC_SRC2;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowJump;
  READ_1 (d, isa_opts, eia_mgr, state, BLINK);   // select BLINK as jump target source
  d.q_field = 1;    // select the .EQ condition
  d.dst2 = &(state.next_pc);
  d.info.isReturn = true;
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (jne_s_task)
{
  d.code = OpCode::JCC_SRC2;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowJump;
  READ_1 (d, isa_opts, eia_mgr, state, BLINK);   // select BLINK as jump target source
  d.q_field = 2;    // select the .NE condition
  d.dst2 = &(state.next_pc);
  d.info.isReturn = true;
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (nop_task)
{
  d.code = OpCode::NOP;
  d.kind = arcsim::isa::arc::Dcode::kHintNop;
}

DECODE_TASK (sync_task)
{
  d.code = OpCode::SYNC;
  d.kind = arcsim::isa::arc::Dcode::kHintSync;
}

DECODE_TASK (j_blink_task)
{
  d.code = OpCode::JCC_SRC2;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowJump;
  READ_1 (d, isa_opts, eia_mgr, state, BLINK);   // select BLINK as jump target source
  d.dslot = BITSEL(inst,24); // get dslot indicator
  if (d.dslot)
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA if delayed branch
  else
    d.dst2 = &(state.next_pc);       // set next_PC if not delayed
  d.info.isReturn = true;
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (jlcc_task)
{
  d.code = OpCode::JCC_SRC2;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowJump;
  d.flag_enable = BITSEL(inst,15);
  d.dslot = BITSEL(inst,16); // get dslot indicator
  
  if (    d.info.rf_renb1 
       && ((d.info.rf_ra1 == ILINK1) || (d.info.rf_ra1 == ILINK2))
       && !isa_opts.is_isa_a6k()
     )
  {
    //the .D and .F forms are mutually exclusive
    //and .F is mandatory for ilink update
    if (d.dslot || !d.flag_enable)
      d.illegal_inst_format = true;
  }
  
  if (d.dslot)
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA if delayed branch
  else
    d.dst2 = &(state.next_pc);       // set next_PC if not delayed
  link_task (d, isa_opts, eia_mgr, state, pc, inst);
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (jl_s_task)
{
  d.code = OpCode::JCC_SRC1;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowJump;
  d.dslot = BITSEL(inst,21); // get dslot indicator
  if (d.dslot)
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA if delayed branch
  else
    d.dst2 = &(state.next_pc);       // set next_PC if not delayed
  link_task (d, isa_opts, eia_mgr, state, pc, inst);
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (jl_task)
{
  d.code = OpCode::JCC_SRC2;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowJump;
  d.dslot = BITSEL(inst,16);// get dslot indicator
  if (d.dslot)
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA if delayed branch
  else
    d.dst2 = &(state.next_pc);       // set next_PC if not delayed
  link_task (d, isa_opts, eia_mgr, state, pc, inst);
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (lpcc_task)
{
  d.code = OpCode::LPCC;
  d.kind = arcsim::isa::arc::Dcode::kMove;
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (rtie_task)
{
  d.code = OpCode::RTIE;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowJump;
  privileged_instruction (d, isa_opts, eia_mgr, state, pc, inst);
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (trap_task)
{
  d.code = OpCode::TRAP0;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowTrap;
  // shimm operand is the fully-formed ECR value
  //
  SHIMM(ECR(isa_opts.EV_Trap, 0, 0));
}

DECODE_TASK (swi_task)
{
  d.code = OpCode::SWI;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowTrap;
  // shimm operand is the fully-formed ECR value
  //
  SHIMM(ECR(isa_opts.EV_SWI, 0, 0));
}

DECODE_TASK (trap_s_task)
{
  d.code = OpCode::TRAP0;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowTrap;
  // shimm operand is the fully-formed ECR value
  //
  SHIMM(ECR(isa_opts.EV_Trap, 0, UNSIGNED_BITS(inst,26,21)));
}

DECODE_TASK (brk_task)
{
  d.code = OpCode::BREAK;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowTrap;
  if (state.U && !(state.auxs[AUX_DEBUG] & 0x10000000UL))
    privilege_violation_task (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (sleep_task)
{
  d.code = OpCode::SLEEP;
  d.kind = arcsim::isa::arc::Dcode::kHintSleep;
  d.info.rf_wa0 = 0;
  CLEAR (d.info.rf_wenb0);
  privileged_instruction (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK(seti_task)
{
  if(isa_opts.is_isa_a6kv2())
  {
    d.code = OpCode::SETI;
    d.kind = arcsim::isa::arc::Dcode::kHintNop;

    privileged_instruction(d, isa_opts, eia_mgr, state, pc, inst);
  }
  else
  {
    d.code = OpCode::EXCEPTION;
    d.illegal_inst_subcode = true;
  }
}

DECODE_TASK(clri_task)
{
  if(isa_opts.is_isa_a6kv2())
  {
    d.code = OpCode::CLRI;
    d.kind = arcsim::isa::arc::Dcode::kHintNop;
  
    d.info.rf_wenb0 = true;
    d.info.rf_wa0 = d.info.rf_ra0;
    
    privileged_instruction(d, isa_opts, eia_mgr, state, pc, inst);
  }
  else
  {
    d.code = OpCode::EXCEPTION;
    d.illegal_inst_subcode = true;
  }
}

DECODE_TASK (br_cond_task)
{
  d.code = OpCode::BCC;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowBranch;
  regs_q_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  d.dslot           = BITSEL(inst,5);
  if (d.dslot)
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA if delayed branch
  else
    d.dst2 = &(state.next_pc);       // set next_PC if not delayed
  REL_JUMP ((SIGNED_BITS(inst,15,6)<<11) | UNSIGNED_BITS(inst,26,17)<<1)
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK2 (bcc_s_task, uint32 o_bits, uint8 q_bits)
{
  if (q_bits == 0)
    d.code = OpCode::BR;
  else
  {
    d.code = OpCode::BCC;
    d.q_field = q_bits;
  }
  d.kind = arcsim::isa::arc::Dcode::kControlFlowBranch;
  d.dst2 = &(state.next_pc);
  REL_JUMP (o_bits << 1)
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (br_ucond_task)
{
  d.code = OpCode::BR;
  d.dslot = BITSEL(inst,5);
  if (d.dslot)
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA if delayed branch
  else
    d.dst2 = &(state.next_pc);       // set next_PC if not delayed
  d.kind = arcsim::isa::arc::Dcode::kControlFlowBranch;
  REL_JUMP ((SIGNED_BITS(inst,3,0)<<21) | (UNSIGNED_BITS(inst,15,6)<<11) | (UNSIGNED_BITS(inst,26,17)<<1))
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (bl_cond_task)
{
  d.code = OpCode::BCC;  
  d.kind = arcsim::isa::arc::Dcode::kControlFlowBranch;
  regs_q_32_task (d, isa_opts, eia_mgr, state, pc, inst);
  d.dslot = BITSEL(inst,5);
  if (d.dslot)
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA if delayed branch
  else
    d.dst2 = &(state.next_pc);       // set next_PC if not delayed
  REL_JUMP ((SIGNED_BITS(inst,15,6)<<11) | (UNSIGNED_BITS(inst,26,18)<<2))
  link_task (d, isa_opts, eia_mgr, state, pc, inst);
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (bl_s_ucond_task)
{
  d.code = OpCode::BR;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowBranch;
  REL_JUMP (SIGNED_BITS(inst,26,16)<<2)
  d.dst2 = &(state.next_pc);
  link_task (d, isa_opts, eia_mgr, state, pc, inst);
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (bl_ucond_task)
{
  d.code = OpCode::BR;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowBranch;
  d.dslot = BITSEL(inst,5);
  if (d.dslot)
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA if delayed branch
  else
    d.dst2 = &(state.next_pc);       // set next_PC if not delayed
  
  REL_JUMP ((SIGNED_BITS(inst,3,0)<<21) | (UNSIGNED_BITS(inst,15,6)<<11) | (UNSIGNED_BITS(inst,26,18)<<2))
  link_task (d, isa_opts, eia_mgr, state, pc, inst);
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK2 (brcc_bbit_task, unsigned char br_op, uint8 q_bits)
{
  //
  // Fundamental tests that can be performed for BRcc or BBITn:
  //
  //  Inst   Test performed  q_bits is_bbit Logic
  //  --------------------------------------------
  //  BBIT0  Zero             0001  1       Z
  //  BBIT1  Non-zero         0010  1      ~Z
  //  BREQ   Zero             0001  0       Z
  //  BRNE   Non-zero         0010  0      ~Z
  //  BRLT   Less-than        1011  0       N != V
  //  BRGE   Greater-or-equal 1010  0      ~(N != V)
  //  BRLO   Unsigned Lower   0101  0       C
  //  BRHS   Unsigned greater 0110  0      ~C
  //  --------------------------------------------
  //
  //  The Q field is the same as that used for Bcc instructions, which
  //  have a greater vocabulary of tests that can be performed.
  //  This allows the condition to be encoded canonically for all
  //  conditional branches.
  
  d.code = br_op;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowBranch;
  d.dslot = BITSEL(inst,5);
  if (d.dslot)
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA if delayed branch
  else
    d.dst2 = &(state.next_pc);       // set next_PC if not delayed
  d.q_field = q_bits;
  REL_JUMP ((SIGNED_BITS(inst,15,15)<<8) | UNSIGNED_BITS(inst,23,17)<<1)
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK1 (brcc_s_task, uint8 q_bits)
{
  d.code = OpCode::BRCC;
  d.kind = arcsim::isa::arc::Dcode::kControlFlowBranch;
  d.q_field = q_bits;
  d.dst2 = &(state.next_pc);
  REL_JUMP (SIGNED_BITS(inst,22,16)<<1)
  disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (jli_s_task)
{
  if (isa_opts.density_option)
  {
    d.code = OpCode::JLI_S;
    d.kind = arcsim::isa::arc::Dcode::kControlFlowJump;
    ABS_JUMP(UNSIGNED_BITS(inst,25,16)<<2);
    d.src1 = &(state.auxs[AUX_JLI_BASE]);
    d.src2 = &d.jmp_target;
    link_task (d, isa_opts, eia_mgr, state, pc, inst);
    disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK (bi_task)
{
  if (isa_opts.density_option)
  {
    d.dst2 = &(state.next_pc);
    d.code = BITSEL(inst, 16) ? OpCode::BIH : OpCode::BI;
    d.kind = arcsim::isa::arc::Dcode::kControlFlowBranch;
    disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK (ei_s_task)
{
  if (isa_opts.density_option > 1)
  {
    d.code = OpCode::EI_S;
    d.kind = arcsim::isa::arc::Dcode::kControlFlowJump;
    ABS_JUMP(UNSIGNED_BITS(inst,25,16)<<2);
    d.src1 = &(state.auxs[AUX_EI_BASE]);
    d.src2 = &d.jmp_target;
    d.dst2 = &(state.auxs[AUX_BTA]); // set BTA as dst2
    disallowed_in_dslot (d, isa_opts, eia_mgr, state, pc, inst);
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK (ld_as_16_task)
{
  d.code = OpCode::LW_SH2;
  d.kind = arcsim::isa::arc::Dcode::kMemLoad;
  d.addr_shift = 2;
  WRITE_1 (d, isa_opts, eia_mgr, state, ((BITSEL(inst,18)<<3) | UNSIGNED_BITS(inst,18,16)));
  disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_upper32_destination (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (ld_rr_u5_16_task)
{
  d.code = OpCode::LW;
  d.kind = arcsim::isa::arc::Dcode::kMemLoad;
  SHIMM ((BITSEL(inst,26)<<4) | (UNSIGNED_BITS(inst,20,19)<<2))
  WRITE_1 (d, isa_opts, eia_mgr, state, UNSIGNED_BITS(inst,25,24));
  disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
  disallow_upper32_destination (d, isa_opts, eia_mgr, state, pc, inst);
}

DECODE_TASK (add_r01_u6_task)
{
  d.code = OpCode::ADD;
  d.kind = arcsim::isa::arc::Dcode::kArithmetic;
  SHIMM ((UNSIGNED_BITS(inst,22,20)<<3) | UNSIGNED_BITS(inst,18,16))
  WRITE_0 (d, isa_opts, eia_mgr, state, BITSEL(inst,23));
}

DECODE_TASK (ldi_s_task)
{
  if(isa_opts.density_option)
  {
    d.code = OpCode::LW;
    d.kind = arcsim::isa::arc::Dcode::kMemLoad;
    d.src1 = &(state.auxs[AUX_LDI_BASE]);
    SHIMM ((UNSIGNED_BITS(inst,23,20)<<5) | (UNSIGNED_BITS(inst,18,16)<<2))
    WRITE_1 (d, isa_opts, eia_mgr, state, ((BITSEL(inst,26)<<3) | UNSIGNED_BITS(inst,26,24)));
    disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
    disallow_upper32_destination (d, isa_opts, eia_mgr, state, pc, inst);
  }else{
    inst_error_task (d, isa_opts, eia_mgr);
  }
}

DECODE_TASK (ldi_task)
{
  if(isa_opts.density_option)
  {
    d.code = OpCode::LDI;
    d.kind = arcsim::isa::arc::Dcode::kMemLoad;
    d.src1 = &(state.auxs[AUX_LDI_BASE]);
    d.addr_shift = 2;
    WRITE_1 (d, isa_opts, eia_mgr, state, ((UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24)));
    disallow_lp_count_destination1 (d, isa_opts, eia_mgr, state, pc, inst);
    disallow_upper32_destination (d, isa_opts, eia_mgr, state, pc, inst);
  }else{
    inst_error_task (d, isa_opts, eia_mgr);
  }
}

DECODE_TASK (mem_r01_gp_s9_task)
{
  uint32 memop = BITSEL(inst,20);
  READ_0 (d, isa_opts, eia_mgr, state, GP_REG);
  if (memop == 0) {
    d.code = OpCode::LW;
    d.kind = arcsim::isa::arc::Dcode::kMemLoad;
    SHIMM ((SIGNED_BITS(inst,26,21)<<5) | (UNSIGNED_BITS(inst,18,16)<<2))
    WRITE_1 (d, isa_opts, eia_mgr, state, 1); // R1 is the load destination
  } else {
    d.code = OpCode::ST_WORD;
    d.kind = arcsim::isa::arc::Dcode::kMemStore;
    OFFSET ((SIGNED_BITS(inst,26,21)<<5) | (UNSIGNED_BITS(inst,18,16)<<2))
    READ_1 (d, isa_opts, eia_mgr, state, 0); // R0 provides the store data
  }
}

DECODE_TASK (group8_task)
{
  if (isa_opts.density_option > 1)
  {
    if (BITSEL(inst,18)) {
      // ld rr,[h,u5]
      regs_h_src1_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      ld_rr_u5_16_task (d, isa_opts, eia_mgr, state, pc, inst);
    } else {
      // mov_s g,h
      regs_h_src2_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      regs_g_dst_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      mov_s_task (d, isa_opts, eia_mgr);
    }
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK (group9_task)
{
  if (isa_opts.density_option > 1)
  {
    regs_b_src1_16_task (d, isa_opts, eia_mgr, state, pc, inst);
    if (BITSEL(inst,19)) {
      // add_s r0/1,b,u6
      add_r01_u6_task (d, isa_opts, eia_mgr, state, pc, inst);
    } else {
      regs_c_src2_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      if (BITSEL(inst,20)) {
        regs_a_dst_16_task (d, isa_opts, eia_mgr, state, pc, inst);
        sub_16_task (d, isa_opts, eia_mgr);
      } else {
        ld_as_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      }
    }
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK (group10_task)
{
  if (isa_opts.density_option > 1)
  {
    if (BITSEL(inst,19)) {
      ldi_s_task (d, isa_opts, eia_mgr, state, pc, inst);
    } else {
      mem_r01_gp_s9_task (d, isa_opts, eia_mgr, state, pc, inst);
    }
  }
  else
    inst_error_task (d, isa_opts, eia_mgr);
}

DECODE_TASK (finalise_task)
{
  flag_enable_task (d, isa_opts, eia_mgr);
  
  // Check for jump/branch with delay slot and a long immediate operand
  //
  if (d.has_limm && d.dslot) {
    inst_error_task (d, isa_opts, eia_mgr);
  }
 
  // Check for any write to r63 (PCL)
  //
  disallow_r63_destination (d, isa_opts, eia_mgr, state, pc, inst);
  
  // Check for any access to reserved register r61
  //
  disallow_r61_access (d, isa_opts, eia_mgr, state, pc, inst);

  // Check for writes to LP_COUNT by multi-cycle operations in ARC6000
  //
  disallow_lpc_a6k_writes (d, isa_opts, eia_mgr, state, pc, inst);
  
  // Check for illegal format/opcode/operand types
  //
  if ( d.illegal_operand || d.illegal_inst_format || d.illegal_inst_subcode )
  {
    inst_error_task (d, isa_opts, eia_mgr);
  }
  // If NO EIA core registers have been defined AND we are NOT simulating the
  // ARC 600 with multiply result registers enabled AND we are trying to access
  // registers r32-r59 AND NOT ARC 700 TSCH (r58) register, this will result in
  // an inst_error_task.
  //
  else if (  /* 1. CHECK that we are NOT simulating A600 with MUL64 option and access r57 to r59 */
             !(   isa_opts.is_isa_a600()
               && isa_opts.mul64_option
               && (   (d.info.rf_renb0 && (d.info.rf_ra0 >= MLO_REG) && (d.info.rf_ra0 <= MHI_REG))
                   || (d.info.rf_renb1 && (d.info.rf_ra1 >= MLO_REG) && (d.info.rf_ra1 <= MHI_REG))   ))
              /* 2. CHECK that we are NOT simulating A700 with access r58  */
               && ( !(   isa_opts.is_isa_a700()
                      && (   (d.info.rf_renb0 && (d.info.rf_ra0 == TSCH_REG)  )
                          || (d.info.rf_renb1 && (d.info.rf_ra1 == TSCH_REG)  ) )))
              /* 3. CHECK that NO EIA core regs are defined and try to access r32-r59 */
           && (   !eia_mgr.are_eia_aux_regs_defined
               && ((   ((d.info.rf_ra0 > BLINK) && (d.info.rf_ra0 < LP_COUNT))
                    || ((d.info.rf_ra1 > BLINK) && (d.info.rf_ra1 < LP_COUNT))
                    || ((d.info.rf_wa0 > BLINK) && (d.info.rf_wa0 < LP_COUNT))
                    || ((d.info.rf_wa1 > BLINK) && (d.info.rf_wa1 < LP_COUNT)))    ))
          )
  {
    inst_error_task (d, isa_opts, eia_mgr);
  }
  // Check for ILINK1 or ILINK2 access in User mode
  //
  else if (   state.U
           && (   (d.info.rf_renb0 && ((d.info.rf_ra0 == ILINK1) || (d.info.rf_ra0 == ILINK2)))
               || (d.info.rf_renb1 && ((d.info.rf_ra1 == ILINK1) || (d.info.rf_ra1 == ILINK2)))
               || (d.info.rf_wenb0 && ((d.info.rf_wa0 == ILINK1) || (d.info.rf_wa0 == ILINK2)))
               || (d.info.rf_wenb1 && ((d.info.rf_wa1 == ILINK1) || (d.info.rf_wa1 == ILINK2))) ))
  {
    // Accessing ILINKx in User mode is a privilege violation, rather than illegal instruction
    //
    privilege_violation_task (d, isa_opts, eia_mgr, state, pc, inst);
  }
}

DECODE_TASK (ext32_operands_task)
{
  switch ( UNSIGNED_BITS(inst,23,22) ) {
    case REG_REG_FMT:  // REG_REG format
      switch ( UNSIGNED_BITS(inst,21,16) ) {
        case SOP_FMT: regs_sop_bc_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;
        default:      regs_abc_32_task (d, isa_opts, eia_mgr, state, pc, inst);    break;
      }
      break;
      
    case REG_U6IMM_FMT:  // REG_U6IMM format
      switch ( UNSIGNED_BITS(inst,21,16) ) {
        case SOP_FMT: regs_sop_bu6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;
        default:      regs_abu6_32_task (d, isa_opts, eia_mgr, state, pc, inst);    break;
      }
      break;
      
    case REG_S12IMM_FMT: // REG_S12IMM format
      regs_bbs12_32_task (d, isa_opts, eia_mgr, state, pc, inst);
      break;
      
    case REG_COND_FMT: // REG_COND format
      switch ( BITSEL(inst,5) ) {
        case false:   regs_c_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break;
        case true:    regs_u6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;
      }
      regs_bbq_32_task (d, isa_opts, eia_mgr, state, pc, inst);
      break;
  }
}

DECODE_TASK (minor6_operators_task)
{
  unsigned minor_op_bits = UNSIGNED_BITS(inst,21,16);
  switch (minor_op_bits)
  {
    case FMUL_OP:
    case FADD_OP:
    case FSUB_OP:
    case DMULH11_OP:  case DMULH12_OP:
    case DMULH21_OP:  case DMULH22_OP:
    case DADDH11_OP:  case DADDH12_OP:
    case DADDH21_OP:  case DADDH22_OP:
    case DSUBH11_OP:  case DSUBH12_OP:
    case DSUBH21_OP:  case DSUBH22_OP:
    case DRSUBH11_OP: case DRSUBH12_OP:
    case DRSUBH21_OP: case DRSUBH22_OP:
    case DEXCL1_OP:   case DEXCL2_OP:
    {
      if (isa_opts.fpx_option) {
        d.code = minor6_map[minor_op_bits];
        d.kind = arcsim::isa::arc::Dcode::kArithmetic;
      } else {
        inst_error_task(d, isa_opts, eia_mgr);
      }
      break;
     } 
    case SOP_FMT: { // All Single or Zero operand 32-bit insns
      switch ( UNSIGNED_BITS(inst,5,0) ) {
        case ZOP_FMT: // All zero-operand 32-bit EXT0 fmts
          zero_operand_task (d, isa_opts, eia_mgr, state, pc, inst);
          switch ( ((UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24)) ) {
            // Decode any ZOP-format instructions in minor group 6
            // No instructions currently defined in this fmt
            default:
              inst_error_task (d, isa_opts, eia_mgr);
              break;
          }
        default:
          inst_error_task (d, isa_opts, eia_mgr);
          break;
      }
      break;
    }
    default: {
      inst_error_task (d, isa_opts, eia_mgr);
      break;
    }
  }
}

// -----------------------------------------------------------------------------
// DECODE METHODS
//

// Constructor
//
Dcode::Dcode() : in_dslot(false)
{ /* EMPTY */ }

// Destructor
//
Dcode::~Dcode()
{ /* EMPTY */ }

void
Dcode::set_instruction_error(const IsaOptions& isa_opts,
                             const arcsim::sys::cpu::EiaExtensionManager& eia_mgr)
{
  inst_error_task (*this, isa_opts, eia_mgr);
  
}
      
void
Dcode::clear (cpuState& state)
{
  code = OpCode::EXCEPTION;
  kind = kException;
  size = 0;
  dst1 = dst2 = 0;
  src1 = src2 = 0;
  limm = shimm = 0;
  has_limm = false;
  q_field = 0;
  jmp_target = 0;
  addr_shift = 0;
  pre_addr = false;
  flag_enable = false;
  z_wen = n_wen = c_wen = v_wen = false;
  link = false;
  link_offset = 0;
  dslot = false;
  taken_branch = false;
  cache_byp = false;
  illegal_operand = illegal_inst_format = illegal_inst_subcode = false;
  illegal_in_dslot = false;
  aps_inst_matches = 0;
  eia_inst = 0;
  eia_cond = 0;
  xpu_required = 0;
  
  // Clear info structure
  //
  info.isReturn = false;
  info.rf_wa0   = info.rf_wa1   = 0;
  info.rf_ra0   = info.rf_ra1   = 0;
  info.rf_wenb0 = info.rf_wenb1 = false;
  info.rf_renb0 = info.rf_renb1 = false;
  
#ifdef REGTRACK_SIM
  dst1_stats = dst2_stats = src1_stats = src2_stats = 0;
#endif /* REGTRACK_SIM */
  
#ifdef CYCLE_ACC_SIM
  mem_cycles = 1;
  src1_avail = src2_avail = &(state.t0);
  dst1_avail = dst2_avail = &(state.ignore);
#endif /* CYCLE_ACC_SIM */
}
      

void
Dcode::decode(const IsaOptions& isa_opts, uint32 inst, uint32 pc, cpuState& state,
              const arcsim::sys::cpu::EiaExtensionManager& eia_mgr,
              bool from_dslot)
{
  Dcode& d  = *this;
  in_dslot  = from_dslot;

  // Clear all microcode fields
  //
  d.clear (state);
  
  // Initialise default instruction size
  //
  d.size = 4;  
  
  // ---------------------------------------------------------------------------
  // THE BIG DECODING SWITCH for the 32 major opcode groups
  //
  switch ( UNSIGNED_BITS(inst,31,27) )
  {
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 0
    //
    case GRP_BRANCH_32:     {
      switch ( BITSEL(inst,16) ) {
        case 0: { br_cond_task  (d, isa_opts, eia_mgr, state, pc, inst); break; }
        case 1: { br_ucond_task (d, isa_opts, eia_mgr, state, pc, inst); break; }
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 1
    //
    case GRP_BL_BRCC_32:    {
      switch ( UNSIGNED_BITS(inst,17,16) ) {
        case 0: { bl_cond_task (d, isa_opts, eia_mgr, state, pc, inst);  break; }
        case 2: { bl_ucond_task (d, isa_opts, eia_mgr, state, pc, inst); break; }
        case 1:
        case 3: {
          switch ( BITSEL(inst,4) ) {
            case 0: { regs_bc_32_task  (d, isa_opts, eia_mgr, state, pc, inst); break; }
            case 1: { regs_bu6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break; }
          }
          switch ( UNSIGNED_BITS(inst,2,0) ) {
            case BREQ_OP:  brcc_bbit_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::BRCC,  BREQ_COND); break;
            case BRNE_OP:  brcc_bbit_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::BRCC,  BRNE_COND); break;
            case BRLT_OP:  brcc_bbit_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::BRCC,  BRLT_COND); break;
            case BRGE_OP:  brcc_bbit_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::BRCC,  BRGE_COND); break;
            case BRLO_OP:  brcc_bbit_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::BRCC,  BRLO_COND); break;
            case BRHS_OP:  brcc_bbit_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::BRCC,  BRHS_COND); break;
            case BBIT0_OP: brcc_bbit_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::BBIT0, BBIT0_COND); break;
            case BBIT1_OP: brcc_bbit_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::BBIT1, BBIT1_COND); break;
          }
        }
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 2
    //
    case GRP_LOAD_32:       {
      load_32_task (d, isa_opts, eia_mgr, state, pc, inst);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 3
    //
    case GRP_STORE_32:      {
      store_32_task (d, isa_opts, eia_mgr, state, pc, inst);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 4
    //
    case GRP_BASECASE_32:   {
      // Firstly, decode the operand format
        if ( (UNSIGNED_BITS(inst,21,19) & 0x07) == LD_RR_FMT) {
          load_rr_32_task (d, isa_opts, eia_mgr, state, pc, inst); // this is a special case
        } else {
          switch ( UNSIGNED_BITS(inst,23,22) ) {
            case REG_REG_FMT:  // REG_REG format for major opcode 0x04
              switch ( UNSIGNED_BITS(inst,21,16) ) {
                  // firstly all exceptions are listed
                case MOV_OP:
                case LR_OP:     regs_mov_bc_task (d, isa_opts, eia_mgr, state, pc, inst); break;    // mov or lr reg-reg
                case SR_OP:
                case TST_OP:
                case BTST_OP:
                case CMP_OP:
                case RCMP_OP:   regs_bc_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;    // source regs only
                case BI_OP:
                case BIH_OP:
                case LDI_OP:
                case FLAG_OP:
                case JCC_OP:
                case JCC_D_OP:
                case JLCC_OP:
                case JLCC_D_OP: regs_c_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;     // J/JL [c]
                case SOP_FMT:   regs_sop_bc_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break; // SOP b,[c|limm]
                case LPCC_OP:   illegal_inst_format = true; break;    // LPcc disallowed
                  // secondly, the default operands are extracted
                default:  regs_abc_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;  // Generic reg-reg
              } break;
              
            case REG_U6IMM_FMT:  // REG_U6IMM format for major opcode 0x04
              switch ( UNSIGNED_BITS(inst,21,16) ) {
                  // firstly all exceptions are listed
                case MOV_OP:
                case LR_OP:     regs_mov_bu6_task (d, isa_opts, eia_mgr, state, pc, inst); break;   // mov or lr reg-u6
                case SR_OP:
                case TST_OP:
                case BTST_OP:
                case CMP_OP:
                case RCMP_OP:   regs_bu6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;     // source b,u6 only
                case BI_OP:
                case BIH_OP:    illegal_inst_format = true; break;
                case LDI_OP:
                case JCC_OP:
                case JCC_D_OP:
                case JLCC_OP:
                case JLCC_D_OP:
                case FLAG_OP:   regs_u6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;
                case SOP_FMT:   regs_sop_bu6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break; // SOP b,u6
                case LPCC_OP:   regs_lpcc_u6_task (d, isa_opts, eia_mgr, state, pc, inst); break;   // lpcc u7
                  // secondly, the default operands are extracted
                default:  regs_abu6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;    // Generic reg-u6
              } break;
              
            case REG_S12IMM_FMT: // REG_S12IMM format for major opcode 0x04
              switch ( UNSIGNED_BITS(inst,21,16) ) {
                  // firstly all exceptions are listed
                case MOV_OP:
                case LR_OP:     regs_mov_bs12_task (d, isa_opts, eia_mgr, state, pc, inst); break;  // mov or lr reg-s12
                case FLAG_OP:   flag_s12_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;
                case SR_OP:
                case TST_OP:
                case BTST_OP:
                case CMP_OP:
                case RCMP_OP:   regs_bs12_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;    // source b,s12 only
                case BI_OP:
                case BIH_OP:    illegal_inst_format = true; break;
                case LDI_OP:
                case JCC_OP:
                case JCC_D_OP:
                case JLCC_OP:
                case JLCC_D_OP: jmp_s12_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break;  // J/JL s12
                case LPCC_OP:   jmp_lpcc_s12_task (d, isa_opts, eia_mgr, state, pc, inst); break;  // ucond LP s13
                  // secondly, the default operands are extracted
                default:  regs_bbs12_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;  // Generic reg-s12
              } break;
              
            case REG_COND_FMT: // REG_COND format for major opcode 0x04
            {
              switch ( UNSIGNED_BITS(inst,21,16) ) {
                  // firstly all exceptions are listed
                case MOV_OP:        // MOV.cc b <- c
                {
                  regs_q_32_task (d, isa_opts, eia_mgr, state, pc, inst);
                  switch (BITSEL(inst,5)) {
                    case false: regs_mov_bc_task (d, isa_opts, eia_mgr, state, pc, inst);  break;
                    case true:  regs_mov_bu6_task (d, isa_opts, eia_mgr, state, pc, inst); break ;
                  }
                } break;
                  
                case BI_OP:
                case BIH_OP:
                case LR_OP:                             // LR disallowed
                case SR_OP:     illegal_inst_format = true; break;    // SR disallowed
                case TST_OP:
                case BTST_OP:
                case CMP_OP:
                case RCMP_OP:       // Two source regs
                {
                  regs_b_32_task (d, isa_opts, eia_mgr, state, pc, inst);
                  regs_q_32_task (d, isa_opts, eia_mgr, state, pc, inst);
                  switch (BITSEL(inst,5)) {
                    case false: regs_c_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break;
                    case true:  regs_u6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;
                  }
                } break;
                  
                case LDI_OP:
                case FLAG_OP:
                case JCC_OP:
                case JCC_D_OP:
                case JLCC_OP:
                case JLCC_D_OP:     // One source + cc field
                {
                  regs_q_32_task (d, isa_opts, eia_mgr, state, pc, inst);
                  switch (BITSEL(inst,5)) {
                    case false: regs_c_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break;
                    case true:  regs_u6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;
                  }
                  break;
                }

                case LPCC_OP: {
                  if (BITSEL(inst,5)) {
                    regs_lpcc_u6q_task (d, isa_opts, eia_mgr, state, pc, inst);
                  } else {
                    illegal_inst_subcode = true;
                  }
                  break;
                }
                  
                case SOP_FMT:
                {
                  illegal_inst_format = true;
                  break;
                }
                  
                  // secondly, the default operands are extracted
                default:  {
                  regs_bbq_32_task (d, isa_opts, eia_mgr, state, pc, inst);    // Generic reg-cond
                  switch (BITSEL(inst,5)) {
                    case false: regs_c_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break;
                    case true:  regs_u6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;
                  }
                  break;
                }
                  // take care of Section 4.9.9.1
              }
            } break;
          }
          
          // Secondly, decode the operator
          switch ( UNSIGNED_BITS(inst,21,16) ) {
            case ADD_OP:   add_task (d, isa_opts, eia_mgr); break;
            case ADC_OP:   adc_task (d, isa_opts, eia_mgr); break;
            case SUB_OP:   sub_task (d, isa_opts, eia_mgr); break;
            case SBC_OP:   sbc_task (d, isa_opts, eia_mgr); break;
            case AND_OP:   and_task (d, isa_opts, eia_mgr); break;
            case OR_OP:    or_task (d, isa_opts, eia_mgr);  break;
            case BIC_OP:   bic_task (d, isa_opts, eia_mgr); break;
            case XOR_OP:   xor_task (d, isa_opts, eia_mgr); break;
            case MAX_OP:   max_task (d, isa_opts, eia_mgr); break;
            case MIN_OP:   min_task (d, isa_opts, eia_mgr); break;
            case MOV_OP:   mov_task (d, isa_opts, eia_mgr, state, pc, inst); break;
            case TST_OP:   tst_task (d, isa_opts, eia_mgr); break;
            case CMP_OP:   cmp_task (d, isa_opts, eia_mgr); break;
            case RCMP_OP:  rcmp_task (d, isa_opts, eia_mgr); break;
            case RSUB_OP:  rsub_task (d, isa_opts, eia_mgr); break;
            case BSET_OP:  bset_task (d, isa_opts, eia_mgr); break;
            case BCLR_OP:  bclr_task (d, isa_opts, eia_mgr); break;
            case BTST_OP:  btst_32_task (d, isa_opts, eia_mgr); break;
            case BXOR_OP:  bxor_task (d, isa_opts, eia_mgr); break;
            case BMSK_OP:  bmsk_task (d, isa_opts, eia_mgr); break;
            case BMSKN_OP: bmskn_task (d, isa_opts, eia_mgr);break;
            case ADD1_OP:  add1_task (d, isa_opts, eia_mgr); break;
            case ADD2_OP:  add2_task (d, isa_opts, eia_mgr); break;
            case ADD3_OP:  add3_task (d, isa_opts, eia_mgr); break;
            case SUB1_OP:  sub1_task (d, isa_opts, eia_mgr); break;
            case SUB2_OP:  sub2_task (d, isa_opts, eia_mgr); break;
            case SUB3_OP:  sub3_task (d, isa_opts, eia_mgr); break;
            case MPY_OP:   mpy32_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::MPY);   break;
            case MPYH_OP:  mpy32_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::MPYH);  break;
            case MPYHU_OP: mpy32_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::MPYHU); break;
            case MPYU_OP:  mpy32_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::MPYU);  break;
            case MPYW_OP:  mpy16_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::MPYW);  break;
            case MPYWU_OP: mpy16_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::MPYWU); break;
            case JCC_D_OP:
            case JCC_OP:   jcc_task (d, isa_opts, eia_mgr, state, pc, inst);  break;
            case JLCC_D_OP:
            case JLCC_OP:  jlcc_task (d, isa_opts, eia_mgr, state, pc, inst); break;
            case BI_OP:
            case BIH_OP:   bi_task (d, isa_opts, eia_mgr, state, pc, inst); break;
            case LDI_OP:   ldi_task (d, isa_opts, eia_mgr, state, pc, inst); break;
            case LPCC_OP:  lpcc_task (d, isa_opts, eia_mgr, state, pc, inst); break;
            case FLAG_OP:  flag_task (d, isa_opts, eia_mgr, state, pc, inst); break;
            case LR_OP:    lr_task (d, isa_opts, eia_mgr, state, pc, inst);   break;
            case SR_OP:    sr_task (d, isa_opts, eia_mgr, state, pc, inst);   break;
            case AEX_OP:   aex_task(d, isa_opts, eia_mgr, state, pc, inst);   break;
              
            case SETEQ_OP: setcc_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SETEQ); break;
            case SETNE_OP: setcc_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SETNE); break;
            case SETLT_OP: setcc_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SETLT); break;
            case SETGE_OP: setcc_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SETGE); break;
            case SETLO_OP: setcc_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SETLO); break;
            case SETHS_OP: setcc_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SETHS); break;
            case SETLE_OP: setcc_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SETLE); break;
            case SETGT_OP: setcc_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SETGT); break;
              
            case SOP_FMT: // All Single or Zero operand 32-bit insns
            {
              switch ( UNSIGNED_BITS(inst,5,0) )
              {
                case ASL_OP:  asl_task (d, isa_opts, eia_mgr);  break;
                case ASR_OP:  asr_task (d, isa_opts, eia_mgr);  break;
                case LSR_OP:  lsr_task (d, isa_opts, eia_mgr);  break;
                case ABS_OP:  abs_task (d, isa_opts, eia_mgr);  break;
                case NOT_OP:  not_task (d, isa_opts, eia_mgr);  break;
                  
                case ROR_OP:
                  switch ( UNSIGNED_BITS(inst,23,22) )
                {
                  case REG_U6IMM_FMT:
                  case REG_REG_FMT: ror_task (d, isa_opts, eia_mgr); break;
                  default: illegal_inst_format = true; break;
                }
                  break;
                  
                case EX_OP:
                  // Decodes EX for ARCv2, or ARC 700.
                  // For ARC 600 this is an illegal opcode
                  //
                  switch ( UNSIGNED_BITS(inst,23,22) ) {
                    case REG_U6IMM_FMT:
                    case REG_REG_FMT:
                      if (isa_opts.is_isa_a600()) { inst_error_task (d, isa_opts, eia_mgr); }
                      else                        { ex_task (d, isa_opts, eia_mgr, state, pc, inst); }
                      break;
                    default: illegal_inst_format = true; break;
                  }
                  break;
                  
                case ROL_OP:
                  // Decodes ROL for ARCv2
                  // For ARC 600 or 700 this is an illegal opcode
                  //
                  switch ( UNSIGNED_BITS(inst,23,22) )
                  {
                    case REG_U6IMM_FMT:
                    case REG_REG_FMT:
                      if (isa_opts.is_isa_a6k())
                        rol_task (d, isa_opts, eia_mgr);
                      else
                        inst_error_task (d, isa_opts, eia_mgr);
                      break;
                    default:
                      illegal_inst_format = true;
                      break;
                  }
                  break;

                case RRC_OP:
                  switch ( UNSIGNED_BITS(inst,23,22) )
                  {
                    case REG_U6IMM_FMT:
                    case REG_REG_FMT:
                      rrc_task (d, isa_opts, eia_mgr);
                      break;
                    default:
                      illegal_inst_format = true;
                      break;
                  }
                  break;
                  
                case SEXB_OP:
                  switch ( UNSIGNED_BITS(inst,23,22) )
                  {
                    case REG_U6IMM_FMT:
                    case REG_REG_FMT:
                      sexb_task (d, isa_opts, eia_mgr);
                      break;
                    default:
                      illegal_inst_format = true;
                      break;
                  }
                  break;
                  
                case SEXW_OP:
                  switch ( UNSIGNED_BITS(inst,23,22) )
                  {
                    case REG_U6IMM_FMT:
                    case REG_REG_FMT:
                      sexw_task (d, isa_opts, eia_mgr);
                      break;
                    default:
                      illegal_inst_format = true;
                      break;
                  }
                  break;
                  
                case EXTB_OP:
                  switch ( UNSIGNED_BITS(inst,23,22) )
                  {
                    case REG_U6IMM_FMT:
                    case REG_REG_FMT:
                      extb_task (d, isa_opts, eia_mgr);
                      break;
                    default:
                      illegal_inst_format = true;
                      break;
                  }
                  break;
                  
                case EXTW_OP:
                  switch ( UNSIGNED_BITS(inst,23,22) )
                  {
                    case REG_U6IMM_FMT:
                    case REG_REG_FMT:
                      extw_task (d, isa_opts, eia_mgr);
                      break;
                    default:
                      illegal_inst_format = true;
                      break;
                  }
                  break;
                  
                case RLC_OP:
                  switch ( UNSIGNED_BITS(inst,23,22) )
                  {
                    case REG_U6IMM_FMT:
                    case REG_REG_FMT:
                      rlc_task (d, isa_opts, eia_mgr);
                      break;
                    default:
                      illegal_inst_format = true;
                      break;
                  }
                  break;
                  
                case LLOCK_OP:
                  switch ( UNSIGNED_BITS(inst,23,22) )
                  {
                    case REG_U6IMM_FMT:
                    case REG_REG_FMT:
                      llock_task (d, isa_opts, eia_mgr, state, pc, inst);
                      break;
                    default:
                      illegal_inst_format = true;
                      break;
                  }
                  break;
                  
                case SCOND_OP:
                  switch ( UNSIGNED_BITS(inst,23,22) )
                  {
                    case REG_U6IMM_FMT:
                    case REG_REG_FMT:
                      scond_task (d, isa_opts, eia_mgr, state, pc, inst);
                      break;
                    default:
                      illegal_inst_format = true;
                      break;
                  }
                  break;
                  
                case ZOP_FMT: // All zero-operand 32-bit insns
                {
                  switch ( ((UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24)) )
                  {
                    case SLEEP_OP:
                    {
                      sleep_task (d, isa_opts, eia_mgr, state, pc, inst);
                      break;
                    }
                    case TRAP0_OP: //  SWI for ARCompact V2 and A600
                    {
                      zero_operand_task (d, isa_opts, eia_mgr, state, pc, inst);
                      if (isa_opts.is_isa_a6k() || isa_opts.is_isa_a600())
                        swi_task (d, isa_opts, eia_mgr, state, pc, inst);
                      else
                        trap_task (d, isa_opts, eia_mgr, state, pc, inst);
                      break;
                    }
                    case SYNC_OP:
                    {
                      zero_operand_task (d, isa_opts, eia_mgr, state, pc, inst);
                      sync_task (d, isa_opts, eia_mgr, state, pc, inst);
                      break;
                    }
                    case RTIE_OP:
                    {
                      zero_operand_task (d, isa_opts, eia_mgr, state, pc, inst);
                      rtie_task (d, isa_opts, eia_mgr, state, pc, inst);
                      break;
                    }
                    case BRK_OP: // 32-bit BRK
                    {
                      zero_operand_task (d, isa_opts, eia_mgr, state, pc, inst);
                      brk_task (d, isa_opts, eia_mgr, state, pc, inst);
                      break;
                    }
                    case SETI_OP:
                    {
                      zero_operand_task(d,isa_opts,eia_mgr,state,pc,inst);
                      seti_task(d, isa_opts, eia_mgr, state, pc, inst);
                      switch ( UNSIGNED_BITS(inst,23,22) )
                      {
                        case REG_U6IMM_FMT:
                          regs_u6_32_task(d, isa_opts, eia_mgr, state, pc, inst);
                          break;
                        case REG_REG_FMT:
                          regs_c_32_task(d, isa_opts, eia_mgr, state, pc, inst);
                          break;
                        default:
                          illegal_inst_format = true;
                          break;
                      }
                      break;
                    }
                    case CLRI_OP:
                    {
                      zero_operand_task(d,isa_opts,eia_mgr,state,pc,inst);
                      clri_task(d, isa_opts, eia_mgr, state, pc, inst);
                      switch ( UNSIGNED_BITS(inst,23,22) )
                      {
                        case REG_U6IMM_FMT:
                          regs_u6_32_task(d, isa_opts, eia_mgr, state, pc, inst);
                          break;
                        case REG_REG_FMT:
                          regs_c_32_task(d, isa_opts, eia_mgr, state, pc, inst);
                          break;
                        default:
                          illegal_inst_format = true;
                          break;
                      }
                      break;
                    }
                    default:
                    {
                      zero_operand_task (d, isa_opts, eia_mgr, state, pc, inst);
                      illegal_inst_format = true;
                      break;
                    }
                  }
                } break;
                  
                default: illegal_inst_format = true; break;
              }
            }
              break;
            default: illegal_inst_format = true; break;
          }
        }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 5
    //
    case GRP_ARC_EXT0_32:   {
      // DECODE OPERAND --------------------------------------------------------
      // Firstly, decode the operand format in the same way as GRP_BASECASE_32,
      // but without the exceptional formats that appear in the basecase group
      //
      switch ( UNSIGNED_BITS(inst,23,22) )
      {
        case REG_REG_FMT: // REG_REG format for major opcode 0x04
          switch ( UNSIGNED_BITS(inst,21,16) ) {
              // first specify all the exceptions
            case SOP_FMT:   regs_sop_bc_32_task (d, isa_opts, eia_mgr, state, pc, inst); break; // CHECK: b is destination
              // secondly, the default operands are extracted
            default:   regs_abc_32_task (d, isa_opts, eia_mgr, state, pc, inst);    // Generic reg-reg
          }
          break;
          
        case REG_U6IMM_FMT: // REG_U6IMM format for major opcode 0x04
          switch ( UNSIGNED_BITS(inst,21,16) ) {
              // first specify all the exceptions
            case SOP_FMT:   regs_sop_bu6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break; // CHECK: b is destination
              // secondly, the default operands are extracted
            default:   regs_abu6_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break; // Generic reg-u6
          }
          break;
          
        case REG_S12IMM_FMT:  // REG_S12IMM format for major opcode 0x04
          switch ( UNSIGNED_BITS(inst,21,16) ) {
              // first specify all the exception
              // (none so far)
              // secondly, the default operands are extracted
            default:    regs_bbs12_32_task (d, isa_opts, eia_mgr, state, pc, inst); break; // Generic reg-s12
          }
          break;
          
        case REG_COND_FMT:  // REG_COND format for major opcode 0x04
          switch ( BITSEL(inst,5) ) {
            case false: regs_c_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break;
            case true:  regs_u6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;
          }
          switch ( UNSIGNED_BITS(inst,21,16) ) {
              // first list all the exceptions
              // (none so far)
              // secondly, the default operands are extracted
            default:    regs_bbq_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break; // Generic reg-cond
              // take care of Section 4.9.9.1
          }
          break;
      }
      // DECODE OPERATOR  ------------------------------------------------------
      // Secondly, decode the operator
      //
      switch ( UNSIGNED_BITS(inst,21,16) )
      {
        case ASLM_OP:   shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASLM); break;
        case LSRM_OP:   shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::LSRM); break;
        case ASRM_OP:   shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASRM); break;
        case RORM_OP:   shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::RORM); break;
          
        case ADDS_OP:   sat_alu_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ADDS);   break;
        case SUBS_OP:   sat_alu_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SUBS);   break;
        case ADDSDW_OP: sat_alu_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ADDSDW); break;
        case SUBSDW_OP: sat_alu_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SUBSDW); break;
        case ASLS_OP:   sat_znv_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASLS);   break;
        case ASRS_OP:   sat_znv_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASRS);   break;
          
        // Some of the Divide/Remainder opcodes encode different operations
        // depending on the selected ISA.
        //
        case DIV_OP: {
          if      (isa_opts.is_isa_a6k())  { div_task  (d, isa_opts, eia_mgr); }
          else if (isa_opts.is_isa_a600()) { mul64_task(d, isa_opts, eia_mgr); }
          else                             { inst_error_task (d, isa_opts, eia_mgr); }
          break;
        }
        case DIVU_OP: {
          if      (isa_opts.is_isa_a6k())  { divu_task   (d, isa_opts, eia_mgr); }
          else if (isa_opts.is_isa_a600()) { mulu64_task (d, isa_opts, eia_mgr); }
          else                             { inst_error_task (d, isa_opts, eia_mgr); }
          break;
        }
        case REM_OP: {
          if (isa_opts.is_isa_a6k()) { rem_task   (d, isa_opts, eia_mgr); }
          else                       { divaw_task (d, isa_opts, eia_mgr); }
          break;
        }
        case REMU_OP: {
          if (isa_opts.is_isa_a6k()) { remu_task       (d, isa_opts, eia_mgr); }
          else                       { inst_error_task (d, isa_opts, eia_mgr); }
          break;
        }
          
        case SOP_FMT: // All Single or Zero operand 32-bit insns
          switch ( UNSIGNED_BITS(inst,5,0) ) {
            case SWAP_OP:  swap_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SWAP);      break;
              
            case SWAPE_OP: {
              if      (isa_opts.is_isa_a6k())  { swap_a6k_task   (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SWAPE); }
              else if (isa_opts.is_isa_a700()) { swap_task       (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SWAPE); }
              else                             { inst_error_task (d, isa_opts, eia_mgr);                                 }
              break;
            }
            case LSL16_OP: swap_a6k_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::LSL16); break;
            case LSR16_OP: swap_a6k_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::LSR16); break;
              
            case ASR16_OP: shas_a6k_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASR16); break;
            case ASR8_OP:  shas_a6k_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASR8);  break;
            case LSR8_OP:  shas_a6k_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::LSR8);  break;
            case LSL8_OP:  shas_a6k_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::LSL8);  break;
            case ROL8_OP:  shas_a6k_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ROL8);  break;
            case ROR8_OP:  shas_a6k_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ROR8);  break;
              
            case NORM_OP:  norm_task     (d, isa_opts, eia_mgr, state, pc, inst, OpCode::NORM);  break;
            case NORMW_OP: norm_task     (d, isa_opts, eia_mgr, state, pc, inst, OpCode::NORMW); break;
              
            case FFS_OP:   norm_a6k_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::FFS);   break;
            case FLS_OP:   norm_a6k_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::FLS);   break;
              
            case ABSSW_OP: sat_znv_task  (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ABSSW); break;
            case ABSS_OP:  sat_znv_task  (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ABSS);  break;
            case NEGS_OP:  sat_znv_task  (d, isa_opts, eia_mgr, state, pc, inst, OpCode::NEGS);  break;
            case NEGSW_OP: sat_znv_task  (d, isa_opts, eia_mgr, state, pc, inst, OpCode::NEGSW); break;
              
            case SAT16_OP: sat_znv_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::SAT16);  break;
            case RND16_OP: sat_znv_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::RND16);  break;
              
            case ZOP_FMT: {
              // All zero-operand 32-bit EXT0 fmts
              zero_operand_task (d, isa_opts, eia_mgr, state, pc, inst);
              switch ( ((UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24)) )
              { // No instructions defined in this fmt
                default: inst_error_task (d, isa_opts, eia_mgr); break;
              }
            }
            default: inst_error_task (d, isa_opts, eia_mgr); break;
              
          }
          break;
        default: inst_error_task (d, isa_opts, eia_mgr); break;
      }
      
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 6
    //
    case GRP_ARC_EXT1_32:   {
      // If modeling ARCv2, decode this group only if fpx_option is enabled
      // This is to ensure that d.size does not reflect the presence of LIMM
      // when decoding absent fpx instructions that use a LIMM. If placed in 
      // the delay slot of a branch, the blink value is incorrectly set just
      // before the illegal instruction sequence exception is taken.
      // Hardware will not decode the operands if the extension group is empty,
      // so this decoder must do the same.
      //
      if (isa_opts.fpx_option || !isa_opts.is_isa_a6k()) {
        ext32_operands_task   (d, isa_opts, eia_mgr, state, pc, inst); // always wire operands
        minor6_operators_task (d, isa_opts, eia_mgr, state, pc, inst); // check for fpx_option occurs inside 'minor6_operators_task'
      } else
        inst_error_task (d, isa_opts, eia_mgr);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 7 (EIA)
    //
    case GRP_USR_EXT2_32:   {
      
      // If an EIA instruction is present and decoded, this boolean variable will become false
      //
      bool   is_instr_error  = true;

      // Fast check if there are any EIA instructions enabled for this MAJOR opcode
      //
      if (eia_mgr.are_eia_instructions_defined
          && eia_mgr.eia_major_opcode_enabled_bitset[GRP_USR_EXT2_32]) {

        // 1. DECODE OPERAND ---------------------------------------------------
        // Firstly, decode the operand format in the same way as GRP_BASECASE_32,
        // but without the exceptional formats that appear in the basecase group
        //
        switch ( UNSIGNED_BITS(inst,23,22) )
        {
          case REG_REG_FMT: // REG_REG format for major opcode 0x04
            switch ( UNSIGNED_BITS(inst,21,16) ) {
                // first specify all the exceptions
              case SOP_FMT:   regs_sop_bc_32_task (d, isa_opts, eia_mgr, state, pc, inst); break; // CHECK: b is destination
                // secondly, the default operands are extracted
              default:   regs_abc_32_task (d, isa_opts, eia_mgr, state, pc, inst);    // Generic reg-reg
            }
            break;
            
          case REG_U6IMM_FMT: // REG_U6IMM format for major opcode 0x04
            switch ( UNSIGNED_BITS(inst,21,16) ) {
                // first specify all the exceptions
              case SOP_FMT:   regs_sop_bu6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break; // CHECK: b is destination
                // secondly, the default operands are extracted
              default:   regs_abu6_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break; // Generic reg-u6
            }
            break;
            
          case REG_S12IMM_FMT:  // REG_S12IMM format for major opcode 0x04
            switch ( UNSIGNED_BITS(inst,21,16) ) {
                // first specify all the exception
                // (none so far)
                // secondly, the default operands are extracted
              default:    regs_bbs12_32_task (d, isa_opts, eia_mgr, state, pc, inst); break; // Generic reg-s12
            }
            break;
            
          case REG_COND_FMT:  // REG_COND format for major opcode 0x04
            switch ( BITSEL(inst,5) ) {
              case false: regs_c_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break;
              case true:  regs_u6_32_task (d, isa_opts, eia_mgr, state, pc, inst); break;
            }
            switch ( UNSIGNED_BITS(inst,21,16) ) {
                // first list all the exceptions
                // (none so far)
                // secondly, the default operands are extracted
              default:    regs_bbq_32_task (d, isa_opts, eia_mgr, state, pc, inst);  break; // Generic reg-cond
                // take care of Section 4.9.9.1
            }
            break;
        }

        // 2. DECODE OPERATOR  ------------------------------------------------------
        // Secondly, decode the operator
        //
        uint32 opcode_major    = GRP_USR_EXT2_32;
        uint32 dop_opcode      = UNSIGNED_BITS(inst,21,16);
        uint32 key             = ( ( (opcode_major & 0x1F) << 6) | (dop_opcode & 0x3F) );
        
        // Define iterators for convenient EIA instruction lookup
        //
        std::map<uint32,ise::eia::EiaInstructionInterface*>::const_iterator   I;
        std::map<uint32,ise::eia::EiaInstructionInterface*>::const_iterator   E = eia_mgr.opcode_eia_instruction_map.end();
        
        // Is this a DOP instruction?
        //
        if (dop_opcode != SOP_FMT
            && ((I = eia_mgr.opcode_eia_instruction_map.find(key)) != E))
        { // Found DOP EIA instruction
          //
          is_instr_error = false;
          
          // Register pointer to EIA instruction and note required XPU permission
          //
          d.eia_inst = I->second;
          REQUIRE_EIA_EXTENSION(I->second->get_id())

          d.kind = arcsim::isa::arc::Dcode::kExtension;
          
          if (d.eia_inst->is_flag_setting())  { d.code = OpCode::EIA_DOP_F; }
          else                                { d.code = OpCode::EIA_DOP;   }
        } 
        // Is this a SOP/ZOP instruction?
        //
        else if (dop_opcode == SOP_FMT)
        {     
          uint32 sop_opcode   = UNSIGNED_BITS(inst,21,16);
          key                 = ( ( (opcode_major & 0x1F) << 6) | (sop_opcode & 0x3F) );
          
          // Is this a SOP instruction?
          //
          if (sop_opcode != ZOP_FMT
              && ((I = eia_mgr.opcode_eia_instruction_map.find(key)) != E))
          { // FOUND SOP EIA instruction
            //
            is_instr_error = false;
            
            // Register pointer to EIA instruction and note required XPU permission
            //
            d.eia_inst = I->second;
            REQUIRE_EIA_EXTENSION(I->second->get_id())
            
            d.kind = arcsim::isa::arc::Dcode::kExtension;
            
            if (d.eia_inst->is_flag_setting())  { d.code = OpCode::EIA_SOP_F; }
            else                                { d.code = OpCode::EIA_SOP;   }            
          }
          // Is this a ZOP instruction?
          //
          else if (sop_opcode == ZOP_FMT) {
            uint32 zop_opcode   = ((UNSIGNED_BITS(inst,14,12)<<3) | UNSIGNED_BITS(inst,26,24));
            key                 = ( ( (opcode_major & 0x1F) << 6) | (zop_opcode & 0x3F) );
            
            if (((I = eia_mgr.opcode_eia_instruction_map.find(key)) != E))
            { // FOUND ZOP EIA instruction
              //
              is_instr_error = false;
              
              zero_operand_task (d, isa_opts, eia_mgr, state, pc, inst); 
                           
              // Register pointer to EIA instruction and note required XPU permission
              //
              d.eia_inst = I->second;
              REQUIRE_EIA_EXTENSION(I->second->get_id())
              
              d.kind = arcsim::isa::arc::Dcode::kExtension;
              
              if (d.eia_inst->is_flag_setting())  { d.code = OpCode::EIA_ZOP_F; }
              else                                { d.code = OpCode::EIA_ZOP;   }
            }            
          }          
        }         
      }
      if (is_instr_error) {          
        inst_error_task (d, isa_opts, eia_mgr);
      }
      
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 8
    //
    case GRP_ARC_EXT0_16:   {
      size = 2;
      group8_task (d, isa_opts, eia_mgr, state, pc, inst);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 9
    //
    case GRP_ARC_EXT1_16:   {
      size = 2;
      group9_task (d, isa_opts, eia_mgr, state, pc, inst);
      break;
    } 
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 10
    //
    case GRP_USR_EXT0_16:   {
      size = 2;
      group10_task (d, isa_opts, eia_mgr, state, pc, inst);
      break;
    } 
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 11
    // FIXME: EIA
    case GRP_USR_EXT1_16:   {
      size = 2;
      if (BITSEL(inst,26) == 0)
        jli_s_task (d, isa_opts, eia_mgr, state, pc, inst);
      else
        ei_s_task (d, isa_opts, eia_mgr, state, pc, inst);
      break;
    }  
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 12
    //
    case GRP_LD_ADD_RR_16:  {
      size = 2;
      if ( UNSIGNED_BITS(inst,20,19) == 3 )
        regs_a_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      else
        regs_a1_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      
      regs_bc_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      switch ( UNSIGNED_BITS(inst,20,19) ) {
        case 0: ld_s_rr_task (d, isa_opts, eia_mgr, state, pc, inst);  break;
        case 1: ldb_s_rr_task (d, isa_opts, eia_mgr, state, pc, inst); break;
        case 2: ldw_s_rr_task (d, isa_opts, eia_mgr, state, pc, inst); break;
        case 3: add_16_task (d, isa_opts, eia_mgr);                  break;
      }
      break;
    } 
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 13
    //
    case GRP_ADD_SUB_SH_16: {
      size = 2;
      regs_cbu3_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      switch ( UNSIGNED_BITS(inst,20,19) )
      {
        case 0: add_16_task (d, isa_opts, eia_mgr); break;
        case 1: sub_16_task (d, isa_opts, eia_mgr); break;
        case 2: shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASLM); break;
        case 3: shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASRM); break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 14
    //
    case GRP_MV_CMP_ADD_16: {
      size = 2;
      int subop = (isa_opts.is_isa_a6k() && isa_opts.new_fmt_14)
                    ? ((BITSEL(inst,18) << 2) | UNSIGNED_BITS(inst,20,19))
                    : UNSIGNED_BITS(inst,20,19);
      
      switch (subop)
      {
        case 0: 
          regs_bbh_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          add_16_task (d, isa_opts, eia_mgr);
          break;
          
        case 1: 
          regs_mov_bh_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          mov_s_task (d, isa_opts, eia_mgr);
          break;
          
        case 2: 
          regs_bh_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          cmp_task (d, isa_opts, eia_mgr);
          break;
          
        case 3: 
          regs_mov_hb_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          mov_s_task (d, isa_opts, eia_mgr);
          break;
          
        case 4: // ARCompact v2 only
          regs_hhs3_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          add_16_task (d, isa_opts, eia_mgr);
          break;
          
        case 5: // ARCompact v2 only
          regs_mov_hs3_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          mov_s_task (d, isa_opts, eia_mgr);
          break;
          
        case 6: // ARCompact v2 only
          regs_hs3_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          cmp_task (d, isa_opts, eia_mgr);
          break;
          
        case 7: // ARCompact v2 only
          regs_mov_bh_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          mov_s_ne_task (d, isa_opts, eia_mgr);
          break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 15
    //
    case GRP_GEN_OPS_16:    {
      size = 2;
      
      // First decode the operands
      switch ( UNSIGNED_BITS(inst,20,16) )
      {
        case 0x0: // SOPs or ZOPS
        {
          switch ( UNSIGNED_BITS(inst,23,21) )
          {
            case 7:  zero_operand_task (d, isa_opts, eia_mgr, state, pc, inst);     // ZOPs
            default: regs_b_16_task (d, isa_opts, eia_mgr, state, pc, inst); break; // SOPs
          }
        } break;
        case 0x0b: // tst_s must not write to dest
        {
          regs_bc_16_task(d, isa_opts, eia_mgr, state, pc, inst);
          break;
        }
        case 0x1e: // trap_s must not read or write registers
          break;
        default: regs_bbc_16_task (d, isa_opts, eia_mgr, state, pc, inst); // not SOPs or ZOPs
      }
      
      // Second decode the operators
      switch ( UNSIGNED_BITS(inst,20,16) )
      {
        case 0x0: // SOPs
          switch ( UNSIGNED_BITS(inst,23,21) ) {
            case 0:
            case 1: j_s_task (d, isa_opts, eia_mgr, state, pc, inst); break;       // j_s [b] or j_s.d [b]
            case 2:
            case 3: jl_s_task (d, isa_opts, eia_mgr, state, pc, inst); break;      // jl_s [b] or jl_s.d [b]
            case 6: sub_s_ne_task (d, isa_opts, eia_mgr, state, pc, inst); break;  // sub_s.ne
            case 7: // ZOPs
              switch ( UNSIGNED_BITS(inst,26,24) ) {
                case 0: nop_task (d, isa_opts, eia_mgr, state, pc, inst);   break;   // nop_s
                case 2: // ARCompact V2 swi_s instruction
                  { 
                    if (isa_opts.is_isa_a6k()) 
                      swi_task (d, isa_opts, eia_mgr, state, pc, inst); 
                    else 
                      inst_error_task (d, isa_opts, eia_mgr); 
                    break;
                  }
                case 4: jeq_s_task (d, isa_opts, eia_mgr, state, pc, inst); break;   // jeq_s [blink]
                case 5: jne_s_task (d, isa_opts, eia_mgr, state, pc, inst); break;   // jne_s [blink]
                case 6:
                case 7: j_blink_task (d, isa_opts, eia_mgr, state, pc, inst);     break; // j_s [blink] or j_s.d [blink]
                default: inst_error_task (d, isa_opts, eia_mgr); break;
              }
              break;
            default: inst_error_task (d, isa_opts, eia_mgr); break;
          }
          break;
        case 0x02: sub_task (d, isa_opts, eia_mgr);  break;
        case 0x04: and_task (d, isa_opts, eia_mgr);  break;
        case 0x05: or_task (d, isa_opts, eia_mgr);   break;
        case 0x06: bic_task (d, isa_opts, eia_mgr);  break;
        case 0x07: xor_task (d, isa_opts, eia_mgr);  break;
        case 0x09: mpy16_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::MPYW);  break;
        case 0x0a: mpy16_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::MPYWU); break;
        case 0x0b: tst_task (d, isa_opts, eia_mgr);  break;
        case 0x0c: 
          {
            if (isa_opts.is_isa_a6k()) {
              mpy32_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::MPY);
            } else if (isa_opts.is_isa_a600()) {
              mul64_task(d, isa_opts, eia_mgr);
            } else {
              illegal_inst_format = true;
            }
            break;
          }
        case 0x0d: sexb_task (d, isa_opts, eia_mgr); break;
        case 0x0e: sexw_task (d, isa_opts, eia_mgr); break;
        case 0x0f: extb_task (d, isa_opts, eia_mgr); break;
        case 0x10: extw_task (d, isa_opts, eia_mgr); break;
        case 0x11: abs_task (d, isa_opts, eia_mgr);  break;
        case 0x12: not_task (d, isa_opts, eia_mgr);  break;
        case 0x13: neg_task (d, isa_opts, eia_mgr);  break;
        case 0x14: add1_task (d, isa_opts, eia_mgr); break;
        case 0x15: add2_task (d, isa_opts, eia_mgr); break;
        case 0x16: add3_task (d, isa_opts, eia_mgr); break;
        case 0x18: shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASLM); break;
        case 0x19: shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::LSRM); break;
        case 0x1a: shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASRM); break;
        case 0x1b: asl_task (d, isa_opts, eia_mgr);  break;
        case 0x1c: asr_task (d, isa_opts, eia_mgr);  break;
        case 0x1d: lsr_task (d, isa_opts, eia_mgr);  break;
        case 0x1e:     // TRAP_S uses nothing more than shimm
        {
          zero_operand_task (d, isa_opts, eia_mgr, state, pc, inst);
          trap_s_task (d, isa_opts, eia_mgr, state, pc, inst); break;
        }
        case 0x1f:     // BRK_S has no operand either
        {
          zero_operand_task (d, isa_opts, eia_mgr, state, pc, inst);
          brk_task (d, isa_opts, eia_mgr, state, pc, inst);  break;
        }
        default: inst_error_task (d, isa_opts, eia_mgr); break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 16
    //
    case GRP_LD_WORD_16:    {
      size = 2;
      load_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      d.code = OpCode::LW_SH2;
      d.kind = arcsim::isa::arc::Dcode::kMemLoad;
      addr_shift = 2;
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 17
    //
    case GRP_LD_BYTE_16:    {
      size = 2;
      load_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      d.code = OpCode::LD_BYTE_U;
      d.kind = arcsim::isa::arc::Dcode::kMemLoad;
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 18
    //
    case GRP_LD_HALF_16:    {
      size = 2;
      load_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      d.code = OpCode::LD_HALF_U;
      d.kind = arcsim::isa::arc::Dcode::kMemLoad;
      addr_shift = 1;
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 19
    //
    case GRP_LD_HALFX_16:   {
      size = 2;
      load_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      d.code = OpCode::LD_HALF_S;
      d.kind = arcsim::isa::arc::Dcode::kMemLoad;
      addr_shift = 1;
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 20
    //
    case GRP_ST_WORD_16:    {
      size = 2;
      store_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      d.code = OpCode::ST_WORD;
      d.kind = arcsim::isa::arc::Dcode::kMemStore;
      addr_shift = 2;
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 21
    //
    case GRP_ST_BYTE_16:    {
      size = 2;
      store_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      d.code = OpCode::ST_BYTE;
      d.kind = arcsim::isa::arc::Dcode::kMemStore;
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 22
    //
    case GRP_ST_HALF_16:    {
      size = 2;
      store_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      d.code = OpCode::ST_HALF;
      d.kind = arcsim::isa::arc::Dcode::kMemStore;
      addr_shift = 1;
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 23
    //
    case GRP_SH_SUB_BIT_16: {
      size = 2;
      if (UNSIGNED_BITS(inst,23,21) == 7) // No write-to-dest for bit-test
        regs_bu5_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      else
        regs_bbu5_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      switch ( UNSIGNED_BITS(inst,23,21) ) {
          
        case 0: shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASLM); break;
        case 1: shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::LSRM); break;
        case 2: shift_task (d, isa_opts, eia_mgr, state, pc, inst, OpCode::ASRM); break;
        case 3: sub_task (d, isa_opts, eia_mgr);  break;    // subtract
        case 4: bset_task (d, isa_opts, eia_mgr); break;    // bit-set
        case 5: bclr_task (d, isa_opts, eia_mgr); break;    // bit-cler
        case 6: bmsk_task (d, isa_opts, eia_mgr); break;    // bit-mask
        case 7: btst_16_task (d, isa_opts, eia_mgr); break; // bit-test
        default:inst_error_task (d, isa_opts, eia_mgr); break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 24
    //
    case GRP_SP_MEM_16:     {
      size = 2;
      switch ( UNSIGNED_BITS(inst,23,21) ) {
        case 0: // ld_s  b,[sp,u7]
          mem_sp_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          regs_b_ld_dest_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          d.code = OpCode::LW_SH2;
          d.kind = arcsim::isa::arc::Dcode::kMemLoad;
          addr_shift = 2;
          disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
          disallow_lp_count_destination (d, isa_opts, eia_mgr, state, pc, inst);
          disallow_upper32_destination (d, isa_opts, eia_mgr, state, pc, inst);
          break;
          
        case 1: // ldb_s b,[sp,u7]
          mem_sp_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          regs_b_ld_dest_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          addr_shift = 2;      /* THIS COULD BE A PRM TYPO!!! */
          d.code = OpCode::LD_BYTE_U;
          d.kind = arcsim::isa::arc::Dcode::kMemLoad;
          disallow_link_reg_destination (d, isa_opts, eia_mgr, state, pc, inst);
          disallow_lp_count_destination (d, isa_opts, eia_mgr, state, pc, inst);
          disallow_upper32_destination (d, isa_opts, eia_mgr, state, pc, inst);
          break;
          
        case 2: // st_s  b,[sp,u7]
          mem_sp_16_task (d, isa_opts, eia_mgr, state, pc, inst);      // Order of this line and
          regs_b_src2_16_task (d, isa_opts, eia_mgr, state, pc, inst); // this one is important.
          d.code = OpCode::ST_WORD;
          d.kind = arcsim::isa::arc::Dcode::kMemStore;
          addr_shift = 2;
          break;
          
        case 3: // stb_s b,[sp,u7]
          mem_sp_16_task (d, isa_opts, eia_mgr, state, pc, inst);      // Order of this line and
          regs_b_src2_16_task (d, isa_opts, eia_mgr, state, pc, inst); // this one is important.
          addr_shift = 2;  /* THIS COULD BE A PRM TYPO!!! */
          d.code = OpCode::ST_BYTE;
          d.kind = arcsim::isa::arc::Dcode::kMemStore;
          break;
          
        case 4: // add_s b,sp,u7
          READ_0 (d, isa_opts, eia_mgr, state, SP_REG);
          SHIMM (UNSIGNED_BITS(inst,20,16) << 2)
          regs_b_dest_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          add_16_task (d, isa_opts, eia_mgr);
          break;
          
        case 5: // [add_s | sub_s] sp, sp, u7
          arith_sp_sp_task (d, isa_opts, eia_mgr, state, pc, inst);
          if (BITSEL(inst,24) == 0)
            add_16_task (d, isa_opts, eia_mgr); // add_s sp, sp, u7
          else
            sub_16_task (d, isa_opts, eia_mgr); // sub_s sp, sp, u7
          break;
          
        case 6:
          if ( BITSEL(inst,16) )
            pop_16_task (d, isa_opts, eia_mgr, state, pc, inst); // pop_s b or pop_s blink
          else
            leave_s_task (d, isa_opts, eia_mgr, state, pc, inst); // leave_s task
          break;
          
        case 7:
          if ( BITSEL(inst,16) )
            push_16_task (d, isa_opts, eia_mgr, state, pc, inst); // push_s b or push_s blink
          else
            enter_s_task (d, isa_opts, eia_mgr, state, pc, inst); // enter_s task
          break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 25
    //
    case GRP_GP_MEM_16:     {
      size = 2;
      switch ( UNSIGNED_BITS(inst,26,25) ) {
        case 0: // ld_s  r0,[gp,s11]
          d.code = OpCode::LW_SH2;
          d.kind = arcsim::isa::arc::Dcode::kMemLoad;
          r0_gp_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          addr_shift = 2;
          break;
          
        case 1: // ldb_s r0,[gp,s9]
          d.code = OpCode::LD_BYTE_U;
          d.kind = arcsim::isa::arc::Dcode::kMemLoad;
          r0_gp_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          break;
          
        case 2: // ldw_s r0,[gp,s10]
          d.code = OpCode::LD_HALF_U;
          d.kind = arcsim::isa::arc::Dcode::kMemLoad;
          r0_gp_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          addr_shift = 1;
          break;
          
        case 3: // add_s r0,gp,s11
          d.code = OpCode::ADD;
          d.kind = arcsim::isa::arc::Dcode::kArithmetic;
          WRITE_0 (d, isa_opts, eia_mgr, state, 0);
          READ_0 (d, isa_opts, eia_mgr, state, GP_REG);
          SHIMM ((SIGNED_BITS(inst,24,24)<<11) | (UNSIGNED_BITS(inst,24,16)<<2))
          break;
      }
      break;
    }  
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 26
    //
    case GRP_LD_PCL_16:     {
      size = 2;
      load_pcl_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      break;
    } 
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 27
    //
    case GRP_MV_IMM_16:     {
      size = 2;
      mv_imm_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 28
    //
    case GRP_ADD_IMM_16:    {
      size = 2;
      switch ( BITSEL(inst,23) ) {
        case false:
          regs_bbu7_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          add_16_task (d, isa_opts, eia_mgr);
          break;
        case true:
          regs_bu7_16_task (d, isa_opts, eia_mgr, state, pc, inst);
          cmp_task (d, isa_opts, eia_mgr);
          break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 29
    //
    case GRP_BRCC_S_16:     {
      size = 2;
      regs_b_16_task (d, isa_opts, eia_mgr, state, pc, inst);
      SHIMM (0);
      switch ( BITSEL(inst,23) ) {
        case false: brcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, BREQ_COND); break;
        case true:  brcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, BRNE_COND); break;
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 30
    //
    case GRP_BCC_S_16:      {
      size = 2;
      switch ( UNSIGNED_BITS(inst,26,25) ) {
        case 0: bcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, SIGNED_BITS(inst,24,16), 0); break; // b_s
        case 1: bcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, SIGNED_BITS(inst,24,16), 1); break; // beq_s
        case 2: bcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, SIGNED_BITS(inst,24,16), 2); break; // bne_s
        case 3: switch ( UNSIGNED_BITS(inst,24,22) ) {
          case 0: bcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, SIGNED_BITS(inst,21,16),  9); break; // bgt_s
          case 1: bcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, SIGNED_BITS(inst,21,16), 10); break; // bge_s
          case 2: bcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, SIGNED_BITS(inst,21,16), 11); break; // blt_s
          case 3: bcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, SIGNED_BITS(inst,21,16), 12); break; // ble_s
          case 4: bcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, SIGNED_BITS(inst,21,16), 13); break; // bhi_s
          case 5: bcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, SIGNED_BITS(inst,21,16),  6); break; // bhs_s
          case 6: bcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, SIGNED_BITS(inst,21,16),  5); break; // blo_s
          case 7: bcc_s_task (d, isa_opts, eia_mgr, state, pc, inst, SIGNED_BITS(inst,21,16), 14); break; // bls_s
        }
      }
      break;
    }
    // -------------------------------------------------------------------------
    // MAJOR OPCODE GROUP 31
    //
    case GRP_BL_S_16:       {
      size = 2;
      bl_s_ucond_task (d, isa_opts, eia_mgr, state, pc, inst);
      break;
    }
  } /* END: switch ( UNSIGNED_BITS(inst,31,27) )  */
  
  // Finalise any post-decode signals
  //
  finalise_task (d, isa_opts, eia_mgr, state, pc, inst);
}

} } }  // namespace arcsim::isa::arc


