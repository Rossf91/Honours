//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Class implementing functionality for opening/loading shared libraries
// and resolving symbols.
//
// =====================================================================

#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <cstdio>

#include <cstring>
#include <string>

#include "concurrent/Mutex.h"
#include "concurrent/ScopedLock.h"
#include "util/system/SharedLibrary.h"

#include "util/Log.h"


namespace arcsim {
  namespace util {
    namespace system {
      
// Open and load shared library setting the handle
//
bool
SharedLibrary::open(void** handle, const std::string& path)
{  
  // ----------------------- SYNCHRONIZED START ------------------------
  static arcsim::concurrent::Mutex  mutex;
  arcsim::concurrent::ScopedLock    lock(mutex);
  
  bool success = true;
  struct stat lib_status;
	
	if ((stat(path.c_str(), &lib_status) == 0) && S_ISREG(lib_status.st_mode)) {
    if ((*handle = dlopen (path.c_str(), RTLD_NOW + RTLD_GLOBAL)) == NULL) {
      // ERROR
      //
      LOG(LOG_ERROR) << "Opening dynamic library '" << path << "' failed: '" << dlerror() << "'";
      success = false;
    }
	} else {
    // ERROR: library does not exist.
    //
    LOG(LOG_ERROR) << "Opening dynamic library '" << path << "' failed: '" << strerror(errno) << "'";
    success = false;
  }
  // ----------------------- SYNCHRONIZED END   ------------------------
  return success;
}

// Open and load shared library
//
bool
SharedLibrary::open(const std::string& path)
{  
  // ----------------------- SYNCHRONIZED START ------------------------
  static arcsim::concurrent::Mutex  mutex;
  arcsim::concurrent::ScopedLock    lock(mutex);
  
  bool success = true;
  struct stat lib_status;
  
  if ((stat(path.c_str(), &lib_status) == 0) && S_ISREG(lib_status.st_mode)) {
    if ((dlopen (path.c_str(), RTLD_LAZY | RTLD_GLOBAL)) == NULL) {
      // ERROR
      //
      LOG(LOG_ERROR) << "Opening dynamic library '" << path << "' failed: '" << dlerror() << "'";
      success = false;
    }
  } else {
    // ERROR: library does not exist.
    //
    LOG(LOG_ERROR) << "Opening dynamic library '" << path << "' failed: '" << strerror(errno) << "'";
    success = false;
  }
  // ----------------------- SYNCHRONIZED END   ------------------------
  return success;
}

      
// Close shared library
//
bool
SharedLibrary::close(void* handle)
{
  static arcsim::concurrent::Mutex  mutex;
  arcsim::concurrent::ScopedLock    lock(mutex);

  if (handle != NULL) {
    // Clear any old error conditions
    dlerror();

    if (dlclose(handle) != 0) {
      char* err_msg = dlerror();
      LOG(LOG_ERROR) << "Closing dynamic library failed: " << err_msg;
      return false;
    }
  }
  return true;
}

// Lookup symbol in loaded library
//
bool
SharedLibrary::lookup_symbol(void**             fun_handle,
                             void*              lib_handle,
                             const char*        fun_name)
{
  static arcsim::concurrent::Mutex  mutex;
  arcsim::concurrent::ScopedLock    lock(mutex);

  char* err_msg = NULL;
  
  // --------------------------------------------------------------------
  // Resolve address for symbol name. Make sure you read 'man dlsym' and
  // understand how to correctly lookup symbols and check for errors before
  // modifying the following code!
  // NOTE THAT CALL TO dlsym and dlerror must be atomic AND synchronized
  //      to enable calling this method from a parrallel or concurrent context
  // --------------------------------------------------------------------
  
  if (lib_handle != NULL) {
    // Clear any old error conditions
    dlerror();
    
    // Try to resolve symbol
    *fun_handle = dlsym(lib_handle, fun_name);
    
    // Did symbol lookup succeed
    if ((err_msg = dlerror()) != NULL)  { 
      // Symbol lookup failed
      LOG(LOG_ERROR) << "Looking up symbol '" << fun_name << "' failed: '"
                     << err_msg;
      return false;
    }
  }
  return true;
}

} } } // namespace arcsim::util::system

