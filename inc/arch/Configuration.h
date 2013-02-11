//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
// Description:
//
// System Architecture Configuration.
//
// =====================================================================

#ifndef INC_ARCH_CONFIGURATION_H_
#define INC_ARCH_CONFIGURATION_H_

#include <list>

#include "api/types.h"

#include "arch/PageArch.h"
#include "arch/CacheArch.h"
#include "arch/SpadArch.h"
#include "arch/BpuArch.h"
#include "arch/WpuArch.h"
#include "arch/MmuArch.h"
#include "arch/IfqArch.h"
#include "arch/SystemArch.h"

// -----------------------------------------------------------------------------
// Forward declarations
//
namespace arcsim {
  namespace util {
    class OutputStream;
  }
}
class ModuleArch;
class CoreArch;

// -----------------------------------------------------------------------------
// Declaration of Configuration class
//
class Configuration {
private:

  int create_new_cache(char *il);
  int create_new_spad (char *il);
  int create_new_mmu  (char *il);
  int create_new_ifq  (char *il);
  int create_new_bpu  (char *il);
  int create_new_wpu  (char *il);
  
  void *create_new_core  (char *il);
  void *create_new_module(char *il);
  void *create_system    (char *il);
  
  int add_cache (int &level, void *section, char *il);
  int add_spad  (int &level, void *section, char *il);
  int add_mmu   (int &level, void *section, char *il);
  int add_ifq   (int &level, void *section, char *il);
  int add_bpu   (int &level, void *section, char *il);
  int add_wpu   (int &level, void *section, char *il);
  int add_core  (int &level, void *section, char *il);
  int add_module(int &level, void *section, char *il);

public:
  // Defined architecture elements
  //
  SystemArch             sys_arch;     // THE SYSTEM ARCHITECTURE

  std::list<IfqArch>     ifq_list;     // list of ALL IFQs defined
  std::list<MmuArch>     mmu_list;     // list of ALL MMUs defined
  std::list<CacheArch>   cache_list;   // list of ALL caches defined
  std::list<SpadArch>    spad_list;    // list of ALL scratchpads defined
  std::list<BpuArch>     bpu_list;     // list of ALL branch predictors defined
  std::list<WpuArch>     wpu_list;     // list of ALL way predictors defined
  std::list<CoreArch*>   core_list;    // list of ALL cores defined
  std::list<ModuleArch*> module_list;  // list of ALL modules defined

  // Constructor
  //
  Configuration();

  // Destructor
  //
  ~Configuration();

  bool read_simulation_options(int argc, char *argv[]);
  
  bool read_architecture(std::string& sarch_file,
                         bool         print_sarch,
                         bool         print_sarch_file);
  
  bool print_architecture(arcsim::util::OutputStream& S,
                          std::string&                afile);

  bool print_caches(arcsim::util::OutputStream& S,
                    uint32                      cache_types,
                    int                         indent,
                    CacheArch                   ic,
                    CacheArch                   dc);
  bool print_spads (arcsim::util::OutputStream& S,
                    uint32                      spad_types,
                    int                         indent,
                    SpadArch                    ic,
                    SpadArch                    *ics,
                    SpadArch                    dc);

};

#endif  /* INC_ARCH_CONFIGURATION_H_ */

