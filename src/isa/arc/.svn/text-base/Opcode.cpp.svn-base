//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================

#include "isa/arc/Opcode.h"


namespace arcsim {
  namespace isa {
    namespace arc {
      
      // declare opcode string table
      static const char * const opcode_string[OpCode::opcode_count] = {
#define DEF_ENUM_C(ignore, name) #name,
        BUILTIN_OPCODE_LIST(DEF_ENUM_C)
#undef DEF_ENUM_C
      };
      
      char const * const
      OpCode::to_string(Op o)
      {
        return opcode_string[o];
      }
      
} } } //  arcsim::isa::arc
