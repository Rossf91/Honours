//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Inversion of Control (IOC) container subsystem API.
//
// =============================================================================

#include "api/ioc/api_ioc.h"

#include "ioc/Context.h"
#include "ioc/ContextItemId.h"

#include "util/Log.h"

#define ASSERT_CONTEXT(_ctx_,_fun_,_ret_)                                       \
  if (_ctx_ == 0) { LOG(LOG_ERROR) << "[API-IOC] '" << (char * const)_fun_      \
                                   << "': passed IocContext is 0.";             \
                    return (_ret_);\
                  }

#define CONTEXT(_ctx_) (reinterpret_cast<arcsim::ioc::Context*>(_ctx_))

// ---------------------------------------------------------------------------
// IOC context item identifiers available to external agents via this API
//
#define ID_NS arcsim::ioc::ContextItemId
const char* kIocContextItemIPTManagerID                    = ID_NS::kIPTManager;
const char* kIocContextItemInterpretedInstructionCount64ID = ID_NS::kInterpInstCount64;
const char* kIocContextItemNativeInstructionCount64ID      = ID_NS::kNativeInstCount64;
const char* kIocContextItemCycleCount64ID                  = ID_NS::kCycleCount64;
// NOTE: expand this list with other context item IDs we may want to export


// -----------------------------------------------------------------------------
// Retrieve global context
//
IocContext
iocGetGlobalContext()
{
  return reinterpret_cast<IocContext>(arcsim::ioc::Context::GlobalPtr());
}

// -----------------------------------------------------------------------------
// Retrieve sub-context based on identifier
//
IocContext
iocGetContext(IocContext ctx, uint32 ctx_id)
{
  ASSERT_CONTEXT(ctx, "iocGetContext", 0);
  return reinterpret_cast<IocContext>(CONTEXT(ctx)->get_context(ctx_id));
}

// ---------------------------------------------------------------------------
// API functions for context query
//
uint32
iocContextGetId(IocContext ctx)
{
  ASSERT_CONTEXT(ctx, "iocContextGetId", 0);
  return CONTEXT(ctx)->get_id();
}

uint32
iocContextGetLevel(IocContext ctx)
{
  ASSERT_CONTEXT(ctx, "iocContextGetLevel", 0);
  return CONTEXT(ctx)->get_level();
}

const uint8*
iocContextGetName(IocContext ctx)
{
  ASSERT_CONTEXT(ctx, "iocContextGetName", 0);
  return CONTEXT(ctx)->get_name();
}

IocContext
iocContextGetParent(IocContext ctx)
{
  ASSERT_CONTEXT(ctx, "iocContextGetParent", 0);
  return reinterpret_cast<IocContext>(CONTEXT(ctx)->get_parent());
}

// ---------------------------------------------------------------------------
// API functions for IOC context item retrieval and manipulation
//

// Retrieve IocContextItem from context, returns '0' if item does not exist
IocContextItem
iocContextGetItem(IocContext ctx, const char* item_name)
{
  ASSERT_CONTEXT(ctx, "iocContextGetItem", 0);
  return reinterpret_cast<IocContextItem>(CONTEXT(ctx)->get_item(item_name));
}




