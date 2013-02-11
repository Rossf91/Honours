//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#ifndef INC_ISE_EIA_EIAAUXREGISTERINTERFACE_H_
#define INC_ISE_EIA_EIAAUXREGISTERINTERFACE_H_

#include "api/types.h"

namespace arcsim {
  namespace ise {
    namespace eia {
      
      /**
       * EiaAuxRegisterInterface.
       * The EiaAuxRegisterInterface is an interface defining the methods/behaviour
       * of extension auxiliary registers. Each extension aux register code MUST
       * implement this interface.
       */
      class EiaAuxRegisterInterface {
      protected:
        // Constructor MUST be protected and empty!
        //
        EiaAuxRegisterInterface()
        { /* EMPTY */ }
        
      public:
        /**
         * Destructor of interface must be declared AND virtual so all implementations
         * of this interface can be destroyed correctly.
         */
        virtual ~EiaAuxRegisterInterface()
        { /* EMPTY */ }
        
        /**
         * Get name of EiaAuxRegister.
         * @return aux register name.
         */
        virtual const char*   get_name()            const = 0;
        
        /**
         * Get number of EiaAuxRegister.
         * @return aux register number.
         */
        virtual uint32        get_number()          const = 0;
        
        /**
         * Get value of EiaAuxRegister.
         * @return aux register value.
         */
        virtual uint32        get_value()           const = 0;
        
        /**
         * Get identity of the owning EiaExtension.
         * @return extension identity.
         */
        virtual uint32  get_id()                    const = 0;
        
        /**
         * Get pointer to EiaAuxRegister contents for fast access.
         * @return pointer to EiaAuxRegister contens.
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


#endif  // INC_ISE_EIA_EIAAUXREGISTERINTERFACE_H_
