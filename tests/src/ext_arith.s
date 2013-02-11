	.file	"ext_arith.s"
	.cpu	ARC700

	.globl	main
	.type 	main, @function
	.section .text

main:
;; ========================= ADDSDW ==========================
	mov	r1, 0x00010005
	mov	r2, 0x001f0011
	addsdw.f	r3, r2, r1
;;
	mov	r1, 0x7ff07044
	mov	r2, 0x40005000
	addsdw.f	r3, r2, r1
;;
	mov	r1, 0x80308020
	mov	r2, 0xa000b0b0
	addsdw.f	r3, r2, r1
;;
	jl	clr_sticky
;;
;; ========================= NORM ============================
	mov	r1, 0x0
	norm.f	r2, r1
;;	
	mov	r1, 0x1
	norm.f	r2, r1
;;	
	mov	r1, 0x1fffffff
	norm.f	r2, r1
;;	
	mov	r1, 0x3fffffff
	norm.f	r2, r1
;;	
	mov	r1, 0x7fffffff
	norm.f	r2, r1
;;	
	mov	r1, 0x80000000
	norm.f	r2, r1
;;	
	mov	r1, 0xc0000000
	norm.f	r2, r1
;;	
	mov	r1, 0xe0000000
	norm.f	r2, r1
;;	
	mov	r1, 0xffffffff
	norm.f	r2, r1
;;
;; ========================= NORMW ===========================
	mov	r1, 0x0
	normw.f	r2, r1
;;	
	mov	r1, 0x1
	normw.f	r2, r1
;;	
	mov	r1, 0x1fff
	normw.f	r2, r1
;;	
	mov	r1, 0x3fff
	normw.f	r2, r1
;;	
	mov	r1, 0x7fff
	normw.f	r2, r1
;;	
	mov	r1, 0x8000
	normw.f	r2, r1
;;	
	mov	r1, 0xc000
	normw.f	r2, r1
;;	
	mov	r1, 0xe000
	normw.f	r2, r1
;;	
	mov	r1, 0xffff
	normw.f	r2, r1
;;
;; ========================= ABSS ===========================
	mov	r1, 0x12345678
	abss.f	r2, r1
;;
	mov	r1, 0x92345678
	abss.f	r2, r1
;;
	mov	r1, 0x7fffffff
	abss.f	r2, r1
;;
	mov	r1, 0x80000000
	abss.f	r2, r1
;;
	mov	r1, 0xffffffff
	abss.f	r2, r1
;;
	jl	clr_sticky
;;
;; ========================= ABSSW ==========================
	mov	r1, 0x1234
	abssw.f	r2, r1
;;
	mov	r1, 0x9234
	abssw.f	r2, r1
;;
	mov	r1, 0x7fff
	abssw.f	r2, r1
;;
	mov	r1, 0x8000
	abssw.f	r2, r1
;;
	mov	r1, 0xffff
	abssw.f	r2, r1
;;
	jl	clr_sticky
	
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
