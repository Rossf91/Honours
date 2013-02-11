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

#ifndef INC_API_APIFUNS_H_
#define INC_API_APIFUNS_H_

#include "api/types.h"
#include "api/api_types.h"

#ifdef __cplusplus
extern "C" {
#endif
  
// -----------------------------------------------------------------------------
// API FUNCTIONS
//

// Create simulation context
//
DLLEXPORT simContext simCreateContext (int argc, char* argv[]);
// Retrieve processor context for given cpu id
//
DLLEXPORT cpuContext simGetCPUcontext (simContext sim, int cpuid);

DLLEXPORT int  simLoadElfBinary   (simContext sim, const char* binFileName);
DLLEXPORT int  simLoadHexBinary   (simContext sim, const char* hexFileName);
DLLEXPORT int  simLoadBinaryImage (simContext sim, const char* imgFileName);

// Load dynamic library
//
DLLEXPORT int  simLoadLibraryPermanently (simContext sim, const char* name);

DLLEXPORT uint32 simGetEntryPoint   (simContext sim);
DLLEXPORT void   simGetUpdateHandle (simContext sim, UpdatePacket** data);

/* API for modifying simulation options */

DLLEXPORT void simDebugOn (simContext sim, unsigned int level);
DLLEXPORT void simDebugOff(simContext sim);
DLLEXPORT void simFastOn(simContext sim);
DLLEXPORT void simFastOff(simContext sim);
DLLEXPORT void simTraceOn (simContext sim);
DLLEXPORT void simTraceOff (simContext sim);
DLLEXPORT void simVerboseOn (simContext sim);
DLLEXPORT void simVerboseOff (simContext sim);
DLLEXPORT void simInteractiveOn (simContext sim);
DLLEXPORT void simInteractiveOff (simContext sim);  
DLLEXPORT void simSymPrintOn (simContext sim);
DLLEXPORT void simSymPrintOff (simContext sim);
DLLEXPORT void simCosimOn (simContext sim);
DLLEXPORT void simCosimOff (simContext sim);  
DLLEXPORT void simEmulateTrapsOn (simContext sim);
DLLEXPORT void simEmulateTrapsOff (simContext sim);
DLLEXPORT void simMemoryModelOn (simContext sim);
DLLEXPORT void simMemoryModelOff (simContext sim);
DLLEXPORT void simCycleAccurateOn (simContext sim);
DLLEXPORT void simCycleAccurateOff (simContext sim);

/* API for modifying ISA options */

DLLEXPORT void simOptA700  (simContext sim, int val);
DLLEXPORT void simOptA600  (simContext sim, int val);
DLLEXPORT void simOptA6K   (simContext sim, int val);
DLLEXPORT void simOptR16   (simContext sim, int val);
DLLEXPORT void simOptShift (simContext sim, int val);
DLLEXPORT void simOptSwap  (simContext sim, int val);
DLLEXPORT void simOptNorm  (simContext sim, int val);
DLLEXPORT void simOptMpy16 (simContext sim, int val);
DLLEXPORT void simOptMpy32 (simContext sim, int val);
DLLEXPORT void simOptMLat  (simContext sim, int val);
DLLEXPORT void simOptDiv   (simContext sim, int val);
DLLEXPORT void simOptDense (simContext sim, int val);
DLLEXPORT void simOptLLSC  (simContext sim, int val);
DLLEXPORT void simOptShAs  (simContext sim, int val);
DLLEXPORT void simOptFFS   (simContext sim, int val);
DLLEXPORT void simOptFpx   (simContext sim, int val);
DLLEXPORT void simOptMul64 (simContext sim, int val);
DLLEXPORT void simOptSat   (simContext sim, int val);
DLLEXPORT void simOptPCSize (simContext sim, int val);
DLLEXPORT void simOptLPCSize (simContext sim, int val);
DLLEXPORT void simOptICfeature (simContext sim, int val);
DLLEXPORT void simOptDCfeature (simContext sim, int val);
DLLEXPORT void simOptHasDMP (simContext sim, int val);
DLLEXPORT void simOptActionpoints (simContext sim, int val);
DLLEXPORT void simOptAPSfull (simContext sim, int val);
DLLEXPORT void simOptTimer0 (simContext sim, int val);
DLLEXPORT void simOptTimer1 (simContext sim, int val);
DLLEXPORT void simOptFmt14 (simContext sim, int val);
DLLEXPORT void simOptHasEIA (simContext sim, int val);
DLLEXPORT void simOpt4PortRF (simContext sim, int val);
DLLEXPORT void simOptFastMpy (simContext sim, int val);
DLLEXPORT void simOptCodeProtection (simContext sim, int val);
DLLEXPORT void simOptStackChecking (simContext sim, int val);
DLLEXPORT void simOptMultipleIccms (simContext sim, int val);

/* API for resetting the simulated system and running simulations */

DLLEXPORT void simHardReset (simContext sim);
DLLEXPORT void simSoftReset (simContext sim);
DLLEXPORT void simHalt      (simContext sim);
DLLEXPORT int  simStep      (simContext sim);
DLLEXPORT int  simRun       (simContext sim);

/* API for interrogating simulator about external plugin options */

DLLEXPORT int         simPluginOptionIsSet    (simContext sim, const char *opt);
DLLEXPORT const char *simPluginOptionGetValue (simContext sim, const char *opt);

/* API for interrogating CPU to obtain state information */

DLLEXPORT void simGetPC (cpuContext cpu, uint32* pc);

#ifdef CYCLE_ACC_SIM
DLLEXPORT void simGetCycleCount   (cpuContext cpu, uint64* c);
DLLEXPORT void simGetIcacheCycles (cpuContext cpu, unsigned int* c);
DLLEXPORT void simGetDcacheCycles (cpuContext cpu, unsigned int* c);
#endif /* CYCLE_ACC_SIM */

/* API for disassembling and retrieving instructions */

DLLEXPORT int             simDisasmInstruction (cpuContext cpu, uint32 inst,
                                                uint32 limm, char *buf);

DLLEXPORT int             simCurrentFunction (simContext sim, char *func_name);
DLLEXPORT int             simCopyActionString(cpuContext cpu,
                                                  char* buf,
                                                  int max_len);
DLLEXPORT void            simLastInstruction (cpuContext cpu,
                                              uint32* inst,
                                              uint32* limm);
DLLEXPORT const char*     simDisasmOperator (unsigned char op);

DLLEXPORT void simTrace      (simContext cpu);
DLLEXPORT void simPrintStats (simContext sim);
 
/* API for accessing shadow memory used during co-simulation */

DLLEXPORT int  simWrite32 (simContext sim, uint32 addr, uint32  data);
DLLEXPORT int  simRead32  (simContext sim, uint32 addr, uint32* data);
DLLEXPORT int  simWrite16 (simContext sim, uint32 addr, uint32  data);
DLLEXPORT int  simRead16  (simContext sim, uint32 addr, uint32* data);
DLLEXPORT int  simWrite8  (simContext sim, uint32 addr, uint32  data);
DLLEXPORT int  simRead8   (simContext sim, uint32 addr, uint32* data);

  
/* API for CCM registration */

DLLEXPORT void simRegisterIccm (cpuContext cpu, uint32 start_addr, uint32 size);
DLLEXPORT void simRegisterIccms (cpuContext cpu, uint32 start_addr[4], uint32 size[4]);
DLLEXPORT void simRegisterDccm (cpuContext cpu, uint32 start_addr, uint32 size);
  
/* API for external debugger to access processor's internal state */

DLLEXPORT int  cpuDebugReadCoreReg  (cpuContext cpu, int addr, uint32* data);
DLLEXPORT int  cpuDebugWriteCoreReg (cpuContext cpu, int addr, uint32 data);
DLLEXPORT int  cpuDebugReadAuxReg   (cpuContext cpu, uint32 addr, uint32* data);
DLLEXPORT int  cpuDebugWriteAuxReg  (cpuContext cpu, uint32 addr, uint32 data);
DLLEXPORT void cpuDebugPrepareCPU   (cpuContext cpu);
DLLEXPORT void cpuDebugClearCounters(cpuContext cpu);
DLLEXPORT void cpuDebugInvalidateDcodeCache (cpuContext cpu);
DLLEXPORT void cpuDebugReset (cpuContext cpu);

/* API for external control of processor and simulation loop interrupts */

// Simulated processor interrupts
// DEPRECATED: see 'api/irq/api_irq.h' for proper replacements.
//
DEPRECATED(DLLEXPORT void cpuDetectInterrupts  (cpuContext cpu));
DEPRECATED(DLLEXPORT void cpuAssertInterrupt   (cpuContext cpu, int irq_no));
DEPRECATED(DLLEXPORT void cpuRescindInterrupt  (cpuContext cpu, int irq_no));

// Simulation loop interrupt
//
DLLEXPORT void cpuSetSimulationLoopInterrupt(cpuContext cpu);
DLLEXPORT void cpuClearSimulationLoopInterrupt(cpuContext cpu);

  
#ifdef __cplusplus
}
#endif

#endif /* INC_API_APIFUNS_H_ */

