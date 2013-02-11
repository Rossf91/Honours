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

#include <cstdlib>

#include "util/SymbolTable.h"

#include "util/Allocate.h"

#include <ELFIO.h>

namespace arcsim {
  namespace util {

    // Structure for efficient symbol table lookup
    struct SymEntry {
      uint32            index; // index into ELF table
      const Elf32_Sym*  entry; // pointer to backing symtab entry
    };

    SymbolTable::SymbolTable(const char * name)
    : tab_(0),
      tab_size_(0),
      elf_tab_(0)
    {
      uint32 i;
      for (i = 0; i < kSymbolTableMaxName - 1 && name[i]; ++i)
        name_[i] = static_cast<uint8>(name[i]);
      name_[i] = '\0';

    }
    
    static int entry_cmp(const void* p1, const void* p2)
    {
      const Elf32_Sym *e1, *e2;
      
      e1 = ((SymEntry*)(p1))->entry;
      e2 = ((SymEntry*)(p2))->entry;
      
      if (e1->st_value < e2->st_value) return -1;
      if (e1->st_value == e2->st_value) return 0;
      return 1;
    }

    void
    SymbolTable::create(const IELFISymbolTable *tab)
    {
      if (elf_tab_ || tab_)
        destroy();
      
      elf_tab_ = tab;

      const Elf32_Word  snum     = elf_tab_->GetSymbolNum();  // symbol count
      const char*      sdata     = elf_tab_->GetData();       // section data
      const Elf32_Word ssize     = elf_tab_->GetEntrySize();  // section size

      // compute size of sorted symbol lookup table
      for (Elf32_Word i = 0; i < snum; ++i) {
        const Elf32_Sym* p = reinterpret_cast<const Elf32_Sym*>(sdata + i * ssize);
        if (p->st_size) 
          ++tab_size_;
      }
      // allocate table
      tab_ = (SymEntry*)arcsim::util::Malloced::New(tab_size_ * sizeof(tab_[0]));
      // fill up table
      int tab_idx = 0;
      for (Elf32_Word i = 0; i < snum; ++i) {
        const Elf32_Sym* p = reinterpret_cast<const Elf32_Sym*>(sdata + i * ssize);
        if (p->st_size) {
          SymEntry * const e = tab_ + tab_idx++;
          e->index = i;
          e->entry = p;
        }
      }
      // sort table by symbol address
      std::qsort(tab_, tab_size_, sizeof(tab_[0]), entry_cmp);    
    }
    
    void
    SymbolTable::destroy()
    {
      if (tab_) {
        Malloced::Delete(tab_);
        tab_ = 0;
      }
      if (elf_tab_) {
        elf_tab_->Release();
        elf_tab_ = 0;
      }
    }
    
    SymbolTable::~SymbolTable()
    {
      destroy();
    }
    
    
    // -----------------------------------------------------------------------
    // Query methods
    //
    
    
    // Perform binary search on symbols to find corresponding entry.
    // Complexity O(log n) where n is the number of symbol table entries
    //
    bool
    SymbolTable::get_symbol(uint32 addr, std::string& name)
    {
      if (tab_size_ == 0)
        return false;
      
      int low, high, mid;
      low  = 0;
      high = tab_size_ - 1; 
      
      while (low <= high) {
        mid = (low + high) / 2;
        SymEntry* p = tab_ + mid;
        if (addr < p->entry->st_value) {
          high = mid - 1;
        } else if (addr >= p->entry->st_value + p->entry->st_size) {
          low = mid + 1;
        } else {            // FOUND MATCH
          Elf32_Addr    val;
          Elf32_Word    siz;
          Elf32_Half    sec;
          unsigned char bind, type;
          elf_tab_->GetSymbol(p->index, name, val, siz, bind, type, sec);
          return true;
        }
      }
      return false;
    }

    
    
} } // arcsim::util
