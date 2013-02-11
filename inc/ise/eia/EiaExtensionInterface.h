//                          Copyright Notice
//
//    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
//  The University Court of the University of Edinburgh. All Rights Reserved.
//
// =============================================================================
//
//  Description:
//
//  Factory class and Interface declarations that must be implemented by
//  and EiaExtension.
//
// =============================================================================


#ifndef INC_ISE_EIA_EIAEXTENSIONINTERFACE_H_
#define INC_ISE_EIA_EIAEXTENSIONINTERFACE_H_

#include "api/types.h"

/**
 * EiaExtensionInterfacePtr type declaration acts as an opaque wrapper for
 * @see arcsim::ise::eia::EiaExtensionInterface() pointer types.
 */  
typedef void* EiaExtensionInterfacePtr;

/**
 * The simLoadEiaExtension() function MUST be implemented by the shared library
 * that contains one or more EiaExtensions. It is called by an external agent
 * (e.g. a simulator) at the appropriate time in order to enable the shared library
 * that implements one or more EiaExtensions to register the extensions via the
 * @see simEiaExtensionRegister() function.
 *
 * @param simContext the simulation context
 *
 */
extern "C" DLLEXPORT void simLoadEiaExtension (simContext sim_ctx);

/**
 * The simRegisterEiaExtension() function is implemented by an external agent
 * (i.e. a simulator) and should be called by the shared library implementing one or
 * more EiaExtensions in order to register an EiaExtension with a cpuContext.
 *
 * Note that the cpuContext the EiaExtension has been registered with takes
 * responsibility for destructing the EiaExtension.
 * 
 * @param simContext the simulation context
 * @param cpu_id the cpu id for which to register the respective extension
 * @param pointer to fully instantiated @see EiaInstructionInterface
 *
 */
extern "C" DLLEXPORT void simRegisterEiaExtension (simContext               sim_ctx,
                                                   uint32                   cpu_id,
                                                   EiaExtensionInterfacePtr eia_ext);


/**
 * The simCreateEiaExtension() function CAN be implemented by a shared library
 * that contains one or more EiaExtensions. It is called by a test harness at the
 * appropriate time in order to retrieve a fully instantiated EiaExtension for
 * testing. If a shared library contains several EiaExtensions the test harness
 * will call this method incrementing the parameter id by 1 starting from 0,
 * until a null pointer is returned. So if a shared library implements 10 EiaExtensions,
 * it should return an instance to the first EiaExtension for id 0, to the second
 * for id 1, etc. until it is called with id '10' when it should return NULL to
 * signal the test harness that all EiaExtensions have been instantiated.
 *
 * Note that the test harness the EiaExtension has been registered with takes
 * responsibility for destructing the EiaExtension.
 *
 * @param the id of the EIA instruction that should be returned
 * @return EiaExtensionInterfacePtr to fully instantiated EiaExtension
 *
 */
extern "C" DLLEXPORT EiaExtensionInterfacePtr simCreateEiaExtension(uint32 eia_id);


// -----------------------------------------------------------------------------


namespace arcsim {
  namespace ise  {
    namespace eia {
      
      // -----------------------------------------------------------------------
      // Forward decls.
      //
      class EiaInstructionInterface;
      class EiaConditionCodeInterface;
      class EiaCoreRegisterInterface;
      class EiaAuxRegisterInterface;
      class EiaExtensionInterface;
      
      // -----------------------------------------------------------------------
      // Factory defined as template:
      // @see http://stackoverflow.com/questions/3313754/static-abstract-methods-in-c
      //
      /**
       * EiaExtensionFactory class.
       * Each EIA extension should implement this static factory method for easy
       * instantiation.
       *
       * \param Class name of EiaExtension.
       */
      template <class TClass>
        class EiaExtensionFactory {
        public:
          static EiaExtensionInterface* create() { return TClass::create_internal(); }
        };

      /**
       * EiaExtensionInterface.
       * The EiaExtensionInterface is an interface defining the methods/behaviour
       * of EIA extensions. Each EIA extension MUST implement this interface.
       */
      class EiaExtensionInterface
      {
      protected:
        // Constructor MUST be protected and empty!
        //
        EiaExtensionInterface()
        { /* EMPTY */ }

      public:
        
        /** 
         * EiaApiVersion Kinds. These version numbers should be returned by
         * @see get_version().
         */
        enum EiaApiVersion {
          EIA_API_VERSION_1 = 0x00000001 /**< EIA_API_VERSION_1 - version 1 of the EIA API.      */
        };
        
        /**
         * Destructor of interface must be declared AND virtual so all implementations
         * of this interface can be destroyed correctly.
         */
        virtual ~EiaExtensionInterface()
        { /* EMPTY */ }
        
        
        /**
         * Get EIA API version number that was used to build this extension.
         * @return extension version number.
         */
        virtual uint32        get_version() const  = 0;
        
        /**
         * Get name of EiaExtension.
         * @return extension name.
         */
        virtual const char*   get_name()    = 0;
        
        /**
         * Get identity of EiaExtension.
         * @return extension identity.
         */
        virtual const uint32  get_id()      = 0;
        
        /**
         * Get comment for EiaExtension.
         * @return extension comment.
         */
        virtual const char*   get_comment() = 0;
        
        /**
         * Get count of EIA instructions.
         *
         * @return count of EIA instructions defined within this extension.
         */
        virtual uint32                     get_eia_instructions_count()          = 0;

        /**
         * Get HEAP allocated array of EiaInstructionInterface pointers of size @see get_eia_instructions_count().
         *
         * @return HEAP allocated array of EiaInstructionInterface pointers.
         */
        virtual EiaInstructionInterface**  get_eia_instructions()                = 0;
        
        /**
         * Get pointer to EiaInstructionInterface for a particular instruction.
         *
         * @param name instruction name.
         * @return pointer to EiaInstructionInterface.
         */        
        virtual EiaInstructionInterface*   get_eia_instruction(const char* name) = 0;

        /**
         * Get count of EIA condition codes.
         *
         * @return count of EIA condition codes defined within this extension.
         */
        virtual uint32                      get_cond_codes_count()               = 0;
        
        /**
         * Get HEAP allocated array of EiaConditionCodeInterface pointers of size @see get_cond_codes_count().
         *
         * @return HEAP allocated array of EiaConditionCodeInterface pointers.
         */
        virtual EiaConditionCodeInterface** get_cond_codes()                     = 0;
        
        /**
         * Get pointer to EiaConditionCodeInterface for a particular condition code.
         *
         * @param name condition code name.
         * @return pointer to EiaConditionCodeInterface.
         */        
        virtual EiaConditionCodeInterface*  get_cond_code(const char* name)      = 0;
        
        /**
         * Get map of EiaCoreRegisters keyed by name.
         *
         * @return map of EiaCoreRegisters.
         */
        virtual uint32                      get_core_registers_count()           = 0;        
        virtual EiaCoreRegisterInterface**  get_core_registers()                 = 0;
        virtual EiaCoreRegisterInterface*   get_core_register(const char* name)  = 0;
        
        /**
         * Get map of EiaAuxRegisters keyed by name.
         *
         * @return map of EiaAuxRegisters.
         */
        virtual uint32                      get_aux_registers_count()            = 0;
        virtual EiaAuxRegisterInterface**   get_aux_registers()                  = 0;
        virtual EiaAuxRegisterInterface*    get_aux_register(const char* name)   = 0;
        
      };
      
} } } //  arcsim::ise::eia

#endif  // INC_ISE_EIA_EIAEXTENSIONINTERFACE_H_
