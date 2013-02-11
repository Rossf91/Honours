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
// This file implements ...
//
// =====================================================================

#include <iomanip>

#include "sys/smt/Smart.h"
#include "sys/cpu/processor.h"
#include "sys/cpu/aux-registers.h"

#include "util/Log.h"

namespace arcsim {
  namespace sys {
    namespace cpu {

      // =====================================================================
      // MACROS
      // =====================================================================

      // Macros to extract bit-fields from SmaRT Control register.
      //
      #define SMT_CTRL_LP(_ac_)        ((_ac_) >> 10)
      #define SMT_CTRL_EN(_ac_)        ((_ac_) & 1)
      #define SMT_CTRL_IDX(_ac_)       (((_ac_) >> 8) & 3)

      // =====================================================================
      // METHODS
      // =====================================================================

      // Destructor must deallocate SmaRT stack if it has been allocated.
      //
      Smart::~Smart()
      {
        if (src_addr) delete [] src_addr;
        if (dst_addr) delete [] dst_addr;
        if (flags)    delete [] flags;
        
        src_addr = dst_addr = flags = 0;
      }
      
      // -----------------------------------------------------------------------
      // Configure the Smart object. Must be called during init.
      //
      void
      Smart::configure(Processor *parent, uint32 num)
      {
        // Note the pointer to the parent Processor object
        //
        cpu = parent;
        
        // Check for a legal number of SmaRT stack entries
        // (return without initializing if not supported)
        //
        switch (num)
        {
          case 0:
          {
            return;
          }
          case 8:
          case 16:
          case 32:
          case 64:
          case 128:
          case 256:
          case 512:
          case 1024:
          case 2048:
          case 4096:
          {
            stack_depth = num;
            index_mask  = stack_depth - 1;
            ctrl_mask   = (0xffffffff << 10) + (3 << 8) + 1;
            break;
          }
          default:
          {
            LOG(LOG_ERROR)  << "[CPU"
                            << parent->core_id 
                            << "] Illegal SmaRT stack depth (" 
                            << num << ")";
            return;
          }
        }

        // Allocate storage space for the SmaRT stack objects
        //
        src_addr = new uint32[stack_depth];
        dst_addr = new uint32[stack_depth];
        flags    = new uint32[stack_depth];
        
        // Clear the SmaRT stack objects
        //
        for (int i = 0; i < stack_depth; i++) {
          src_addr[i] = 0;
          dst_addr[i] = 0;
          flags[i]    = 0;
        }

        // Initialize the build configuration register
        //
        aux_smt_build = (stack_depth << 10) + 3;

        LOG(LOG_DEBUG1) << "[CPU" 
                        << parent->core_id 
                        << "] SmaRT stack depth = " << num ;
      }

      // -----------------------------------------------------------------------
      // Indicate if SmaRT is properly configured and ready for use
      //
      bool
      Smart::is_configured() const
      {
        return (stack_depth > 0);
      }

      // -----------------------------------------------------------------------
      // Read a SmaRT auxiliary register
      //
      bool
      Smart::read_aux_register  (uint32 aux_addr, uint32 *data)
      {
        bool success = true;
        LOG(LOG_DEBUG1) << "[SmaRT] Reading aux 0x"
                        << std::hex << std::setw(8) << std::setfill('0')
                        << aux_addr
                        << " index_mask = "
                        << index_mask;
        switch (aux_addr)
        {
          default:                { success = false;           break; }
          case AUX_SMART_BUILD:   { *data   = aux_smt_build;   break; }
          case AUX_SMART_CONTROL: { *data   = aux_smt_control; break; }
          case AUX_SMART_DATA:    
          { 
            uint32 off = SMT_CTRL_LP(aux_smt_control);
            
            if (off < stack_depth) {
              uint32 idx = (head + off) & index_mask;

              switch (SMT_CTRL_IDX(aux_smt_control))
              {
                case SRC_ADDR:    { aux_smt_data = src_addr[idx]; break; }
                case DEST_ADDR:   { aux_smt_data = dst_addr[idx]; break; }
                case FLAGS_VALUE: { aux_smt_data = flags[idx];    break; }
                case Reserved:    { aux_smt_data = 0;             break; }
              }
              LOG(LOG_DEBUG2) << "[SmaRT] Control [LP,IDX] = ["
                              << SMT_CTRL_LP(aux_smt_control)
                              << "," 
                              << SMT_CTRL_IDX(aux_smt_control)
                              << "] head = " << head
                              << ", ptr = " << idx
                              << ", value = "
                              << std::hex << std::setw(8) << std::setfill('0')
                              << aux_smt_data;

              *data = aux_smt_data;    
            } else {
              LOG(LOG_DEBUG2) << "[SmaRT] LP=" 
                              << off 
                              << " is out of range (0.." 
                              << stack_depth-1 
                              << "), stack read returns 0";
              *data = 0;    
            }
            break;
          }
        }
        
        return success;
      }
      
      // -----------------------------------------------------------------------
      // Write a SmaRT auxiliary register
      //
      bool
      Smart::write_aux_register (uint32 aux_addr, uint32 aux_data)
      {
        bool success = true;
        LOG(LOG_DEBUG1) << "[SmaRT] Write to aux 0x"
                        << std::hex << std::setw(8) << std::setfill('0')
                        << aux_addr
                        << " <= 0x" << std::setw(8) << std::setfill('0')
                        << aux_data;
        switch (aux_addr)
        {
          case AUX_SMART_CONTROL: 
          {
            aux_smt_control = aux_data & ctrl_mask;
            is_enabled      = (SMT_CTRL_EN(aux_smt_control) == TRACE_ENABLED);
            LOG(LOG_DEBUG2) << "[SmaRT] SMART_CONTROL <= 0x"
                            << std::hex << std::setw(8) << std::setfill('0')
                            << aux_smt_control
                            << ", enabled = "
                            << (is_enabled ? 1 : 0);
            break;
          }
          default:
          {
            success = false;
            break;
          }
        }
        
        return success;
      }

      // -----------------------------------------------------------------------
      // Push a branch/jump/loop entry onto the SmaRT stack
      //
      void
      Smart::push_branch (uint32 br_src, uint32 br_dst)
      {
        if (    (br_src == src_addr[head]) 
             && (br_dst == dst_addr[head])
             && (flags[head] & FLAGS_V)) {
          // this is a repeated branch, so just set the RP bit in the
          // current head entry.
          //
          flags[head] |= FLAGS_RP;
          LOG(LOG_DEBUG2) << "[SmaRT] repeating from "
                          << std::hex << std::setw(8) << std::setfill('0')
                          << br_src << " to " << br_dst;
          LOG(LOG_DEBUG4) << "[SmaRT] src_addr["
                          << head << "] = "
                          << std::hex << std::setw(8) << std::setfill('0')
                          << src_addr[head]
                          << ", dst_addr[" << head << "] = "
                          << std::setw(8) << std::setfill('0')
                          << dst_addr[head]
                          << ", flags[" << head << "] <= " << flags[head];
        } else {
          // this is not a repeated branch, so push a new entry into
          // the stack.
          //
          head = (head - 1) & index_mask;
          src_addr[head] = br_src;
          dst_addr[head] = br_dst;
          flags[head]    = FLAGS_V + ((cpu->state.U & 1) << 8);
          LOG(LOG_DEBUG2) << "[SmaRT] Branching from 0x"
                          << std::hex << std::setw(8) << std::setfill('0')
                          << br_src
                          << " to 0x" << std::setw(8) << std::setfill('0')
                          << br_dst;
          LOG(LOG_DEBUG4) << "[SmaRT] src_addr["
                          << head << "] <= "
                          << std::hex << std::setw(8) << std::setfill('0')
                          << src_addr[head]
                          << ", dst_addr[" << head << "] <= "
                          << std::setw(8) << std::setfill('0')
                          << dst_addr[head]
                          << ", flags[" << head << "] <= " << flags[head];
        }
      }
      
      // -----------------------------------------------------------------------
      // Push an exception entry onto the SmaRT stack
      //
      void
      Smart::push_exception (uint32 ex_src, uint32 ex_dst)
      {
        if (    (ex_src == src_addr[head]) 
             && (ex_dst == dst_addr[head])
             && (flags[head] & FLAGS_V)) {
          // this is a repeated branch, so just set the RP bit in the
          // current head entry.
          //
          flags[head] |= FLAGS_RP;
          LOG(LOG_DEBUG2) << "[SmaRT] Exception repeat from 0x"
                          << std::hex << std::setw(8) << std::setfill('0')
                          << ex_src
                          << " to 0x" << std::setw(8) << std::setfill('0')
                          << ex_dst;
          LOG(LOG_DEBUG4) << "[SmaRT] src_addr["
                          << head << "] = "
                          << std::hex << std::setw(8) << std::setfill('0')
                          << src_addr[head]
                          << ", dst_addr[" << head << "] = "
                          << std::setw(8) << std::setfill('0')
                          << dst_addr[head]
                          << ", flags[" << head << "] <= " << flags[head];
        } else {
          // this is not a repeated branch, so push a new entry into
          // the stack.
          //
          head = (head - 1) & index_mask;
          src_addr[head] = ex_src;
          dst_addr[head] = ex_dst;
          flags[head]    = FLAGS_V + FLAGS_EX + ((cpu->state.U & 1) << 8);
          LOG(LOG_DEBUG2) << "[SmaRT] Exception taken from 0x"
                          << std::hex << std::setw(8) << std::setfill('0')
                          << ex_src
                          << " to 0x" << std::setw(8) << std::setfill('0')
                          << ex_dst;
          LOG(LOG_DEBUG4) << "[SmaRT] src_addr["
                          << head << "] <= "
                          << std::hex << std::setw(8) << std::setfill('0')
                          << src_addr[head]
                          << "], dst_addr[" << head << "] <= "
                          << std::setw(8) << std::setfill('0')
                          << dst_addr[head]
                          << ", flags[" << head << "] <= " << flags[head];
        }
      }

} } } //  arcsim::sys::cpu

