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

#include "Assertion.h"

#include "ioc/ContextItemInterfaceFactory.h"

#include "util/Counter.h"
#include "util/CounterTimer.h"
#include "util/Histogram.h"
#include "util/SymbolTable.h"
#include "util/MultiHistogram.h"
#include "ipt/IPTManager.h"
#include "profile/PhysicalProfile.h"


namespace arcsim {
  namespace ioc {
    
    ContextItemInterface*
    ContextItemInterfaceFactory::create(Context&                    ctx,
                                        ContextItemInterface::Type  type,
                                        const char*                 name)
    {
      ContextItemInterface* item = 0;
      switch (type) {
        case ContextItemInterface::kTCounter:         item = new arcsim::util::Counter(name);         break;
        case ContextItemInterface::kTCounter64:       item = new arcsim::util::Counter64(name);       break;
        case ContextItemInterface::kTCounterTimer:    item = new arcsim::util::CounterTimer(name);    break;
        case ContextItemInterface::kTHistogram:       item = new arcsim::util::Histogram(name);       break;
        case ContextItemInterface::kTMultiHistogram:  item = new arcsim::util::MultiHistogram(name);  break;
        case ContextItemInterface::kTSymbolTable:     item = new arcsim::util::SymbolTable(name);     break;
        case ContextItemInterface::kTIPTManager:      item = new arcsim::ipt::IPTManager(ctx, name);  break;
        case ContextItemInterface::kTPhysicalProfile: item = new arcsim::profile::PhysicalProfile(ctx, name); break;
        case ContextItemInterface::kTProcessor: {
          UNIMPLEMENTED1("[ContextItemInterfaceFactory] Context instantiation of Processor is not supported yet!");
        }
      }
      return item;
    }
    
  } } // namespace arcsim::ioc
