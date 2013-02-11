//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2003-2010 The University of Edinburgh
//                        All Rights Reserved
//
// =====================================================================

#ifndef _enter_leave_h_
#define _enter_leave_h_

//=====================================================================
// Macros for enter_s and leave_s instructions
//=====================================================================


// -- Execute uop instruction: MOV sp, fp
//
# define EXEC_UOP_MOV_SP()    \
    do {                                                              \
      rn = SP_REG;                                                    \
      rd = state.gprs[rn] = state.gprs[FP_REG];                       \
    } while(0)

// -- Execute uop instruction: MOV fp, sp
//
# define EXEC_UOP_MOV_FP()    \
    do {                                                              \
      rn = FP_REG;                                                    \
      rd = state.gprs[rn] = state.gprs[SP_REG];                       \
    } while(0)

// -- Execute uop instruction: LD a,[sp,s9]
//
# define EXEC_UOP_LD_R(a,s9)  \
    do {                                                              \
      ra = state.gprs[SP_REG] + (s9);                                 \
      rn = a;                                                         \
      success = read32(ra, rd);                                       \
      if (success) state.gprs[rn] = rd;                               \
      MEMORY_ACCESS(ra);                                               \
    } while(0)

// -- Execute uop instruction: ST c,[sp,s9]
//
# define EXEC_UOP_ST_R(c,s9) \
    do {                                                              \
      ra = state.gprs[SP_REG] + (s9);                                 \
      rd = state.gprs[c];                                             \
      rn = c;                                                         \
      success = write32(ra, rd);                                      \
      MEMORY_ACCESS(ra);                                               \
    } while(0)

// -- Execute uop instruction: ADD_S sp,sp,u7
//
# define EXEC_UOP_ADD_S(u7) \
    do {                                                              \
      rn = SP_REG;                                                    \
      rd = state.gprs[rn] + ((u7) & 0x7fUL);                          \
      state.gprs[rn] = rd;                                            \
    } while(0)

// -- Execute uop instruction: SUB_S sp,sp,u7
//
# define EXEC_UOP_SUB_S(u7) \
    do {                                                              \
      rn = SP_REG;                                                    \
      rd = state.gprs[rn] - ((u7) & 0x7fUL);                          \
      state.gprs[rn] = rd;                                            \
    } while(0)

// -- Execute uop instruction: J_S [blink]
//
# define EXEC_UOP_J_S()     \
    do {                                                              \
      state.next_pc = state.gprs[BLINK];                              \
      inst->taken_branch  = true;                                     \
      end_of_block        = true;                                     \
    } while(0)

// -- Execute uop instruction: J_S.D [blink]
//
# define EXEC_UOP_J_SD()    \
    do {                                                              \
      state.auxs[AUX_BTA] = state.gprs[BLINK];                        \
      inst->taken_branch  = true;                                     \
      state.next_pc       = state.gprs[BLINK];                        \
      end_of_block        = true;                                     \
    } while(0)

#ifdef STEP
// -- Init update packet for new uop
//
# define INIT_UOP_DELTA(inst, limm)     \
    do {                                                              \
      cur_uop->next_uop = new UpdatePacket(state.pc,                  \
                                    (inst), (limm), (s32));           \
      cur_uop           = cur_uop->next_uop;                          \
    } while(0)


// -- Print trace for the following uop instructions
//      MOV sp, fp
//      MOV fp, sp
//      ADD_S sp,sp,u7
//      SUB_S sp,sp,u7
//
# define TRACE_UOP_REG_OP() {                                                  \
  char buf[BUFSIZ];                                                            \
  int len = snprintf (buf, sizeof(buf), " : (w0) r%d <= 0x%08x", rn, rd);       \
  IS.write(buf, len);                                                          \
}

// -- Pack result delta of uop instruction: LD a,[sp,s9]
//
# define TRACE_UOP_LD_R()  {                                                      \
  char buf[BUFSIZ];                                                               \
  int len = snprintf (buf, sizeof(buf), " : lw [%08x] => %08x : (w1) r%d <= 0x%08x",\
                      ra, rd, rn, rd);                                            \
  IS.write(buf, len);                                                             \
}

// -- Print trace for uop instruction: ST c,[sp,s9]
//
# define TRACE_UOP_ST_R() {                                                     \
  char buf[BUFSIZ];                                                             \
  int len = snprintf (buf, sizeof(buf), " : sw [%08x] <= %08x (r%d)", ra, rd, rn);\
  IS.write(buf, len);                                                           \
}

#else // !STEP
// Some of above macros are defined to do-nothing for non-STEP code
//
# define TRACE_UOP_REG_OP()
# define TRACE_UOP_LD_R()
# define TRACE_UOP_ST_R()
#endif  // STEP
#endif // _enter_leave_h_
