;; Simple program to test alignment of branch instructions
;;
;; compile with: arc-elf32-gcc -static -mA7 -o br_align br_align.s
;;
	.file	"br_align.s"
	.cpu	ARC700
   .globl main
   .type main, @function
   .section .text
   
main:
   mov     r5, 100
loop:
   mov     r1, 1
   neg     r2, r1
;;
   mov     r1, 0xfffffffe
   neg     r2, r1
   
;; Insert a 16-bit nop into the loop body so that
;; all subsequent instructions are not 32-bit aligned.
;; Comment this nop_s out to see different behaviour
;; of the bne.d when predicted later on.
   nop_s
   
   neg     r1, r2
   
   sub.f   r5, r5, 1
   bne.d   loop
   sub     r7, r5, 1
   
   j     [blink]
