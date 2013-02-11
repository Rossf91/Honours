//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================

#ifndef _INC_ISE_EIA_EIAAUXREGISTERDEF_H_
#define _INC_ISE_EIA_EIAAUXREGISTERDEF_H_

#include "ise/eia/EiaAuxRegisterInterface.h"

#include "api/types.h"

#include <string>

namespace arcsim {
  namespace ise {
    namespace eia {
      
      // Forward declare EiaExtension
      //
      class EiaExtension;
      
      /**
       * EiaAuxRegister class.
       * Extension Auxiliary Register.
       */
      class EiaAuxRegister:
        public EiaAuxRegisterInterface
      {
      public:      
        /**
         * Identity of parent extension
         */
        const uint32      id;
        /**
         * Register name.
         */
        const std::string name;
        /**
         * Register number.
         */
        const uint32      number;
        /**
         * Register value.
         */
        uint32            value;
        /**
         * Write direct permission.
         */
        const bool        w_direct;
        /**
         * Write protected permission.
         */
        const bool        w_prot;
        /**
         * Write only permission.
         */
        const bool        w_only;
        /**
         * Read only permission.
         */
        const bool        r_only;

        /**
         * EiaAuxRegister constructor.
         * @param _name of register.
         * @param _number of register.
         * @param _initial_value of register.
         * @param _w_direct permission.
         * @param _w_prot permission.
         * @param _w_only permission.
         * @param _r_only permission.         
         */
        explicit EiaAuxRegister(EiaExtension* _parent,
                                std::string   _name,
                                uint32        _number,
                                uint32        _initial_value = 0,
                                bool          _w_direct      = true,
                                bool          _w_prot        = false,
                                bool          _w_only        = false,
                                bool          _r_only        = false);

        ~EiaAuxRegister();
        
        const char* get_name()      const { return name.c_str();  }
        uint32      get_number()    const { return number;        }
        uint32      get_id()        const { return id;            }
        
        uint32      get_value()     const { return value;         }
        uint32*     get_value_ptr()       { return &value;        }
        
        bool is_write_direct()      const { return w_direct;      }
        bool is_write_protected()   const { return w_prot;        }
        bool is_write_only()        const { return w_only;        }
        bool is_read_only()         const { return r_only;        }
        
      };          

} } } //  arcsim::ise::eia

#endif  // _INC_ISE_EIA_EIAAUXREGISTERDEF_H_
