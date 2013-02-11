//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Profiling counters API.
//
// =============================================================================

#include "api/prof/api_prof.h"

#include "ioc/ContextItemInterface.h"
#include "ioc/Context.h"
#include "ioc/ContextItemId.h"

#include "util/Counter.h"
#include "util/Log.h"

#define CHECK_ITEM(_item_,_fun_,_ret_)                                         \
if (_item_ == 0) { LOG(LOG_ERROR) << "[API-PROF] '" << (char * const)_fun_      \
                                  << "': passed IocContextItem is 0.";          \
                   return (_ret_);                                              \
                 }

#define CHECK_ITEM_TYPE(_item_,_fun_,_type_,_ret_)                             \
if (_item_->get_type() != _type_) {                                             \
  LOG(LOG_ERROR) << "[API-PROF] '" << (char * const)_fun_                       \
                 << "': passed IocContextItem has wrong type.";                 \
  return (_ret_);                                                               \
}

#define ITEM(_item_) (reinterpret_cast<arcsim::ioc::ContextItemInterface*>(_item_))

uint32
profCounter32GetValue(IocContextItem   item)
{
  CHECK_ITEM(item, "profCounter32GetValue", 0);
  CHECK_ITEM_TYPE(ITEM(item), "profCounter32GetValue",
                   arcsim::ioc::ContextItemInterface::kTCounter, 0);
  return static_cast<arcsim::util::Counter*>(ITEM(item))->get_value();
}

uint64
profCounter64GetValue(IocContextItem   item)
{
  CHECK_ITEM(item, "profCounter64GetValue", 0);
  CHECK_ITEM_TYPE(ITEM(item), "profCounter64GetValue",
                   arcsim::ioc::ContextItemInterface::kTCounter64, 0);
  return static_cast<arcsim::util::Counter64*>(ITEM(item))->get_value();
}

