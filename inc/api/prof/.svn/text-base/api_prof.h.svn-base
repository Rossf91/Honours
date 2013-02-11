//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Profiling counters API.
//
// Values of profiling IocContextItems that have been extracted from an IocContext
// can be accessed via the following API functions.
// 
// Example Code:
//    
//    /*  Retrieve system context with ID 0 from global context */
//    IocContext sys_ctx = iocGetContext(iocGetGlobalContext(), 0);
//    /* Retrieve module context with ID 0 */
//    IocContext mod_ctx = iocGetContext(sys_ctx, 0);
//    /* Retrieve processor context with ID 0 */
//    IocContext cpu_ctx = iocGetContext(mod_ctx, 0);
//
//    /* Retrieve interpreted instruction counter IocContextItem */
//    IocContextItem item = iocContextGetItem(cpu_ctx, 
//                                            kIocContextItemInterpretedInstructionCount64);
//
//    /* Extract counter value */
//    uint64 interp_cnt64 = profCounter64GetValue(item);
//
// =============================================================================

#ifndef INC_API_PROF_API_PROF_H_
#define INC_API_PROF_API_PROF_H_

#include "api/types.h"
#include "api/ioc/api_ioc.h"

#ifdef __cplusplus
extern "C" {
#endif
  
  // ---------------------------------------------------------------------------
  // API functions used to retrieve IocContextItem Counter values
  //
  DLLEXPORT uint32 profCounter32GetValue(IocContextItem   item);
  DLLEXPORT uint64 profCounter64GetValue(IocContextItem   item);
  
#ifdef __cplusplus
}
#endif

#endif  // INC_API_PROF_API_PROF_H_

