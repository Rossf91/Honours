//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2010 The University of Edinburgh
//                        All Rights Reserved
//                                             
// =====================================================================
//
// Description:
//
// Datastructures and methods for block profiling and analysis.
//
// =====================================================================

#include <iomanip>
#include <cstring>

#include "Assertion.h"

#include "isa/arc/Opcode.h"

#include "sys/cpu/processor.h"

#include "profile/PageProfile.h"

#include "translate/TranslationModule.h"
#include "translate/TranslationWorkUnit.h"
#include "translate/TranslationEmit.h"

#include "util/Log.h"

#define HEX(_addr_) std::hex << std::setw(8) << std::setfill('0') << _addr_

namespace arcsim {
  namespace profile {

  // -----------------------------------------------------------------------------
  // Constructor/Destructor
  //
  PageProfile::PageProfile (uint32 addr)
    : page_address(addr),
      module_count_(0),
      interp_count_(0)
  {
    for (uint32 i = 0; i < NUM_INTERRUPT_STATES; ++i) {
      prev_block_[i] = kInvalidBlockEntryAddress;
    }
  }

  PageProfile::~PageProfile()
  { // Free translations for all modules that have been compiled, mark 'in-flight'
    // TranslationModules as dirty.
    //
    while (!module_map_.empty()) {
      TranslationModule* const m = module_map_.begin()->second;
      // -----------------------------------------------------------------------
      // BEGIN SYNCHRONISATION on module
      //
      m->lock();
      if (m->is_translated()) {                           // MODULE IS TRANSLATED
        m->erase_block_entries(); // remove translations for all blocks in module
      } else {                                            // 'IN FLIGHT' MODULE
        m->mark_as_dirty(); // mark module as dirty, responsible JIT compilation thread will release it
      }
      m->unlock();
      // 
      // END SYNCHRONISATION on module
      // -----------------------------------------------------------------------
      
      // erase Module from mapping data structure
      module_map_.erase(module_map_.begin());    
      // GC module if it was translated and reference count dropped to zero
      if (m->is_translated())
        delete m;
    }
  }

    // --------------------------------------------------------------------------- 
    // Determine wheter a BlockEntry is present for the given address
    //
    bool
    PageProfile::is_block_entry_present(uint32 addr) const
    {
      for (std::map<uint32,BlockEntry*>::const_iterator
           I = block_map_.upper_bound(addr),
           E = block_map_.begin();
           I != E; /* left blank on purpose */ )
      {
        --I; // Pay attention! We need to decrement the iterator right at the
        // start to ensure we include begin()! Why?
        
        // See http://bytes.com/topic/c/answers/790809-stl-map-reverse-iterator-lower_bound
        // Andrew Koenig: "Rule of thumb: decrement before you use the iterator; increment after you
        // use it."
        
        const BlockEntry& b = *I->second;
        if ((addr >= b.phys_addr) && (addr < (b.phys_addr + b.size_bytes)))
          return true;
      }
      return false;
    }
    
    
    // --------------------------------------------------------------------------- 
    // Determine wheter a translation for a BlockEntry is present given an address
    //
    bool
    PageProfile::is_block_entry_translation_present(uint32 addr) const
    {
      for (std::map<uint32,BlockEntry*>::const_iterator
           I = block_map_.upper_bound(addr),
           E = block_map_.begin();
           I != E; /* left blank on purpose */ )
      {
        --I; // Pay attention! We need to decrement the iterator right at the
        // start to ensure we include begin()! Why?
        
        // See http://bytes.com/topic/c/answers/790809-stl-map-reverse-iterator-lower_bound
        // Andrew Koenig: "Rule of thumb: decrement before you use the iterator; increment after you
        // use it."
        
        const BlockEntry& b = *I->second;
        if (!b.is_not_translated()
            && (addr >= b.phys_addr) && (addr < (b.phys_addr + b.size_bytes)))
          return true;
      }
      return false;
    }

    
// -----------------------------------------------------------------------------
// This method is critical during tracing. It is called for each yet unseen
// or untranslated BlockEntry on this page during a trace interval to record
// the dynamic control flow.
void
PageProfile::trace_block (BlockEntry&     block,
                          InterruptState  irq_state,
                          bool            is_sequence_active) 
{
	++interp_count_; // increment page interpretation count
  
  // ---------------------------------------------------------------------------
  // NODES - check if this block entry has been registered as a node
  //
	if (nodes_.find(block.virt_addr) == nodes_.end())
    nodes_[block.virt_addr] = &block;

  // ---------------------------------------------------------------------------
  // EDGES - add an edge if this block belongs to an active trace sequence
  //
	if (is_sequence_active)
  { // Any node that belongs to an active trace sequence, must have a previous
    // node.
    //
    ASSERT(prev_block_[irq_state] != kInvalidBlockEntryAddress);

    bool   matching_edge  = false;
    
    std::pair<std::multimap<uint32,BlockEntry*>::const_iterator,
              std::multimap<uint32,BlockEntry*>::const_iterator> from_edges;
    
    // Find all Edges from previous node
    //
    from_edges = edges_.equal_range(prev_block_[irq_state]);
    
		// Linear search for edges that match destination node
    //
		for (std::multimap<uint32,BlockEntry*>::const_iterator
         I = from_edges.first, E = from_edges.second; I != E; ++I)
    {
			if (I->second->virt_addr == block.virt_addr) {
        matching_edge = true;
        break;
      }
		}
    // If no edge with matching destination was found, we need to insert it
    //
    if (!matching_edge) {
      edges_.insert(std::pair<uint32,BlockEntry*>(prev_block_[irq_state], &block));	
    }
	}
  // Remember this block as the last one added for this interrupt state, so we
  // know where to draw the edge from, for the next block in this trace sequence
  //
  prev_block_[irq_state] = block.virt_addr;
}

// -----------------------------------------------------------------------------
// Depending on the translation mode the semantics of 'hotspot' change. The
// following are rather ad-hoc implementations
//
bool
PageProfile::has_hotspots(CompilationMode mode, uint32 threshold) const
{
  if (nodes_.empty()) // No blocks were touched - page is cold
      return false;

  // ---- kCompilationModePageControlFlowGraph mode
  //
  if (mode == kCompilationModePageControlFlowGraph)
    return (interp_count_ >= threshold);
  
  // ---- For all other compilation modes
  //

  // Trace is hot if there is one basic block that has been executed often enough
  //      
  for (std::map<uint32,BlockEntry*>::const_iterator I = nodes_.begin(), E = nodes_.end();
       I != E; ++I) {
    if (I->second->interp_count >= threshold)
      return true;
  }
  
  return false;  
}

// -----------------------------------------------------------------------------
// Generate translation work units once hot blocks have been found
//
bool
PageProfile::create_translation_work_unit(arcsim::sys::cpu::Processor& cpu,
                                          CompilationMode              mode,
                                          TranslationWorkUnit&         work_unit)
{
  bool success = true;

  // add lp_end_to_lp_start_map to work_unit
  work_unit.lp_end_to_lp_start_map = cpu.lp_end_to_lp_start_map;
  
  
  for (std::map<uint32,BlockEntry*>::const_iterator I = nodes_.begin(), E = nodes_.end();
       success && (I != E); ++I)
  {
    BlockEntry* block = I->second;
    
    // HEAP allocate TranslationBlockUnit
    TranslationBlockUnit* block_unit = new TranslationBlockUnit(block);
    work_unit.blocks.push_back(block_unit);
    
    // Compute CFG edges from this block if mode is 'kCompilationModePageControlFlowGraph'
    //
    if (mode == kCompilationModePageControlFlowGraph)
      get_block_edges(*block, block_unit->edges_);
    
    // create block of instructions for translation
    success = create_translation_block_unit(cpu, *block, work_unit, *block_unit);
    
    // adjust block-execution frequency of TranslationWorkUnit
    work_unit.exec_freq += block->interp_count;
  }
  
  return success;
}

// Finds all outgoing edges from a given block
//
void
PageProfile::get_block_edges(BlockEntry&              block,
                             std::list<BlockEntry*>&  dest_blocks) const
{ // Compute start and end range for all edges from a given block
  //
  std::pair<std::multimap<uint32,BlockEntry*>::const_iterator,
            std::multimap<uint32,BlockEntry*>::const_iterator> from_edges = edges_.equal_range(block.virt_addr);

  // Add all edges leading away from a block
  //
  for (std::multimap<uint32,BlockEntry*>::const_iterator I  = from_edges.first;
       I != from_edges.second; ++I)
  {
    dest_blocks.push_back(I->second);
  } 
}


// Create TranslationBlockUnit representing a basic block that is a part of a
// TranslationWorkUnit.
//
bool
PageProfile::create_translation_block_unit(arcsim::sys::cpu::Processor& cpu,
                                           BlockEntry&                  entry,
                                           TranslationWorkUnit&         work_unit,
                                           TranslationBlockUnit&        block_unit)
{
  using namespace arcsim::isa::arc;
  
  bool          prev_had_dslot  = false;
  bool          end_of_block    = false;
  uint32        pc              = entry.virt_addr;
  uint32        efa             = 0;
  uint32        ecause          = 0;
  uint32        block_bytes     = 0;
  uint32        block_insns     = 0;
  
  // Store operating mode
  //
  cpu.state_trace.U = entry.mode;

  // Create TranslationInstructionUnits for this TranslationBlockUnit
  //
  while (!end_of_block)
  {  
    end_of_block = prev_had_dslot;
    
    // create TranslationInstructionUnit
    TranslationInstructionUnit* instr_unit = new TranslationInstructionUnit();
            
    // decode instruction
    ecause = cpu.decode_instruction (pc,
                                     instr_unit->inst,
                                     cpu.state_trace,
                                     efa,
                                     prev_had_dslot);
    
    // If something goes wrong free allocated resources and bail out of this loop
    //
    if ( ecause || instr_unit->inst.code == OpCode::EXCEPTION) {      
      LOG(LOG_DEBUG)
        << "[TRACE-CONSTRUCT] DECODE "
        << ((instr_unit->inst.code == OpCode::EXCEPTION)
          ? "SIM_EXCEPTION: PC = '0x"
          : "FAILED: PC = '0x" )
        << HEX(pc)
        << "', OPCODE = '"   << OpCode::to_string(static_cast<OpCode::Op>(instr_unit->inst.code))
        << "', ECAUSE = '0x" << HEX(ecause)
        << "', EFA = '0x"    << HEX(efa) << "'.";

      delete instr_unit;  // clean up HEAP allocated instruction
      return false;       // break out of while loop
    }
    
    // Add TranslationInstructionUnit to TranslationBlockUnit
    //
    block_unit.add_instruction(instr_unit);
    
    // Increment block instruction count and block size
    //
    ++block_insns;
    block_bytes += instr_unit->inst.size;

    // Compute next PC
    //
    pc += instr_unit->inst.size;
    
    // Remember if this instruction has a delay slot instruction
    //
    prev_had_dslot =  instr_unit->inst.has_dslot_inst(); 
    
    // Determine end of block caused by explicit control flow
    //
    switch (instr_unit->inst.code) {
      // Control flow with instructions that might have a potential delay slot
      //
      case OpCode::BCC:        case OpCode::BR:       case OpCode::BRCC:
      case OpCode::BBIT0:      case OpCode::BBIT1:
      case OpCode::JCC_SRC1:   case OpCode::JCC_SRC2: {
        end_of_block = !instr_unit->inst.has_dslot_inst();
        break;
      }
      // 'LEAVE' CAN be an explicit control flow instruction if 'iunit->inst.info.isReturn'
      // is set.
      //  
      case OpCode::LEAVE: {
        end_of_block |= instr_unit->inst.info.isReturn;
        break;
      }
      // Control flow instructions that can not have a delay slot
      //
      
      // ARCompactV2 control flow instructions
      //
      case OpCode::JLI_S:                   
      case OpCode::BI:      case OpCode::BIH:
      // Baseline ARC control flow instructions
      //        
      case OpCode::LPCC:        case OpCode::FLAG:
      case OpCode::SLEEP:       case OpCode::BREAK:
      case OpCode::J_F_ILINK1:  case OpCode::J_F_ILINK2:
      case OpCode::TRAP0:       case OpCode::RTIE: {
        end_of_block = true;
        break;
      }
      default: break;
    }
    
    // Determine end of block caused by implicit control flow (i.e. ZOL)
    //
    if (!end_of_block && !prev_had_dslot) {
      // We need to check if the next 'pc' is an 'lp_end' instruction
      //
      if (work_unit.lp_end_to_lp_start_map.find(pc) != work_unit.lp_end_to_lp_start_map.end()) {
        end_of_block = true;
      }   
    }
  } /*  END: while (!end_of_block) */
  
  // Store block size and amount of instructions in BlockEntry object
  //
  entry.size_bytes = block_bytes;
  entry.inst_count = block_insns;
  
  return true;
}

// ----------------------------------------------------------------------------
// Remove translations registered for this page profile
//
int
PageProfile::remove_translations()
{
  int num_removed = 0;
  
  // Remove translations for all modules that have been compiled
  //
  while (!module_map_.empty()) {
    TranslationModule* const m = module_map_.begin()->second;

    // -----------------------------------------------------------------------
    // BEGIN SYNCHRONISATION on module
    //
    m->lock();
    
    ASSERT(!m->is_dirty() && "[PageProfile] Module state must not equal 'dirty'.");
    ASSERT(!(m->is_dirty() && m->is_translated())
           && "[PageProfile] Module state must not equal 'dirty' AND 'translated'.");
    
    if (m->is_translated()) {         // MODULE IS TRANSLATED
      // remove translations for all blocks in module
      num_removed += m->erase_block_entries();

      // after we have removed ALL block entries the module reference count must be zero
      ASSERT((m->get_ref_count() == 0)
             && "[PageProfile] TranslationModule reference count is not '0'.");

    } else {                          // 'IN FLIGHT' MODULE
      // Mark module as dirty, the responsible JIT compilation thread will release it
      m->mark_as_dirty();
      LOG(LOG_DEBUG) << "[PageProfile] marking module '" << m->get_id()
                     << " as dirty on page 0x" << HEX(page_address) << ".";

    }
    m->unlock();
    // 
    // END SYNCHRONISATION on module
    // -----------------------------------------------------------------------
        
    // erase Module from mapping data structure
    module_map_.erase(module_map_.begin());
    
    // GC module if it was translated and reference count dropped to zero
    if (m->is_translated()) {
      LOG(LOG_DEBUG) << "[PageProfile] removing module '" << m->get_id()
                     << " on page 0x" << HEX(page_address);
      delete m;
    }
  }
    
  return num_removed;
}

// ----------------------------------------------------------------------------
// Create a new library for this page
//
TranslationModule*
PageProfile::create_module (SimOptions&       sim_opts)
{
  // Create new TranslationModule for this PageProfile
  TranslationModule* m = new TranslationModule(module_count_, sim_opts);

  if (m->init(page_address)) {
    // add TranslationModule to PageProfile library map
    module_map_[module_count_++] = m;
    return m;
  }  
  delete m;    // Initialisation failed so free up resources and return '0'
  return 0;
}

} } // arcsim::profile
