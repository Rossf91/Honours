	.file	"cache.s"
	.cpu	ARC700

	.globl	main
	.type 	main, @function
	.section .text

main:
;; ========================= loop ==========================
;;

	mov	r1, 0x0006acac
   mov   r2,0x8000
   
   mov   r3,r1
   st    r2, [r3]
   add   r3,r3,0x8000
   st    r2, [r3]
   add   r3,r3,0x8000
   st    r2, [r3]
   add   r3,r3,0x8000
   st    r2, [r3]
   add   r3,r3,0x8000
   st    r2, [r3]
   add   r3,r3,0x8000
   st    r2, [r3]
   add   r3,r3,0x8000
   st    r2, [r3]
   add   r3,r3,0x8000
   st    r2, [r3]
   
   
   mov   r14,0x00076828
   mov   sp, 0x0027aa50
   
   mov   r0,0x00076220
   st    r0,[sp,0x4]

   mov   r0,0x0027ad70
   st    r0,[sp,0]

   mov   r0,0x00000000
   st    r0,[sp,0x8]

   mov   r0,0x00000000
   st    r0,[sp,0xc]

   mov   r0,0x0003ef08
   st    r0,[sp,0x14]

   mov   r0,0x0000796c
   j     [r0]
   
   .org     0x00007704
   
   mov      r0, r14
   ld.ab    fp,[sp,0x4]
   ld.as    blink,[sp,0x4]
   ld_s     r13,[sp,0]
   ld_s     r14,[sp,4]
   ld_s     r15,[sp,8]
   j_s.d    [blink]
   add      sp,sp,0x14
   
   .org     0x0003ec90
l1:mov      r10,1
   flag     1
   
   .org     0x0003eca0
   st       r0,[0x0006acac]
   ld       r0,[0x0006acac]
   bl       l2
   nop
   nop
   flag     1
   nop
   nop
   
l2:
   nop
   nop
   j_s     [blink]
   nop
   nop
   
   flag     1
