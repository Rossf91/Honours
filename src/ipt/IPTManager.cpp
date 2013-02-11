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
// =====================================================================

#include <iomanip>

#include "Assertion.h"

#include "ipt/IPTManager.h"

#include "ioc/ContextItemInterface.h"
#include "ioc/ContextItemId.h"
#include "ioc/Context.h"

#include "sys/cpu/processor.h"
#include "profile/PhysicalProfile.h"

#include "util/Log.h"

#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_
#define PROCESSOR   (*static_cast<arcsim::sys::cpu::Processor*>(ctx_.get_item(arcsim::ioc::ContextItemId::kProcessor)))

namespace arcsim {
  namespace ipt {

    // -------------------------------------------------------------------------
    // Constructor/Destructor
    //
    IPTManager::IPTManager(arcsim::ioc::Context& ctx, const char* name)
    : active_ipts_(0),
      ctx_(ctx),
      phys_prof_(*(arcsim::profile::PhysicalProfile*)ctx.get_item(arcsim::ioc::ContextItemId::kPhysicalProfile))
    {
      ASSERT(&phys_prof_ != 0 && "[IPTManager] PhysicalProfile does not exist in Context!");
      
      uint32 i;
      for (i = 0; i < kIPTManagerMaxNameSize - 1 && name[i]; ++i)
        name_[i] = static_cast<uint8>(name[i]);
      name_[i] = '\0';      
    }

    
    IPTManager::~IPTManager() {
      // destroy HEAP allocated AboutToExecuteInstructionIPTEntry's
      while (!about_to_execute_instruction_map_.empty()) {
        delete about_to_execute_instruction_map_.begin()->second;
      }
    }
    
    // -----------------------------------------------------------------------
    // ----- AboutToExecuteInstructionIPT
    //
    
    // Insert - AboutToExecuteInstructionIPT
    //
    bool
    IPTManager::insert_about_to_execute_instruction_ipt(uint32                             addr,
                                                        HandleAboutToExecuteInstructionObj obj,
                                                        HandleAboutToExecuteInstructionFun fun)
    {
      LOG(LOG_DEBUG) << "[IPTManager] Inserting AboutToExecuteInstructionIPT for address: 0x" << HEX(addr);
      // Check if AboutToExecuteInstructionIPT has not already been registered
      if (about_to_execute_instruction_map_.find(addr) != about_to_execute_instruction_map_.end()) 
      { // retrieve AboutToExecuteInstructionIPTEntries for given key
        std::pair<std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::const_iterator,
                  std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::const_iterator> elems;
        elems = about_to_execute_instruction_map_.equal_range(addr);

        AboutToExecuteInstructionIPTEntry e = AboutToExecuteInstructionIPTEntry(obj,fun);
        // check if entry already exists
        for (std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::const_iterator I = elems.first;
             I != elems.second; ++I)
        {
          AboutToExecuteInstructionIPTEntry * ipt = I->second;
          if ( *ipt == e ) { // entry already exists!
            LOG(LOG_WARNING) << "[IPTManager] AboutToExecuteInstructionIPT for address: 0x" << HEX(addr) << " already exists.";
            return false;
          }
        }        
      }
      // Check if translation exists for given physical address
      if (phys_prof_.is_translation_present(addr)) {
        LOG(LOG_DEBUG) << "[IPTManager] AboutToExecuteInstructionIPT for address: '0x" << HEX(addr)
                       << "' resides in translated code - flushing.";
        PROCESSOR.remove_translation(addr);
      }
      // Add heap allocated AboutToExecuteInstructionIPTEntry to map
      AboutToExecuteInstructionIPTEntry* p = new AboutToExecuteInstructionIPTEntry(obj, fun);
      about_to_execute_instruction_map_.insert(std::pair<uint32,AboutToExecuteInstructionIPTEntry*>(addr,p));
      active_ipts_ |= kIPTAboutToExecuteInstruction; // mark AboutToExecuteInstructionIPT as active
      return true;
    }

    // Remove ALL AboutToExecuteInstructionIPTs for given address
    bool
    IPTManager::remove_about_to_execute_instruction_ipt(uint32 addr)
    {      
      if (about_to_execute_instruction_map_.find(addr) != about_to_execute_instruction_map_.end()) 
      { // destruct heap allocated AboutToExecuteInstructionIPTEntry instances
        std::pair<std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::const_iterator,
                  std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::const_iterator> elems;
        elems = about_to_execute_instruction_map_.equal_range(addr);
        
        for (std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::const_iterator I = elems.first;
             I != elems.second; ++I)
        {
          delete I->second; // delete AboutToExecuteInstructionIPTEntry instance
        }        
        about_to_execute_instruction_map_.erase(addr);  
        LOG(LOG_DEBUG) << "[IPTManager] Removed ALL AboutToExecuteInstructionIPT subscribers for address: 0x" << HEX(addr);
        if (about_to_execute_instruction_map_.empty()) {
          active_ipts_ &= ~kIPTAboutToExecuteInstruction; // mark AboutToExecuteInstructionIPT as NOT active
        }
        return true;
      }
      return false;
    }

    // Remove AboutToExecuteInstructionIPT subscriber for given address
    bool
    IPTManager::remove_about_to_execute_instruction_ipt(uint32                             addr,
                                                        HandleAboutToExecuteInstructionObj obj,
                                                        HandleAboutToExecuteInstructionFun fun)
    {
      if (about_to_execute_instruction_map_.find(addr) != about_to_execute_instruction_map_.end()) 
      { // destruct heap allocated AboutToExecuteInstructionIPTEntry instances
        std::pair<std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::iterator,
                  std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::iterator> elems;
        elems = about_to_execute_instruction_map_.equal_range(addr);
        
        AboutToExecuteInstructionIPTEntry e = AboutToExecuteInstructionIPTEntry(obj,fun);
        // check if entry exists
        for (std::multimap<uint32,AboutToExecuteInstructionIPTEntry*>::iterator I = elems.first;
             I != elems.second; ++I)
        {
          AboutToExecuteInstructionIPTEntry * ipt = I->second;
          if ( *ipt == e ) { // entry exists
            LOG(LOG_DEBUG) << "[IPTManager] Removed AboutToExecuteInstructionIPT subscriber for address: 0x" << HEX(addr);
            active_about_to_execute_handlers_.erase(ipt);
            delete ipt; // delete AboutToExecuteInstructionIPTEntry instance
            about_to_execute_instruction_map_.erase(I); // erase by iterator
            break;
          }
        }        
        if (about_to_execute_instruction_map_.empty()) {
          active_ipts_ &= ~kIPTAboutToExecuteInstruction; // mark AboutToExecuteInstructionIPT as NOT active
        }
        return true;
      }
      return false;
    }

    // -----------------------------------------------------------------------
    // ----- HandleBeginInstructionExecutionIPT
    //
    
    // Insert - HandleBeginInstructionExecutionIPT
    bool 
    IPTManager::insert_begin_instruction_execution_ipt(HandleBeginInstructionExecutionObj obj,
                                                       HandleBeginInstructionExecutionFun fun)
    {
      LOG(LOG_DEBUG) << "[IPTManager] Inserting HandleBeginInstructionExecutionIPT.";
      HandleBeginInstructionExecutionIPTEntry e = HandleBeginInstructionExecutionIPTEntry(obj,fun);
      for (std::set<HandleBeginInstructionExecutionIPTEntry*>::const_iterator
            I = begin_instruction_execution_set_.begin(),
            E = begin_instruction_execution_set_.end(); I != E; ++I)
      {
        if ( *(*I) == e ) { // entry already exists!
          LOG(LOG_WARNING) << "[IPTManager] HandleBeginInstructionExecutionIPT already exists.";
          return false;
        }
      }
      if (!(active_ipts_ & kIPTBeginInstruction)) {
        LOG(LOG_DEBUG) << "[IPTManager] HandleBeginInstructionExecutionIPT activated - flushing translations.";
        PROCESSOR.remove_translations(); // remove ALL translations
      }
      // Add heap allocated HandleBeginInstructionExecutionIPTEntry to map
      begin_instruction_execution_set_.insert(new HandleBeginInstructionExecutionIPTEntry(obj, fun));
      active_ipts_ |= kIPTBeginInstruction; // mark HandleBeginInstructionExecutionIPTEntry as active
      return true;
    }
    
    // Remove ALL HandleBeginInstructionExecutionIPTs for given address
    bool
    IPTManager::remove_begin_instruction_execution_ipt()
    {
      while (!begin_instruction_execution_set_.empty()) {
        delete *(begin_instruction_execution_set_.begin());
        begin_instruction_execution_set_.erase(begin_instruction_execution_set_.begin());
      }
      if (active_ipts_ & kIPTBeginInstruction) {
        LOG(LOG_DEBUG) << "[IPTManager] Removed ALL HandleBeginInstructionExecutionIPTEntry subscribers.";
        active_ipts_ &= ~kIPTBeginInstruction; // mark AboutToExecuteInstructionIPT as NOT active
        PROCESSOR.remove_translations(); // remove ALL translations
        return true;
      }
      return false;
    }
    
    // Remove HandleBeginInstructionExecutionIPT subscriber for given address
    bool
    IPTManager::remove_begin_instruction_execution_ipt(HandleBeginInstructionExecutionObj obj,
                                                       HandleBeginInstructionExecutionFun fun)
    {
      HandleBeginInstructionExecutionIPTEntry e = HandleBeginInstructionExecutionIPTEntry(obj,fun);
      
      for (std::set<HandleBeginInstructionExecutionIPTEntry*>::iterator
            I = begin_instruction_execution_set_.begin(),
            E = begin_instruction_execution_set_.end(); I != E; ++I) {
        HandleBeginInstructionExecutionIPTEntry* ipt = *I;
        if ( *ipt == e ) { // entry exists
          LOG(LOG_DEBUG) << "[IPTManager] Removed HandleBeginInstructionExecutionIPTEntry.";
          active_begin_instruction_execution_handlers_.erase(ipt); 
          delete ipt; // // delete HandleBeginInstructionExecutionIPTEntry instance
          begin_instruction_execution_set_.erase(I); // erase by iterator
          if (begin_instruction_execution_set_.empty()) {
            active_ipts_ &= ~kIPTBeginInstruction; // mark HandleBeginInstructionExecutionIPTEntries as NOT active
            PROCESSOR.remove_translations(); // remove ALL translations
          }
          return true;
        }
      }      
      return false;
    }
    
    // -----------------------------------------------------------------------
    // ----- HandleEndInstructionExecutionIPT
    //
    
    // Insert - HandleEndInstructionExecutionIPT
    bool
    IPTManager::insert_end_instruction_execution_ipt(HandleEndInstructionExecutionObj obj,
                                                     HandleEndInstructionExecutionFun fun)
    {
      // FIXME
      return true;
    }
    
    // Remove ALL HandleEndInstructionExecutionIPTs for given address
    bool
    IPTManager::remove_end_instruction_execution_ipt()
    {
      // FIXME
      return true;
    }
    
    // Remove HandleEndInstructionExecutionIPT subscriber for given address
    bool
    IPTManager::remove_end_instruction_execution_ipt(HandleEndInstructionExecutionObj obj,
                                                     HandleEndInstructionExecutionFun fun)
    {
      // FIXME
      return true;
    }

    // -----------------------------------------------------------------------
    // ----- HandleBeginBasicBlockInstructionIPT
    //
    
    // Insert - HandleBeginBasicBlockInstructionIPT
    bool
    IPTManager::insert_begin_basic_block_instruction_execution_ipt(HandleBeginBasicBlockObj obj,
                                                                   HandleBeginBasicBlockFun fun)
    {
      LOG(LOG_DEBUG) << "[IPTManager] Inserting HandleBeginBasicBlockInstructionIPT.";
      HandleBeginBasicBlockIPTEntry e = HandleBeginBasicBlockIPTEntry(obj,fun);
      for (std::set<HandleBeginBasicBlockIPTEntry*>::const_iterator
           I = begin_basic_block_instruction_execution_set_.begin(),
           E = begin_basic_block_instruction_execution_set_.end(); I != E; ++I)
      {
        if ( *(*I) == e ) { // entry already exists!
          LOG(LOG_WARNING) << "[IPTManager] HandleBeginBasicBlockIPTEntry already exists.";
          return false;
        }
      }
      if (!(active_ipts_ & kIPTBeginBasicBlockInstruction)) {
        LOG(LOG_DEBUG) << "[IPTManager] HandleBeginBasicBlockIPTEntry activated - flushing translations.";
        PROCESSOR.remove_translations(); // remove ALL translations
      }
      // Add heap allocated HandleBeginBasicBlockIPTEntry to map
      begin_basic_block_instruction_execution_set_.insert(new HandleBeginBasicBlockIPTEntry(obj, fun));
      active_ipts_ |= kIPTBeginBasicBlockInstruction; // mark HandleBeginBasicBlockIPTEntry as active
      return true;
    }
    
    // Remove ALL HandleBeginBasicBlockInstructionIPTs for given address
    bool
    IPTManager::remove_begin_basic_block_instruction_execution_ipt()
    {
      while (!begin_basic_block_instruction_execution_set_.empty()) {
        delete *(begin_basic_block_instruction_execution_set_.begin());
        begin_basic_block_instruction_execution_set_.erase(begin_basic_block_instruction_execution_set_.begin());
      }
      if (active_ipts_ & kIPTBeginBasicBlockInstruction) {
        LOG(LOG_DEBUG) << "[IPTManager] Removed ALL HandleBeginBasicBlockIPTEntry subscribers.";
        active_ipts_ &= ~kIPTBeginBasicBlockInstruction; // mark HandleBeginBasicBlockIPTEntry as NOT active
        PROCESSOR.remove_translations(); // remove ALL translations
        return true;
      }
      return false;
    }
    
    // Remove HandleBeginBasicBlockInstructionIPT subscriber for given address
    bool
    IPTManager::remove_begin_basic_block_instruction_execution_ipt(HandleBeginBasicBlockObj obj,
                                                                   HandleBeginBasicBlockFun fun)
    {
      HandleBeginBasicBlockIPTEntry e = HandleBeginBasicBlockIPTEntry(obj,fun);
      
      for (std::set<HandleBeginBasicBlockIPTEntry*>::iterator
           I = begin_basic_block_instruction_execution_set_.begin(),
           E = begin_basic_block_instruction_execution_set_.end(); I != E; ++I) {
        HandleBeginBasicBlockIPTEntry* ipt = *I;
        if ( *ipt == e ) { // entry exists
          LOG(LOG_DEBUG) << "[IPTManager] Removed HandleBeginBasicBlockInstructionIPT.";
          active_begin_basic_block_instruction_execution_handlers_.erase(ipt); 
          delete ipt; // // delete HandleBeginBasicBlockInstructionIPT instance
          begin_basic_block_instruction_execution_set_.erase(I); // erase by iterator
          if (begin_basic_block_instruction_execution_set_.empty()) {
            active_ipts_ &= ~kIPTBeginBasicBlockInstruction; // mark HandleBeginBasicBlockInstructionIPT as NOT active
            PROCESSOR.remove_translations(); // remove ALL translations
          }
          return true;
        }
      }      
      return false;
    }
    
} } // arcsim::ipt

