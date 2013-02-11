//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Everything that should be managed by our minimalistic and light-weight
// Dependency Injection (DI) framework must implement the ContextItemInterface.
//
// =====================================================================


#ifndef INC_DI_CONTEXTITEMINTERFACE_H_
#define INC_DI_CONTEXTITEMINTERFACE_H_

#include "api/types.h"

namespace arcsim {
  namespace ioc {
    
    // -------------------------------------------------------------------------
    // All managed Items must implement this interface.
    //
    class ContextItemInterface
    {
    protected:
      // Constructor MUST be protected and empty!
      //
      ContextItemInterface() { /* EMPTY */ }
      
    public:
      
      // List of managed dependency injected Items
      //
      enum Type {
        kTCounter,
        kTCounter64,
        kTCounterTimer,
        kTHistogram,
        kTMultiHistogram,
        kTSymbolTable,
        kTIPTManager,
        kTPhysicalProfile,
        kTProcessor
      };
      
      // Destructor MUST be declared AND virtual so all implementations of
      // this interface can be destroyed correctly.
      //
      virtual ~ContextItemInterface() { /* EMPTY */ }
      
      // -------------------------------------------------------------------------
      // Interface methods
      //
      virtual const uint8* get_name() const = 0;
      virtual const Type   get_type() const = 0;
    };
    
    // -------------------------------------------------------------------------
    // ContextItemInterfaceComparator implementation comparing ContextItems based
    // on their name
    //
    struct ContextItemInterfaceComparator
    {
      bool operator() (const ContextItemInterface* lhs,
                       const ContextItemInterface* rhs) const;
    };
    
    
  } } // namespace arcsim::ioc

#endif  // INC_DI_CONTEXTITEMINTERFACE_H_
