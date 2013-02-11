//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Inversion of Control (IOC) container subsystem API.
//
// A Context holds all Items that have been created via appropriate
// factory methods (i.e. it is a container for manged objects).
//
// All Items in a Context are Singletons identified by a unique name within
// that Context. Contexts are organised in a Hierarchy. The 'root' context
// at the top of the Hierarchy can be accessed by the 'iocGetGlobalContext()'
// API function.
//
// Example Code:
//    
//    // Retrieve system context with ID 0 from global context
//    IocContext sys_ctx = iocGetContext(iocGetGlobalContext(), 0);
//    // Retrieve module context with ID 0
//    IocContext mod_ctx = iocGetContext(sys_ctx, 0);
//    // Retrieve processor context with ID 0
//    IocContext cpu_ctx = iocGetContext(mod_ctx, 0);
//
// =============================================================================

#ifndef INC_API_IOC_API_IOC_H_
#define INC_API_IOC_API_IOC_H_

#include "api/ioc_types.h"

#ifdef __cplusplus
extern "C" {
#endif
  
  // ---------------------------------------------------------------------------
  // IOC context item identifiers available to external agents via this API
  //
  extern const char* kIocContextItemIPTManagerID;
  extern const char* kIocContextItemInterpretedInstructionCount64ID;
  extern const char* kIocContextItemNativeInstructionCount64ID;
  extern const char* kIocContextItemCycleCount64ID;
  // NOTE: expand this list with other context item IDs we may want to export
  
  // ---------------------------------------------------------------------------
  // API functions for context retrieval
  //  

  // Retrieve global context
  DLLEXPORT IocContext iocGetGlobalContext();
  
  // Retrieve sub-context based on identifier
  DLLEXPORT IocContext iocGetContext(IocContext ctx,
                                     uint32     ctx_id);
  
  // ---------------------------------------------------------------------------
  // API functions to query IOC context
  //
  DLLEXPORT uint32         iocContextGetId(IocContext ctx);
  DLLEXPORT uint32         iocContextGetLevel(IocContext ctx);
  DLLEXPORT const uint8*   iocContextGetName(IocContext ctx);
  DLLEXPORT IocContext     iocContextGetParent(IocContext ctx);

  // ---------------------------------------------------------------------------
  // API functions for IOC context item retrieval and manipulation
  //
  
  // Retrieve IocContextItem from context, returns '0' if item does not exist
  DLLEXPORT IocContextItem   iocContextGetItem(IocContext   ctx,
                                               const char*  item_name);

  
#ifdef __cplusplus
}
#endif

  
#endif  // INC_API_IOC_API_IOC_H_
