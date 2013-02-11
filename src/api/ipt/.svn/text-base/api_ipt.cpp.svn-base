//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
// Description: Instrumentation PoinTs (IPT) API.
//
// =============================================================================

#include "api/ipt/api_ipt.h"

#include "ioc/ContextItemInterface.h"
#include "ioc/Context.h"
#include "ioc/ContextItemId.h"

#include "ipt/IPTManager.h"
#include "util/Log.h"

#define CHECK_ITEM(_item_,_fun_,_ret_)                                         \
if (_item_ == 0) { LOG(LOG_ERROR) << "[API-IPT] '" << (char * const)_fun_       \
                                  << "': passed IocContextItem is 0.";          \
                   return (_ret_);                                              \
}

#define CHECK_ITEM_TYPE(_item_,_fun_,_type_,_ret_)                             \
if (_item_->get_type() != _type_) {                                             \
  LOG(LOG_ERROR) << "[API-IPT] '" << (char * const)_fun_                        \
                 << "': passed IocContextItem has wrong type.";                 \
  return (_ret_);                                                               \
}

#define ITEM(_item_) (reinterpret_cast<arcsim::ioc::ContextItemInterface*>(_item_))
#define IPT(_item_)  (static_cast<arcsim::ipt::IPTManager*>(_item_))

// ---------------------------------------------------------------------------
// API for AboutToExecuteInstructionIPT registration
//

int
iptInsertAboutToExecuteInstructionIpt(IocContextItem                     item,
                                      HandleAboutToExecuteInstructionObj obj,
                                      HandleAboutToExecuteInstructionFun fun,
                                      uint32                             addr)
{
  CHECK_ITEM(item, "iptRegisterAboutToExecuteInstructionIpt", API_IPT_REGISTER_FAILURE);
  CHECK_ITEM_TYPE(ITEM(item), "iptRegisterAboutToExecuteInstructionIpt",
                   arcsim::ioc::ContextItemInterface::kTIPTManager, API_IPT_REGISTER_FAILURE)
  
  bool success = IPT(ITEM(item))->insert_about_to_execute_instruction_ipt(addr, obj, fun);
  return (success) ? API_IPT_REGISTER_SUCCESS : API_IPT_REGISTER_FAILURE;
}


int
iptRemoveAboutToExecuteInstructionIpt(IocContextItem                     item,
                                      uint32                             addr)
{
  CHECK_ITEM(item, "iptRemoveAboutToExecuteInstructionIpt", API_IPT_REGISTER_FAILURE);
  CHECK_ITEM_TYPE(ITEM(item), "iptRemoveAboutToExecuteInstructionIpt",
                   arcsim::ioc::ContextItemInterface::kTIPTManager, API_IPT_REGISTER_FAILURE)
  
  bool success = IPT(ITEM(item))->remove_about_to_execute_instruction_ipt(addr);
  return (success) ? API_IPT_REGISTER_SUCCESS : API_IPT_REGISTER_FAILURE;
}

// Remove specific subscriber for AboutToExecuteInstructionIPT, returns API_IPT_REGISTER_SUCCESS
// when successful, otherwise API_IPT_REGISTER_FAILURE
int
iptRemoveAboutToExecuteInstructionIptSubscriber(IocContextItem                     item,
                                                HandleAboutToExecuteInstructionObj obj,
                                                HandleAboutToExecuteInstructionFun fun,
                                                uint32                             addr)
{
  CHECK_ITEM(item, "iptRemoveAboutToExecuteInstructionIptSubscriber", API_IPT_REGISTER_FAILURE);
  CHECK_ITEM_TYPE(ITEM(item), "iptRemoveAboutToExecuteInstructionIptSubscriber",
                   arcsim::ioc::ContextItemInterface::kTIPTManager, API_IPT_REGISTER_FAILURE)

  bool success = IPT(ITEM(item))->remove_about_to_execute_instruction_ipt(addr, obj, fun);
  return (success) ? API_IPT_REGISTER_SUCCESS : API_IPT_REGISTER_FAILURE;
}

// ---------------------------------------------------------------------------
// API for HandleBeginInstructionExecutionIPT registration
//


// Register - HandleBeginInstructionExecutionIPT, returns API_IPT_REGISTER_SUCCESS
// when successful, otherwise API_IPT_REGISTER_FAILURE
int iptInsertBeginInstructionExecutionIpt(IocContextItem                     item,
                                          HandleBeginInstructionExecutionObj obj,
                                          HandleBeginInstructionExecutionFun fun)
{
  CHECK_ITEM(item, "iptInsertBeginInstructionExecutionIpt", API_IPT_REGISTER_FAILURE);
  CHECK_ITEM_TYPE(ITEM(item), "iptInsertBeginInstructionExecutionIpt",
                   arcsim::ioc::ContextItemInterface::kTIPTManager, API_IPT_REGISTER_FAILURE)
  bool success = IPT(ITEM(item))->insert_begin_instruction_execution_ipt(obj, fun);
  return (success) ? API_IPT_REGISTER_SUCCESS : API_IPT_REGISTER_FAILURE;
}

// Remove - HandleBeginInstructionExecutionIPT, returns API_IPT_REGISTER_SUCCESS
// when successful, otherwise API_IPT_REGISTER_FAILURE
int iptRemoveBeginInstructionExecutionIpt(IocContextItem             item)
{
  CHECK_ITEM(item, "iptRemoveBeginInstructionExecutionIpt", API_IPT_REGISTER_FAILURE);
  CHECK_ITEM_TYPE(ITEM(item), "iptRemoveBeginInstructionExecutionIpt",
                   arcsim::ioc::ContextItemInterface::kTIPTManager, API_IPT_REGISTER_FAILURE)
  bool success = IPT(ITEM(item))->remove_begin_instruction_execution_ipt();
  return (success) ? API_IPT_REGISTER_SUCCESS : API_IPT_REGISTER_FAILURE;
}

// Remove specific subscriber for HandleBeginInstructionExecutionIPT, returns API_IPT_REGISTER_SUCCESS
// when successful, otherwise API_IPT_REGISTER_FAILURE
int iptRemoveBeginInstructionExecutionIptSubscriber(IocContextItem                     item,
                                                    HandleBeginInstructionExecutionObj obj,
                                                    HandleBeginInstructionExecutionFun fun)
{
  CHECK_ITEM(item, "iptRemoveBeginInstructionExecutionIptSubscriber", API_IPT_REGISTER_FAILURE);
  CHECK_ITEM_TYPE(ITEM(item), "iptRemoveBeginInstructionExecutionIptSubscriber",
                   arcsim::ioc::ContextItemInterface::kTIPTManager, API_IPT_REGISTER_FAILURE)
  bool success = IPT(ITEM(item))->remove_begin_instruction_execution_ipt(obj, fun);
  return (success) ? API_IPT_REGISTER_SUCCESS : API_IPT_REGISTER_FAILURE;
}

// ---------------------------------------------------------------------------
// API for HandleBeginBasicBlockInstructionIPT registration
//

// Register - HandleBeginBasicBlockInstructionIPT, returns API_IPT_REGISTER_SUCCESS
// when successful, otherwise API_IPT_REGISTER_FAILURE
int iptInsertBeginBasicBlockInstructionIpt(IocContextItem           item,
                                           HandleBeginBasicBlockObj obj,
                                           HandleBeginBasicBlockFun fun)
{
  CHECK_ITEM(item, "iptInsertBeginBasicBlockInstructionIpt", API_IPT_REGISTER_FAILURE);
  CHECK_ITEM_TYPE(ITEM(item), "iptInsertBeginBasicBlockInstructionIpt",
                  arcsim::ioc::ContextItemInterface::kTIPTManager, API_IPT_REGISTER_FAILURE)
  bool success = IPT(ITEM(item))->insert_begin_basic_block_instruction_execution_ipt(obj, fun);
  return (success) ? API_IPT_REGISTER_SUCCESS : API_IPT_REGISTER_FAILURE;
 
}

// Remove - HandleBeginBasicBlockInstructionIPT, returns API_IPT_REGISTER_SUCCESS
// when successful, otherwise API_IPT_REGISTER_FAILURE
int iptRemoveBeginBasicBlockInstructionIpt(IocContextItem             item)
{
  CHECK_ITEM(item, "iptRemoveBeginBasicBlockInstructionIpt", API_IPT_REGISTER_FAILURE);
  CHECK_ITEM_TYPE(ITEM(item), "iptRemoveBeginBasicBlockInstructionIpt",
                  arcsim::ioc::ContextItemInterface::kTIPTManager, API_IPT_REGISTER_FAILURE)
  bool success = IPT(ITEM(item))->remove_begin_basic_block_instruction_execution_ipt();
  return (success) ? API_IPT_REGISTER_SUCCESS : API_IPT_REGISTER_FAILURE;
}

// Remove specific subscriber for HandleBeginBasicBlockInstructionIPT, returns API_IPT_REGISTER_SUCCESS
// when successful, otherwise API_IPT_REGISTER_FAILURE
int iptRemoveBeginBasicBlockInstructionIptSubscriber(IocContextItem           item,
                                                     HandleBeginBasicBlockObj obj,
                                                     HandleBeginBasicBlockFun fun)
{
  CHECK_ITEM(item, "iptRemoveBeginBasicBlockInstructionIptSubscriber", API_IPT_REGISTER_FAILURE);
  CHECK_ITEM_TYPE(ITEM(item), "iptRemoveBeginBasicBlockInstructionIptSubscriber",
                  arcsim::ioc::ContextItemInterface::kTIPTManager, API_IPT_REGISTER_FAILURE)
  bool success = IPT(ITEM(item))->remove_begin_basic_block_instruction_execution_ipt(obj, fun);
  return (success) ? API_IPT_REGISTER_SUCCESS : API_IPT_REGISTER_FAILURE;
}

