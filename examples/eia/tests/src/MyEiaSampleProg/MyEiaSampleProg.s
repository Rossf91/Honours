;;                          Copyright Notice
;;
;;    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
;;  The University Court of the University of Edinburgh. All Rights Reserved.
;;
;; =============================================================================

  .file "MyEiaSampleProg.s"
  .cpu  ARC700

;; myeia and whizz extension instructions
;;
    .extInstruction myeia, 0x07, 0x01, SUFFIX_COND, SYNTAX_3OP
    .extInstruction whizz, 0x07, 0x02, SUFFIX_COND, SYNTAX_3OP

;; myeia and whizz extension instructions
;;
    .extCoreRegister r55,            55, w|r, cannot_shortcut, CORE
    .extCoreRegister whizz_core_reg, 56, w|r, cannot_shortcut, CORE
                         
;;    .extCondCode     exhausted_cc,     0x10
;;    .extCoreRegister acc,      38,     w|r, cannot_shortcut, CORE
;;    .extAuxRegister  tomaux,   0x111,  r|w


  .globl  main
  .type   main, @function
  .section .text

main:
;; ========================= EIA EXTENSION TEST ==========================
;;
  lr  r1,[identity]
;; LOOP
  mov   r60,0x00ffffff
  lp    lpend
  add   r1,r55,0
  add   r2,whizz_core_reg,0
  add   r55,r1,0xf
  add   whizz_core_reg,r55,r55
  myeia r11,r11,1
  whizz r12,r12,r11
  myeia r55,whizz_core_reg,r11
  whizz whizz_core_reg,r55,3
lpend:
  nop
  flag 1

