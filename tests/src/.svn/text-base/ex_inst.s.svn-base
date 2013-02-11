	.globl	main
	.section .text

main:
	mov	r1, 0xaaaa5555
	mov	r2, 0x00400000
	st 	r1, [r2]
	nop
	nop
	mov	r1, 0x11112222
	ex 	r1, [r2]
	nop
	nop
	ld 	r3, [r2]
	
;; Use trap_s 0 when traps are handled as exceptions
;; until then, we generate a privilege violation.
;;
;;	trap_s 0

	rtie	;; disallowed in user mode -> exception

	
	
	
