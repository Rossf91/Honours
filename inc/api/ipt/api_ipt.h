//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Instrumentation PoinTs (IPT) API.
//
// An IPT IocContextItems that has been extracted from an IocContext can be
// accessed via the following API functions.
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
//    /* Retrieve IPT IocContextItem */
//    IocContextItem ipt = iocContextGetItem(cpu_ctx, 
//                                           kIocContextItemIPTManagerID);
//
//    /* REGISTER IPT */
//    int ret = iptInsertAboutToExecuteInstructionIpt(ipt,
//                                                    0,
//                                                    AboutToExecuteInstructionHandler,
//                                                    0x00000004);
//
// =============================================================================

#ifndef INC_API_IPT_API_IPT_H_
#define INC_API_IPT_API_IPT_H_

#include "api/ipt_types.h"
#include "api/ioc/api_ioc.h"

#ifdef __cplusplus
extern "C" {
#endif
    
  // ---------------------------------------------------------------------------
  // API for AboutToExecuteInstructionIPT registration
  //
  
  // Register - AboutToExecuteInstructionIPT, returns API_IPT_REGISTER_SUCCESS
  // when successful, otherwise API_IPT_REGISTER_FAILURE
  DLLEXPORT int iptInsertAboutToExecuteInstructionIpt(
                                              /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                              IocContextItem                     item,
                                              /* Object instance (for example Virtualizer 'AboutToExecuteInstructionIPT.h') this parameter may be 0 */
                                              HandleAboutToExecuteInstructionObj obj,
                                              /* Callback function executed before instruction is executed  */
                                              HandleAboutToExecuteInstructionFun fun,
                                              /* Instruction address for which to call HandleAboutToExecuteInstructionFun  */
                                              uint32                             addr);

  // Remove - AboutToExecuteInstructionIPT, returns API_IPT_REGISTER_SUCCESS
  // when successful, otherwise API_IPT_REGISTER_FAILURE
  DLLEXPORT int iptRemoveAboutToExecuteInstructionIpt(
                                              /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                              IocContextItem             item,
                                              /* Instruction address for which to remove callback execution  */
                                              uint32                     addr);

  // Remove specific subscriber for AboutToExecuteInstructionIPT, returns API_IPT_REGISTER_SUCCESS
  // when successful, otherwise API_IPT_REGISTER_FAILURE
  DLLEXPORT int iptRemoveAboutToExecuteInstructionIptSubscriber(
                                              /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                              IocContextItem             item,
                                              /* Object instance (for example Virtualizer 'AboutToExecuteInstructionIPT.h') this parameter may be 0 */
                                              HandleAboutToExecuteInstructionObj obj,
                                              /* Callback function executed before instruction is executed  */
                                              HandleAboutToExecuteInstructionFun fun,
                                              /* Instruction address for which to remove callback execution  */
                                              uint32                     addr);

  // ---------------------------------------------------------------------------
  // API for HandleBeginInstructionExecutionIPT registration
  //
  
  // Register - HandleBeginInstructionExecutionIPT, returns API_IPT_REGISTER_SUCCESS
  // when successful, otherwise API_IPT_REGISTER_FAILURE
  DLLEXPORT int iptInsertBeginInstructionExecutionIpt(
                                              /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                              IocContextItem                     item,
                                              /* Object instance (for example Virtualizer 'BeginInstructionExecutionIPT.h') this parameter may be 0 */
                                              HandleBeginInstructionExecutionObj obj,
                                              /* Callback function executed before instruction is executed  */
                                              HandleBeginInstructionExecutionFun fun);
  
  // Remove - HandleBeginInstructionExecutionIPT, returns API_IPT_REGISTER_SUCCESS
  // when successful, otherwise API_IPT_REGISTER_FAILURE
  DLLEXPORT int iptRemoveBeginInstructionExecutionIpt(
                                              /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                              IocContextItem             item);
  
  // Remove specific subscriber for HandleBeginInstructionExecutionIPT, returns API_IPT_REGISTER_SUCCESS
  // when successful, otherwise API_IPT_REGISTER_FAILURE
  DLLEXPORT int iptRemoveBeginInstructionExecutionIptSubscriber(
                                              /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                              IocContextItem                     item,
                                              /* Object instance (for example Virtualizer 'BeginInstructionExecutionIPT.h') this parameter may be 0 */
                                              HandleBeginInstructionExecutionObj obj,
                                              /* Callback function executed before instruction is executed  */
                                              HandleBeginInstructionExecutionFun fun);

  
  // ---------------------------------------------------------------------------
  // API for HandleBeginBasicBlockInstructionIPT registration
  //
  
  // Register - HandleBeginBasicBlockInstructionIPT, returns API_IPT_REGISTER_SUCCESS
  // when successful, otherwise API_IPT_REGISTER_FAILURE
  DLLEXPORT int iptInsertBeginBasicBlockInstructionIpt(
                                              /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                              IocContextItem           item,
                                              /* Object instance (for example Virtualizer 'BeginBasicBlockIPT.h') this parameter may be 0 */
                                              HandleBeginBasicBlockObj obj,
                                              /* Callback function executed before first basic block instruction is executed  */
                                              HandleBeginBasicBlockFun fun);
  
  // Remove - HandleBeginBasicBlockInstructionIPT, returns API_IPT_REGISTER_SUCCESS
  // when successful, otherwise API_IPT_REGISTER_FAILURE
  DLLEXPORT int iptRemoveBeginBasicBlockInstructionIpt(
                                              /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                              IocContextItem             item);
  
  // Remove specific subscriber for HandleBeginBasicBlockInstructionIPT, returns API_IPT_REGISTER_SUCCESS
  // when successful, otherwise API_IPT_REGISTER_FAILURE
  DLLEXPORT int iptRemoveBeginBasicBlockInstructionIptSubscriber(
                                              /* IocContextItem retrieved using kIocContextItemIPTManagerID */
                                              IocContextItem           item,
                                              /* Object instance (for example Virtualizer 'BeginBasicBlockIPT.h') this parameter may be 0 */
                                              HandleBeginBasicBlockObj obj,
                                              /* Callback function executed before first basic block instruction is executed */
                                              HandleBeginBasicBlockFun fun);

#ifdef __cplusplus
}
#endif

#endif  // INC_API_IPT_API_IPT_H_

