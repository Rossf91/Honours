//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: API for asserting/rescinding processor interrupts.
//
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
//    /* Assert IRQ 3 on processor context */
//    irqAssertInterrupt (cpu_ctx, 3);
//    /* Rescind IRQ 3 on processor context */
//    irqRescindInterrupt(cpu_ctx, 3);
//
// =============================================================================

#ifndef INC_API_IRQ_API_IRQ_H_
#define INC_API_IRQ_API_IRQ_H_

#include "api/ioc/api_ioc.h"

#ifdef __cplusplus
extern "C" {
#endif
  
#define API_IRQ_FAILURE 0x0  
#define API_IRQ_SUCCESS 0x1
  
  DLLEXPORT int irqAssertInterrupt (IocContext cpu_ctx, int num);
  DLLEXPORT int irqRescindInterrupt(IocContext cpu_ctx, int num);
  
#ifdef __cplusplus
}
#endif

#endif  // INC_API_IRQ_API_IRQ_H_

