//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2004 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  This file defines a C++ class to disassemble ARCompact code.
//
//  API:
//      Disasm disasm(inst, limm);  // create Disasm object and disassemble
//      printf ("%s\n", disasm.buf);// access the disassembled inst
//
// =====================================================================

#ifndef _disasm_h_
#define _disasm_h_

// =====================================================================
// HEADERS
// =====================================================================

#include <cstring>

#include "define.h"

#include "api/types.h"
#include "sim_types.h"

// =====================================================================
// MACRO DEFINITIONS
// =====================================================================

#define DISASM_BUF_SIZE 256

// -----------------------------------------------------------------------------
// FORWARD DECLARATION
//
class IsaOptions;

namespace arcsim {
  // -----------------------------------------------------------------------------
  // FORWARD DECLARATION
  //  
  namespace sys {
    namespace cpu {
      class EiaExtensionManager;
  } }
  
  namespace isa {
    namespace arc {

      class Disasm {
      private:
        // Disallow copy constructor and assignment for Disasm objects. If you really
        // need to copy an object of this class write a clone(const Disasm&) method.
        //
        DISALLOW_COPY_AND_ASSIGN(Disasm);
        
        // Create string represenation of instruction in buf[]
        //
        void to_string ();
          
        // Disassemble instruction
        //
        void disasm (uint32 limm_val);

      public:
        uint32      inst;           // Instruction code
        int         fmt;            // Operand format
        const char* opcode;         // ptr to op-code string
        const char* cc_test;        // ptr to conditional test
        const char* reg_a;          // ptr to name of A reg
        const char* reg_b;          // ptr to name of B reg
        const char* reg_c;          // ptr to name of C reg
        const char* dslot;          // ptr to dslot string
        const char* write_back_mode;// ptr to mem-addr write back mode
        const char* cache_byp_mode; // ptr to cacheability attribute
        const char* size_suffix;    // ptr to size suffix
        const char* extend_mode;    // ptr to extension mode string
        const char* f_bit;          // ptr to Flag enable string
        uint32 abs_val;             // an absolute address or operand
        sint32 int_val;             // a signed offset value
        
        char    buf[DISASM_BUF_SIZE]; // the output buffer
        size_t  len;                  // length of disassembled string
        
        char    flags[4];           // set of modification flags
        char    limm_str[16];       // disassembled long immediate value
        
        bool cc_op;
        
        bool flag_enable;
        
        bool z_write, n_write, c_write, v_write;
        
        bool invld_ins;
        bool has_limm;
        bool is_16bit;
        
        // Reference to IsaOptions for which we are disassembling
        //
        const IsaOptions&                         isa_opts;
        
        // Reference to EiaExtensionManager
        //
        const arcsim::sys::cpu::EiaExtensionManager&    eia_mgr;
                
        // Constructor
        //
        explicit Disasm (const IsaOptions&                            _opts,
                         const arcsim::sys::cpu::EiaExtensionManager& _eia_mgr,
                         uint32 inst,
                         uint32 limm_val);
                        
        // Mapping from reg number to 
        //
        static const char* dis_reg(uint32 reg_num);
        
        int encoded_size () const {
          return  (has_limm ? INST_SIZE_HAS_LIMM : INST_SIZE_BASE)
          | (is_16bit ? INST_SIZE_16BIT_IR : INST_SIZE_BASE);
        }
        
      };

} } }  // namespace arcsim::isa::arc
      
#endif /* _disasm_h_ */
