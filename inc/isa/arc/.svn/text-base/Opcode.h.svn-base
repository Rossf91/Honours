//                      Confidential Information
//           Limited Distribution to Authorized Persons Only
//         Copyright (C) 2011 The University of Edinburgh
//                        All Rights Reserved
// =====================================================================
//
// Description:
//
// Internal 'byte-codes'/'opcocdes'. ARCompact instructions are mapped
// onto these OpCodes (@see code field in Dcode class)
//
// =====================================================================


#ifndef INC_ISA_ARC_OPCODE_H_
#define INC_ISA_ARC_OPCODE_H_

namespace arcsim {
  namespace isa {
    namespace arc {
      // Built in list of opcodes we map ARCompact instructions onto
#define BUILTIN_OPCODE_LIST(V)                                           \
      V(BCC,bcc)                                                         \
      V(BR,br)                                                           \
      V(BRCC,brcc)                                                       \
      V(JCC_SRC1,jcc_src1)                                               \
      V(JCC_SRC2,jcc_src2)                                               \
      V(LPCC,lpcc)                                                       \
      V(LD_WORD,ld_word)                                                 \
      V(LD_HALF_S,ld_half_s)                                             \
      V(LD_BYTE_S,ld_byte_s)                                             \
      V(LD_HALF_U,ld_half_u)                                             \
      V(LD_BYTE_U,ld_byte_u)                                             \
      V(ST_WORD,st_word)                                                 \
      V(ST_HALF,st_half)                                                 \
      V(ST_BYTE,st_byte)                                                 \
      V(LR,lr)                                                           \
      V(SR,sr)                                                           \
      V(AEX,aex)                                                         \
      V(SETI, seti)                                                      \
      V(CLRI, clri)                                                      \
      V(TST,tst)                                                         \
      V(BTST,btst)                                                       \
      V(CMP,cmp)                                                         \
      V(RCMP,rcmp)                                                       \
      V(MOV,mov)                                                         \
      V(ADD,add)                                                         \
      V(SUB,sub)                                                         \
      V(AND,and)                                                         \
      V(OR,or)                                                           \
      V(MOV_F,mov_f)                                                     \
      V(ADD_F,add_f)                                                     \
      V(SUB_F,sub_f)                                                     \
      V(AND_F,and_f)                                                     \
      V(OR_F,or_f)                                                       \
      V(RSUB,rsub)                                                       \
      V(ADC,adc)                                                         \
      V(SBC,sbc)                                                         \
      V(XOR,xor)                                                         \
      V(BIC,bic)                                                         \
      V(MAX,max)                                                         \
      V(MIN,min)                                                         \
      V(BSET,bset)                                                       \
      V(BCLR,bclr)                                                       \
      V(BXOR,bxor)                                                       \
      V(BMSK,bmsk)                                                       \
      V(ADD1,add1)                                                       \
      V(ADD2,add2)                                                       \
      V(ADD3,add3)                                                       \
      V(SUB1,sub1)                                                       \
      V(SUB2,sub2)                                                       \
      V(SUB3,sub3)                                                       \
      V(MPY,mpy)                                                         \
      V(MPYH,mpyh)                                                       \
      V(MPYHU,mpyhu)                                                     \
      V(MPYU,mpyu)                                                       \
      V(MUL64,mul64)                                                     \
      V(MULU64,mulu64)                                                   \
      V(ASL,asl)                                                         \
      V(ASR,asr)                                                         \
      V(LSR,lsr)                                                         \
      V(ROR,ror)                                                         \
      V(RRC,rrc)                                                         \
      V(SEXBYTE,sexb)                                                    \
      V(SEXWORD,sexw)                                                    \
      V(EXTBYTE,extb)                                                    \
      V(EXTWORD,extw)                                                    \
      V(ABS,abs)                                                         \
      V(NOT,not)                                                         \
      V(RLC,rlc)                                                         \
      V(EX,ex)                                                           \
      V(ASLM,aslm)                                                       \
      V(LSRM,lsrm)                                                       \
      V(ASRM,asrm)                                                       \
      V(RORM,rorm)                                                       \
      V(ADDS,adds)                                                       \
      V(SUBS,subs)                                                       \
      V(SUBSDW,subsdw)                                                   \
      V(ADDSDW,addsdw)                                                   \
      V(ASLS,asls)                                                       \
      V(ASRS,asrs)                                                       \
      V(DIVAW,divaw)                                                     \
      V(SWAP,swap)                                                       \
      V(NORM,norm)                                                       \
      V(NORMW,normw)                                                     \
      V(NEGS,negs)                                                       \
      V(NEGSW,negsw)                                                     \
      V(SAT16,sat16)                                                     \
      V(RND16,rnd16)                                                     \
      V(ABSSW,abssw)                                                     \
      V(ABSS,abss)                                                       \
      V(FLAG,flag)                                                       \
      V(SLEEP,sleep)                                                     \
      V(TRAP0,trap0)                                                     \
      V(RTIE,rtie)                                                       \
      V(BBIT0,bbit0)                                                     \
      V(BBIT1,bbit1)                                                     \
      V(NEG,neg)                                                         \
      V(LW,lw)                                                           \
      V(LW_PRE,lw_pre)                                                   \
      V(LW_SH2,lw_sh2)                                                   \
      V(LW_PRE_SH2,lw_pre_sh2)                                           \
      V(LW_A,lw_a)                                                       \
      V(LW_PRE_A,lw_pre_a)                                               \
      V(LW_SH2_A,lw_sh2_a)                                               \
      V(LW_PRE_SH2_A,lw_pre_sh2_a)                                       \
      V(BREAK,break)                                                     \
      V(J_F_ILINK1,j_f_ilink1)                                           \
      V(J_F_ILINK2,j_f_ilink2)                                           \
      V(FMUL,fmul)                                                       \
      V(FADD,fadd)                                                       \
      V(FSUB,fsub)                                                       \
      V(DMULH11,dmulh11)                                                 \
      V(DMULH12,dmulh12)                                                 \
      V(DMULH21,dmulh21)                                                 \
      V(DMULH22,dmulh22)                                                 \
      V(DADDH11,daddh11)                                                 \
      V(DADDH12,daddh12)                                                 \
      V(DADDH21,daddh21)                                                 \
      V(DADDH22,daddh22)                                                 \
      V(DSUBH11,dsubh11)                                                 \
      V(DSUBH12,dsubh12)                                                 \
      V(DSUBH21,dsubh21)                                                 \
      V(DSUBH22,dsubh22)                                                 \
      V(DRSUBH11,drsubh11)                                               \
      V(DRSUBH12,drsubh12)                                               \
      V(DRSUBH21,drsubh21)                                               \
      V(DRSUBH22,drsubh22)                                               \
      V(DEXCL1,dexcl1)                                                   \
      V(DEXCL2,dexcl2)                                                   \
      V(NOP,nop)                                                         \
      V(MULDW,muldw)                                                     \
      V(MULUDW,muludw)                                                   \
      V(MULRDW,mulrdw)                                                   \
      V(MACDW,macdw)                                                     \
      V(MACUDW,macudw)                                                   \
      V(MACRDW,macrdw)                                                   \
      V(MSUBDW,msubdw)                                                   \
      V(CMACRDW,cmacrdw)                                                 \
      V(MULULW,mululw)                                                   \
      V(MULLW,mullw)                                                     \
      V(MULFLW,mulflw)                                                   \
      V(MACLW,maclw)                                                     \
      V(MACFLW,macflw)                                                   \
      V(MACHULW,machulw)                                                 \
      V(MACHLW,machlw)                                                   \
      V(MACHFLW,machflw)                                                 \
      V(MULHLW,mulhlw)                                                   \
      V(MULHFLW,mulhflw)                                                 \
      V(DIV,div)                                                         \
      V(DIVU,divu)                                                       \
      V(REM,rem)                                                         \
      V(REMU,remu)                                                       \
      V(SWAPE,swape)                                                     \
      V(BMSKN,bmskn)                                                     \
      V(LSL16,lsl16)                                                     \
      V(LSR16,lsr16)                                                     \
      V(ASR16,asr16)                                                     \
      V(ASR8,asr8)                                                       \
      V(LSR8,lsr8)                                                       \
      V(LSL8,lsl8)                                                       \
      V(ROL8,rol8)                                                       \
      V(ROR8,ror8)                                                       \
      V(FFS,ffs)                                                         \
      V(FLS,fls)                                                         \
      V(JLI_S,jli_s)                                                     \
      V(ROL,rol)                                                         \
      V(EI_S,ei_s)                                                       \
      V(LDI,ldi)                                                         \
      V(SETEQ,seteq)                                                     \
      V(SETNE,setne)                                                     \
      V(SETLT,setlt)                                                     \
      V(SETGE,setge)                                                     \
      V(SETLO,setlo)                                                     \
      V(SETHS,seths)                                                     \
      V(SETLE,setle)                                                     \
      V(SETGT,setgt)                                                     \
      V(MPYW,mpyw)                                                       \
      V(MPYWU,mpywu)                                                     \
      V(ENTER,enter)                                                     \
      V(LEAVE,leave)                                                     \
      V(LLOCK,llock)                                                     \
      V(SCOND,scond)                                                     \
      V(BI,bi)                                                           \
      V(BIH,bih)                                                         \
      V(SIMD,simd)                                                       \
      V(PASTA,pasta)                                                     \
      V(SYNC,sync)                                                       \
      V(SWI,swi)                                                         \
      V(EXCEPTION,exception)                                             \
      V(EIA_ZOP,eia_zop)                                                 \
      V(EIA_SOP,eia_sop)                                                 \
      V(EIA_DOP,eia_dop)                                                 \
      V(EIA_ZOP_F,eia_zop_f)                                             \
      V(EIA_SOP_F,eia_sop_f)                                             \
      V(EIA_DOP_F,eia_dop_f)                                             \
      V(AP_BREAK,ap_break)                                               \

      class OpCode
      {
      public:
        // declare enum of OpCodes
        enum Op {
#define DEF_OPENUM_C(name,ignore) name,
          BUILTIN_OPCODE_LIST(DEF_OPENUM_C)
#undef DEF_OPENUM_C
          opcode_count
        };
        
        // retrieve string representation of OpCode
        static char const * const to_string(Op o);
        
      };
      
      
} } } //  arcsim::isa::arc

#endif /* INC_ISA_ARC_OPCODE_H_ */

