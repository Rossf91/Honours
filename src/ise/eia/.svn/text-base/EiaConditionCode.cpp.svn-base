//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================
//
//  Description:
//
//  EiaConditionBase class
//
// =====================================================================


#include "ise/eia/EiaConditionCode.h"
#include "ise/eia/EiaExtension.h"


namespace arcsim {
  namespace ise {
    namespace eia {
      
      // Constructor
      //
      EiaConditionCode::EiaConditionCode(EiaExtension* _parent,
                                         std::string   _name,
                                         uint32        _number)
      : parent(*_parent),
        id(_parent->get_id()),
        name(_name),
        number(_number)
      { /* EMPTY */ }
      
      
      // Destructor
      //
      EiaConditionCode::~EiaConditionCode()
      { /* EMPTY */ }      
      
      
      bool
      EiaConditionCode::eval_condition_code (uint8 cc)
      {
        return true;
      }
      
} } } //  arcsim::ise::eia
