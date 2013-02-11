//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#ifndef INC_ISE_EIA_EIACOREREGISTERINTERFACE_H_
#define INC_ISE_EIA_EIACOREREGISTERINTERFACE_H_

#include "api/types.h"

namespace arcsim {
  namespace ise {
    namespace eia {
      
      /**
       * EiaCoreRegisterInterface.
       * The EiaCoreRegisterInterface is an interface defining the methods/behaviour
       * of extension core registers. Each extension core register code MUST implement
       * this interface.
       */
      class EiaCoreRegisterInterface {
      protected:
        // Constructor MUST be protected and empty!
        //
        EiaCoreRegisterInterface()
        { /* EMPTY */ }
        
      public:
        /**
         * Destructor of interface must be declared AND virtual so all implementations
         * of this interface can be destroyed correctly.
         */
        virtual ~EiaCoreRegisterInterface()
        { /* EMPTY */ }
        
        /**
         * Get name of EiaCoreRegister.
         * @return core register name.
         */
        virtual const char*   get_name()            const = 0;
        
        /**
         * Get identity of the owning EiaExtension.
         * @return extension identity.
         */
        virtual uint32  get_id()                    const = 0;
        
        /**
         * Get number of EiaCoreRegister.
         * @return core register number.
         */
        virtual uint32        get_number()          const = 0;

        /**
         * Get value of EiaCoreRegister.
         * @return core register value.
         */
        virtual uint32        get_value()           const = 0;

        /**
         * Get pointer to EiaCoreRegister contents for fast access.
         * @return pointer to EiaCoreRegister contens.
         */
        virtual uint32*       get_value_ptr()             = 0;
        
        /**
         * Write direct permission.
         * @return write direct permission status.
         */
        virtual bool          is_write_direct()     const = 0;

        /**
         * Write protected permission.
         * @return write protected permission status.
         */
        virtual bool          is_write_protected()  const = 0;
        
        /**
         * Write only permission.
         * @return write only permission status.
         */        
        virtual bool          is_write_only()       const = 0;
        
        /**
         * Read only permission.
         * @return read only permission status.
         */        
        virtual bool          is_read_only()        const = 0;

        
      };          
      
    } } } //  arcsim::ise::eia


#endif  // INC_ISE_EIA_EIACOREREGISTERINTERFACE_H_
