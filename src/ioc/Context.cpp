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

#include <cstring>

#include "concurrent/ScopedLock.h"

#include "ioc/Context.h"
#include "ioc/ContextItemInterface.h"
#include "ioc/ContextItemInterfaceFactory.h"

namespace arcsim {
  namespace ioc {
    
    // ----------------------------------------------------------------------- 
    // Predefined Context Level Name Tokens
    //
    const char* Context::kNGlobal     = "Global Context";
    const char* Context::kNSystem     = "System Context";
    const char* Context::kNModule     = "Module Context";
    const char* Context::kNProcessor  = "Processor Context";
    
    // ----------------------------------------------------------------------- 
    // Global Context is special - it is the root of our hierarchy. It must be
    //                             locked using a global mutex. This mutex must
    //                             be instantiated in a thread-safe way.
    //
    
    // @igor - We need to construct the  'global_ctx_mutex_' in a thread safe
    //         way which given that initialisation order in C++ is not really
    //         well defined can be a bit of a problem.
    //
    //         We take advantage of the fact that initiliasation before main() 
    //         is called will take place on a SINGLE thread, and that 'instance_'
    //         must be zero initialised before dynamic initialisation of static
    //         objects takes place (3.6.2 "Initialization of non-local objects")
    //
    class GlobalContextMutex {
    public:  
      static arcsim::concurrent::Mutex& Get() {
        if (!instance_) {
          static arcsim::concurrent::Mutex default_instance;
          instance_ = &default_instance;
        }
        return *instance_;
      }
    private:     
      static arcsim::concurrent::Mutex * instance_;      
      GlobalContextMutex() { }; // DEFAULT CTOR must be private!
    };
    
    // NOTE: @igor - order of initialisation matters here: GlobalContextMutex::instance_
    //       must be initialised before 'global_ctx_mutex_'.
    
    // initialise 'GlobalContextMutex::instance_' first - THIS IS THREAD-SAFE as it runs BEFORE main
    arcsim::concurrent::Mutex * GlobalContextMutex::instance_ = &GlobalContextMutex::Get();
    
    // now we can safely initiliase 'global_ctx_mutex_'
    static arcsim::concurrent::Mutex& global_ctx_mutex_       = GlobalContextMutex::Get();
    static Context *                  global_ctx_             = 0;

    // -------------------------------------------------------------------------
    // Constructor a Context
    //
    Context::Context(Context * const parent,
                     Level           level,
                     uint32          id,
                     const char*     name)
    : parent_(parent),
      id_(id),
      level_(level)
    {
      uint32 i;
      // Name
      //
      for (i = 0; i < kContextNameSize - 1 && name[i]; ++i)
        name_[i] = static_cast<uint8>(name[i]);
      name_[i] = '\0';
    }
    
    // -------------------------------------------------------------------------
    // Destruct Context
    //
    Context::~Context()
    {
      // Call destructor for EACH child context. NOTE that each child context
      // will in turn call its destructor recursively.
      while (!children_.empty()) {
        delete (children_.begin())->second;
      }

      // Call destructor for EACH context item
      while (!items_.empty()) {
        delete *items_.begin();
        items_.erase(items_.begin());
      }
      // If there is a parent context remove its link to 'this' context
      if (parent_) {
        parent_->children_.erase(id_);
      }
    }
    
    // ------------------------------------------------------------------------- 
    // Get Global context, the root of our Hierarchy
    //
    Context*
    Context::GlobalPtr()
    {
      arcsim::concurrent::ScopedLock lock(global_ctx_mutex_);
      if (global_ctx_) {
        return global_ctx_;
      } else {
        global_ctx_ = new Context(0, Context::kLGlobal, 0, kNGlobal);
        return global_ctx_;
      }
    }
    
    Context&
    Context::Global()
    {
      return *GlobalPtr();
    }
    
    // ------------------------------------------------------------------------- 
    // Retrieve a ContextItemInterface with item_id if it exists, otherwise return 0
    //
    ContextItemInterface*
    Context::get_item(char const * item_name) const
    {
      for (std::set<ContextItemInterface*,ContextItemInterfaceComparator>::const_iterator
           I = items_.begin(),
           E = items_.end();
           I != E; ++I)
      {
        if (std::strcmp((char const*)((*I)->get_name()),item_name) == 0 ) {
          return *I;
        }
      }      
      return 0;      
    }

    // ----------------------------------------------------------------------- 
    // Register item - allow registration of context items if they do not exist
    //                 yet
    //
    bool
    Context::register_item(ContextItemInterface* item)
    {
      arcsim::concurrent::ScopedLock lock(items_mtx_);
      
      if (item != 0 && (items_.find(item) == items_.end()) ) {
        items_.insert(item);
        return true;
      }
      return false;
    }

    
    // -------------------------------------------------------------------------
    // Create ContextItemInterface instance if it does not exist, otherwise
    // return pointer to already existing ContextItemInterface
    //
    ContextItemInterface*
    Context::create_item(ContextItemInterface::Type item_type,
                         const char*                item_name)
    {
      arcsim::concurrent::ScopedLock lock(items_mtx_);

      // Call get_item() to retrieve a potentially existing ContextItem
      //
      ContextItemInterface* item = get_item(item_name);
      
      if (item == 0) {
        // Item does not exist so we need to create it
        //
        item = ContextItemInterfaceFactory::create(*this, item_type, item_name);
        // Insert Item into set of managed items in this context
        //
        items_.insert(item);
      }
      
      return item;
    }
    
    // ------------------------------------------------------------------------- 
    // Retrieve a Context with ctx_id if it exists, otherwise return 0
    //
    Context*
    Context::get_context(uint32   ctx_id)
    {
      std::map<uint32,Context*>::const_iterator I = children_.find(ctx_id);
      return (I != children_.end()) ? I->second : 0;
    }
    
    // ------------------------------------------------------------------------- 
    // Create pointer to Context instance if it does not exist, otherwise
    // return pointer to already existing Context instance
    //
    Context*
    Context::create_context(uint32       ctx_id,
                            const char*  ctx_name)
    {
      arcsim::concurrent::ScopedLock lock(children_mtx_);
      
      // Catt get_context() to retrieve a potentially existing Context
      //
      Context* ctx = get_context(ctx_id);
      
      if (ctx == 0) {
        // Context does not exist so we create it
        //
        ctx = new Context(this, (Level)(get_level()+1), ctx_id, ctx_name);
        // Insert Context into map of child contexts
        //
        children_[ctx_id] = ctx;
      }
      
      return ctx;
    }
    
} }
