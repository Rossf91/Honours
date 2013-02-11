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

#ifndef _SharedLibrary_h_
#define _SharedLibrary_h_


namespace arcsim {
  namespace util {
    namespace system {
      
// Static class
//
class SharedLibrary {
private:
  // Constructor is private to avoid instantiation 
  //
  SharedLibrary() { /* EMPTY */ }
  
  SharedLibrary(const SharedLibrary & m);   // DO NOT COPY
  void operator=(const SharedLibrary &);    // DO NOT ASSIGN
  
public:
  // Open and load shared library setting the handle
  //
  static bool open(void** handle, const std::string& path);
  
  // Open and load shared library 
  //
  static bool open(const std::string& path);

  // Close shared library
  //
  static bool close(void* handle);

  // Lookup symbol in loaded library
  //
  static bool lookup_symbol(void**        fun_handle,
                            void*         lib_handle,
                            const char *  fun_name);

};

} } } // namespace arcsim::util::system
  
#endif /*  _SharedLibrary_h_ */
