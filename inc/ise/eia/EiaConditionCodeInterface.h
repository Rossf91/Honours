//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#ifndef INC_ISE_EIA_EIACONDITIONCODEINTERFACE_H_
#define INC_ISE_EIA_EIACONDITIONCODEINTERFACE_H_

#include "api/types.h"

namespace arcsim {
  namespace ise {
    namespace eia {

      /**
       * EiaConditionCodeInterface.
       * The EiaConditionCodeInterface is an interface defining the methods/behaviour
       * of extension condition codes. Each extension condition code MUST implement this
       * interface.
       */
      class EiaConditionCodeInterface {
      protected:
        // Constructor MUST be protected and empty!
        //
        EiaConditionCodeInterface()
        { /* EMPTY */ }

      public:
        /**
         * Destructor of interface must be declared AND virtual so all implementations
         * of this interface can be destroyed correctly.
         */
        virtual ~EiaConditionCodeInterface()
        { /* EMPTY */ }

        /**
         * Get name of EiaConditionCode.
         * @return condition code name.
         */
        virtual const char*   get_name()    const = 0;

        /**
         * Get number of EiaConditionCode.
         * @return condition code number.
         */
        virtual uint32        get_number()  const = 0;

        /**
         * Get identity of the owning EiaExtension.
         * @return extension identity.
         */
        virtual uint32  get_id()            const = 0;
        
        // Evaluate method ----------------------------------------------------
        //
        
        /**
         * This method is called for evaluating the condition code.
         *
         * @param cc condition code.
         * @return true/false.
         */
        virtual bool          eval_condition_code (uint8 cc) = 0;
        
      };          
      
    } } } //  arcsim::ise::eia


#endif  // INC_ISE_EIA_EIACONDITIONCODEINTERFACE_H_
