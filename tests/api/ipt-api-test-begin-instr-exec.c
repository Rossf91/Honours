#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>

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


static uint64 kIptExecutionCount1         = 0;
static uint64 kIptExecutionCount2         = 0;

static const uint64 kExpectedIptExecutionCount2 = 6547;

// Handlers for about  AboutToExecute instruction IPT
//
static void
BeginInstructionExecutionHandler1(IocContext                         ctx,
                                  IocContextItem                     ipt,
                                  HandleBeginInstructionExecutionObj obj,
                                  uint32                             addr,
                                  uint32                             len)
{
  ++kIptExecutionCount1;
  if (!(kIptExecutionCount1 % 128))
    usleep(10000);
}

static void
BeginInstructionExecutionHandler2(IocContext                         ctx,
                                  IocContextItem                     ipt,
                                  HandleBeginInstructionExecutionObj obj,
                                  uint32                             addr,
                                  uint32                             len)
{
  ++kIptExecutionCount2;
  if (!(kIptExecutionCount1 % 64))
    usleep(10000);
  
  if (kIptExecutionCount2 == kExpectedIptExecutionCount2) {
    printf("[BeginInstructionExecutionHandler2] Removing BeginInstructionExecutionHandler2\n");
    // test case - remove BeginInstructionExecutionHandler2 as a side-effect
    // of notification call
    int ret = iptRemoveBeginInstructionExecutionIptSubscriber(ipt, 0, BeginInstructionExecutionHandler2);
    assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to remove BeginInstructionExecutionIpt.");
  }
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
  
  // Retrieve cpuContext
  cpu = simGetCPUcontext (sys, 0);
  printf ("Retrieved processor context.\n");
  
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
  
  int ret = iptInsertBeginInstructionExecutionIpt(ipt, 0, BeginInstructionExecutionHandler1);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register BeginInstructionExecutionIpt.");
  ret = iptInsertBeginInstructionExecutionIpt(ipt, 0, BeginInstructionExecutionHandler2);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to register BeginInstructionExecutionIpt.");
  
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
  
  // Remove last BeginInstructionExecutionIpt
  ret = iptRemoveBeginInstructionExecutionIptSubscriber(ipt, 0, BeginInstructionExecutionHandler1);
  assert(ret == API_IPT_REGISTER_SUCCESS && "Failed to remove BeginInstructionExecutionIpt.");
  
  // Test double remove
  ret = iptRemoveBeginInstructionExecutionIptSubscriber(ipt, 0, BeginInstructionExecutionHandler1);
  assert(ret == API_IPT_REGISTER_FAILURE && "Failed to remove BeginInstructionExecutionIpt.");
  
  // Retrive Native and Interpreted ContextItems
  IocContextItem native_inst_cnt64_item = iocContextGetItem(cpu_ctx, kIocContextItemNativeInstructionCount64ID);
  assert(native_inst_cnt64_item != 0 && "Retrieved IocContextItem kIocContextItemNativeInstructionCount64ID is NULL!");
  uint64 native_inst_cnt64 = profCounter64GetValue(native_inst_cnt64_item);
  
  IocContextItem interp_inst_cnt64_item = iocContextGetItem(cpu_ctx, kIocContextItemInterpretedInstructionCount64ID);
  assert(interp_inst_cnt64_item != 0 && "Retrieved IocContextItem kIocContextItemInterpretedInstructionCount64ID is NULL!");
  uint64 interp_inst_cnt64 = profCounter64GetValue(interp_inst_cnt64_item);
  
  printf("Interpreted instructions = %llu [inst]\n", interp_inst_cnt64);
  printf("Native instructions = %llu [inst]\n", native_inst_cnt64);
  printf("Total instructions = %llu [inst]\n", (native_inst_cnt64 + interp_inst_cnt64));
  
  printf("Executed Instrumentation PoinTs: %llu [IPTs]\n", kIptExecutionCount1 + kIptExecutionCount2);
  assert((kIptExecutionCount1 + kIptExecutionCount2) == ((native_inst_cnt64 + interp_inst_cnt64) + kExpectedIptExecutionCount2));
  
  return EXIT_SUCCESS;
}
