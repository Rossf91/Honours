//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// A Context holds all Items that have been created via appropriate
// factory methods (i.e. it is a container for DI manged objects).
//
// All Items in a Context are Singletons identified by a unique name within
// that Context. Contexts are organised in a Hierarchy. The 'root' context
// at the top of the Hierarchy can be refered to as the 'kLGlobal' context.
//
// =====================================================================

#ifndef INC_DI_CONTEXT_H_
#define INC_DI_CONTEXT_H_

#include "api/types.h"

#include "ioc/ContextItemInterface.h"
#include "concurrent/Mutex.h"

#include <set>
#include <map>

namespace arcsim {
  namespace ioc {
    
    // -------------------------------------------------------------------------
    // A Context holds all Itmes that have been created via appropriate
    // factory methods (i.e. it is a container for DI managed objects).
    //
    class Context
    {
    public:
      // Predefined Context Levels
      //
      enum Level {
       kLGlobal     = 0U,
       kLSystem     = 1U,
       kLModule     = 2U,
       kLProcessor  = 3U,
       kLevelCount  = 4U
      };
      
      // Predefined Context Level Name Tokens
      //
      static const char* kNGlobal;
      static const char* kNSystem;
      static const char* kNModule;
      static const char* kNProcessor;
      
      // Maximum name lenght
      //
      static const int kContextNameSize = 256;

      // Retrieve Context id, level, name, or parent
      //
      uint32          get_id()     const { return id_;     }
      Level           get_level()  const { return level_;  }
      const uint8*    get_name()   const { return name_;   }
      Context * const get_parent() const { return parent_; }
      
      // ----------------------------------------------------------------------- 
      // Retrieve a ContextItemInterface with item_id if it exists, otherwise return 0
      //
      ContextItemInterface* get_item(const char* item_name) const;

      // ----------------------------------------------------------------------- 
      // Register item - allow registration of context items if they do not exist
      //                 yet
      //
      bool register_item(ContextItemInterface* item);
      
      // -----------------------------------------------------------------------
      // Create ContextItemInterface instance if it does not exist, otherwise
      // return pointer to already existing ContextItemInterface
      //
      ContextItemInterface* create_item(ContextItemInterface::Type item_type,
                                        const char*                item_name);
      
      // ----------------------------------------------------------------------- 
      // Retrieve a Context with ctx_id if it exists, otherwise return 0
      //
      Context* get_context(uint32   ctx_id);

      // ----------------------------------------------------------------------- 
      // Create pointer to Context instance if it does not exist, otherwise
      // return pointer to already existing Context instance
      //
      Context* create_context(uint32       ctx_id,
                              const char*  ctx_name);
            
      // ----------------------------------------------------------------------- 
      // Get Global context, the root of our Hierarchy
      //
      static Context& Global();
      static Context* GlobalPtr();
      
      ~Context();

    private:
      Context * const     parent_;
      const uint32        id_;
      const Level         level_;
      uint8               name_[kContextNameSize];

      // -----------------------------------------------------------------------
      // Children Contexts
      //
      arcsim::concurrent::Mutex     children_mtx_;
      std::map<uint32,Context*>     children_;
      
      // -----------------------------------------------------------------------
      // Managed ContextItems
      //
      arcsim::concurrent::Mutex                                       items_mtx_;
      std::set<ContextItemInterface*,ContextItemInterfaceComparator>  items_;
      
      // -----------------------------------------------------------------------
      // Constructor is private because only a Context knows how to create itself
      // via special methods
      //
      explicit Context(Context * const parent,
                       Level           level,
                       uint32          id,
                       const char *    name);

    };
    
  } } // namespace arcsim::ioc

#endif  // INC_DI_CONTEXT_H_

