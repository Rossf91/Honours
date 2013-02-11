//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
// 
//  main -- This is the top-level source file for the command-line
//          instruction-set simulator
// 
// =====================================================================

#include <climits>
#include <csignal>
#include <cstdlib>
#include <cstdio>

#include <limits.h>
#include <unistd.h>

#include "system.h"

#include "api/types.h"
#include "api/api_funs.h"

#include "arch/Configuration.h"

#include "ioc/Context.h"

#include "util/OutputStream.h"
#include "util/Log.h"

// -----------------------------------------------------------------------------
// Simulation context accessed by handlers
//
static simContext  sim_ctx = 0;

// -----------------------------------------------------------------------------
// Handler declarations
//

// SIG_INT hander (CTRL-C)
//
static void sigint_handler (int x);


// -----------------------------------------------------------------------------
// Main
//
int
main(int argc, char *argv[])
{  
  System*       system    = 0;
  Configuration arch_conf;
  
  // Get simulation options
  //
  if (arch_conf.read_simulation_options(argc, argv)) {
    
    if (arch_conf.sys_arch.sim_opts.verbose) {
      // Print simulation options if verbose mode is turned on
      //
      arcsim::util::Log L;
      std::ostringstream& os = L.Get(LOG_INFO);
      for (int a=0; a < argc; ++a) { os << ' ' << argv[a]; }
    }

    // Print copyright if quiet is not specified
    //
    if (!arch_conf.sys_arch.sim_opts.quiet) { FPRINTF(stderr) << ARCSIM_COPYRIGHT; }
            
    // Read target architecture definition
    //
    arch_conf.read_architecture(arch_conf.sys_arch.sim_opts.sys_arch_file,
                                arch_conf.sys_arch.sim_opts.print_sys_arch,
                                arch_conf.sys_arch.sim_opts.print_arch_file); 
    
    // Initialise System object for target architecture
    //
    system = new System(arch_conf);
    
    // Initialise simulation context for this simulation run
    //
    sim_ctx = (simContext)system;
    
    // Create/configure System object
    //
    system->create_system();
    
    // Start devices before starting simulation
    //
    system->io_iom_device_manager.start_devices();
    
    // Install SIGINT handler to break into simulation loop cleanly
    //
    sighandler_t old_sigint = signal (SIGINT, &sigint_handler);
    
    // Start simulation
    //
    system->start_simulation(argc, argv);
    
    // Reinstate old SIGINT handler now that simulation is done.
    //
    (void) signal (SIGINT, old_sigint);

    // Stop devices after simulation
    //
    system->io_iom_device_manager.stop_devices();
    
    // Print statistics
    //
    system->print_stats ();
    
    // Destroy System after simulation to clean up resources
    //
    system->destroy_system();
  }

  if (system != 0) {
    // Store system id
    //
    uint32 sys_id = system->id;
    
    // Destroy system
    //
    delete system;

    // Destroy container managed memory for system
    //
    delete arcsim::ioc::Context::Global().get_context(sys_id);
  }
  
  return EXIT_SUCCESS;
}

// -----------------------------------------------------------------------------
// Handler for terminal interrupt using CTRL-C.
//
static void
sigint_handler (int x)
{
  // Break simulation when in interactive mode or halt simulation when user hits CTRL-C
  //
  if (sim_ctx != 0) {
    simInteractiveOn(sim_ctx);
  }
}


