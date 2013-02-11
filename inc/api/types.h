//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description:
//
// This file defines basic type definitions, compiler-independent macros to
// provide declaration specifiers for alignment, name exporting, volatility,
// and any other non-standard language features.
//
// =====================================================================

#ifndef INC_API_TYPES_H_
#define INC_API_TYPES_H_

// Include types defined in 'types.def'
//
#include "api/types.def"

#ifdef __cplusplus
extern "C" {
#endif

// Expand base types
//
T_TYPES_BASE

// -----------------------------------------------------------------------------
// Opaque handle to simulation context
//  
DECLARE_HANDLE(simContext);

// -----------------------------------------------------------------------------
// Opaque handle to processor context
//  
DECLARE_HANDLE(cpuContext);
  
#ifdef __cplusplus
}
#endif

#endif /* INC_API_TYPES_H_ */
