//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
//  Description:
//
//  EiaConditionBase class
//
// =============================================================================


#ifndef _INC_ISE_EIA_CONDITIONCODEDEF_H_
#define _INC_ISE_EIA_CONDITIONCODEDEF_H_

#include "ise/eia/EiaConditionCodeInterface.h"
#include <string>

namespace arcsim {
  namespace ise {
    namespace eia {
      
      // Forward declare EiaExtension
      //
      class EiaExtension;

      /**
       * EiaConditionCode base class.
       * When defining new extension condition codes one should derive the new
       * extension condition code from this class.
       */
      class EiaConditionCode :
        public EiaConditionCodeInterface
      {
      private:
        std::string       name;
        uint32            number;
        const uint32      id;
        
      protected:
        /**
         * Extension condition code has direct access to state defined by their enclosing extension.
         */
        EiaExtension& parent;
        
      public:
        
        /**
         * EiaConditionCode constructor.
         * @param _parent enclosing EiaExtension.
         * @param _name of condition code.
         * @param _number of condition code.
         */
        explicit EiaConditionCode(EiaExtension* _parent,
                                  std::string   _name,
                                  uint32        _number);
        
        ~EiaConditionCode();
        
        
        // Interface methods ---------------------------------------------------
        //

        uint32        get_id()      const    { return id;            }
        const char*   get_name()    const    { return name.c_str();  }
        uint32        get_number()  const    { return number;        }
        bool          eval_condition_code (uint8 cc);
        
      };          
      
    } } } //  arcsim::ise::eia

#endif  // _INC_ISE_EIA_CONDITIONCODEDEF_H_
