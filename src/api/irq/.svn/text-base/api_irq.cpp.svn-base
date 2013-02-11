//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#include "api/irq/api_irq.h"

#include "ioc/ContextItemInterface.h"
#include "ioc/Context.h"
#include "ioc/ContextItemId.h"

#include "sys/cpu/processor.h"

#include "util/Log.h"

#define CTX(_ctx_) (reinterpret_cast<arcsim::ioc::Context*>(_ctx_))

#define CHECK_CTX(_ctx_,_fun_,_ret_)                                            \
  if (_ctx_ == 0) { LOG(LOG_ERROR) << "[API-IRQ] '" << (char * const)_fun_      \
                                   << ": passed IocContext is 0.";              \
    return (_ret_);                                                             \
  }

#define CHECK_CTX_TYPE(_ctx_,_fun_,_ret_)                                       \
  if (CTX(_ctx_)->get_level() != arcsim::ioc::Context::kLProcessor) {           \
    LOG(LOG_ERROR) << "[API-IRQ] '" << (char * const)_fun_                      \
                                    << "': passed IocContext has wrong type.";  \
    return (_ret_);                                                             \
  }



int
irqAssertInterrupt (IocContext cpu_ctx, int num)
{
  CHECK_CTX(cpu_ctx, "irqAssertInterrupt", API_IRQ_FAILURE);
  CHECK_CTX_TYPE(cpu_ctx, "irqAssertInterrupt", API_IRQ_FAILURE);
  
  // retrieve processor instance from context
  arcsim::ioc::ContextItemInterface* const item = CTX(cpu_ctx)->get_item(arcsim::ioc::ContextItemId::kProcessor);
  arcsim::sys::cpu::Processor* const cpu = static_cast<arcsim::sys::cpu::Processor*>(item);
    
  // assert requested interrupt, thereby notifying the CPU that it should react
  cpu->assert_interrupt_line(num);
  
  LOG(LOG_DEBUG1) << "[API-IRQ] 'irqAssertInterrupt': asserting IRQ '" << num << "'.";
  
  return API_IRQ_SUCCESS;
}

int
irqRescindInterrupt(IocContext cpu_ctx, int num)
{
  CHECK_CTX(cpu_ctx, "irqRescindInterrupt", API_IRQ_FAILURE);
  CHECK_CTX_TYPE(cpu_ctx, "irqRescindInterrupt", API_IRQ_FAILURE);
  
  // retrieve processor instance from context
  arcsim::ioc::ContextItemInterface* item = CTX(cpu_ctx)->get_item(arcsim::ioc::ContextItemId::kProcessor);
  arcsim::sys::cpu::Processor* cpu = static_cast<arcsim::sys::cpu::Processor*>(item);
    
  // rescind requested interrupt
  cpu->rescind_interrupt_line(num);
  
  LOG(LOG_DEBUG1) << "[API-IRQ] 'irqRescindInterrupt': rescinding IRQ '" << num << "'.";

  return API_IRQ_SUCCESS;
}

