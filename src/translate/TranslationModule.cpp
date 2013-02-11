//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                             
// =====================================================================
//
// Description:
//
// This class represents a compilation unit (i.e. Module) containing a trace.
//
// =====================================================================

#include <sys/stat.h>
#include <fcntl.h>

#include <iomanip>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <cstring>

#include <string>

#include "Assertion.h"

#include "concurrent/Mutex.h"
#include "concurrent/ScopedLock.h"
#include "util/system/SharedLibrary.h"

#include "arch/Configuration.h"

#include "profile/BlockEntry.h"

#include "translate/TranslationModule.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/TranslationWorker.h"

#include "util/Log.h"

#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_

// -----------------------------------------------------------------------------
// Constructor/Destructor
//
TranslationModule::TranslationModule (uint32 key, SimOptions& sim_opts)
: sim_opts_(sim_opts),
  key_(key),
  module_state_(0),
  module_(0),
  engine_(0),
  ref_count_(0)
{ /* EMPTY */ }

TranslationModule::~TranslationModule ()
{
  if (module_ == 0)
    return;
  
  if (sim_opts_.fast_use_default_jit) {
    LOG(LOG_DEBUG1) << "[TranslationModule] Marking Module '" << name_ << "' for GC.";
    engine_->mark_module_for_gc(module_);
  } else {
    LOG(LOG_DEBUG1) << "[TranslationModule] GC Module '" << name_ << "'.";
    close_shared_library();
  }
}

// -----------------------------------------------------------------------------
// Initialise a TranslationModule object
//
bool
TranslationModule::init (uint32 addr)
{
  static char const * const kDirPathFmt = "%s/P0x%08x/"; // P ... Page
  static char const * const kModPathFmt = "T%04x";       // T ... Trace
  char buf[BUFSIZ];

  // Initialise TranslationModule directory
  buf[0] = '\0';
  std::snprintf (buf, sizeof(buf), kDirPathFmt, sim_opts_.fast_tmp_dir.c_str(), addr);  
  name_.append(buf);
  
  // ----------------------- SYNCHRONIZED START ---------------------------
  if (!sim_opts_.fast_use_default_jit || sim_opts_.keep_files)
  {
    static arcsim::concurrent::Mutex  mutx_fstat;
    arcsim::concurrent::ScopedLock    lock(mutx_fstat);    
    struct stat                       status;
    
    // Check if fast_tmp_dir exists
    int res = stat(sim_opts_.fast_tmp_dir.c_str(), &status);
    if ((res < 0) || !S_ISDIR(status.st_mode)) {
      if (mkdir(sim_opts_.fast_tmp_dir.c_str(), 0755)) {
        LOG(LOG_ERROR) << "[TranslationModule] Failed to create directory '"
                       << sim_opts_.fast_tmp_dir << "' during translation.";      
        return  false;
      }
    }
    // Stat the project/page directory, creating if non-existent
    res = stat(buf, &status);
    if ((res < 0) || !S_ISDIR(status.st_mode)) {
      if (mkdir(buf, 0755)) {
        LOG(LOG_ERROR) << "[TranslationModule] Failed to create directory '"
                       << buf << "' during translation.";      
        return false;
      }
    }      
  }
  // ----------------------- SYNCHRONIZED END ---------------------------
  
  // Initialise full path of TranslationModule
  buf[0] = '\0';
  std::snprintf (buf, sizeof(buf), kModPathFmt, key_);
  name_.append(buf);
  
  return true;
}

// -----------------------------------------------------------------------------
// Add a BlockEntry to this Module
//
void
TranslationModule::add_block_entry(arcsim::profile::BlockEntry& block,
                                  TranslationBlock              native)
{
  if (block_map_.find(block.phys_addr) == block_map_.end()) {
    block.set_translation(native),
    block.set_module(this);
    block_map_[block.phys_addr] = &block;
    retain();
  }
}

// -----------------------------------------------------------------------------
// Removes all BlockEntries and returns true if one of them was 'in translation'
//
int
TranslationModule::erase_block_entries ()
{
  int num_erased = block_map_.size();
  while (!block_map_.empty()) {
    arcsim::profile::BlockEntry& b = *block_map_.begin()->second;
    LOG(LOG_DEBUG2) << "[TranslationModule] removing NATIVE BlockEntry @ 0x" << HEX(b.phys_addr);
    b.remove_translation();
    b.set_module(0);
    block_map_.erase(block_map_.begin());
    release();
  }
  return num_erased;
}



// ------------------------------------------------------------------------
// Retrieve pointer to function in module
//
TranslationBlock
TranslationModule::get_pointer_to_function (const char * sym)
{
  TranslationBlock p = 0;
  
  if (arcsim::util::system::SharedLibrary::lookup_symbol((void **)(&p), module_, sym)) {
    return p;
  }
  return 0;
}

bool
TranslationModule::close_shared_library ()
{
  return arcsim::util::system::SharedLibrary::close(module_);
}

bool
TranslationModule::load_shared_library ()
{
  std::string path(name_);
  
  if (!sim_opts_.keep_files)
    path.append(".tmp");
  path.append(".dll");

  return arcsim::util::system::SharedLibrary::open(&module_, path);
}
