	.file	"ext_arith.s"
	.cpu	ARC700

	.globl	main
	.type 	main, @function
	.section .text

main:
;; ========================= ADDSDW ==========================
	mov	r6, 1000000000
loop1:
	mov	r1, 0x00010005
	mov	r2, 0x001f0011
	addsdw	r3, r2, r1
;;
	mov	r1, 0x7ff07044
	mov	r2, 0x40005000
	addsdw	r3, r2, r1
;;
	mov	r1, 0x80308020
	mov	r2, 0xa000b0b0
	addsdw	r3, r2, r1
;;
	sub.f r6, r6, 1
	bne	loop1
;;
;; Use trap_s 0 when traps are handled as exceptions
;; until then, we call a user-mode function and it raises
;; an exception to halt the program.
;;
;;	trap_s 0
;;
	mov	r0, user_fun
	sr	r0, [0x400]	; store entry point in ERET
	mov	r0, 0x80	; STATUS32 has U-bit set for user-mode execution
	sr	r0, [0x402]	; store status to ERSTATUS
	rtie
	.size main, .-main
	
;; ===================Clear Sticky Flags ===================
	.globl	clr_sticky
	.type 	clr_stick, @function
clr_sticky:
	mov	r15, 0		; get zero
	sr	r15, [0x041]	; write to AUX_MACMODE
	j	[r31]		; return
	.size clr_sticky, .-clr_sticky
	
;; =================== User-mode function ==================
;; This is a trivial user-mode function which purposefully
;; raises a privilege violation exception to gracelessly 
;; halt the program.
;;
	.globl	user_fun
	.type 	clr_stick, @function
user_fun:
	rtie		;; not allowed in user mode => ev_privilege
	.size user_fun, .-user_fun

