//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//            Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
//
// =====================================================================
//
// Description:
//
// This file implements the management of Actionpoints. It provides
// methods to be called when an Actionpoint auxiliary register is
// written, as this may require other side effects to be modelled.
//
// There are four types of Actionpoint; instruction, load-store,
// auxiliary register, and external. For each of them, an Actionpoint
// may be matched against either the address or data values involved
// those activities.
//
// Instruction-based Actionpoints are detected when an instruction is
// decoded, and is retained in the Dcode object for that instruction.
// For this reason, the Dcode cache must be flushed whenever the
// Actionpoints registers are written.
//
// Load-store-based Actionpoints are detected when a memory operation
// takes place. Whenever an address-sensitive Actionpoint is programmed,
// the PageCache structures are flushed. Hence, all subsequent memory
// operations will go to one of the three Processor::*_page_miss methods
// to find their memory page. These methods will check whether the
// address is in range for an address-sensitive Actionpoint. If not,
// the page will be cached as usual. If it is, then the page will never
// be cached, but will always be checked when the page is accessed
// during simulation.
// Data-sensitive memory Actionpoints do not affect the PageCache
// structures, but instead require an explicit check when any data
// is read or written (according to the Actionpoint mode). Hence, the
// interpreter maintains a boolean flag to indicate whether data
// checks are needed on read data, and a separate flag for write data.
// When translating load/store operations, checks are compiled into
// the load/store code as required by the current setting of the
// Actionpoint control registers. When Actionpoint control registers
// are written, all existing translations are discarded.
//
// Auxiliary register-based Actionpoints are detected in the read/write
// methods for Auxiliary registers.
//
// Externally-triggered Actionpoints are implemented via the external
// API. This provides methods for setting the two external parameters
// to a given value. A side-effect of setting these parameters may be
// the triggering of an Actionpoint.
//
// When Actionpoints are grouped in pairs or quads, the existence of
// a single match will not be sufficient to trigger an Actionpoint.
// During instruction execution, the current set of matching Actionpoints
// is maintained in a field within the processor state structure.
// If sufficient Actionpoints match, then the ASR[7:0] field of the
// processor's DEBUG aux register will be updated to indicate the
// Actionpoints that matched, and the Actionpoint behaviour will be
// triggered (raising an exception or forcing a breakpoint).
// After an Actionpoint has triggered, the matching Actionpoints have
// their Match Value registers updated with the matching value.
// Several different values may be possible, depending on the setting
// of the corresponding Match Mask register.
//
// If any Actionpoint matches during simulation of a given instruction,
// the state.pending_actions flag will be set. If executing in a JIT
// translated block, control will be returned immediately. If executing
// in interpretive mode, the simulation loop will immediately check
// the status of Actionpoints when the pending_actions flag is
// detected. The matching of an Actionpoint is therefore treated as
// a speculative interrupt, which may possibly lead to the Actionpoints
// mechanism being triggered.
//
// =====================================================================

#include <iomanip>

#include "sys/aps/Actionpoints.h"
#include "sys/cpu/processor.h"
#include "sys/cpu/aux-registers.h"

#include "util/Log.h"

namespace arcsim {
  namespace sys {
    namespace cpu {

      // =====================================================================
      // MACROS
      // =====================================================================

      // Macros to position the read and write bits in the TT field
      //
      #define AC_TT_READ            (TT_READ<<4)
      #define AC_TT_WRITE           (TT_WRITE<<4)

      // Macros to extract bit-fields from Actionpoint Control registers.
      //
      #define AP_CTRL_AT(_ac_)      ((_ac_) & 0xf)
      #define AP_CTRL_TT(_ac_)      (((_ac_) >> 4) & 0x3)
      #define AP_CTRL_M(_ac_)       (((_ac_) >> 6) & 0x1)
      #define AP_CTRL_P(_ac_)       (((_ac_) >> 7) & 0x1)
      #define AP_CTRL_AA(_ac_)      (((_ac_) >> 8) & 0x1)
      #define AP_CTRL_Q(_ac_)       (((_ac_) >> 9) & 0x1)
      #define AP_MATCH_CTRL(_ac_)   ((_ac_) & 0x7f)
      #define AP_MATCH_TYPE(_ac_)   ((_ac_) & 0x3f)
      #define AP_MATCH_READ(_ac_)   ((_ac_) & 0x2f)
      
      #define ACTIVE_PC_AP          (AC_TT_READ |AP_INST_ADDR)
      #define ACTIVE_IR_AP          (AC_TT_READ |AP_INST_DATA)

      #define IN_RANGE_LD_ADDR      ((M_WITHIN_RANGE<<6) |AC_TT_READ |AP_LD_ST_ADDR)
      #define OUT_RANGE_LD_ADDR     ((M_OUTWITH_RANGE<<6)|AC_TT_READ |AP_LD_ST_ADDR)

      #define IN_RANGE_ST_ADDR      ((M_WITHIN_RANGE<<6) |AC_TT_WRITE|AP_LD_ST_ADDR)
      #define OUT_RANGE_ST_ADDR     ((M_OUTWITH_RANGE<<6)|AC_TT_WRITE|AP_LD_ST_ADDR)

      #define IN_RANGE_LD_DATA      ((M_WITHIN_RANGE<<6) |AC_TT_READ |AP_LD_ST_DATA)
      #define OUT_RANGE_LD_DATA     ((M_OUTWITH_RANGE<<6)|AC_TT_READ |AP_LD_ST_DATA)

      #define IN_RANGE_ST_DATA      ((M_WITHIN_RANGE<<6) |AC_TT_WRITE|AP_LD_ST_DATA)
      #define OUT_RANGE_ST_DATA     ((M_OUTWITH_RANGE<<6)|AC_TT_WRITE|AP_LD_ST_DATA)

      #define IN_RANGE_ST_MASK      ((M_WITHIN_RANGE<<6) |AC_TT_WRITE|AP_LD_ST_ADDR|AP_LD_ST_DATA)
      #define OUT_RANGE_ST_MASK     ((M_OUTWITH_RANGE<<6)|AC_TT_WRITE|AP_LD_ST_ADDR|AP_LD_ST_DATA)
      #define ANY_RANGE_ST_MASK     ((1<<6)              |AC_TT_WRITE|0xf)

      #define IN_RANGE_LD_MASK      ((M_WITHIN_RANGE<<6) |AC_TT_READ |AP_LD_ST_ADDR)
      #define OUT_RANGE_LD_MASK     ((M_OUTWITH_RANGE<<6)|AC_TT_READ |AP_LD_ST_ADDR)
      #define ANY_RANGE_LD_MASK     ((1<<6)              |AC_TT_READ |0xf)

      #define IN_RANGE_LR_ADDR      ((M_WITHIN_RANGE<<6) |AC_TT_READ |AP_AUX_ADDR)
      #define OUT_RANGE_LR_ADDR     ((M_OUTWITH_RANGE<<6)|AC_TT_READ |AP_AUX_ADDR)
      #define ANY_RANGE_LR_ADDR     ((1<<6)              |AC_TT_READ |0xf)

      #define IN_RANGE_SR_ADDR      ((M_WITHIN_RANGE<<6) |AC_TT_WRITE|AP_AUX_ADDR)
      #define OUT_RANGE_SR_ADDR     ((M_OUTWITH_RANGE<<6)|AC_TT_WRITE|AP_AUX_ADDR)
      #define ANY_RANGE_SR_ADDR     ((1<<6)              |AC_TT_WRITE|0xf)

      #define IN_RANGE_LR_DATA      ((M_WITHIN_RANGE<<6) |AC_TT_READ |AP_AUX_DATA)
      #define OUT_RANGE_LR_DATA     ((M_OUTWITH_RANGE<<6)|AC_TT_READ |AP_AUX_DATA)
      #define ANY_RANGE_LR_DATA     ((1<<6)              |AC_TT_READ |0xf)

      #define IN_RANGE_SR_DATA      ((M_WITHIN_RANGE<<6) |AC_TT_WRITE|AP_AUX_DATA)
      #define OUT_RANGE_SR_DATA     ((M_OUTWITH_RANGE<<6)|AC_TT_WRITE|AP_AUX_DATA)
      #define ANY_RANGE_SR_DATA     ((1<<6)              |AC_TT_WRITE|0xf)

      #define ANY_RANGE_MASK        (0x7fUL)

      // Macros to decode functionality of Actionpoint Type (AT) fields
      //
      #define AP_IS_PC(_at_)        (((_at_) & 0xe) == AP_INST_ADDR)
      #define AP_IS_IR(_at_)        (((_at_) & 0xe) == AP_INST_DATA)
      #define AP_IS_LD_ST(_at_)     (((_at_) & 0xe) == AP_LD_ST_ADDR)
      #define AP_IS_AUX(_at_)       (((_at_) & 0xe) == AP_AUX_ADDR)
      #define AP_IS_EXT(_at_)       (((_at_) & 0xe) == AP_EXT_P0)
      #define AP_IS_RESERVED(_at_)  (((_at_) & 0xe)  > AP_EXT_P1)
      
      // =====================================================================
      // METHODS
      // =====================================================================

      // -----------------------------------------------------------------------
      // Configure the Actionpoint object. Must be called during init.
      //
      void
      Actionpoints::configure(Processor *parent, uint32 num, bool full)
      {
        LOG(LOG_DEBUG1) << "[CPU" 
                        << parent->core_id 
                        << "] number of Actionpoints = " << num 
                        << ", Feature level = " << full;
        
        cpu              = parent;
        num_aps          = num < MAX_ACTIONPOINTS ? num : MAX_ACTIONPOINTS;
        num_mask         = (num_aps < 32) ? ((1 << num_aps) - 1) : 0xffffffffUL;
        full_aps         = full;
        inst_matches     = 0;
        matching_address = 0;
        matching_data    = 0;

        for (int i = 0; i < NUM_AP_AUX_REGS; i++) {
          ap_regs[i] = 0;
        }

        for (int i = 0; i < MAX_ACTIONPOINTS; i++) {
          mvalues[i] = 0;
          pvalues[i] = 0;
          leader[i]  = i;
          groups[i]  = 0xffffffffUL; // 'never match' pattern
          mcopies[i] = 0;
        }

        classify_actionpoints ();
        is_enabled = (num_aps > 0);
      }

      // -----------------------------------------------------------------------
      // Determine the set of Actionpoints that can be individually operated,
      // or which may be the lead Actionpoint in a pair or quad grouping.
      // This method also computes the group match-set required for each primary.
      //
      void
      Actionpoints::compute_primaries ()
      {
        int i;
        uint32 m = 1;
        
        primaries = num_mask; // Initially any AP could be a primary
        
        for (i = 0; i < num_aps; i++)
        {
          leader[i] = i;            // initially every AP leads itself
          groups[i] = 0xffffffffUL; // 'never match' pattern        
        }
        
        for (i = 0; i < num_aps; i++, m = m << 1)
        { 
          uint32 ctrl_reg = ap_regs[(i*3)+2];
          groups[i] = (1 << i);
          
          // Compute the secondary APs for APi, even if APi is disabled.
          //
          if (AP_CTRL_Q(ctrl_reg)) {
            // Next 3 APs, mod num_aps, cannot be primary APs
            //
            LOG(LOG_DEBUG4) << "  [APS] Defining quad AP" << i << ", containing:";
            
            for (int j = 1; j < 4; j++)
            {
              int k      = (i + j) & (num_aps - 1);
              leader[k]  = i;
              uint32 s   = (1 << k);
              primaries &= ~s;
              groups[k]  = 0xffffffffUL;
              groups[i] |= s;
              LOG(LOG_DEBUG4) << "    [APS] AP" << k << ", primaries <= "
                              << std::hex << std::setw(8) << std::setfill('0') << primaries
                              << ", groups[" << k << "] <= " << groups[k]
                              << ", groups[" << i << "] <= " << groups[i]                            
                              << ", num_mask = " << num_mask;                              
            }
          } else if (AP_CTRL_P(ctrl_reg)) {
            // AP ((i+1) mod num_aps) cannot be a primary AP
            //
            int k      = (i + 1) & (num_aps - 1);
            leader[k]  = i;
            uint32 s   = (1 << k);
            primaries &= ~s;
            groups[k]  = 0xffffffffUL;
            groups[i] |= s;
          }
        }
      }
      
      // -----------------------------------------------------------------------
      // Classify and count the number of each type of Actionpoint.
      //
      void
      Actionpoints::classify_actionpoints ()
      {
        int i;         // Actionpoint iterator
        uint32 m = 1;  // one-hot representation of 'i'
        
        compute_primaries();
        
        n_pc_aps       = 0;
        n_ir_aps       = 0;
        
        n_inst_aps     = 0;

        n_ld_addr_aps  = 0;
        n_st_addr_aps  = 0;
        n_ld_data_aps  = 0;
        n_st_data_aps  = 0;

        n_lr_addr_aps  = 0;
        n_sr_addr_aps  = 0;
        n_lr_data_aps  = 0;
        n_sr_data_aps  = 0;

        n_extparam_aps = 0;
        singleton_insts = 0;
        
        for (i = 0; i < num_aps; i++, m = m << 1)
        {
          mvalues[i]      = ap_regs[i*3] | ap_regs[(i*3)+1];
          pvalues[i]      = mvalues[i] | cpu->core_arch.page_arch.page_byte_offset_mask;
          uint32 ctrl_reg = ap_regs[(i*3)+2];

          LOG(LOG_DEBUG4) << "[CLASSIFY] AP[" << i << "], reg[" << (3*i)+2 << "] = "
            << std::hex << std::setw(8) << std::setfill('0')
            << ctrl_reg;
          
          if (AP_CTRL_TT(ctrl_reg) != TT_DISABLED)
          {
            switch (AP_CTRL_AT(ctrl_reg))
            {
              case AP_INST_ADDR:
                {
                  ++n_pc_aps;
                  
                  // If the set of APs required to trigger AP_i is just
                  // i alone, then include it's bit-vector representation
                  // in the set of singleton_insts
                  //
                  if (groups[i] == m) singleton_insts |= m;
                  break;
                }            
              case AP_INST_DATA:
                {
                  if (full_aps) {
                    ++n_ir_aps;

                    // If the set of APs required to trigger AP_i is just
                    // i alone, then include it's bit-vector representation
                    // in the set of singleton_insts
                    //
                    if (groups[i] == m) singleton_insts |= m;
                  }
                  break;
                }
              case AP_LD_ST_ADDR:
                {
                  if ((AP_CTRL_TT(ctrl_reg) & TT_WRITE) == TT_WRITE)
                    ++n_st_addr_aps;
                  if ((AP_CTRL_TT(ctrl_reg) & TT_READ) == TT_READ)
                    ++n_ld_addr_aps;
                  break;
                }
              case AP_LD_ST_DATA:
                {
                  if (full_aps) {
                    if ((AP_CTRL_TT(ctrl_reg) & TT_WRITE) == TT_WRITE)
                      ++n_st_data_aps;
                    if ((AP_CTRL_TT(ctrl_reg) & TT_READ) == TT_READ)
                      ++n_ld_data_aps;
                  }
                  break;
                }
              case AP_AUX_ADDR:
                {
                  if ((AP_CTRL_TT(ctrl_reg) & TT_WRITE) == TT_WRITE)
                    ++n_sr_addr_aps;
                  if ((AP_CTRL_TT(ctrl_reg) & TT_READ) == TT_READ)
                    ++n_lr_addr_aps;
                  break;
                }
              case AP_AUX_DATA:
                {
                  if (full_aps) {
                    if ((AP_CTRL_TT(ctrl_reg) & TT_WRITE) == TT_WRITE)
                      ++n_sr_data_aps;
                    if ((AP_CTRL_TT(ctrl_reg) & TT_READ) == TT_READ)
                      ++n_lr_data_aps;
                  }
                  break;
                }
              case AP_EXT_P0:
                {
                  if (full_aps)
                    ++n_extparam_aps;
                  break;
                }
              case AP_EXT_P1:
                {
                  if (full_aps)
                    ++n_extparam_aps;
                  break;
                }
              default:;
            } /* END switch (AP_CTRL_AT(ctrl_reg)) */
          } /* END if (AP_CTRL_TT(ctrl_reg) != TT_DISABLED) */
        } /* END for (i = 0; i < num_aps; i++) */
        
        n_inst_aps     = n_pc_aps       + n_ir_aps;
        n_st_aps       = n_st_addr_aps  + n_st_data_aps;
        n_ld_aps       = n_ld_addr_aps  + n_ld_data_aps;
        n_mem_addr_aps = n_ld_addr_aps  + n_st_addr_aps;
        n_mem_data_aps = n_ld_data_aps  + n_st_data_aps;
        n_mem_aps      = n_mem_addr_aps + n_mem_data_aps;

        n_aux_addr_aps = n_lr_addr_aps  + n_sr_addr_aps;
        n_aux_data_aps = n_lr_data_aps  + n_sr_data_aps;
        n_aux_aps      = n_aux_addr_aps + n_aux_data_aps;
        n_sr_aps       = n_sr_addr_aps  + n_sr_data_aps;
        n_lr_aps       = n_lr_addr_aps  + n_lr_data_aps;
        
#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG4) << "[APS] n_pc_aps      = " << (uint32)n_pc_aps;
        LOG(LOG_DEBUG4) << "[APS] n_ir_aps      = " << (uint32)n_ir_aps;
        LOG(LOG_DEBUG4) << "[APS] n_ld_addr_aps = " << (uint32)n_ld_addr_aps;
        LOG(LOG_DEBUG4) << "[APS] n_st_addr_aps = " << (uint32)n_st_addr_aps;
        LOG(LOG_DEBUG4) << "[APS] n_lr_addr_aps = " << (uint32)n_lr_addr_aps;
        LOG(LOG_DEBUG4) << "[APS] n_sr_addr_aps = " << (uint32)n_sr_addr_aps;
        LOG(LOG_DEBUG4) << "[APS] n_ld_data_aps = " << (uint32)n_ld_data_aps;
        LOG(LOG_DEBUG4) << "[APS] n_st_data_aps = " << (uint32)n_st_data_aps;
        LOG(LOG_DEBUG4) << "[APS] n_lr_data_aps = " << (uint32)n_lr_data_aps;
        LOG(LOG_DEBUG4) << "[APS] n_sr_data_aps = " << (uint32)n_sr_data_aps;
        LOG(LOG_DEBUG4) << "[APS] n_sr_aps      = " << (uint32)n_sr_aps;
        LOG(LOG_DEBUG4) << "[APS] n_inst_aps    = " << (uint32)n_inst_aps;
        LOG(LOG_DEBUG4) << "[APS] n_mem_aps     = " << (uint32)n_mem_aps;
        LOG(LOG_DEBUG4) << "[APS] n_aux_aps     = " << (uint32)n_aux_aps;
        LOG(LOG_DEBUG4) << "[APS] primaries     = " 
                        << std::hex << std::setw(8) << std::setfill('0')
                        << primaries;
        for (i = 0; i < num_aps; i++)
        LOG(LOG_DEBUG4) << "[APS] groups[" << i << "]     = " 
                          << std::hex << std::setw(8) << std::setfill('0')
                          << groups[i];
        
#endif
      }

      // -----------------------------------------------------------------------
      // Handle writes to the Actionpoints registers. This method is called
      // from within Processor::write_aux_register, and therefore has the
      // write data pre-conditioned to clear any bits that are not expected
      // to be significant in the target register. The given aux register is
      // guaranteed to exist.
      //
      bool
      Actionpoints::write_aux_register (uint32 aux_addr, uint32 data)
      {
        if ((aux_addr < 0x220) || (aux_addr > 0x27F))
          return false;
        else {
          // 1. Update the appropriate Actionpoints register.
          //
          ap_regs[aux_addr - AP_AUX_BASE_ADDR] = data;

          // 2. Clear the ASR bit corresponding to the Actionpoint written
          //    by this call.
          //
          uint32 ap_num   = (aux_addr - AP_AUX_BASE_ADDR)/3;
          uint32 dbg_mask = ~(1 << (ap_num + 3));
          cpu->state.auxs[AUX_DEBUG] = (cpu->state.auxs[AUX_DEBUG] & dbg_mask);
          
          LOG(LOG_DEBUG4) << "[APS] SR to AP #" << ap_num << " clears bit "
                          << (ap_num+3) << " in the DEBUG aux register, now "
                          << std::hex << std::setw(8) << std::setfill('0') << cpu->state.auxs[AUX_DEBUG];

          // 3. Update the partial decoding of the Actionpoint settings, to allow
          //    the simulator to quickly determine what should be checked.
          //
          classify_actionpoints();

          // 4. Schedule the flushing of all cache structures and the
          //    elimination of all JIT translations.
          //    i.e. treat this as an example of self-modifying code.
          //
          cpu->set_pending_action(kPendingAction_FLUSH_TRANSLATION);

#ifdef DEBUG_ACTIONPOINTS        
          LOG(LOG_DEBUG4) << "[CPU" 
                          << cpu->core_id 
                          << "] write to Actionpoints register will flush translations";
#endif
          return true;
        }
      }

      // -----------------------------------------------------------------------
      // Handle reads from the Actionpoints registers. This method is called
      // from within Processor::read_aux_register, and therefore guarantees
      // the register exists. The caller will also mask out bits that are RAZ.
      //
      // This method checks that aux_addr is in the range 0x220 to 0x27F,
      // corresponding to a range of addresses from AUX_AP_AMV0 to AUX_AP_AC31.
      //
      bool
      Actionpoints::read_aux_register (const uint32 aux_addr, uint32 *data)
      {
        if ((aux_addr < 0x220) || (aux_addr > 0x27F))
          return false;
        else {
          *data = ap_regs[aux_addr - AP_AUX_BASE_ADDR];
          return true;
        }
      }

      // Matching methods   --------------------------------------------------
      //
      bool
      Actionpoints::match_instruction (uint32 inst_addr, uint32 inst_data, uint32* match, uint32* action)
      {
        inst_matches = 0;
        bool trigger = false;
        int i;
        int m;
        uint32 primary_action = 0;

#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG4) << "[APS] match_instruction, PC = "
                        << std::hex << std::setw(8) << std::setfill('0') << inst_addr
                        << ", IR = "
                        << std::hex << std::setw(8) << std::setfill('0') << inst_data;
#endif
          
        // Perform a search, but only if there exist instruction-based Actionpoints
        // Stop searching when the first Actionpoint trigger is detected.
        //
        if (n_inst_aps > 0)
        {
          for (i = 0, m = 1; (i < num_aps); i++, m = m << 1)
          {
            uint32 ctrl_reg  = ap_regs[(3*i)+2];
            uint32 amm_reg   = ap_regs[(3*i)+1];

            LOG(LOG_DEBUG4) << "  [APS] checking AP " << i 
                            << ", ctrl = " << std::hex << std::setw(8) << std::setfill('0') << ctrl_reg
                            << ", amv = "  << std::hex << std::setw(8) << std::setfill('0') << mvalues[i]
                            << ", amm = "  << std::hex << std::setw(8) << std::setfill('0') << amm_reg
                            << ", match_type = " << AP_MATCH_TYPE(ctrl_reg);

            
            if (AP_MATCH_READ(ctrl_reg) == ACTIVE_PC_AP)
            {
              uint32 a_pattern = (inst_addr | amm_reg);
                            
              if (AP_CTRL_M(ctrl_reg))
              {
                if (a_pattern != mvalues[i]) {
#ifdef DEBUG_ACTIONPOINTS        
                  LOG(LOG_DEBUG4) << "    [APS] matched PC address: M = " << AP_CTRL_M(ctrl_reg) 
                                  << ", a_pattern = "
                                  << std::hex << std::setw(8) << std::setfill('0') << a_pattern
                                  ;
#endif
                  handle_brk_hit (trigger, i, leader[i], inst_matches, m, primary_action, inst_addr);
                }
              }
              else 
              {
                if (a_pattern == mvalues[i]) {
#ifdef DEBUG_ACTIONPOINTS        
                  LOG(LOG_DEBUG4) << "    [APS] matched PC address: M = " << AP_CTRL_M(ctrl_reg) 
                                  << ", a_pattern = "
                                  << std::hex << std::setw(8) << std::setfill('0') << a_pattern
                                  ;
#endif
                  handle_brk_hit (trigger, i, leader[i], inst_matches, m, primary_action, inst_addr);
                }
              }
            }
            else if ((AP_MATCH_READ(ctrl_reg) == ACTIVE_IR_AP) && full_aps)
            {
              uint32 d_pattern = (inst_data | amm_reg);
              
              if (AP_CTRL_M(ctrl_reg))
              {
                if (d_pattern != mvalues[i]) {
#ifdef DEBUG_ACTIONPOINTS        
                  LOG(LOG_DEBUG4) << "    [APS] matched IR value: M = " << AP_CTRL_M(ctrl_reg) 
                                  << ", d_pattern = "
                                  << std::hex << std::setw(8) << std::setfill('0') << d_pattern
                                  ;
#endif
                  handle_brk_hit (trigger, i, leader[i], inst_matches, m, primary_action, inst_data);
                }
              }
              else
              {
                if (d_pattern == mvalues[i]) {
#ifdef DEBUG_ACTIONPOINTS        
                  LOG(LOG_DEBUG4) << "    [APS] matched IR value: M = " << AP_CTRL_M(ctrl_reg) 
                                  << ", d_pattern = "
                                  << std::hex << std::setw(8) << std::setfill('0') << d_pattern
                                  ;
#endif
                  handle_brk_hit (trigger, i, leader[i], inst_matches, m, primary_action, inst_data);
                }
              }
            }
          }

#ifdef DEBUG_ACTIONPOINTS        
          LOG(LOG_DEBUG4) << "inst_matches <= " 
                          << std::hex << std::setw(8) << std::setfill('0') 
                          << inst_matches;
#endif

          // Set the action on detecting the first trigger. Subsequent triggers
          // should not affect the action.
          //
          if (trigger) {
            *action = primary_action;
#ifdef DEBUG_ACTIONPOINTS        
            LOG(LOG_DEBUG4) << "[APS] returning a triggered AP, inst_matches = " 
                            << std::hex << std::setw(8) << std::setfill('0') 
                            << inst_matches
                            << ", action = " << *action
                            << ", aps_hits = " << aps_hits;
#endif
            // Set inst_matches to be the set of all groups for which the
            // primaries generated a trigger.
            //
            inst_matches = aps_hits;
          }
        }

        // Set 'matches' to the set of all APs that matched this instruction,
        // including the singleton, paired and quadded instruction APs.
        //
        *match = inst_matches;
        
        // Return a boolean indication of whether there was an immediate trigger.
        //    
        return trigger;
      }

      // Match the given load_addr argument with each of the memory read address
      // Actionpoints. 'no_caching' is set true if any address in the same page
      // as the given load_addr argument would match with any of the APs.
      // This allows the page caching algorithm to avoid caching pages that
      // could trigger a read-address AP.
      // The return value is true if an AP was triggered.
      //
      bool
      Actionpoints::match_load_addr (uint32 load_addr, bool& no_caching)
      {
        int i;
        int m;
        
        // If there are any kPendingAction_WATCHPOINT actions then
        // a previous watchpoint has already been discovered for this 
        // instruction, in which case we return without matching.
        //
        if (cpu->is_pending_action(kPendingAction_WATCHPOINT))
          return false;
        
        // Reset the boolean state variable, which records any trigger
        // on load addresses. This may be used in a subsequent call to
        // match_load_data.
        //
        clear_trigger();

#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG4) << "[APS] match_load_addr, addr = "
                        << std::hex << std::setw(8) << std::setfill('0') << load_addr;
#endif
        // Perform a search, but only if there are load-address Actionpoints
        //
        if (n_ld_addr_aps > 0)
        {
          for (i = 0, m = 1; (i < num_aps); i++, m = m << 1)
          {
            uint32 ctrl_reg  = ap_regs[(3*i)+2];
            uint32 amm_reg   = ap_regs[(3*i)+1];
            uint32 pattern   = load_addr | amm_reg;
            uint32 page_chk  = pattern   | cpu->core_arch.page_arch.page_byte_offset_mask;

#ifdef DEBUG_ACTIONPOINTS        
            LOG(LOG_DEBUG4) << "  [APS] checking AP" << i
                            << ", pattern = "
                            << std::hex << std::setw(8) << std::setfill('0') << pattern
                            << ": page_chk = "
                            << std::hex << std::setw(8) << std::setfill('0') << page_chk
                            << ", pvalue = "
                            << std::hex << std::setw(8) << std::setfill('0') << pvalues[i]
                            << ", amm = "  << std::hex << std::setw(8) << std::setfill('0')
                            << std::hex << std::setw(8) << std::setfill('0') << amm_reg
                            << ", mvalue = "
                            << std::hex << std::setw(8) << std::setfill('0') << mvalues[i];
#endif
            if (   ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_LD_MASK) == IN_RANGE_LD_ADDR)
                && (page_chk == pvalues[i])) {
              //
              // load_addr is the same page as this load address
              // Actionpoint, and hence we cannot allow the page 
              // cache to load this page.
              //
              no_caching = true;
              
              if ((pattern == mvalues[i]) && (ctrl_reg & AC_TT_READ)) {
                handle_aps_hit (has_triggered, i, leader[i], m, load_addr);
              }
            }
            else if ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_LD_MASK) == OUT_RANGE_LD_ADDR) {
              // 
              // If we have an inverted read-address matching AP, then we 
              // never allow a read page to be cached.
              //
              no_caching = true;
              
              if ((pattern != mvalues[i]) && (ctrl_reg & AC_TT_READ)) {
                handle_aps_hit (has_triggered, i, leader[i], m, load_addr);
              }
            }
          }
        }        
        return has_triggered;
      }

      bool
      Actionpoints::match_load_data (uint32 load_data)
      {
        int i;
        int m;
        
        // If there are any kPendingAction_WATCHPOINT actions then
        // a previous watchpoint has already been discovered for this 
        // instruction, in which case we return without matching.
        //
        if (cpu->is_pending_action(kPendingAction_WATCHPOINT))
          return false;

#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG4) << "[APS] match_load_data, data = "
                        << std::hex << std::setw(8) << std::setfill('0') << load_data;
#endif
        // Perform a search, but only if there are load-data Actionpoints
        //
        if (n_ld_data_aps > 0)
        {
          for (i = 0, m = 1; (i < num_aps); i++, m = m << 1)
          {
            uint32 ctrl_reg  = ap_regs[(3*i)+2];
            uint32 amm_reg   = ap_regs[(3*i)+1];
            uint32 pattern   = load_data | amm_reg;

#ifdef DEBUG_ACTIONPOINTS        
            LOG(LOG_DEBUG4) << "  [APS] checking AP " << i 
                            << ", ctrl = " << std::hex << std::setw(8) << std::setfill('0') << ctrl_reg
                            << ", amv = "  << std::hex << std::setw(8) << std::setfill('0') << mvalues[i]
                            << ", amm = "  << std::hex << std::setw(8) << std::setfill('0') << amm_reg
                            << ", match_type = " << AP_MATCH_TYPE(ctrl_reg);
#endif

            if (   ((AP_MATCH_CTRL(ctrl_reg) & IN_RANGE_LD_DATA) == IN_RANGE_LD_DATA)
                && (pattern == mvalues[i])) {
                handle_aps_hit (has_triggered, i, leader[i], m, load_data);                
            }
            else if (   ((AP_MATCH_CTRL(ctrl_reg) & OUT_RANGE_LD_DATA) == OUT_RANGE_LD_DATA)
                     && (pattern != mvalues[i])) {
                handle_aps_hit (has_triggered, i, leader[i], m, load_data);                
            }
          }
        }
        return has_triggered;
      }

      bool
      Actionpoints::match_store (uint32 store_addr, bool& no_caching, uint32 store_data)
      {
        int i;
        int m;
        bool triggered = false;
        
        // If there are any kPendingAction_WATCHPOINT actions then
        // a previous watchpoint has already been discovered for this 
        // instruction, in which case we return without matching.
        //
        if (cpu->is_pending_action(kPendingAction_WATCHPOINT))
          return false;

#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG4) << "[APS] match_store, addr = "
                        << std::hex << std::setw(8) << std::setfill('0') << store_addr
                        << ", data = "
                        << std::hex << std::setw(8) << std::setfill('0') << store_data;
#endif
        // Perform a search, but only if there are store-based Actionpoints
        //
        if (n_st_aps > 0)
        {
          for (i = 0, m = 1; (i < num_aps); i++, m = m << 1)
          {
            uint32 ctrl_reg  = ap_regs[(3*i)+2];
            uint32 amm_reg   = ap_regs[(3*i)+1];
            uint32 a_pattern = store_addr | amm_reg;
            uint32 page_chk  = a_pattern  | cpu->core_arch.page_arch.page_byte_offset_mask;
            uint32 d_pattern = store_data | amm_reg;

#ifdef DEBUG_ACTIONPOINTS        
            LOG(LOG_DEBUG4) << "  [APS] checking AP" << i
                            << ", a_pattern = "
                            << std::hex << std::setw(8) << std::setfill('0') << a_pattern
                            << ", d_pattern = "
                            << std::hex << std::setw(8) << std::setfill('0') << d_pattern
                            << ": page_chk = "
                            << std::hex << std::setw(8) << std::setfill('0') << page_chk
                            << ", pvalue = "
                            << std::hex << std::setw(8) << std::setfill('0') << pvalues[i]
                            << ", amm = "  
                            << std::hex << std::setw(8) << std::setfill('0') << amm_reg
                            << ", mvalue = "
                            << std::hex << std::setw(8) << std::setfill('0') << mvalues[i];
#endif
            if (   ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_ST_MASK) == IN_RANGE_ST_ADDR)
                && (page_chk == pvalues[i])) {
              //
              // store_addr is the same page as this store address
              // Actionpoint, and hence we cannot allow the page 
              // cache to load this page.
              //
#ifdef DEBUG_ACTIONPOINTS        
              LOG(LOG_DEBUG4) << "    [APS] addr is in page and M = " << AP_CTRL_M(ctrl_reg) 
                              << ", set no_caching = 1";
#endif
              no_caching = true;
              
              if (a_pattern == mvalues[i]) {
                handle_aps_hit (triggered, i, leader[i], m, store_addr);
              }
            }
            else if ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_ST_MASK) == OUT_RANGE_ST_ADDR) {
              // 
              // If we have an inverted write-address matching AP, then we 
              // never allow a read page to be cached.
              //
              no_caching = true;
#ifdef DEBUG_ACTIONPOINTS        
              LOG(LOG_DEBUG4) << "    [APS] AP matches write-address and M = " << AP_CTRL_M(ctrl_reg) 
                              << ", always set no_caching = 1";
#endif
              
              if (a_pattern != mvalues[i]) {
                handle_aps_hit (triggered, i, leader[i], m, store_addr);
              }
            }
            
            if (n_st_data_aps > 0) {
              if (   ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_ST_MASK) == IN_RANGE_ST_DATA)
                       && (d_pattern == mvalues[i])) {
                handle_aps_hit (triggered, i, leader[i], m, store_data);
              }
              else if (   ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_ST_MASK) == OUT_RANGE_ST_DATA)
                       && (d_pattern != mvalues[i])) {
                handle_aps_hit (triggered, i, leader[i], m, store_data);
              }
            }
          }
        }
        return triggered;
      }

      // Match the given lr_addr argument with each of the aux read address
      // Actionpoints. The return value is true if the given argument matched
      // with any of the APs.
      //
      bool
      Actionpoints::match_lr_addr (uint32 lr_addr)
      {
        int i;
        int m;
        
        clear_trigger();

#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG4) << "[APS] match_lr_addr, addr = "
                        << std::hex << std::setw(8) << std::setfill('0') << lr_addr;
#endif
        // Perform a search, but only if there are lr-address Actionpoints
        //
        if (n_lr_addr_aps > 0)
        {
          for (i = 0, m = 1; (i < num_aps); i++, m = m << 1)
          {
            uint32 ctrl_reg  = ap_regs[(3*i)+2];
            uint32 amm_reg   = ap_regs[(3*i)+1];
            uint32 pattern   = lr_addr | amm_reg;

#ifdef DEBUG_ACTIONPOINTS        
            LOG(LOG_DEBUG4) << "  [APS] checking AP" << i
                            << ", pattern = "
                            << std::hex << std::setw(8) << std::setfill('0') << pattern
                            << ", mvalue = "
                            << std::hex << std::setw(8) << std::setfill('0') << mvalues[i]
                            << ", M = " << AP_CTRL_M(ctrl_reg)
                            << ", test = (" 
                            << (AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_LR_ADDR) << " == " << IN_RANGE_LR_ADDR << ")";
#endif
            if (   ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_LR_ADDR) == IN_RANGE_LR_ADDR)
                && (pattern == mvalues[i])) {
#ifdef DEBUG_ACTIONPOINTS        
                  LOG(LOG_DEBUG4) << "    [APS] matched LR address: M = " << AP_CTRL_M(ctrl_reg);
#endif
                handle_aps_hit (has_triggered, i, leader[i], m, lr_addr);
            }
            else if (    ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_LR_ADDR) == OUT_RANGE_LR_ADDR)
                      && (pattern != mvalues[i])) {
#ifdef DEBUG_ACTIONPOINTS        
                  LOG(LOG_DEBUG4) << "    [APS] matched LR address: M = " << AP_CTRL_M(ctrl_reg);
#endif
                handle_aps_hit (has_triggered, i, leader[i], m, lr_addr);
            }
          }
        }
        return has_triggered;
      }

      bool
      Actionpoints::match_lr_data (uint32 lr_data)
      {
        int i;
        int m;

#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG4) << "[APS] match_lr_data, data = "
                        << std::hex << std::setw(8) << std::setfill('0') << lr_data;
#endif
        // Perform a search, but only if there are lr-data Actionpoints
        //
        if (n_lr_data_aps > 0)
        {
          for (i = 0, m = 1; (i < num_aps); i++, m = m << 1)
          {
            uint32 ctrl_reg  = ap_regs[(3*i)+2];
            uint32 amm_reg   = ap_regs[(3*i)+1];
            uint32 pattern   = lr_data | amm_reg;

            if (   ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_LR_DATA) == IN_RANGE_LR_DATA)
                && (pattern == mvalues[i])) {
                handle_aps_hit (has_triggered, i, leader[i], m, lr_data);                
            }
            else if (   ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_LR_DATA) == OUT_RANGE_LR_DATA)
                     && (pattern != mvalues[i])) {
                handle_aps_hit (has_triggered, i, leader[i], m, lr_data);                
            }
          }
        }
        return has_triggered;
      }

      // Match the given sr arguments with each of the aux write address and data
      // Actionpoints. The return value is true if the given argument matched
      // with any of the APs.
      //
      bool
      Actionpoints::match_sr (uint32 sr_addr, uint32 sr_data)
      {
        int i;
        int m;
        bool triggered = false;

#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG4) << "[APS] match_sr, addr = "
                        << std::hex << std::setw(8) << std::setfill('0') << sr_addr
                        << ", data = "
                        << std::hex << std::setw(8) << std::setfill('0') << sr_data;
#endif

        // Perform a search, but only if there are SR-related Actionpoints
        //
        if (n_sr_aps != 0)
        {
          for (i = 0, m = 1; (i < num_aps); i++, m = m << 1)
          {
            uint32 ctrl_reg  = ap_regs[(3*i)+2];
            uint32 amm_reg   = ap_regs[(3*i)+1];
            uint32 a_pattern = sr_addr | amm_reg;
            uint32 d_pattern = sr_data | amm_reg;

#ifdef DEBUG_ACTIONPOINTS        
            LOG(LOG_DEBUG4) 
              << "  [APS] comparing AP" 
              << i 
              << ", AP_CTRL = " 
              << AP_MATCH_CTRL(ctrl_reg)
              << ", IN_RANGE_SR_DATA = "
              << IN_RANGE_SR_DATA
              << ", mvalues[" << i << "] = " 
              << std::hex << std::setw(8) << std::setfill('0')
              << mvalues[i];
#endif

            if (   ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_SR_ADDR) == IN_RANGE_SR_ADDR)
                && (a_pattern == mvalues[i])) {
                handle_aps_hit (triggered, i, leader[i], m, sr_addr);
            }
            else if (    ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_SR_ADDR) == OUT_RANGE_SR_ADDR)
                      && (a_pattern != mvalues[i])) {
                handle_aps_hit (triggered, i, leader[i], m, sr_addr);
            }
            
            if (n_sr_data_aps > 0) {
              if (   ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_SR_DATA) == IN_RANGE_SR_DATA)
                  && (d_pattern == mvalues[i])) {
                  handle_aps_hit (triggered, i, leader[i], m, sr_data);
              }
              else if (    ((AP_MATCH_CTRL(ctrl_reg) & ANY_RANGE_SR_DATA) == OUT_RANGE_SR_DATA)
                        && (d_pattern != mvalues[i])) {
                  handle_aps_hit (triggered, i, leader[i], m, sr_data);
              }
            }
          }
        }
#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG4) << "[APS] match_sr, result = "
                        << triggered ? 1 : 0;
#endif
        return triggered;
      }

      bool
      Actionpoints::match_ext_param (uint32 param0, uint32 param1)
      {
        // @@@
        LOG(LOG_WARNING) << "[CPU" 
                         << cpu->core_id 
                         << "] external parameter Actionpoints not implemented!";

        
        return false;
      }
      
      void
      Actionpoints::handle_brk_hit (bool   &trigger, 
                                    uint32 ap,
                                    uint32 primary, 
                                    uint32 &inst_matches, 
                                    uint32 match,  
                                    uint32 &action, 
                                    uint32 value)
      {
#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG4) << "      [APS] handle_brk_hit";
        LOG(LOG_DEBUG4) << "        [APS] ap           = " << ap;
        LOG(LOG_DEBUG4) << "        [APS] primary      = " << primary;
        LOG(LOG_DEBUG4) << "        [APS] inst_matches = " << std::hex << std::setw(8) << std::setfill('0') <<match;
        LOG(LOG_DEBUG4) << "        [APS] match        = " << std::hex << std::setw(8) << std::setfill('0') <<match;
        LOG(LOG_DEBUG4) << "        [APS] value        = " << std::hex << std::setw(8) << std::setfill('0') <<value;
#endif
        
        // Add the match given by the fifth argument to the current set of
        // instruction matches given by the fourth argument, and record the
        // value that must be assigned to AC_AMM[ap] if there is a trigger.
        //
        inst_matches  |= match;
        mcopies[ap]    = value;

        // The third argument indicates the leader of the match, and from this
        // we check whether groups[primary] is a subset of the updated set 
        // of inst_matches. If so, we get the Action associated with this 
        // primary, and set trigger to be true.
        //
        if ((groups[primary] & inst_matches) == groups[primary]) {
          //
          // If the given Actionpoint triggers group, then that 
          // entire group is a member of the set of triggered APs.
          //
          aps_hits |= groups[primary];
          
          // If the current search has not previously triggered,
          // then get the required action for this (first) trigger.
          //
          if (!trigger) {
            action = AP_CTRL_AA(ap_regs[(3*primary)+2]);

#ifdef DEBUG_ACTIONPOINTS        
            LOG(LOG_DEBUG3) << "[APS] triggered Actionpoint " << primary 
                            << ", group-set = 0x"
                            << std::hex << std::setw(8) << std::setfill('0') << groups[primary]
                            << ", action = " << action
                            << ", inst_matches = " << inst_matches;
#endif
            // Indicate that a primary Actionpoint has triggered.
            //
            trigger = true;
          }
        }
      }
      
      void
      Actionpoints::handle_aps_hit (bool   &trigger,
                                    uint32 ap,
                                    uint32 primary,
                                    uint32 match,
                                    uint32 value)
      {
#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG4) << "      [APS] handle_aps_hit";
        LOG(LOG_DEBUG4) << "        [APS] ap      = " << ap;
        LOG(LOG_DEBUG4) << "        [APS] primary = " << primary;
        LOG(LOG_DEBUG4) << "        [APS] match   = " << std::hex << std::setw(8) << std::setfill('0') <<match;
        LOG(LOG_DEBUG4) << "        [APS] value   = " << std::hex << std::setw(8) << std::setfill('0') <<value;
#endif

        // Add the match given by the second argument to aps_hits
        // with primary in the first argument, and check whether groups[i]
        // is a subset of state.aps_hits. If so, get the Action associated
        // with this primary, and apply that action to the cpu.
        //
        aps_matches |= match;
        mcopies[ap]  = value;
        
        if ((groups[primary] & aps_matches) == groups[primary]) {
#ifdef DEBUG_ACTIONPOINTS        
          LOG(LOG_DEBUG3) << "[APS] triggered primary " << primary 
                          << ", group-set = 0x"
                          << std::hex << std::setw(8) << std::setfill('0') << groups[primary]
                          << ", aps_matches = " << aps_matches;
#endif
          // The given primary Actionpoint is recorded.
          //
          aps_hits |= (1 << primary);

          // If the current search has not previously triggered,
          // then get the required action for this (first) trigger.
          //
          if (!trigger) {
            aps_action = AP_CTRL_AA(ap_regs[(3*primary)+2]);
            cpu->set_pending_action(kPendingAction_WATCHPOINT);
            cpu->end_of_block = true;
            cpu->state.auxs[AUX_AP_WP_PC] = cpu->state.pc;

#ifdef DEBUG_ACTIONPOINTS        
            LOG(LOG_DEBUG3) << "  [APS] initial trigger is AP" << primary 
                            << ", group-set = 0x"
                            << std::hex << std::setw(8) << std::setfill('0') << groups[primary]
                            << ", action = " << aps_action
                            << ", AUX_WP_PC <= " << cpu->state.auxs[AUX_AP_WP_PC];
 #endif
            // Indicate that a primary Actionpoint has triggered.
            //
            trigger = true;
          }
        }
      }

      void
      Actionpoints::copy_matching_values ()
      {
#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG3) << "[APS] copy_matching_values for primary AP set "
                        << std::hex << std::setw(8) << std::setfill('0') << aps_hits;
#endif
        uint32 matches = 0;
        
        // Compute the intersection of groups for all matching primaries
        //
        for (int i = 0; i < num_aps; i++) {
          if (((aps_hits >> i) & 1) == 1) {
            matches |= groups[i];
          }
        }
        
#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG3) << "  [APS] copying matching values for AP set "
                        << std::hex << std::setw(8) << std::setfill('0') << matches;
#endif
        // Copy all values in the mcopies array at index positions
        // corresponding to a 1 in the groups of all matching primaries.
        //
        for (int i = 0; i < num_aps; i++)
        {
          if (((matches >> i) & 1) == 1) {
            ap_regs[(3*i)] = mcopies[i];
#ifdef DEBUG_ACTIONPOINTS        
            LOG(LOG_DEBUG3) << "    [APS] AP_AMV" << i << " <= "
                            << std::hex << std::setw(8) << std::setfill('0')
                            << mcopies[i];
#endif
          }
        }
        
        // Record the set of matches that contributed to all
        // triggered actionpoints.
        //
        aps_matches = matches;
      }
      
      // Called when the simulator executes a SIM_AP_BREAK instruction
      // Based on the vector of inst_primaries, it will update the AP_AMV[i]
      // aux register with PC or IR, according to the setting of the
      // AP_AC[i].AT field.
      //
      void
      Actionpoints::take_breakpoint (uint32 inst_primaries, uint32 pc, uint32 ir)
      {
#ifdef DEBUG_ACTIONPOINTS        
        LOG(LOG_DEBUG3) << "[APS] taking breakpoint set "
                        << std::hex << std::setw(8) << std::setfill('0') << inst_primaries
                        << ", PC = " << pc << ", IR = " << ir;
#endif
        // Set mcopies[i] to PC or IR, depending on the AT field of AP_AC[i]
        //
        for (int i = 0; i < num_aps; i++) {
          mcopies[i] = (AP_CTRL_AT(ap_regs[(3*i)+2]) == AP_INST_ADDR) ? pc : ir;
        }
        
        // Set aps_hits to be the set of instruction primary hits.
        //
        aps_hits = inst_primaries & primaries;
        
        // Copy the PC or IR values from mcopies, for all APs that are 
        // members of a triggered group.
        //
        copy_matching_values();
      }
      
} } } //  arcsim::internal::system

