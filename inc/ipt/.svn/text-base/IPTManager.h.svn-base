//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// The IPTManager class is responsible for mainting various Instrumentation PoinTs. 
//
// IPTManagers are arcsim::ioc::Context managed. In order to retrieve an IPTManager
// instance for a given processor one can use the following code:
//
//  using namespace arcsim::ioc;
//  Context& sysctx = *Context::Global().get_context(sys_id);
//  Context& modctx = *(sysctx.get_context(0));
//  Context& cpuctx = *(modctx.get_context(0));
//  IPTManager& iptmgr = static_cast<IPTManager*>(cpuctx.get_item(ContextItemId::kIPTManager));
//
// =====================================================================


#ifndef INC_IPT_IPTMANAGER_H_
#define INC_IPT_IPTMANAGER_H_

#include <map>
#include <set>

#include "api/ipt_types.h"
#include "ioc/ContextItemInterface.h"

namespace arcsim {
  // -------------------------------------------------------------------------
  // Forward declaration
  //
  namespace ioc {
    class Context;
  }
  namespace profile {
    class PhysicalProfile;
  }
  
  namespace ipt {
        
    // -------------------------------------------------------------------------
    // IPTManager class
    //
    class IPTManager  : public arcsim::ioc::ContextItemInterface
    {
    private:
      
      // -----------------------------------------------------------------------
      // Generic IPTEntry Template structure definition
      //
      template<typename Obj, typename Fun>
      struct IPTEntry {
        Obj     obj;    // handle to IPT instance
        Fun     fun;    // handle to IPT callback function

        // Constructors
        IPTEntry()
        : obj(0), fun(0)
        { /* EMPTY */ }
        IPTEntry(Obj obj, Fun fun)
        : obj(obj), fun(fun)
        { /* EMPTY */ }
        
        // Equality operator
        bool operator==(const IPTEntry& that) const {
          return (this->obj == that.obj) && (this->fun == that.fun);
        }
      };

      // -----------------------------------------------------------------------
      // IPTEntry Structure Template instantiations
      //
      
      // AboutToExecuteInstructionIPTEntry structure
      //
      typedef IPTEntry<HandleAboutToExecuteInstructionObj,
                       HandleAboutToExecuteInstructionFun> AboutToExecuteInstructionIPTEntry;
      // HandleBeginInstructionExecutionIPTEntry structure
      //
      typedef IPTEntry<HandleBeginInstructionExecutionObj,
                       HandleBeginInstructionExecutionFun> HandleBeginInstructionExecutionIPTEntry;
      // HandleEndInstructionExecutionIPTEntry structure
      //
      typedef IPTEntry<HandleEndInstructionExecutionObj,
                       HandleEndInstructionExecutionFun>   HandleEndInstructionExecutionIPTEntry;      
      // HandleBeginBasicBlockIPTEntry structure
      //
      typedef IPTEntry<HandleBeginBasicBlockObj,
                       HandleBeginBasicBlockFun>           HandleBeginBasicBlockIPTEntry;      
      
    public:
      // Maximum name lenght
      //
      static const int kIPTManagerMaxNameSize = 256;

      // Constructor/Destructor
      //
      explicit IPTManager(arcsim::ioc::Context& ctx, const char* name);
      ~IPTManager();

      const uint8* get_name() const { return name_; };
      const Type   get_type() const { return arcsim::ioc::ContextItemInterface::kTIPTManager;  };

      // -----------------------------------------------------------------------
      // Available instrumentation point kinds
      //
      enum IPTKind {
        kIPTAboutToExecuteInstruction = 0x1,
        kIPTBeginInstruction          = 0x2,
        kIPTEndInstruction            = 0x4,
        kIPTBeginBasicBlockInstruction= 0x8,
      };
      
      // -----------------------------------------------------------------------
      // Query
      //
      
      // Quick check if any IPTs are enabled
      inline bool is_enabled()             const { return (active_ipts_ != 0x0); }

      // Check if specific IPT is enabled
      inline bool is_enabled(IPTKind kind) const { return (active_ipts_ & kind); }
      
      // O(log n) worst case execution time to check if address corresponds to
      // registered AboutToExecuteInstructionIPT (NOTE: n corresponds to number
      // of addresses for which AboutToExecuteInstructionIPT are registered)
      inline bool is_about_to_execute_instruction(uint32 addr) const {
        return (about_to_execute_instruction_map_.find(addr) != about_to_execute_instruction_map_.end());
      }
      
      // -----------------------------------------------------------------------
      // Registration
      //
      
      // ----- AboutToExecuteInstructionIPT
      
      // Insert - AboutToExecuteInstructionIPT
      bool insert_about_to_execute_instruction_ipt(uint32                             addr,
                                                   HandleAboutToExecuteInstructionObj obj,
                                                   HandleAboutToExecuteInstructionFun fun);
      
      // Remove ALL AboutToExecuteInstructionIPTs for given address
      bool remove_about_to_execute_instruction_ipt(uint32 addr);
      
      // Remove AboutToExecuteInstructionIPT subscriber for given address
      bool remove_about_to_execute_instruction_ipt(uint32                             addr,
                                                   HandleAboutToExecuteInstructionObj obj,
                                                   HandleAboutToExecuteInstructionFun fun);

      // ----- HandleBeginInstructionExecutionIPT
      
      // Insert - HandleBeginInstructionExecutionIPT
      bool insert_begin_instruction_execution_ipt(HandleBeginInstructionExecutionObj obj,
                                                  HandleBeginInstructionExecutionFun fun);
      
      // Remove ALL HandleBeginInstructionExecutionIPTs for given address
      bool remove_begin_instruction_execution_ipt();
      
      // Remove HandleBeginInstructionExecutionIPT subscriber for given address
      bool remove_begin_instruction_execution_ipt(HandleBeginInstructionExecutionObj obj,
                                                  HandleBeginInstructionExecutionFun fun);

      // ----- HandleEndInstructionExecutionIPT

      // Insert - HandleEndInstructionExecutionIPT
      bool insert_end_instruction_execution_ipt(HandleEndInstructionExecutionObj obj,
                                                HandleEndInstructionExecutionFun fun);
      
      // Remove ALL HandleEndInstructionExecutionIPTs for given address
      bool remove_end_instruction_execution_ipt();
      
      // Remove HandleEndInstructionExecutionIPT subscriber for given address
      bool remove_end_instruction_execution_ipt(HandleEndInstructionExecutionObj obj,
                                                HandleEndInstructionExecutionFun fun);

      
      // ----- HandleBeginBasicBlockInstructionIPT
      
      // Insert - HandleBeginBasicBlockInstructionIPT
      bool insert_begin_basic_block_instruction_execution_ipt(HandleBeginBasicBlockObj obj,
                                                              HandleBeginBasicBlockFun fun);
      
      // Remove ALL HandleBeginBasicBlockInstructionIPTs for given address
      bool remove_begin_basic_block_instruction_execution_ipt();
      
      // Remove HandleBeginBasicBlockInstructionIPT subscriber for given address
      bool remove_begin_basic_block_instruction_execution_ipt(HandleBeginBasicBlockObj obj,
                                                              HandleBeginBasicBlockFun fun);

      
      // -----------------------------------------------------------------------
      // Execute/Notification handlers
      //
      
      // Execute - AboutToExecuteInstructionIPT
      inline bool exec_about_to_execute_instruction_ipt_handlers(uint32 addr)
      { // find subscribers for AboutToExecuteInstructionIPTEntry
        std::pair<std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::const_iterator,
                  std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::const_iterator> elems;
        elems = about_to_execute_instruction_map_.equal_range(addr);

        // NOTE: We need to collect all subscribers first BEFORE we execute their
        //       callback functions as they can modify the 'about_to_execute_instruction_map_'
        //       as a side-effect (i.e. remove/insert elements) of the callback,
        //       destroying iterators.
        
        // COLLECT all registered subscribers
        for (std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::const_iterator I = elems.first;
             I != elems.second; ++I) {
          active_about_to_execute_handlers_.insert(I->second);
        }        
        
        // CALL registered subscribers
        bool ret = false;
        while (!active_about_to_execute_handlers_.empty()) {
          AboutToExecuteInstructionIPTEntry * ipt = *active_about_to_execute_handlers_.begin();
          active_about_to_execute_handlers_.erase(ipt);
          if (ipt->fun && ipt->fun(reinterpret_cast<IocContext>(&ctx_),
                                   reinterpret_cast<IocContextItem>(this),
                                   ipt->obj, addr))
            ret = true;
        }
        // it is enough if one subscriber want's back control for simulation to be 'paused'
        return ret;        
      }
      
      inline void notify_begin_instruction_execution_ipt_handlers(uint32 addr, uint32 len)
      { // COLLECT all registered subscribers
        active_begin_instruction_execution_handlers_ = begin_instruction_execution_set_;
        
        // CALL registered subscribers
        while (!active_begin_instruction_execution_handlers_.empty()) {
          HandleBeginInstructionExecutionIPTEntry * ipt = *active_begin_instruction_execution_handlers_.begin();
          active_begin_instruction_execution_handlers_.erase(ipt);
          if (ipt->fun)
            ipt->fun(reinterpret_cast<IocContext>(&ctx_),
                     reinterpret_cast<IocContextItem>(this),
                     ipt->obj, addr, len);
        }
      }
      
      inline void notify_begin_basic_block_instruction_execution_ipt_handlers(uint32 addr)
      { // COLLECT all registered subscribers
        active_begin_basic_block_instruction_execution_handlers_ = begin_basic_block_instruction_execution_set_;
        
        // CALL registered subscribers
        while (!active_begin_basic_block_instruction_execution_handlers_.empty()) {
          HandleBeginBasicBlockIPTEntry * ipt = *active_begin_basic_block_instruction_execution_handlers_.begin();
          active_begin_basic_block_instruction_execution_handlers_.erase(ipt);
          if (ipt->fun)
            ipt->fun(reinterpret_cast<IocContext>(&ctx_),
                     reinterpret_cast<IocContextItem>(this),
                     ipt->obj, addr);
        }

      }
      
    private:      
      uint8  name_[kIPTManagerMaxNameSize];
      uint32 active_ipts_;  // 32-bit bit-set denoting active IPTs

      // Enclosing context
      //
      arcsim::ioc::Context&            ctx_;
      
      // PhysicalProfile
      //
      const arcsim::profile::PhysicalProfile& phys_prof_;
      
      // -----------------------------------------------------------------------
      // AboutToExecuteInstructionIPT container structures
      
      // Multimap of AboutToExecuteInstructionIPT subscribers.
      // NOTE: there can be more than one subscriber per address.
      //
      std::multimap<uint32, AboutToExecuteInstructionIPTEntry*> about_to_execute_instruction_map_;
      // Temporary container for storing currently active AboutToExecuteInstructionIPT subscribers
      // before executing their callback method. @see exec_about_to_execute_instruction_ipt_handlers()
      //
      std::set<AboutToExecuteInstructionIPTEntry*> active_about_to_execute_handlers_;

      // -----------------------------------------------------------------------
      // HandleBeginInstructionExecutionIPT container structures
      
      // Set of HandleBeginInstructionExecutionIPT subscribers
      //
      std::set<HandleBeginInstructionExecutionIPTEntry*> begin_instruction_execution_set_;
      // Temporary container for storing currently active HandleBeginInstructionExecutionIPT
      // subscribers before executing their callback method.
      //
      std::set<HandleBeginInstructionExecutionIPTEntry*> active_begin_instruction_execution_handlers_;

      // -----------------------------------------------------------------------
      // HandleEndInstructionExecutionIPT container structures
      
      // Set of HandleEndInstructionExecutionIPT subscribers
      //
      std::set<HandleEndInstructionExecutionIPTEntry*> end_instruction_execution_set_;
      // Temporary container for storing currently active HandleEndInstructionExecutionIPT subscribers
      // before executing their callback method.
      //
      std::set<HandleEndInstructionExecutionIPTEntry*> active_end_instruction_execution_handlers_;

      // -----------------------------------------------------------------------
      // HandleBeginBasicBlockInstructionIPT container structures
      
      // Set of HandleBeginBasicBlockIPTEntry subscribers
      //
      std::set<HandleBeginBasicBlockIPTEntry*> begin_basic_block_instruction_execution_set_;
      // Temporary container for storing currently active HandleBeginBasicBlockIPTEntry subscribers
      // before executing their callback method.
      //
      std::set<HandleBeginBasicBlockIPTEntry*> active_begin_basic_block_instruction_execution_handlers_;
      
    };
    
    
} } // arcsim::ipt

#endif  // INC_IPT_IPTMANAGER_H_
