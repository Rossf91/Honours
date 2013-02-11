//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2005 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
//  This file implements an API to the ISS using C linkage. It can be
//  used to build a co-simulation environment with Verilog and PLI.
//
// =====================================================================

#include "api/api_funs.h"

#include <string>
#include <cstring>
#include <map>

#include "arch/Configuration.h"

#include "system.h"
#include "isa/arc/Disasm.h"
#include "isa/arc/Opcode.h"
#include "sys/cpu/processor.h"

#include "util/Log.h"
#include "util/OutputStream.h"
#include "util/system/SharedLibrary.h"

#include "util/Counter.h"


// -----------------------------------------------------------------------------
// Temporary macro for casting cpuContext into processor class.
// It provides better readability.
//
#define PROCESSOR(_cpu_) (reinterpret_cast<arcsim::sys::cpu::Processor*>(_cpu_))
#define SYSTEM(_sys_)    (reinterpret_cast<System*>(_sys_))


// -----------------------------------------------------------------------------
// Create/Retrieve context
//
simContext
simCreateContext (int argc, char* argv[])
{
  // Create HEAP ALLOCATED Configuration object
  //
  Configuration*  arch_conf = new Configuration();
  
  // System class pointer that will be returned as a simContext
  //
  System* sys = 0;
  
  // Get simulation options
  //
  if (arch_conf->read_simulation_options(argc, argv)) {

    if (!arch_conf->sys_arch.sim_opts.quiet) {
      arcsim::util::OutputStream S(stderr);
      for (int a=0; a < argc; ++a)  { S.Get() << ' ' << argv[a]; }
      S.Get() << ARCSIM_COPYRIGHT;
    }

    // Read in System Architecture
    //
    arch_conf->read_architecture(arch_conf->sys_arch.sim_opts.sys_arch_file,
                                 arch_conf->sys_arch.sim_opts.print_sys_arch,
                                 arch_conf->sys_arch.sim_opts.print_arch_file);

    // Set default values for cosimulation environment
    //
    arch_conf->sys_arch.sim_opts.trace_on     = false;
    arch_conf->sys_arch.sim_opts.verbose      = false;
    arch_conf->sys_arch.sim_opts.cosim        = true;
    arch_conf->sys_arch.sim_opts.cycle_sim    = true;
    arch_conf->sys_arch.sim_opts.memory_sim   = true;
        
    // Create HEAP ALLOCATED System object
    //
    sys = new System (*arch_conf);
    sys->create_system();

    // Setup simulated application's cmd-line arguments on it's stack
    //
    sys->setup_simulated_stack (argc,
                                arch_conf->sys_arch.sim_opts.app_args,
                                arch_conf->sys_arch.sim_opts.obj_name.c_str(),
                                argv);
  }

  // Delta structure
  //
  sys->cpu[0]->delta.pc       = 0;
  sys->cpu[0]->delta.wmask    = 0;
  sys->cpu[0]->delta.aux.a    = 0;
  sys->cpu[0]->delta.aux.w    = 0;
  for (int p = 0; p < NUM_RF_WRITE_PORTS; ++p) {
    sys->cpu[0]->delta.rf[p].w = sys->cpu[0]->delta.rf[p].a= 0;
  }

  return reinterpret_cast<simContext>(sys);
}


cpuContext
simGetCPUcontext (simContext sys, int cpuid)
{ // cpuid ignored for now; place-holder for multicpu systems.
	//
	return reinterpret_cast<cpuContext>(SYSTEM(sys)->cpu[0]);
}


// -----------------------------------------------------------------------------
// Various methods of loading different binaries and libraries
//
int simLoadElfBinary (simContext sim, const char* name)
{
	return SYSTEM(sim)->load_elf32(name);
}

int simLoadHexBinary (simContext sim, const char* name)
{
	return SYSTEM(sim)->load_quicksim_hex(name);
}

int simLoadBinaryImage (simContext sim, const char* name)
{
	return SYSTEM(sim)->load_binary_image(name);
}

int  simLoadLibraryPermanently (simContext sim, const char* name)
{
  return arcsim::util::system::SharedLibrary::open(std::string(name));
}

// -----------------------------------------------------------------------------
// Methods for driving the simulation
//
void simHardReset (simContext sim)
{
	SYSTEM(sim)->hard_reset();
}

void simSoftReset (simContext sim)
{
	SYSTEM(sim)->soft_reset();
}

void simHalt (simContext sim)
{
  SYSTEM(sim)->halt_cpu();
}

int simStep (simContext sim)
{
  System* sys = SYSTEM(sim);
  // Remove any uop delta left from last instruction
  //
  if (sys->cpu[0]->delta.next_uop != NULL) {
    delete sys->cpu[0]->delta.next_uop;
  }
  memset(&sys->cpu[0]->delta, 0, sizeof(sys->cpu[0]->delta));
  // C++ bool has the size of a char, whereas C99 defines bool type as an int.
  // Here the return value is casted to an int before being passed back to
  // C-side.
  return sys->step(&sys->cpu[0]->delta);
}

int simRun (simContext sim)
{
	return SYSTEM(sim)->simulate();
}

void simTrace (simContext sim)
{
  SYSTEM(sim)->trace(&SYSTEM(sim)->cpu[0]->delta);
}

void simPrintStats (simContext sim)
{
	SYSTEM(sim)->print_stats();
}

void simGetUpdateHandle (simContext sim, UpdatePacket** data)
{
  *data = &SYSTEM(sim)->cpu[0]->delta;
}

uint32 simGetEntryPoint (simContext sim)
{
	return SYSTEM(sim)->get_entry_point();
}

// -----------------------------------------------------------------------------
// API for querying plugin options
// -----------------------------------------------------------------------------

// Test if a plugin option is set
////
int simPluginOptionIsSet(simContext sim, const char *opt)
{
  System* sys = SYSTEM(sim);

  // Iterate over all potential plugin options registered for this system
  for(std::map<std::string,std::string>::const_iterator
      I = sys->sim_opts.builtin_mem_dev_opts.begin(),
      E = sys->sim_opts.builtin_mem_dev_opts.end();
      I != E; ++I)
  {
    // Test if an option is present
    if (strcmp((I->first).c_str(), opt) == 0) {
      return true;
    }
  }
  return false;
}

// Plugin options can have the form 'opt=value' the following API function returns
// a const char pointer to 'value' or NULL if the option does not exist or does
// not have a value.
////
const char *simPluginOptionGetValue(simContext sim, const char *opt)
{
  System* sys = SYSTEM(sim);
  // Iterate over all potential plugin options registered for this system
  ////
  for(std::map<std::string,std::string>::const_iterator
      I = sys->sim_opts.builtin_mem_dev_opts.begin(),
      E = sys->sim_opts.builtin_mem_dev_opts.end();
      I != E; ++I)
  {
    // Test if an option is present
    ////
    if (strcmp((I->first).c_str(), opt) == 0) {
      if (I->second.size()) {
        // Retrun pointer to option value
        ////
        return (I->second).c_str();
      }
    }
  }
  return NULL;
}


// -----------------------------------------------------------------------------
// API for modifying simulation options
// -----------------------------------------------------------------------------

void simDebugOn (simContext sim, unsigned int level)
{
  switch (level) {
    case 0:  { arcsim::util::Log::reportingLevel = LOG_DEBUG;  break; }      
    case 1:  { arcsim::util::Log::reportingLevel = LOG_DEBUG1; break; }
    case 2:  { arcsim::util::Log::reportingLevel = LOG_DEBUG2; break; }
    case 3:  { arcsim::util::Log::reportingLevel = LOG_DEBUG3; break; }
    default: { arcsim::util::Log::reportingLevel = LOG_DEBUG4; break; }
  }
}

void simDebugOff(simContext sim) {
  arcsim::util::Log::reportingLevel = LOG_ERROR;
}

void simFastOn(simContext sim) {
  SYSTEM(sim)->sim_opts.fast = true;
  SYSTEM(sim)->trans_mgr.configure(&(SYSTEM(sim)->sim_opts), SYSTEM(sim)->sim_opts.fast_num_worker_threads);
  SYSTEM(sim)->trans_mgr.start_workers();
}

void simFastOff(simContext sim) {
  SYSTEM(sim)->sim_opts.fast = false;  
}

void simTraceOn (simContext sim)      {
  SYSTEM(sim)->sim_opts.trace_on = true;
}
void simTraceOff (simContext sim)     {
  SYSTEM(sim)->sim_opts.trace_on = false;
}
void simVerboseOn (simContext sim)    {
  SYSTEM(sim)->sim_opts.verbose = true;
}
void simVerboseOff (simContext sim)   {
  SYSTEM(sim)->sim_opts.verbose = false;
}

void simInteractiveOn (simContext sim)
{
  SYSTEM(sim)->sim_opts.interactive     = true;
  SYSTEM(sim)->sim_opts.halt_simulation = true;
}
void simInteractiveOff(simContext sim)
{
  SYSTEM(sim)->sim_opts.interactive = false;
}  

void simCosimOn (simContext sim)      {
  SYSTEM(sim)->sim_opts.cosim = true;
}
void simCosimOff (simContext sim)     {
  SYSTEM(sim)->sim_opts.cosim = false;
}

void simMemoryModelOn (simContext sim)
{
  SYSTEM(sim)->sim_opts.memory_sim   = true;
}
void simMemoryModelOff (simContext sim) {
  SYSTEM(sim)->sim_opts.memory_sim = false;
}

void simCycleAccurateOn (simContext sim)
{
  SYSTEM(sim)->sim_opts.cycle_sim    = true;
  SYSTEM(sim)->sim_opts.memory_sim   = true;
}
void simCycleAccurateOff (simContext sim) {
  SYSTEM(sim)->sim_opts.cycle_sim  = false;
}

void simEmulateTrapsOn (simContext sim)
{
  SYSTEM(sim)->sim_opts.emulate_traps = true;
}
void simEmulateTrapsOff (simContext sim) {
  SYSTEM(sim)->sim_opts.emulate_traps = false;
}


// -----------------------------------------------------------------------------
// API for modifying the ISA options
// -----------------------------------------------------------------------------

void simOptA700  (simContext sim, int v) {
  if (v != 0) {
    SYSTEM(sim)->sys_conf.sys_arch.isa_opts.set_isa(IsaOptions::kIsaA700);
  }
}
                             
void simOptA6K   (simContext sim, int v) {
  if (v != 0) {
    SYSTEM(sim)->sys_conf.sys_arch.isa_opts.set_isa(IsaOptions::kIsaA6K);
  }
}

void simOptA600  (simContext sim, int v) {
  if (v != 0) {
    SYSTEM(sim)->sys_conf.sys_arch.isa_opts.set_isa(IsaOptions::kIsaA600);
  }
}

void simOptR16   (simContext sim, int val) { 
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.only_16_regs   = (val != 0);
}

void simOptShift (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.shift_option   = (val != 0);
}
void simOptSwap  (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.swap_option    = (val != 0);
}

void simOptNorm  (simContext sim, int val) { 
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.norm_option    = (val != 0);
}

void simOptMpy16 (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.mpy16_option   = (val != 0);
}

void simOptMpy32 (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.mpy32_option   = (val != 0);
}

void simOptMLat  (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.mpy_lat_option = val;
}

void simOptDiv   (simContext sim, int val) { 
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.div_rem_option   = val; 
}
 
void simOptDense (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.density_option = val;
}

void simOptLLSC  (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.atomic_option  = val;
}

void simOptShAs  (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.shas_option    = (val != 0);
}

void simOptFFS   (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.ffs_option    = (val != 0);
}

void simOptFpx   (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.fpx_option     = (val != 0);
}

void simOptMul64 (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.mul64_option   = (val != 0);
}

void simOptSat   (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.sat_option     = (val != 0);
}

void simOptPCSize(simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.pc_size = val;
}

void simOptLPCSize(simContext sim, int val){
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.lpc_size = val;
}

void simOptFmt14 (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.new_fmt_14     = (val != 0);
}

void simOptHasEIA (simContext sim, int val){
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.has_eia        = (val != 0);
}

void simOptICfeature(simContext sim, int val)     {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.ic_feature = val;
}
void simOptDCfeature(simContext sim, int val)     {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.dc_feature = val;
}
void simOptHasDMP(simContext sim, int val)        {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.has_dmp_memory  = (val != 0);
}
void simOptActionpoints (simContext sim, int val) {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.num_actionpoints = val;
}
void simOptAPSfull (simContext sim, int val)      {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.aps_full   = (val != 0);
}
void simOptTimer0 (simContext sim, int val)       {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.has_timer0 = (val != 0);
}
void simOptTimer1 (simContext sim, int val)       {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.has_timer1 = (val != 0);
}
void simOpt4PortRF (simContext sim, int val)      {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.rf_4port   = (val != 0);
}
void simOptFastMpy (simContext sim, int val)      {
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.mpy_fast   = (val != 0);
}
void simOptCodeProtection (simContext sim, int val){
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.code_protect_bits = val;
}
void simOptStackChecking (simContext sim, int val){
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.stack_checking = (val!=0);
}
void simOptMultipleIccms (simContext sim, int val){
  SYSTEM(sim)->sys_conf.sys_arch.isa_opts.multiple_iccms = (val!=0);
}
// -----------------------------------------------------------------------------
// API for interrogating CPU to obtain state information
// -----------------------------------------------------------------------------

void simGetPC (cpuContext cpu, uint32* pc)
{
  *pc = PROCESSOR(cpu)->state.pc;
}

#ifdef CYCLE_ACC_SIM
void simGetCycleCount (cpuContext cpu, uint64* cyc)
{
  *cyc = PROCESSOR(cpu)->cnt_ctx.cycle_count.get_value();
}

void simGetIcacheCycles (cpuContext cpu, unsigned int* icyc) {
  *icyc = PROCESSOR(cpu)->current_interpreted_inst()->fet_cycles;
}

void simGetDcacheCycles (cpuContext cpu, unsigned int* dcyc) {
  *dcyc = PROCESSOR(cpu)->current_interpreted_inst()->mem_cycles;
}

#endif /* CYCLE_ACC_SIM */

// -----------------------------------------------------------------------------
// API for disassembling and retrieving instructions
// -----------------------------------------------------------------------------

int  simCurrentFunction (simContext sim, char *dptr)
{
  std::string fname;
  int retval = SYSTEM(sim)->get_symbol( SYSTEM(sim)->cpu[0]->state.pc, fname);
  if (retval) strncpy(dptr, fname.c_str(), fname.length());
  return retval;
}

int simCopyActionString (cpuContext cpu, char* buf, int max_len)
{
  return PROCESSOR(cpu)->TS.flush(buf, max_len);
}

void simLastInstruction (cpuContext cpu, uint32* inst, uint32* limm)
{
  *inst = PROCESSOR(cpu)->current_interpreted_inst()->info.ir;
  *limm = PROCESSOR(cpu)->current_interpreted_inst()->limm;
}


const char* simDisasmOperator (unsigned char op)
{
  if (op <= arcsim::isa::arc::OpCode::opcode_count) {
    return arcsim::isa::arc::OpCode::to_string(static_cast<arcsim::isa::arc::OpCode::Op>(op));
  }
  return NULL;
}

int simDisasmInstruction (cpuContext _cpu, uint32 inst, uint32 limm, char *dptr)
{
  arcsim::sys::cpu::Processor* cpu = PROCESSOR(_cpu);
  const IsaOptions&                      isa_opts    = cpu->sys_arch.isa_opts;
  arcsim::sys::cpu::EiaExtensionManager& eia_mgr = cpu->eia_mgr;
  
  arcsim::isa::arc::Disasm disasm(isa_opts, eia_mgr, inst, limm);
  
  strncpy(dptr, disasm.buf, disasm.len);
  *(dptr + disasm.len) = '\0';
  return disasm.encoded_size();
}


//DLLEXPORT void simRegisterIccm (cpuContext cpu, uint32 start_addr, uint32 size)
void simRegisterIccm (cpuContext _cpu, uint32 start_addr, uint32 size)
{
  arcsim::sys::cpu::Processor& cpu = *PROCESSOR(_cpu);

  // Configure ICCM
  //
  cpu.core_arch.iccm.is_configured = true;
  cpu.core_arch.iccm.start_addr    = start_addr;
  cpu.core_arch.iccm.size          = size;
  cpu.core_arch.iccm.bus_width     = 32;
  cpu.core_arch.iccm.latency       = 1;
  
  // After setting the CCM configuration we can register the CCM device
  //
  cpu.register_iccm();
  
  // Reset processor to initial state to make sure ALL changes are picked up
  //
  cpu.reset_to_initial_state();
}

//DLLEXPORT void simRegisterIccm (cpuContext cpu, uint32 start_addr, uint32 size)
void simRegisterIccms (cpuContext _cpu, uint32 start_addr[4], uint32 size[4])
{
  arcsim::sys::cpu::Processor& cpu = *PROCESSOR(_cpu);

  for(int i=0; i<4; i++){
    // Configure ICCM
    //
    cpu.core_arch.iccm.is_configured = (size[i]>0);
    cpu.core_arch.iccm.start_addr    = start_addr[i];
    cpu.core_arch.iccm.size          = size[i];
    cpu.core_arch.iccm.bus_width     = 32;
    cpu.core_arch.iccm.latency       = i+1;
  }
  // After setting the CCM configuration we can register the CCM device
  //
  cpu.register_iccm();

  // Reset processor to initial state to make sure ALL changes are picked up
  //
  cpu.reset_to_initial_state();
}

//DLLEXPORT void simRegisterDccm (cpuContext cpu, uint32 start_addr, uint32 size)
void simRegisterDccm (cpuContext _cpu, uint32 start_addr, uint32 size)
{
  arcsim::sys::cpu::Processor& cpu = *PROCESSOR(_cpu);
  
  // Configure ICCM
  //
  cpu.core_arch.dccm.is_configured = true;
  cpu.core_arch.dccm.start_addr    = start_addr;
  cpu.core_arch.dccm.size          = size;
  cpu.core_arch.dccm.bus_width     = 32;
  cpu.core_arch.dccm.latency       = 1;
  
  // After setting the CCM configuration we can register the CCM device
  //
  cpu.register_dccm();
  
  // Reset processor to initial state to make sure ALL changes are picked up
  //
  cpu.reset_to_initial_state();
}

// -----------------------------------------------------------------------------
// API for read/write to shadow memory used for co-simulation
// -----------------------------------------------------------------------------

int simWrite32 (simContext sim, uint32 addr, uint32 data)
{
	return SYSTEM(sim)->writeShadow32 (addr, data);
}

int simWrite16 (simContext sim, uint32 addr, uint32 data)
{
	return SYSTEM(sim)->writeShadow16 (addr, data);
}

int simWrite8 (simContext sim, uint32 addr, uint32 data)
{
	return SYSTEM(sim)->writeShadow8 (addr, data);
}

int simRead32 (simContext sim, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = SYSTEM(sim)->readShadow32 (addr, datum);
	*data = datum;
	return status;
}

int simRead16 (simContext sim, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = SYSTEM(sim)->readShadow16 (addr, datum);
	*data = datum;
	return status;
}

int simRead8 (simContext sim, uint32 addr, uint32* data)
{
	uint32 datum = 0;
	int status = SYSTEM(sim)->readShadow8 (addr, datum);
	*data = datum;
	return status;
}


// -----------------------------------------------------------------------------
// CPU functions
// -----------------------------------------------------------------------------

void cpuAssertInterrupt (cpuContext cpu, int irq_no)
{
	PROCESSOR(cpu)->assert_interrupt_line (irq_no);
}

void cpuRescindInterrupt (cpuContext cpu, int irq_no)
{
	PROCESSOR(cpu)->rescind_interrupt_line (irq_no);
}

void cpuDetectInterrupts (cpuContext cpu)
{
	PROCESSOR(cpu)->detect_interrupts ();
}

void cpuSetSimulationLoopInterrupt(cpuContext cpu)
{
  PROCESSOR(cpu)->set_pending_action(kPendingAction_EXTERNAL_AGENT);
}

void cpuClearSimulationLoopInterrupt(cpuContext cpu)
{
  PROCESSOR(cpu)->clear_pending_action(kPendingAction_EXTERNAL_AGENT);
}

// -----------------------------------------------------------------------------
// API for external debugger to access processor's internal state
// -----------------------------------------------------------------------------

int  cpuDebugReadCoreReg (cpuContext cpu, int addr, uint32* data)
{
  *data = PROCESSOR(cpu)->state.gprs[addr];
  return true;
}

int  cpuDebugWriteCoreReg (cpuContext cpu, int addr, uint32 data)
{
  PROCESSOR(cpu)->state.gprs[addr] = data;
  return true;
}

int  cpuDebugReadAuxReg (cpuContext cpu, uint32 addr, uint32* data)
{
	return PROCESSOR(cpu)->read_aux_register (addr, data, false);
}

int  cpuDebugWriteAuxReg (cpuContext cpu, uint32 addr, uint32 data)
{
	return PROCESSOR(cpu)->write_aux_register (addr, data, false);
}

void cpuDebugPrepareCPU (cpuContext cpu)
{
  PROCESSOR(cpu)->prepare_cpu();
}

void cpuDebugClearCounters (cpuContext cpu)
{
  PROCESSOR(cpu)->clear_cpu_counters();
}

void cpuDebugInvalidateDcodeCache (cpuContext cpu)
{
  PROCESSOR(cpu)->purge_dcode_cache();
}

void cpuDebugReset (cpuContext cpu)
{
  PROCESSOR(cpu)->reset_to_initial_state();
}

