//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//            Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
//
//                                             
// =====================================================================
//
// Description:
//
//
// =====================================================================

#ifndef INC_INTERNAL_SYSTEM_APS_ACTIONPOINTS_H_
#define INC_INTERNAL_SYSTEM_APS_ACTIONPOINTS_H_

// uncomment the next line to enable extra debug info for Actionpoints
// #define DEBUG_ACTIONPOINTS

// =====================================================================
// HEADERS
// =====================================================================

#include "api/types.h"

namespace arcsim {
  namespace sys {
    namespace cpu {
      
      // -----------------------------------------------------------------------
      // Forward class declarations
      //
      class Processor;
      
      static const unsigned int MAX_ACTIONPOINTS = 32;
      static const unsigned int NUM_AP_AUX_REGS  = 3 * MAX_ACTIONPOINTS;
      static const unsigned int AP_AUX_BASE_ADDR = 0x220;
      
      // -----------------------------------------------------------------------
      // Actionpoints
      //
      class Actionpoints {
      private:

        // Actionpoint auxiliary registers
        //
        uint32 ap_regs [NUM_AP_AUX_REGS];
        
        // Masked values for comparions
        //
        uint32 mvalues [MAX_ACTIONPOINTS];
        
        // Matched values, for assigning to AMV fields on global match
        //
        uint32 mcopies [MAX_ACTIONPOINTS];
        
        // Masked values for page-level address comparions
        //
        uint32 pvalues [MAX_ACTIONPOINTS];
        
        // Actionpoint matches expected for each AP to trigger
        //
        uint32 groups [MAX_ACTIONPOINTS];
	
        // Primary Actionpoint number for each Actionpoint that is
        // one of its secondary Actionpoints.
        //
        uint32 leader [MAX_ACTIONPOINTS];
        
        // Bit-vector of primary Actionpoints
        // 
        uint32 primaries;
        
        // Bit-vector of singleton instruction Actionpoints, i.e. those that
        // are based on PC or IR, and are not paired or quadded with other
        // actionpoints.
        //
        uint32 singleton_insts;
        
        // pointer to parent cpu
        //
        Processor* cpu;
        
        // Number of Actionpoints configured (range: 0 to 32)
        //
        uint8 num_aps;
        
        // Bit-vector representation of num_aps
        //
        uint32 num_mask;
        
        // Feature level: simple or full
        //
        bool full_aps;
        
        // Vector of matching Actionpoints for current instruction
        //
        uint32 inst_matches;
        
        // Matching address for current instruction
        //
        uint32 matching_address;
        
        // Matching data for current instruction
        //
        uint32 matching_data;
                
        // Indicator of whether Actionpoints are enabled
        // 
        bool is_enabled;
        
        // Intra-search record of triggering state.
        //
        bool has_triggered;
        
        // Cached counts of each type of Actionpoint, for quick reference
        // (range: 0 to 32)
        //
        uint8 n_inst_aps;
        uint8 n_pc_aps;
        uint8 n_ir_aps;
        
        uint8 n_mem_aps;
        uint8 n_mem_addr_aps;
        uint8 n_mem_data_aps;        
        uint8 n_ld_addr_aps;
        uint8 n_st_addr_aps;
        uint8 n_ld_data_aps;
        uint8 n_st_data_aps;
        uint8 n_st_aps;
        uint8 n_ld_aps;
        
        uint8 n_aux_aps;
        uint8 n_aux_addr_aps;
        uint8 n_aux_data_aps;
        uint8 n_lr_addr_aps;
        uint8 n_sr_addr_aps;
        uint8 n_lr_data_aps;
        uint8 n_sr_data_aps;
        uint8 n_sr_aps;
        uint8 n_lr_aps;

        uint8 n_extparam_aps;
        
        Actionpoints(const Actionpoints & m);     // DO NOT COPY
        void operator=(const Actionpoints &);     // DO NOT ASSIGN
        
        void classify_actionpoints ();
        void compute_primaries ();
        
      public:
        
        // Set of triggering primary Actionpoints for the current instruction
        //
        uint32 aps_hits;
        
        // Set of matching Actionpoints for the current instruction
        //
        uint32 aps_matches;
        
        // Action associated with the primary matching Actionpoint
        //
        uint32 aps_action;
        
        // Actionpoint Type (AT) field values, in registers AP_AC[0..31]
        //
        enum ApsType {
          AP_INST_ADDR  = 0,
          AP_INST_DATA  = 1,
          AP_LD_ST_ADDR = 2,
          AP_LD_ST_DATA = 3,
          AP_AUX_ADDR   = 4,
          AP_AUX_DATA   = 5,
          AP_EXT_P0     = 6,
          AP_EXT_P1     = 7
        };
        
        // Enumerated Transaction Type (TT) field values
        //
        enum ApsTT { 
          TT_DISABLED   = 0,
          TT_WRITE      = 1,
          TT_READ       = 2,
          TT_READ_WRITE = 3
        };
      
        // Enumerated trigger Mode field values
        //
        enum ApsMode {
          M_WITHIN_RANGE  = 0,
          M_OUTWITH_RANGE = 1
        };
        
        // Enumerated field values for pairing and quading of Actionpoints
        //
        enum ApsPairing {
          P_UNPAIRED      = 0,
          P_PAIRED        = 1
        };
        
        enum ApsAction {
          AA_BREAK        = 0,
          AA_EXCEPTION    = 1
        };
        
        enum ApsQuading {
          Q_UNQUADED      = 0,
          Q_QUADED        = 1
        };
        
        // Constructor
        //
        Actionpoints() 
          : is_enabled(false),
            cpu(0),
            num_aps(0),
            num_mask(0),
            primaries(0),
            singleton_insts(0),
            full_aps(false),
            inst_matches(0),
            matching_address(0),
            matching_data(0),
            n_inst_aps(0),
            n_pc_aps(0),
            n_ir_aps(0),
            n_mem_aps(0),
            n_ld_aps(0),
            n_st_aps(0),
            n_mem_addr_aps(0),
            n_mem_data_aps(0),    
            n_ld_addr_aps(0),
            n_st_addr_aps(0),
            n_ld_data_aps(0),
            n_st_data_aps(0),
            n_aux_aps(0),
            n_aux_addr_aps(0),
            n_aux_data_aps(0),
            n_lr_addr_aps(0),
            n_sr_addr_aps(0),
            n_lr_data_aps(0),
            n_sr_data_aps(0),
            n_sr_aps(0),
            n_lr_aps(0),
            n_extparam_aps(0),
            has_triggered(false),
            aps_hits(0),
            aps_matches(0),
            aps_action(0)
        { /* EMPTY */ }
        
        // Destructor
        //
        ~Actionpoints()
        { /* EMPTY */ }
        
        // Init    -------------------------------------------------------------
        //
        void configure(Processor *parent, uint32 num, bool full);
 
        // Auxiliary register read/write methods    ----------------------------
        //
        bool read_aux_register  (uint32 aux_addr, uint32 *data);
        bool write_aux_register (uint32 aux_addr, uint32 aux_data);
        
        // Matching methods   --------------------------------------------------
        //
        bool match_instruction (uint32 inst_addr, uint32 inst_data, uint32* match, uint32* action);
        bool match_load_addr   (uint32 load_addr, bool& page_match);
        bool match_store       (uint32 store_addr, bool& page_match, uint32 store_data);
        bool match_load_data   (uint32 load_data);
        bool match_lr_addr     (uint32 lr_addr);
        bool match_lr_data     (uint32 lr_data);
        bool match_sr          (uint32 sr_addr, uint32 sr_data);
        bool match_ext_param   (uint32 param0, uint32 param1);
        
        // Post-matching methods   ---------------------------------------------
        //
        void handle_aps_hit         (bool &trigger, uint32 ap, uint32 primary, uint32 match, uint32 value);
        void handle_brk_hit         (bool &trigger, uint32 ap, uint32 primary, uint32 &inst_matches, 
                                     uint32 match, uint32 &action, uint32 value);
        void copy_matching_values   ();
        void take_breakpoint        (uint32 inst_matches, uint32 pc, uint32 ir);

        void set_matching_pc        (uint32 match, uint32 inst_addr);
        void set_matching_ir        (uint32 match, uint32 inst_data);
        void set_matching_ldst_addr (uint32 match, uint32 ldst_addr);
        void set_matching_ldst_data (uint32 match, uint32 ldst_data);
        void set_matching_aux_addr  (uint32 match, uint32 aux_addr);
        void set_matching_aux_data  (uint32 match, uint32 aux_data);
        void set_matching_ext_param (uint32 match, uint32 ext_data);
        
        // Inline methods, on fast path   --------------------------------------
        //
        inline void set_inst_matches (uint32 m){ inst_matches = m;            }
        inline void clear_trigger ()           { has_triggered = false; aps_hits = aps_matches = 0;}
        inline void init_aps_matches(uint32 m) { aps_matches = m; aps_hits = 0; }
        inline bool enabled ()                 { return is_enabled;           }
        inline bool has_inst_aps ()            { return (n_inst_aps     != 0);}
        inline bool has_pc_aps ()              { return (n_pc_aps       != 0);}
        inline bool has_ir_aps ()              { return (n_ir_aps       != 0);}
        inline bool has_mem_aps ()             { return (n_mem_aps      != 0);}
        inline bool has_mem_addr_aps ()        { return (n_mem_addr_aps != 0);}
        inline bool has_mem_data_aps ()        { return (n_mem_data_aps != 0);}
        inline bool has_ld_addr_aps ()         { return (n_ld_addr_aps  != 0);}
        inline bool has_st_addr_aps ()         { return (n_st_addr_aps  != 0);}
        inline bool has_ld_data_aps ()         { return (n_ld_data_aps  != 0);}
        inline bool has_st_data_aps ()         { return (n_st_data_aps  != 0);}
        inline bool has_ld_aps ()              { return (n_ld_aps       != 0);}
        inline bool has_st_aps ()              { return (n_st_aps       != 0);}
        inline bool has_aux_aps ()             { return (n_aux_aps      != 0);}
        inline bool has_aux_addr_aps ()        { return (n_aux_addr_aps != 0);}
        inline bool has_aux_data_aps ()        { return (n_aux_data_aps != 0);}
        inline bool has_lr_addr_aps ()         { return (n_lr_addr_aps  != 0);}
        inline bool has_sr_addr_aps ()         { return (n_sr_addr_aps  != 0);}
        inline bool has_lr_data_aps ()         { return (n_lr_data_aps  != 0);}
        inline bool has_sr_data_aps ()         { return (n_sr_data_aps  != 0);}
        inline bool has_sr_aps ()              { return (n_sr_aps       != 0);}
        inline bool has_lr_aps ()              { return (n_lr_aps       != 0);}
        inline bool has_extparam_aps ()        { return (n_extparam_aps != 0);}
      };

    }
  }
} //  arcsim::sys::cpu

#endif
