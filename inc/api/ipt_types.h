//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Opaque Handlers and types for Instrumentation PoinTs (IPTs).
//
// =============================================================================

#ifndef INC_IPT_TYPES_H_
#define INC_IPT_TYPES_H_

#include "api/ioc_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define API_IPT_REGISTER_FAILURE 0x0  
#define API_IPT_REGISTER_SUCCESS 0x1

// -------------------------------------------------------------------------
// HandleAboutToExecuteInstruction callback function and instance type
//
DECLARE_HANDLE(HandleAboutToExecuteInstructionObj);

typedef int  (*HandleAboutToExecuteInstructionFun)( /* Processor IocContext */
                                                    IocContext                         ctx,
                                                    /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                                    IocContextItem                     ipt,
                                                    /* Object instance (for example Virtualizer 'AboutToExecuteInstructionIPT.h') that has been registered */
                                                    HandleAboutToExecuteInstructionObj obj,
                                                    /* Instruction address that triggered HandleAboutToExecuteInstructionIPT */
                                                    uint32                             addr);

// -------------------------------------------------------------------------
// HandleBeginInstructionExecution callback function and instance type
//
DECLARE_HANDLE(HandleBeginInstructionExecutionObj);

typedef void (*HandleBeginInstructionExecutionFun)(/* Processor IocContext */
                                                   IocContext                         ctx,
                                                   /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                                   IocContextItem                     ipt,
                                                   /* Object instance (for example Virtualizer 'HandleBeginInstructionExecutionIPT.h') that has been registered */
                                                   HandleBeginInstructionExecutionObj obj,
                                                   /* Instruction address that triggered HandleBeginInstructionExecutionIPT */
                                                   uint32                             addr,
                                                   /* The length of the instruction (in bytes) */ 
                                                   uint32                             len);

// -------------------------------------------------------------------------
// HandleEndInstructionExecution callback function and instance type
//
DECLARE_HANDLE(HandleEndInstructionExecutionObj);

typedef void (*HandleEndInstructionExecutionFun)(/* Processor IocContext */
                                                IocContext                         ctx,
                                                /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                                IocContextItem                     ipt,
                                                /* Object instance (for example Virtualizer 'HandleEndInstructionExecutionIPT.h') that has been registered */
                                                HandleEndInstructionExecutionObj   obj,
                                                /* Instruction address that triggered HandleEndInstructionExecutionIPT */
                                                uint32                             addr,
                                                /* The number of cycles it took to execute this instruction */ 
                                                uint64                             cycles);

// -------------------------------------------------------------------------
// HandleBeginBasicBlock callback function and instance type
//
DECLARE_HANDLE(HandleBeginBasicBlockObj);

typedef void (*HandleBeginBasicBlockFun)(/* Processor IocContext */
                                         IocContext                         ctx,
                                         /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                         IocContextItem                     ipt,
                                         /* Object instance (for example Virtualizer 'HandleBeginBasicBlockIF.h') that has been registered */
                                         HandleBeginBasicBlockObj           obj,
                                         /* Basic block start address */
                                         uint32                             addr);

  
#ifdef __cplusplus
}
#endif

#endif  // INC_IPT_TYPES_H_
