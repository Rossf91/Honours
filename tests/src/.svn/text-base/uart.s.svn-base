	.file	"uart.s"
	.cpu	ARC700

	.globl	main
	.type 	main, @function
	.section .text
main:
;; 
	mov	r0, 0xc0fc1000 ;; UART base address
	mov	r1, 60
	mov	r4, 10
loop:
	st 	r1, [r0] 		;; arbitrary write to UART
	ld 	r2, [r0] 		;; arbitrary read from UART
	sub.f r4, r4, 1
	bne	loop
;;
	flag	1					;; halt when done
	.size main, .-main

