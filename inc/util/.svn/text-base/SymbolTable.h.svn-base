//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Symbol Table class optimised for lookup speed.
//
// =============================================================================

#ifndef INC_UTIL_SYMBOLTABLE_H_
#define INC_UTIL_SYMBOLTABLE_H_

#include "api/types.h"

#include <string>

#include "ioc/ContextItemInterface.h"

class IELFISymbolTable;

namespace arcsim {
  namespace util {
    
    // Forward declare SymEntry struct
    //
    struct SymEntry;
    
    class SymbolTable  : public arcsim::ioc::ContextItemInterface
    {
    public:
      static const uint32 kSymbolTableMaxName = 256;
      
      explicit SymbolTable(const char * name);
      ~SymbolTable();
      
      
      const uint8*   get_name()       const { return name_;       }
      const Type     get_type()       const { return arcsim::ioc::ContextItemInterface::kTSymbolTable; }

      
      void create(const IELFISymbolTable* tab);
      void destroy(); 
      
      // -----------------------------------------------------------------------
      // Query methods
      //
      bool get_symbol(uint32 addr, std::string& name);
      
    private:
      // pre-sorted symbol table for efficient and fast lookup
      SymEntry*         tab_;
      int               tab_size_;
      
      // read only target symbol table
      const IELFISymbolTable* elf_tab_;
      
      
      uint8      name_[kSymbolTableMaxName];
      
    };
    
} } // arcsim::util


#endif  // INC_UTIL_SYMBOLTABLE_H_
