	.file	"align.s"
	.cpu	ARC700

	.globl	main
	.type 	main, @function
	.section .text

main:
    j     l0
    
;; mov_s      r0,r14
;; tst_s      r0,r0
;; mov_s      r0,0x12ec
;; bz         9b0 <main+0x2fc>
;;
;; bl         10a4 <prints>
;;
;; mov_s      r0,1
;; cmp_s      r14,0x00104000
;; add_s      r14,r14,0x8000
;; bnz        ab8 <main+0x404>
;;
;; Note that the bz at 88e is taken, and the code at 9b0 looks like this:
;;
;; mov_s      r0,0x12f0
;; bl         10a4 <prints>
;; 
;; b.d        898 <main+0x1e4>



  .org      0x0000630 ;; 0x0000898 - 0x00000268
l1:
  cmp_s     r14, 0x00104000
  add_s     r14, r14, 0x00008000
  bnz       l2
  
  nop
  nop
  nop
  flag      1
  
  .org      0x00000752  ;; 0x000009ba - 0x00000268
l0:
  b.d     l1
  nop_s
  
  .org      0x00000850 ;; 0x00000ab8 - 0x00000268
l2:
  nop
  nop
  nop
  flag      1
