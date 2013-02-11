//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Opaque handles and types for IoC (Inversion of Control) container.
//
// =====================================================================


#ifndef INC_IOC_TYPES_H_
#define INC_IOC_TYPES_H_

#include "api/types.h"

#ifdef __cplusplus
extern "C" {
#endif
  
// ---------------------------------------------------------------------------
// Opaque handle to IOC container context
//  
DECLARE_HANDLE(IocContext);

// ---------------------------------------------------------------------------
// Opaque handle to IOC container context item
//  
DECLARE_HANDLE(IocContextItem);

#ifdef __cplusplus
}
#endif

#endif  // INC_IOC_TYPES_H_
