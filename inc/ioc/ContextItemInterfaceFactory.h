//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Central factory responsible for creating instances of ContextItemInterfaces
// that are then used by our Dependency Injection (DI) framework.
//
// This factory is the only place that actually needs to know how instances
// of ContextItemInterfaces are constructed.
//
// =====================================================================


#ifndef INC_DI_CONTEXTITEMINTERFACEFACTORY_H_
#define INC_DI_CONTEXTITEMINTERFACEFACTORY_H_

#include "api/types.h"

#include "ioc/ContextItemInterface.h"

namespace arcsim {
  namespace ioc {
    
    // ------------------------------------------------------------------------
    // Forward declaration
    //
    class Context;
    
    // ------------------------------------------------------------------------
    // THE main factory for creating instances of DI managed ContextItemInterfaces.
    //
    class ContextItemInterfaceFactory
    {
    private:
      // private constructor to prevent anyone from creating instances
      //
      ContextItemInterfaceFactory() { /* EMPTY */ };
      
      // Disallow copy and assign
      //
      ContextItemInterfaceFactory(const ContextItemInterfaceFactory&);
      void operator=(const ContextItemInterfaceFactory&);
      
    public:
      
      static ContextItemInterface* create(Context&                    ctx,
                                          ContextItemInterface::Type  type,
                                          const char*                 name);
      
    };
    
  } } // namespace arcsim::ioc

#endif  // INC_DI_CONTEXTITEMINTERFACEFACTORY_H_
