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
//
// =====================================================================

#ifndef INC_INTERNAL_SYSTEM_SMT_SMART_H_
#define INC_INTERNAL_SYSTEM_SMT_SMART_H_

// =====================================================================
// HEADERS
// =====================================================================

#include "api/types.h"

namespace arcsim {
  namespace sys {
    namespace cpu {
      
      // -----------------------------------------------------------------------
      // Forward class declarations
      //
      class Processor;
      
      static const unsigned int MAX_STACKSIZE = 128;
      static const unsigned int NUM_SMT_AUX_REGS  = 3;
      
      // -----------------------------------------------------------------------
      // SmaRT - Small Real-time Trace
      //
      class Smart {
      private:

        // Boolean 'enabled' flag, tested on the fast path
        //
        bool is_enabled;
        
        // Pointer back to parent processor object
        //
        Processor *cpu;
        
        // Pointers to SmaRT FIFO queue element field arrays
        //
        uint32 *src_addr;
        uint32 *dst_addr;
        uint32 *flags;
        
        // Array index and mask for fast wrap around and bound limits
        //
        uint32 stack_depth;                 // Size of SmaRT stack
        uint32 head;                        // array index for head location
        uint32 index_mask;                  // stack_depth - 1
        uint32 ctrl_mask;                   // write mask for SMART_CONTROL
        
        // SmaRT aux registers
        //
        uint32 aux_smt_build;               // SMART_BUILD   (0x0FF)
        uint32 aux_smt_control;             // SMART_CONTROL (0x700)
        uint32 aux_smt_data;                // SMART_DATA    (0x701)
        
        Smart(const Smart & m);             // DO NOT COPY
        void operator=(const Smart &);      // DO NOT ASSIGN
                
      public:
        
        // SmaRT control (IDX) field values
        //
        enum SmtCtrlIdx {
          SRC_ADDR    = 0,
          DEST_ADDR   = 1,
          FLAGS_VALUE = 2,
          Reserved    = 3
        };
        
        // SmaRT control (EN) field values
        //
        enum SmtCtrlEn {
          TRACE_DISABLED  = 0,
          TRACE_ENABLED   = 1
        };
        
        // SmaRT flags field values
        //
        enum SmtFlagFields {
          FLAGS_U          = 0x00000100UL,
          FLAGS_EX         = 0x00000200UL,
          FLAGS_RP         = 0x00000400UL,
          FLAGS_V          = 0x80000000UL
        };
        
        // Constructor
        //
        Smart() :
          is_enabled(false),
          cpu(0),
          src_addr(0),
          dst_addr(0),
          flags(0),
          stack_depth(0),
          head(0),
          index_mask(0),
          ctrl_mask(0),
          aux_smt_build(0),
          aux_smt_control(0),
          aux_smt_data(0)
        { /* EMPTY */ }
        
        // Destructor, must deallocate SmaRT stack
        //
        ~Smart();
        
        // Init    -------------------------------------------------------------
        //
        void configure(Processor *parent, uint32 num);
 
        // Auxiliary register read/write methods    ----------------------------
        //
        bool read_aux_register  (uint32 aux_addr, uint32 *data);
        bool write_aux_register (uint32 aux_addr, uint32 aux_data);
        
        // Smart methods   -----------------------------------------------------
        //
        void push_branch    (uint32 br_src, uint32 br_dst);
        void push_exception (uint32 ex_src, uint32 ex_dst);
        
        // Inline methods, on fast path   --------------------------------------
        //
        inline bool enabled ()       const  { return is_enabled;           }
        
        bool        is_configured()  const;

        
      };

    }
  }
} //  arcsim::sys::cpu

#endif
