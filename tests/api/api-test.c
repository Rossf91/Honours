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

enum IptAddresses {
  kIptAddr0x000003c0 = 0x000003c0,
  kIptAddr0x000003da = 0x000003da,
  kIptAddr0x000003dc = 0x000003dc,
  kIptAddr0x0000032a = 0x0000032a
};

static uint32 kIptExecutionCount         = 0; // count of 'hit' IPTS

// Handler for about  AboutToExecute instruction IPT
//
static int
AboutToExecuteInstructionHandler(IocContext                         ctx,
                                 IocContextItem                     ipt,
                                 HandleAboutToExecuteInstructionObj obj,
                                 uint32                             addr)
{
  printf("[AboutToExecuteInstructionHandler] called for address: '0x%08lx'\n", addr);
  ++kIptExecutionCount;
  // after having encountered an IPT once we remove it
  switch (addr) {
    case kIptAddr0x000003c0: { iptRemoveAboutToExecuteInstructionIpt(ipt, addr); break; }
    case kIptAddr0x000003da: { iptRemoveAboutToExecuteInstructionIpt(ipt, addr); break; }
    case kIptAddr0x000003dc: { iptRemoveAboutToExecuteInstructionIpt(ipt, addr); break; }
    case kIptAddr0x0000032a: { iptRemoveAboutToExecuteInstructionIpt(ipt, addr); break; }
    default:
      assert("[AboutToExecuteInstructionHandler] executed for invalid instruction.");
  }
  return 1;
}

void usage(void)
{
  printf(
      "api-test: API Test harness.\n"
      "Usage: api-test <EXECUTABLE> [OPTIONS]\n"
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
  simCosimOn (sys);
  simMemoryModelOn (sys);
  simCycleAccurateOn (sys);
  simTraceOn (sys);
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
  // Register AboutToExecuteInstructionIPT for address 0x000003c0 (first instruction in main),
  // jump and its delay slot instruction in main at 0x000003da and 0x000003dc,
  // and 0x0000032a (last instruction program)
  //
  IocContextItem ipt = iocContextGetItem(cpu_ctx, kIocContextItemIPTManagerID);
  
  // main            [000003c0] 71a9               K Z     mov_s          r1,r13 : (w0) r1 <= 0x00000000 *
  int ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddr0x000003c0);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register AboutToExecuteInstructionIpt.");
  
  // main            [000003da] 7fe0c0a3           K Z     j_s.d          [blink] *
  ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddr0x000003da);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register AboutToExecuteInstructionIpt.");
  // NOTE: the following instruction is the delay slot instruction for the given program
  // main            [000003dc] c0a3               KDZ     add_s          sp,sp,0xc : (w0) r28 <= 0x00413548 *
  ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddr0x000003dc);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register AboutToExecuteInstructionIpt.");
  
  // _handle_trap    [0000032a] 07e9ffcf          LK Z     b              0xffffffe8 *
  ret = iptInsertAboutToExecuteInstructionIpt(ipt, 0, AboutToExecuteInstructionHandler, kIptAddr0x0000032a);
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

  // Main loop for stepping of instructions
  while (simStep(sys))
    ;
  
  // Retrive Native, Interpreted, and Cycle Counter ContextItems
  IocContextItem native_inst_cnt64_item = iocContextGetItem(cpu_ctx, kIocContextItemNativeInstructionCount64ID);
  assert(native_inst_cnt64_item != 0 && "Retrieved IocContextItem kIocContextItemNativeInstructionCount64ID is NULL!");
  uint64 native_inst_cnt64 = profCounter64GetValue(native_inst_cnt64_item);
  
  IocContextItem interp_inst_cnt64_item = iocContextGetItem(cpu_ctx, kIocContextItemInterpretedInstructionCount64ID);
  assert(interp_inst_cnt64_item != 0 && "Retrieved IocContextItem kIocContextItemInterpretedInstructionCount64ID is NULL!");
  uint64 interp_inst_cnt64 = profCounter64GetValue(interp_inst_cnt64_item);
  
  IocContextItem cycle_cnt64_item = iocContextGetItem(cpu_ctx, kIocContextItemCycleCount64ID);
  assert(cycle_cnt64_item != 0 && "Retrieved IocContextItem kIocContextItemCycleCount64ID is NULL!");
  uint64 cycle_cnt64 = profCounter64GetValue(cycle_cnt64_item);
  
  printf("Total instructions = %llu [inst]\n", (native_inst_cnt64 + interp_inst_cnt64));
  printf("Total cycles = %llu [cycles]\n", cycle_cnt64);
  printf("Executed Instrumentation PoinTs: %d [IPTs]\n", kIptExecutionCount);
  
  return EXIT_SUCCESS;
}
