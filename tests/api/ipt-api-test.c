#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "api/api_funs.h"
#include "api/ioc/api_ioc.h"
#include "api/prof/api_prof.h"
#include "api/ipt/api_ipt.h"

enum ExecutableKind {
  kExecNone,
  kExecElf32,
  kExecHex,
  kExecBin
};

// ------------------------------------------------------------------------------------------
//  We register IPTs for the following addresses/instructions:

// 
// Here we register for an instruction that is at the beginning of a basic block @0x00000298
// to test correctness of native mode IPT triggering:
// make            [000002ea]                            ld             r2,[fp,0xfffffffc]
// ---
// make            [000002c6] 72c7      0000b3b4 K  NC   add_s          r2,r2,0000b3b4
// make            [000002cc] a260               K  NC   st_s           r3,[r2,0x0]
// ---
// isprime         [0000039c] 7fe02440           K       j_s.d          [blink]
// isprime         [0000039e] 2440311c           KD      add            sp,sp,0x4
// ---
// main            [000003e0] 6a41               K Z     add_s          r2,r2,0x1
// main            [000003e2] 1bf8b080           K Z     st             r2,[fp,0xfffffff8]
//
enum IptAddresses {
  kIptAddrMakeFun0x000002ea    = 0x000002ea,
  kIptAddrMakeFun0x000002c6    = 0x000002c6,
  kIptAddrMakeFun0x000002cc    = 0x000002cc,
  kIptAddrIsPrimeFun0x0000039c = 0x0000039c,
  kIptAddrIsPrimeFun0x0000039e = 0x0000039e,
  kIptAddrMainFun0x000003e0    = 0x000003e0,
  kIptAddrMainFun0x000003e2    = 0x000003e2
};

static uint64 kIptExecutionCount         = 0; // count of IPT callback calls
const  uint32 kFirstInstrInBlockIptExecutionCount = 3956;
// expected count of IPT callback calls
const  uint64 kExpectedIptExecutionCount = 420006 + 3956; 

static uint32 kFirstInstrInBlockCountDown0 = 3950;
static uint32 kFirstInstrInBlockCountDown1 = 5;
static uint32 kIptMakeFunCountDown0    = 10000;
static uint32 kIptMakeFunCountDown1    = 10000;
static uint32 kIptIsPrimeFunCountDown0 = 100000;
static uint32 kIptIsPrimeFunCountDown1 = 100000;
static uint32 kIptMainFunCountDown0    = 100000;
static uint32 kIptMainFunCountDown1    = 100000;

// Handlers for about  AboutToExecute instruction IPT
//
static int
AboutToExecuteInstructionHandler(IocContext                         ctx,
                                 IocContextItem                     ipt,
                                 HandleAboutToExecuteInstructionObj obj,
                                 uint32                             addr)
{
  bool retval = 0;
  ++kIptExecutionCount;
  // after having encountered an IPT several (i.e. CountDownN times) we remove it
  switch (addr) {
    case kIptAddrMakeFun0x000002ea: {
      if (kFirstInstrInBlockCountDown0) {
        --kFirstInstrInBlockCountDown0;
      } else {
        if (kFirstInstrInBlockCountDown1) {
          --kFirstInstrInBlockCountDown1;
        } else {
          int ret = iptRemoveAboutToExecuteInstructionIptSubscriber(ipt, obj, AboutToExecuteInstructionHandler, kIptAddrMakeFun0x000002ea);
          assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to remove AboutToExecuteInstructionIpt subscriber.");
          // test duplicate deletetion
          ret = iptRemoveAboutToExecuteInstructionIptSubscriber(ipt, obj, AboutToExecuteInstructionHandler, kIptAddrMakeFun0x000002ea);
          assert(ret == API_IPT_REGISTER_FAILURE && "Failed to detect duplicate removal of AboutToExecuteInstructionIpt subscriber.");
        }
        retval = 1;
      }
      break;
    }
    case kIptAddrMakeFun0x000002c6: {
      if (kIptMakeFunCountDown0) {
        --kIptMakeFunCountDown0;
      } else {
        int ret = iptRemoveAboutToExecuteInstructionIptSubscriber(ipt, obj, AboutToExecuteInstructionHandler, kIptAddrMakeFun0x000002c6);
        assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to remove AboutToExecuteInstructionIpt subscriber.");
        retval = 1;
      }
      break;
    }
    case kIptAddrMakeFun0x000002cc: { 
      if (kIptMakeFunCountDown1) {
        --kIptMakeFunCountDown1;
      } else {
        int ret = iptRemoveAboutToExecuteInstructionIptSubscriber(ipt, obj, AboutToExecuteInstructionHandler, kIptAddrMakeFun0x000002cc);
        assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to remove AboutToExecuteInstructionIpt subscriber.");
        retval = 1;
      }
      break;
    }
    case kIptAddrIsPrimeFun0x0000039c: { 
      if (kIptIsPrimeFunCountDown0) {
        --kIptIsPrimeFunCountDown0;
      } else {
        int ret = iptRemoveAboutToExecuteInstructionIptSubscriber(ipt, obj, AboutToExecuteInstructionHandler, kIptAddrIsPrimeFun0x0000039c);
        assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to remove AboutToExecuteInstructionIpt subscriber.");
        retval = 1;
        
        // At this point we have been called from a native translation and try to
        // set new AboutToExecuteInstructionHandlers for PCs that are in that
        // particular translation. This will trigger an immediate return from the
        // translation and the removal of the respective translation so the new
        // AboutToExecuteInstructionHandlers can be triggered properly.
        //
        // main            [000003e0] 6a41               K Z     add_s          r2,r2,0x1
        // main            [000003e2] 1bf8b080           K Z     st             r2,[fp,0xfffffff8]
        // --
        ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddrMainFun0x000003e0);
        assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register AboutToExecuteInstructionIpt.");
        ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddrMainFun0x000003e2);
        assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register AboutToExecuteInstructionIpt.");
      }
      break;
    }
    case kIptAddrIsPrimeFun0x0000039e: { 
      if (kIptIsPrimeFunCountDown1) {
        --kIptIsPrimeFunCountDown1;
      } else {
        int ret = iptRemoveAboutToExecuteInstructionIptSubscriber(ipt, obj, AboutToExecuteInstructionHandler, kIptAddrIsPrimeFun0x0000039e);
        assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to remove AboutToExecuteInstructionIpt subscriber.");
        retval = 1;
      }
      break;
    }
    case kIptAddrMainFun0x000003e0: { 
      if (kIptMainFunCountDown0) {
        --kIptMainFunCountDown0;
      } else {
        int ret = iptRemoveAboutToExecuteInstructionIptSubscriber(ipt, obj, AboutToExecuteInstructionHandler, kIptAddrMainFun0x000003e0);
        assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to remove AboutToExecuteInstructionIpt subscriber.");
        retval = 1;
      }
      break;
    }
    case kIptAddrMainFun0x000003e2: { 
      if (kIptMainFunCountDown1) {
        --kIptMainFunCountDown1;
      } else {
        int ret = iptRemoveAboutToExecuteInstructionIptSubscriber(ipt, obj, AboutToExecuteInstructionHandler, kIptAddrMainFun0x000003e2);
        assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to remove AboutToExecuteInstructionIpt subscriber.");
        retval = 1;
      }
      break;
    }
    default:
      assert("[AboutToExecuteInstructionHandler] executed for invalid instruction.");
  }
  
  if (retval)
    printf("[AboutToExecuteInstructionHandler] Activated for address: '0x%08x'\n", addr);
  
  return retval;
}

// This is a second about to AboutToExecuteInstructionIPT handler that we will registered
// for a PC address we already registered 'AboutToExecuteInstructionHandler' for.
//
static uint64 kAboutToExecuteInstructionHandlerSecondCount = 0;
static const uint64 kExpectedAboutToExecuteInstructionHandlerSecondCount = 100000001UL;
static int
AboutToExecuteInstructionHandlerSecond(IocContext                         ctx,
                                       IocContextItem                     ipt,
                                       HandleAboutToExecuteInstructionObj obj,
                                       uint32                             addr)
{
  ++kAboutToExecuteInstructionHandlerSecondCount;
  return 0;
}


void usage(void)
{
  printf(
      "ipt-api-test: API Test harness.\n"
      "Usage: ipt-api-test <EXECUTABLE> [OPTIONS]\n"
      " EXECUTABLE: path and format of executable to run\n"
      "   -e <path>   ELF32 executable given by <path>\n"
      "   -x <path>   Intel Hex executable given by <path>\n"
      "   -b <path>   Binary image file given by <path> for preloaded executable\n"
      " OPTIONS: \n"
      "   -h          Print this usage message and exit\n"
      "   --          Pass the rest of the command line options to Arcsim\n"
      "\n"
      );

}

int
main(int argc, char **argv)
{
  char        exec_path[2048];
  int         exec_attr = kExecNone;
  int         sargc = 0;
  char*       sargv[128];
  int         argp;
  
  // Simulation and IOC managed contexts
  //
  simContext    sys;
  cpuContext    cpu;
  IocContext    sys_ctx;
  IocContext    mod_ctx;
  IocContext    cpu_ctx;

  // Process command line arguments
  for (argp = 1; argp < argc; argp ++) {
    // Pass the rest of command line args to simulator
    if (!strcmp(argv[argp], "--")) {
      do {
        sargv[sargc++] = argv[argp++];
      } while (argp < argc);
      break;
    }
    // Accept command line arguments for cosim-harness
    if (*argv[argp] == '-') {
      switch (*(argv[argp]+1)) {
        case 'e': /* ELF32 format */
          exec_attr = kExecElf32;
          goto copy_execpath;
        case 'x': /* Hex format */
          exec_attr = kExecHex;
          goto copy_execpath;
        case 'b': /* Bin format */
          exec_attr = kExecBin;
copy_execpath:
          if (++ argp == argc) {
            fprintf(stderr, "Fatal: Missing ELF32 executable path at argv[%d]\n", argp-1);
            usage();
            return -1;
          }
          strcpy(exec_path, argv[argp]);
          break;
        case 'h': /* Print help message */
        default: ;
          usage();
          return 0;
      }
    }
  }

  // Load simulator shared library
  dlopen("libsim.so", RTLD_NOW+RTLD_GLOBAL);
  printf ("Shared simulator library 'libsim.so' loaded...\n");

  // Create system
  sys  = simCreateContext (sargc, sargv);
  printf ("System context created.\n");
  
  // Enable various options
  simDebugOn (sys, 6);
  simEmulateTrapsOn(sys);
  simFastOn (sys);
  simCosimOff (sys);
  simMemoryModelOff (sys);
  simCycleAccurateOff (sys);
  // simTraceOn (sys);
  simVerboseOn (sys);
  
  // Retrieve cpuContext
  cpu = simGetCPUcontext (sys, 0);
  printf ("Retreived processor context.\n");
  
  // ----------------------------------------------------------------------
  // Retrieve system context with ID 0
  //
  sys_ctx = iocGetContext(iocGetGlobalContext(), 0);
  assert(sys_ctx != 0 && "Retrieved IoC System context is NULL!");
  // Retrieve module context with ID 0
  mod_ctx = iocGetContext(sys_ctx, 0);
  assert(mod_ctx != 0 && "Retrieved IoC Module context is NULL!");
  // Retrieve processor context with ID 0
  cpu_ctx = iocGetContext(mod_ctx, 0);
  assert(cpu_ctx != 0 && "Retrieved IoC Processor context is NULL!");
  
  // ----------------------------------------------------------------------
  // Register AboutToExecuteInstructionIPT for the following addresses
  //
  IocContextItem ipt = iocContextGetItem(cpu_ctx, kIocContextItemIPTManagerID);
  
  // make            [000002ea]                            ld             r2,[fp,0xfffffffc]
  // ---
  int ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddrMakeFun0x000002ea);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register AboutToExecuteInstructionIpt.");
  
  // make            [000002c6] 72c7      0000b3b4 K  NC   add_s          r2,r2,0000b3b4
  // make            [000002cc] a260               K  NC   st_s           r3,[r2,0x0]
  // ---
  ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddrMakeFun0x000002c6);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register AboutToExecuteInstructionIpt.");
  ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddrMakeFun0x000002cc);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register AboutToExecuteInstructionIpt.");
  
  // isprime         [0000039c] 7fe02440           K       j_s.d          [blink]
  // isprime         [0000039e] 2440311c           KD      add            sp,sp,0x4
  // ---
  ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddrIsPrimeFun0x0000039c);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register AboutToExecuteInstructionIpt.");
  // test detection of duplicate subscriber for a specific IPT
  ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddrIsPrimeFun0x0000039c);
  assert(ret == API_IPT_REGISTER_FAILURE && "Failed to detect duplicate AboutToExecuteInstructionIpt subscribers.");
  // test insertion of multiple subscribers for a specific IPT
  ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandlerSecond, kIptAddrIsPrimeFun0x0000039c);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register AboutToExecuteInstructionHandlerSecond.");
  // test detection of duplicate subscriber for a specific IPT
  ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandlerSecond, kIptAddrIsPrimeFun0x0000039c);
  assert(ret == API_IPT_REGISTER_FAILURE && "Failed to detect duplicate AboutToExecuteInstructionIpt subscribers.");
  
  ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddrIsPrimeFun0x0000039e);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register AboutToExecuteInstructionIpt.");
  
  // ----------------------------------------------------------------------
  // Load executable
  int status;
  switch (exec_attr) {
    case kExecNone: fprintf(stderr, "Fatal: No executable file was given to simulate.\n"); usage(); return -1;
    case kExecElf32: { status = simLoadElfBinary(sys, exec_path);   break; }
    case kExecHex:   { status = simLoadHexBinary(sys, exec_path);   break; }
    case kExecBin:   { status = simLoadBinaryImage(sys, exec_path); break; }
  }
  if (status != 0) {
    fprintf(stderr, "Fatal: Cannot open %s as an executable to simulate.\n", exec_path);
    usage();
    return -1;
  }

  // Main run loop
  while (simRun(sys))
    ;
  
  // Retrive Native and Interpreted ContextItems
  IocContextItem native_inst_cnt64_item = iocContextGetItem(cpu_ctx, kIocContextItemNativeInstructionCount64ID);
  assert(native_inst_cnt64_item != 0 && "Retrieved IocContextItem kIocContextItemNativeInstructionCount64ID is NULL!");
  uint64 native_inst_cnt64 = profCounter64GetValue(native_inst_cnt64_item);
  
  IocContextItem interp_inst_cnt64_item = iocContextGetItem(cpu_ctx, kIocContextItemInterpretedInstructionCount64ID);
  assert(interp_inst_cnt64_item != 0 && "Retrieved IocContextItem kIocContextItemInterpretedInstructionCount64ID is NULL!");
  uint64 interp_inst_cnt64 = profCounter64GetValue(interp_inst_cnt64_item);
    
  // Now test the removal of IPTs
  printf("Second Multiple-Subscriber Instrumentation PoinTs: %llu [IPTs]\n", kAboutToExecuteInstructionHandlerSecondCount);
  assert(kAboutToExecuteInstructionHandlerSecondCount == kExpectedAboutToExecuteInstructionHandlerSecondCount
          && "IPT callback execution count for 'AboutToExecuteInstructionHandlerSecond' does not match.");
  
  ret = iptRemoveAboutToExecuteInstructionIpt(ipt, kIptAddrIsPrimeFun0x0000039c);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to remove all subscribers for AboutToExecuteInstructionIpt.");
  // test duplicate removal
  ret = iptRemoveAboutToExecuteInstructionIpt(ipt, kIptAddrIsPrimeFun0x0000039c);
  assert(ret == API_IPT_REGISTER_FAILURE && "Failed to detect duplicate remove all AboutToExecuteInstructionIpt subscribers call.");
  
  printf("Interpreted instructions = %llu [inst]\n", interp_inst_cnt64);
  printf("Native instructions = %llu [inst]\n", native_inst_cnt64);
  printf("Total instructions = %llu [inst]\n", (native_inst_cnt64 + interp_inst_cnt64));
  
  printf("Executed Instrumentation PoinTs: %llu [IPTs]\n", kIptExecutionCount);
  assert(kIptExecutionCount == kExpectedIptExecutionCount
         && "IPT callback execution count for 'AboutToExecuteInstructionHandler' does not match.");
  
  return EXIT_SUCCESS;
}
