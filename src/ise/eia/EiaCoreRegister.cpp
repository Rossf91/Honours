//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
//  Description:
//
//  EiaCoreRegister class
//
// =====================================================================


#include "ise/eia/EiaCoreRegister.h"
#include "ise/eia/EiaExtension.h"


namespace arcsim {
  namespace ise {
    namespace eia {
      
      // Constructor
      //
      EiaCoreRegister::EiaCoreRegister(EiaExtension* _parent,
                                       std::string   _name,
                                       uint32        _number,
                                       uint32        _initial_value,
                                       bool          _w_direct,
                                       bool          _w_prot,
                                       bool          _w_only,
                                       bool          _r_only)
      : name(_name),
        number(_number),
        id(_parent->get_id()),
        value(_initial_value),
        w_direct(_w_direct),
        w_prot(_w_prot),
        w_only(_w_only),
        r_only(_r_only)
      { /* EMPTY */ }
      
      // Destructor
      //
      EiaCoreRegister::~EiaCoreRegister()
      { /* EMPTY */ }
      
      
} } } //  arcsim::ise::eia
