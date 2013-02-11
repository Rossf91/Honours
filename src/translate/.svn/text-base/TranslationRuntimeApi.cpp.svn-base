//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2009 The University of Edinburgh
//                        All Rights Reserved
//                                             
// =====================================================================
//
// Description: JIT-Runtime callback function API.
//
// =====================================================================

#include <iomanip>
#include <set>

#ifdef REGTRACK_SIM
# include <cmath>
#endif

#include "sim_types.h"

#include "translate/TranslationRuntimeApi.h"

#include "sys/cpu/state.h"
#include "sys/cpu/processor.h"
#include "sys/mem/BlockData.h"

#include "ise/eia/EiaInstructionInterface.h"
#include "ise/eia/EiaConditionCodeInterface.h"

#include "ipt/IPTManager.h"

#include "mem/MemoryDeviceInterface.h"

#include "uarch/bpu/BranchPredictorInterface.h"

#include "util/Log.h"
#include "util/Histogram.h"

// -----------------------------------------------------------------------------
// Temporary macro for casting cpuContext and simContext into processor class.
// It provides better readability.
//
#define PROCESSOR(_cpu_) (reinterpret_cast<arcsim::sys::cpu::Processor*>(_cpu_))
#define SYSTEM(_sys_)    (reinterpret_cast<System*>(_sys_))


//DLLEXPORT void cpuEmulateTrap (cpuContext cpu);
void cpuEmulateTrap (cpuContext cpu)
{
	PROCESSOR(cpu)->emulate_trap();
}

//DLLEXPORT int  cpuReadAuxReg (cpuContext cpu, uint32 addr, uint32* data);
int cpuReadAuxReg (cpuContext cpu, uint32 addr, uint32* data)
{
	return PROCESSOR(cpu)->read_aux_register (addr, (uint32*)data, true);
}

//DLLEXPORT int  cpuWriteAuxReg (cpuContext cpu, uint32 addr, uint32 data);
int cpuWriteAuxReg (cpuContext cpu, uint32 addr, uint32 data)
{
	return PROCESSOR(cpu)->write_aux_register (addr, (uint32)data, true);
}

//DLLEXPORT void cpuRegisterLpEndJmpTarget (cpuContext cpu, uint32 lp_end_addr, uint32 lp_start_addr);
void cpuRegisterLpEndJmpTarget (cpuContext cpu, uint32 lp_end_addr, uint32 lp_start_addr)
{
	PROCESSOR(cpu)->lp_end_to_lp_start_map[lp_end_addr] = lp_start_addr;
}

//DLLEXPORT void cpuEnterException (cpuContext cpu, uint32 ecr, uint32 efa, uint32 eret);
void cpuEnterException (cpuContext cpu, uint32 ecr, uint32 efa, uint32 eret)
{
	PROCESSOR(cpu)->enter_exception (ecr, efa, eret);
}

//DLLEXPORT void cpuExitException (cpuContext cpu);
void cpuExitException (cpuContext cpu)
{
	PROCESSOR(cpu)->exit_exception ();
}

void  cpuExitInterrupt (cpuContext cpu, int irq)
{
  PROCESSOR(cpu)->exit_interrupt(irq);
}

//DLLEXPORT int cpuAtomicExchange (cpuContext cpu, uint32 addr, uint32* reg);
int cpuAtomicExchange (cpuContext cpu, uint32 addr, uint32* reg)
{
  int status = PROCESSOR(cpu)->atomic_exchange (addr, reg);
  return status;
}

// -----------------------------------------------------------------------------
// API for emulating complex CPU operations from dynamic functions
//

//DLLEXPORT void cpuHalt (cpuContext cpu);
void cpuHalt (cpuContext cpu)
{
	PROCESSOR(cpu)->halt_cpu ();
}

//DLLEXPORT void cpuBreak (cpuContext cpu);
void cpuBreak (cpuContext cpu)
{
	PROCESSOR(cpu)->break_inst ();
}

//DLLEXPORT void cpuSleep (cpuContext cpu, uint32 int_enables);
void cpuSleep (cpuContext cpu, uint32 int_enables)
{
	PROCESSOR(cpu)->sleep_inst (int_enables);
}

//DLLEXPORT void cpuSPFP (cpuContext cpu, int spfp_op, uint32 *dst, uint32 s1, uint32 s2, int flag_enb);
void cpuSPFP (cpuContext cpu, uint32 op, uint32 *dst, uint32 s1, uint32 s2, int flag_enb)
{
	PROCESSOR(cpu)->spfp_emulation (op, dst, s1, s2, flag_enb);
}

//DLLEXPORT void cpuDPFP (cpuContext cpu, int dpfp_op, uint32 *dst, uint32 s1, uint32 s2, int flag_enb);
void cpuDPFP (cpuContext cpu, uint32 op, uint32 *dst, uint32 s1, uint32 s2, int flag_enb)
{
	PROCESSOR(cpu)->dpfp_emulation (op, dst, s1, s2, flag_enb);
}

//DLLEXPORT void cpuDEXCL (cpuContext cpu, uint32 dpfp_op, uint32 *dst, uint32 s1, uint32 s2);
void cpuDEXCL (cpuContext cpu, uint32 op, uint32 *dst, uint32 s1, uint32 s2)
{
	PROCESSOR(cpu)->dexcl_emulation(op, dst, s1, s2);
}

//DLLEXPORT void cpuFlag (cpuContext cpu, uint32 opd, uint32 kflag);
void cpuFlag (cpuContext cpu, uint32 opd, uint32 kflag)
{
	PROCESSOR(cpu)->flag_inst (opd, kflag);
}

// -----------------------------------------------------------------------------
// API for JIT access to EIA extension instruction eval functions
//

//DLLEXPORT void cpuEiaDopF (cpuContext cpu, void* extn, uint32* dst, uint32 src1, uint32 src2, int f);
void
cpuEiaDopF (cpuContext cpu, void* extn, uint32* dst, uint32 src1, uint32 src2, int f)
{
  arcsim::sys::cpu::Processor* _cpu  =  PROCESSOR(cpu);
    
  arcsim::ise::eia::EiaInstructionInterface* eia_inst = 
    reinterpret_cast<arcsim::ise::eia::EiaInstructionInterface*>(extn);
  
  // Setup arguments
  // 
  arcsim::ise::eia::EiaBflags bflags = {_cpu->state.Z,  _cpu->state.N,  _cpu->state.C,  _cpu->state.V };
  arcsim::ise::eia::EiaXflags xflags = {_cpu->state.X3, _cpu->state.X2, _cpu->state.X1, _cpu->state.X0};
  
  // Call EIA instruction method evaluation
  //
  *dst = eia_inst->eval_dual_opd(src1, src2, bflags, xflags, &bflags, &xflags);
  
  // Update bflags and xflags if flag_enable is set
  //
  if (f) {
    _cpu->state.Z  = bflags.Z;
    _cpu->state.N  = bflags.N;
    _cpu->state.C  = bflags.C;
    _cpu->state.V  = bflags.V;
    _cpu->state.X3 = xflags.X3;
    _cpu->state.X2 = xflags.X2;
    _cpu->state.X1 = xflags.X1;
    _cpu->state.X0 = xflags.X0;
  }
}

//DLLEXPORT void cpuEiaDop (cpuContext cpu, void* extn, uint32* dst, uint32 src1, uint32 src2);
void
cpuEiaDop  (cpuContext cpu, void* extn, uint32* dst, uint32 src1, uint32 src2)
{
  arcsim::sys::cpu::Processor* _cpu  =  PROCESSOR(cpu);
    
  arcsim::ise::eia::EiaInstructionInterface* eia_inst = 
    reinterpret_cast<arcsim::ise::eia::EiaInstructionInterface*>(extn);
  
  // Setup arguments
  // 
  arcsim::ise::eia::EiaBflags bflags = {_cpu->state.Z,  _cpu->state.N,  _cpu->state.C,  _cpu->state.V };
  arcsim::ise::eia::EiaXflags xflags = {_cpu->state.X3, _cpu->state.X2, _cpu->state.X1, _cpu->state.X0};
  
  // Call EIA instruction method evaluation
  //
  *dst = eia_inst->eval_dual_opd(src1, src2, bflags, xflags);
}

//DLLEXPORT void cpuEiaSopF (cpuContext cpu, void* extn, uint32* dst, uint32 src1, int f);
void
cpuEiaSopF (cpuContext cpu, void* extn, uint32* dst, uint32 src1, int f)
{
  arcsim::sys::cpu::Processor* _cpu  =  PROCESSOR(cpu);
    
  arcsim::ise::eia::EiaInstructionInterface* eia_inst = 
    reinterpret_cast<arcsim::ise::eia::EiaInstructionInterface*>(extn);
  
  // Setup arguments
  // 
  arcsim::ise::eia::EiaBflags bflags = {_cpu->state.Z,  _cpu->state.N,  _cpu->state.C,  _cpu->state.V };
  arcsim::ise::eia::EiaXflags xflags = {_cpu->state.X3, _cpu->state.X2, _cpu->state.X1, _cpu->state.X0};
  
  // Call EIA instruction method evaluation
  //
  *dst = eia_inst->eval_single_opd(src1, bflags, xflags, &bflags, &xflags);
  
  // Update bflags and xflags if flag_enable is set
  //
  if (f) {
    _cpu->state.Z  = bflags.Z;
    _cpu->state.N  = bflags.N;
    _cpu->state.C  = bflags.C;
    _cpu->state.V  = bflags.V;
    _cpu->state.X3 = xflags.X3;
    _cpu->state.X2 = xflags.X2;
    _cpu->state.X1 = xflags.X1;
    _cpu->state.X0 = xflags.X0;
  }
}

//DLLEXPORT void cpuEiaSop (cpuContext cpu, void* extn, uint32* dst, uint32 src1);
void
cpuEiaSop  (cpuContext cpu, void* extn, uint32* dst, uint32 src1)
{
  arcsim::sys::cpu::Processor* _cpu  = PROCESSOR(cpu);
    
  arcsim::ise::eia::EiaInstructionInterface* eia_inst = 
    reinterpret_cast<arcsim::ise::eia::EiaInstructionInterface*>(extn);
  
  // Setup arguments
  // 
  arcsim::ise::eia::EiaBflags bflags = {_cpu->state.Z,  _cpu->state.N,  _cpu->state.C,  _cpu->state.V };
  arcsim::ise::eia::EiaXflags xflags = {_cpu->state.X3, _cpu->state.X2, _cpu->state.X1, _cpu->state.X0};
  
  // Call EIA instruction method evaluation
  //
  *dst = eia_inst->eval_single_opd(src1, bflags, xflags);
}

//DLLEXPORT void cpuEiaZopF (cpuContext cpu, void* extn, uint32* dst, int f);
void
cpuEiaZopF (cpuContext cpu, void* extn, uint32* dst, int f)
{
  arcsim::sys::cpu::Processor* _cpu  = PROCESSOR(cpu);
    
  arcsim::ise::eia::EiaInstructionInterface* eia_inst = 
    reinterpret_cast<arcsim::ise::eia::EiaInstructionInterface*>(extn);
  
  // Setup arguments
  // 
  arcsim::ise::eia::EiaBflags bflags = {_cpu->state.Z,  _cpu->state.N,  _cpu->state.C,  _cpu->state.V };
  arcsim::ise::eia::EiaXflags xflags = {_cpu->state.X3, _cpu->state.X2, _cpu->state.X1, _cpu->state.X0};
  
  // Call EIA instruction method evaluation
  //
  *dst = eia_inst->eval_zero_opd(bflags, xflags, &bflags, &xflags);
  
  // Update bflags and xflags if flag_enable is set
  //
  if (f) {
    _cpu->state.Z  = bflags.Z;
    _cpu->state.N  = bflags.N;
    _cpu->state.C  = bflags.C;
    _cpu->state.V  = bflags.V;
    _cpu->state.X3 = xflags.X3;
    _cpu->state.X2 = xflags.X2;
    _cpu->state.X1 = xflags.X1;
    _cpu->state.X0 = xflags.X0;
  }
}

//DLLEXPORT void cpuEiaZop (cpuContext cpu, void* extn, uint32* dst);
void
cpuEiaZop  (cpuContext cpu, void* extn, uint32* dst)
{
  arcsim::sys::cpu::Processor* _cpu  = PROCESSOR(cpu);
    
  arcsim::ise::eia::EiaInstructionInterface* eia_inst = 
    reinterpret_cast<arcsim::ise::eia::EiaInstructionInterface*>(extn);
  
  // Setup arguments
  // 
  arcsim::ise::eia::EiaBflags bflags = {_cpu->state.Z,  _cpu->state.N,  _cpu->state.C,  _cpu->state.V };
  arcsim::ise::eia::EiaXflags xflags = {_cpu->state.X3, _cpu->state.X2, _cpu->state.X1, _cpu->state.X0};
  
  // Call EIA instruction method evaluation
  //
  *dst = eia_inst->eval_zero_opd(bflags, xflags);
}

//DLLEXPORT void cpuEiaCond (cpuContext cpu, void* extn, uint8 cc);
int
cpuEiaCond (cpuContext cpu, void* extn, uint8 cc)
{
  arcsim::ise::eia::EiaConditionCodeInterface* eia_cond = 
    reinterpret_cast<arcsim::ise::eia::EiaConditionCodeInterface*>(extn);

  return eia_cond->eval_condition_code (cc);
}

// -----------------------------------------------------------------------------
// Memory function API
// -----------------------------------------------------------------------------


uint32
cpuTranslateVirtual (cpuContext cpu, uint32 virt_addr)
{
  uint32 phys_addr = 0;
  PROCESSOR(cpu)->mmu.lookup_data(virt_addr, PROCESSOR(cpu)->state.U, phys_addr);
  return phys_addr;
}

//DLLEXPORT int cpuIoWrite (cpuContext cpu, void* device, uint32 addr, uint32 data, uint32 size);
int
cpuIoWrite (cpuContext cpu, void* page, uint32 virt_addr, uint32 data, uint32 size)
{ // retrieve memory device
  arcsim::sys::mem::BlockData const * const block = reinterpret_cast<arcsim::sys::mem::BlockData*>(page);
  arcsim::mem::MemoryDeviceInterface* const dev   = block->get_mem_dev();
  
  // determine physical address
  const uint32 phys_addr = (block->page_frame | (PROCESSOR(cpu)->core_arch.page_arch.page_byte_offset_mask & virt_addr));
  
  // perform memory device write
  const int mem_error = dev->mem_dev_write(phys_addr, (uint8*)&data, size);
  
  if (mem_error) { // in case of a memory error raise appropriate interrupt
    LOG(LOG_ERROR) << "MEMORY ERROR: at address 0x"
                   << std::hex << std::setfill('0') << std::setw(8) << virt_addr
                   << " [write_fun]";
    PROCESSOR(cpu)->assert_interrupt_line (PROCESSOR(cpu)->sys_arch.isa_opts.get_memory_error_irq_num());
    return false;
  }
  return true;
}

//DLLEXPORT int cpuIoRead (cpuContext cpu, void* device, uint32 addr, uint32* data, uint32 size);
int
cpuIoRead (cpuContext cpu, void* page, uint32 virt_addr, uint32* data, uint32 size)
{ // retrieve memory device
  arcsim::sys::mem::BlockData const * const block = reinterpret_cast<arcsim::sys::mem::BlockData*>(page);  
  arcsim::mem::MemoryDeviceInterface* const dev   = block->get_mem_dev();
  
  // determine physical address
  const uint32 phys_addr = (block->page_frame | (PROCESSOR(cpu)->core_arch.page_arch.page_byte_offset_mask & virt_addr));
  
  *data = 0; // initialise pass by reference variable for memory read
  
  // perform memory device read
  const int mem_error = dev->mem_dev_read(phys_addr, (uint8*)data, size);
  
  if (mem_error) { // in case of a memory error raise appropriate interrupt
    LOG(LOG_ERROR) << "MEMORY ERROR: at address 0x"
                   << std::hex << std::setfill('0') << std::setw(8) << virt_addr
                   << " [read_fun]";
    PROCESSOR(cpu)->assert_interrupt_line (PROCESSOR(cpu)->sys_arch.isa_opts.get_memory_error_irq_num());
    return false;
  }  
  return true;
}

//DLLEXPORT int cpuWriteWord (cpuContext cpu, uint32 addr, uint32 data);
int cpuWriteWord (cpuContext cpu, uint32 addr, uint32 data)
{
	return PROCESSOR(cpu)->write32 (addr, data);
}

//DLLEXPORT int cpuWriteHalf (cpuContext cpu, uint32 addr, uint32 data);
int cpuWriteHalf (cpuContext cpu, uint32 addr, uint32 data)
{
	return PROCESSOR(cpu)->write16 (addr, data);
}

//DLLEXPORT int cpuWriteByte (cpuContext cpu, uint32 addr, uint32 data);
int cpuWriteByte (cpuContext cpu, uint32 addr, uint32 data)
{
	return PROCESSOR(cpu)->write8 (addr, data);
}

//DLLEXPORT int cpuReadWord (cpuContext cpu, uint32 addr, uint32* data);
int cpuReadWord (cpuContext cpu, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = PROCESSOR(cpu)->read32 (addr, datum);
	if (status)
		*data = datum;
	return status;
}

//DLLEXPORT int cpuReadHalf (cpuContext cpu, uint32 addr, uint32* data);
int cpuReadHalf (cpuContext cpu, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = PROCESSOR(cpu)->read16 (addr, datum);
	if (status)
		*data = datum;
	return status;
}

//DLLEXPORT int cpuReadByte (cpuContext cpu, uint32 addr, uint32* data);
int cpuReadByte (cpuContext cpu, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = PROCESSOR(cpu)->read8 (addr, datum);
	if (status)
		*data = datum;
	return status;
}

//DLLEXPORT int cpuReadHalfSigned (cpuContext cpu, uint32 addr, uint32* data);
int cpuReadHalfSigned (cpuContext cpu, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = PROCESSOR(cpu)->read16_extend (addr, datum);
	if (status)
		*data = datum;
	return status;
}

//DLLEXPORT int cpuReadByteSigned (cpuContext cpu, uint32 addr, uint32* data);
int cpuReadByteSigned (cpuContext cpu, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = PROCESSOR(cpu)->read8_extend (addr, datum);
	if (status)
		*data = datum;
	return status;
}

// -----------------------------------------------------------------------------

//DLLEXPORT uint16 cpuFetchCycles (cpuContext cpu, uint32 addr);
uint16
cpuFetchCycles (cpuContext cpu, uint32 addr)
{
	return  PROCESSOR(cpu)->mem_model->fetch(addr, PROCESSOR(cpu)->state.pc);
}

//DLLEXPORT uint16 cpuReadCycles (cpuContext cpu, uint32 addr);
uint16
cpuReadCycles (cpuContext cpu, uint32 addr)
{
	return PROCESSOR(cpu)->mem_model->read(addr, PROCESSOR(cpu)->state.pc, false);
}

//DLLEXPORT uint16 cpuWriteCycles (cpuContext cpu, uint32 addr);
uint16
cpuWriteCycles (cpuContext cpu, uint32 addr)
{
	return PROCESSOR(cpu)->mem_model->write(addr, PROCESSOR(cpu)->state.pc, false);
}

//DLLEXPORT void cpuBPred (cpuContext cpu, uint32 pc, int size, int link_offset, char isReturn, char link, uint64 ft);
void cpuBPred (cpuContext _cpu, uint32 pc, int size, int link_offset, char isReturn, char link, uint64 fet_st)
{
#ifdef ENABLE_BPRED
  arcsim::sys::cpu::Processor& cpu = *((arcsim::sys::cpu::Processor*)_cpu);

	BranchPredictorInterface::PredictionOutcome
    pred_out = cpu.bpu->commit_branch(pc,
                                      (pc+ (uint32)size),
                                      (cpu.state.pc),     //next_pc
                                      (pc+ (uint32)link_offset),
                                      (isReturn || (cpu.state.delayed_return)),
                                      (link     || (cpu.state.delayed_call)));
  
    bool hit = (   (pred_out == BranchPredictorInterface::CORRECT_PRED_TAKEN)
                || (pred_out == BranchPredictorInterface::CORRECT_PRED_NOT_TAKEN)
                || (pred_out == BranchPredictorInterface::CORRECT_PRED_NONE)
               );

    if (hit) {
      // FIXME: This is wrong, 'fet_st' is a function parameter we are assigning to?
      //
      fet_st += cpu.core_arch.bpu.miss_penalty;
    }

#endif /* ENABLE_BPRED */
}

// -----------------------------------------------------------------------------
// Trace function API
//

//DLLEXPORT void cpuTraceString (cpuContext cpu, const char* str);
void cpuTraceString (cpuContext cpu, const char* str)
{
  TRACE_INST(PROCESSOR(cpu), trace_string(str));
}

//DLLEXPORT extern void cpuTraceCommit (cpuContext cpu, int commit);
void cpuTraceCommit (cpuContext cpu, int commit)
{
  TRACE_INST(PROCESSOR(cpu), trace_commit(commit));
  TRACE_INST(PROCESSOR(cpu), trace_emit());
}

//DLLEXPORT void cpuTraceInstruction (cpuContext cpu, uint32 inst, uint32 limm);
void cpuTraceInstruction (cpuContext cpu, uint32 pc, uint32 inst, uint32 limm)
{
  TRACE_INST(PROCESSOR(cpu), trace_instruction(pc, inst, limm));
}

//DLLEXPORT void cpuTraceWriteBack (cpuContext cpu, int wa0, int wenb0, uint32 *wdata0, int wa1, int wenb1, uint32 *wdata1);
void cpuTraceWriteBack (cpuContext cpu, int wa0, int wenb0, uint32 *wdata0, int wa1, int wenb1, uint32 *wdata1)
{
  TRACE_INST(PROCESSOR(cpu), trace_write_back(wa0, wenb0, wdata0, wa1, wenb1, wdata1));
}

//DLLEXPORT void cpuTraceException (cpuContext cpu, char *message);
void cpuTraceException (cpuContext cpu, char *message)
{
  TRACE_INST(PROCESSOR(cpu), trace_exception(message));
}

//DLLEXPORT void cpuTraceLpBack (cpuContext cpu); 
void cpuTraceLpBack (cpuContext cpu)
{
  TRACE_INST(PROCESSOR(cpu), trace_loop_back());
}

//DLLEXPORT void cpuTraceLpCount (cpuContext cpu); 
void cpuTraceLpCount (cpuContext cpu)
{
  TRACE_INST(PROCESSOR(cpu), trace_loop_count());
}

//DLLEXPORT void cpuTraceLpInst (cpuContext cpu, int taken, uint32 target);
void cpuTraceLpInst (cpuContext cpu, int taken, uint32 target)
{
  TRACE_INST(PROCESSOR(cpu), trace_loop_inst(taken, target));
}

//DLLEXPORT void cpuTraceLoad (cpuContext cpu, int fmt, uint32 addr, uint32 data);
void cpuTraceLoad (cpuContext cpu, int fmt, uint32 addr, uint32 data)
{
  TRACE_INST(PROCESSOR(cpu), trace_load(fmt, addr, data));
}

//DLLEXPORT void cpuTraceMul64Inst (cpuContext cpu);
void cpuTraceMul64Inst (cpuContext cpu)
{
  TRACE_INST(PROCESSOR(cpu), trace_mul64_inst());
}


//DLLEXPORT void cpuTraceStore (cpuContext cpu, int fmt, uint32 addr, uint32 data);
void cpuTraceStore (cpuContext cpu, int fmt, uint32 addr, uint32 data)
{
  TRACE_INST(PROCESSOR(cpu), trace_store(fmt, addr, data));
}

//DLLEXPORT void cpuTraceLR (cpuContext cpu, uint32 addr, uint32 data, int success);
void cpuTraceLR (cpuContext cpu, uint32 addr, uint32 data, int success)
{
  TRACE_INST(PROCESSOR(cpu), trace_lr(addr, data, success));
}

//DLLEXPORT void cpuTraceSR (cpuContext cpu, uint32 addr, uint32 data, int success);
void cpuTraceSR (cpuContext cpu, uint32 addr, uint32 data, int success)
{
  TRACE_INST(PROCESSOR(cpu), trace_sr(addr, data, success));
}

//DLLEXPORT void cpuTraceRTIE (cpuContext cpu);
void cpuTraceRTIE (cpuContext cpu)
{
  TRACE_INST(PROCESSOR(cpu), trace_rtie());
}

// -----------------------------------------------------------------------------
// Profiling function API
//
void  cpuHistogramInc (void* hist_ptr, uint32 idx)
{
  using namespace arcsim::util;
  // If histogram exists increment it at given index
  //
  if (hist_ptr != 0) {
    ((Histogram*)hist_ptr)->inc(idx);
  } else {
    LOG(LOG_ERROR) << "[cpuHistogramInc] Trying to increment invalid Histogram.";
  }
}

// -----------------------------------------------------------------------------
// Instrumentation Point API
//
int
cpuIptAboutToExecuteInstr (cpuContext cpu)
{
  arcsim::ipt::IPTManager& ipt_mgr = PROCESSOR(cpu)->ipt_mgr;
  
  // AboutToExecuteInstructionIPT check
  if (ipt_mgr.is_enabled()
      && ipt_mgr.is_enabled(arcsim::ipt::IPTManager::kIPTAboutToExecuteInstruction)
      && ipt_mgr.is_about_to_execute_instruction(PROCESSOR(cpu)->state.pc))
  {
    if (ipt_mgr.exec_about_to_execute_instruction_ipt_handlers(PROCESSOR(cpu)->state.pc)) {
      PROCESSOR(cpu)->set_pending_action(kPendingAction_IPT);
      return true;
    }
  }
  return false;
}

void
cpuIptNotifyBeginInstrExecution(cpuContext cpu, uint32 addr, uint32 len)
{
  arcsim::ipt::IPTManager& ipt_mgr = PROCESSOR(cpu)->ipt_mgr;
  
  // BeginInstructionExecutionIPT check
  if (ipt_mgr.is_enabled()
      && ipt_mgr.is_enabled(arcsim::ipt::IPTManager::kIPTBeginInstruction))
  {
    ipt_mgr.notify_begin_instruction_execution_ipt_handlers(addr, len);
  }
}

void
cpuIptNotifyBeginBasicBlockExecution(cpuContext cpu, uint32 addr)
{
  arcsim::ipt::IPTManager& ipt_mgr = PROCESSOR(cpu)->ipt_mgr;
  
  // BeginBasicBlockInstructionIPT check
  if (ipt_mgr.is_enabled()
      && ipt_mgr.is_enabled(arcsim::ipt::IPTManager::kIPTBeginBasicBlockInstruction))
  {
    ipt_mgr.notify_begin_basic_block_instruction_execution_ipt_handlers(addr);
  }

}

// -----------------------------------------------------------------------------
// Translation management API
//
void cpuRegisterTranslationForRemoval(cpuContext cpu, uint32 addr)
{
  PROCESSOR(cpu)->remove_translation(addr);
}

// -----------------------------------------------------------------------------
// Timer API
//
void  cpuTimerSync(cpuContext cpu)
{
  PROCESSOR(cpu)->timer_sync();
}

// -----------------------------------------------------------------------------
// Register tracking function API
//
#ifdef REGTRACK_SIM
void cpuRegisterRead (cpuContext _cpu, int reg)
{
  arcsim::sys::cpu::Processor& cpu = *((arcsim::sys::cpu::Processor*)_cpu);
	if (cpu.sim_opts.track_regs) {
		++cpu.state.gprs_stats[reg].read;
		uint64 temp = cpu.instructions() - cpu.state.gprs_stats[reg].last;
		if (temp) {
			cpu.state.gprs_stats[reg].arith += temp;
			cpu.state.gprs_stats[reg].geom  += log(temp);
		}
		cpu.state.gprs_stats[reg].last = cpu.instructions();
	}
}

void cpuRegisterWrite (cpuContext _cpu, int reg)
{
  arcsim::sys::cpu::Processor& cpu = *((arcsim::sys::cpu::Processor*)_cpu);
	if (cpu.sim_opts.track_regs) {
		++cpu.state.gprs_stats[reg].write;
		uint64 temp = cpu.instructions() - cpu.state.gprs_stats[reg].last;
		if (temp) {
			cpu.state.gprs_stats[reg].arith += temp;
			cpu.state.gprs_stats[reg].geom  += log(temp);
		}
		cpu.state.gprs_stats[reg].last = cpu.instructions();
	}
}
#endif /* REGTRACK_SIM */
