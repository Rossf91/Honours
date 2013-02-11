	.file	"abort_mpy.s"
	.cpu	ARC700

	.globl	main
	.type 	main, @function
	.section .text

main:
;; ========================= mpy abort test ==========================
;;

   mov   r3,0x10000000
   mov   r1,23
   mov   r2,46
   mov   r12, 4
   
   st    r1, [r3,0]
   st    r2, [r3,4]
   
   mov   r4, r3
   mov   r5, r3
   mov   r6, r3
   
lp1:
   
   ld    r8, [r3,0]
   ld    r9, [r3,4]
   mpy   r10,r8,r9
   mov   r11, r10
   brne.d  r12,0,lp1
   sub   r12,r12,1
   
   nop
   flag     1
   nop
   nop
