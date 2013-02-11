	.file	"pasta_test.s"
	.cpu	ARC700

	.globl	main
	.type 	main, @function
	.section .text
	
	.extCoreRegister r32, 32, r|w, can_shortcut
	.extCoreRegister r33, 33, r|w, can_shortcut
	.extCoreRegister r34, 34, r|w, can_shortcut
	.extCoreRegister r35, 35, r|w, can_shortcut
	.extCoreRegister r36, 36, r|w, can_shortcut
	.extCoreRegister r37, 37, r|w, can_shortcut
	.extCoreRegister r38, 38, r|w, can_shortcut
	.extCoreRegister r39, 39, r|w, can_shortcut
	.extCoreRegister r40, 40, r|w, can_shortcut
	.extCoreRegister r41, 41, r|w, can_shortcut
	.extCoreRegister r42, 42, r|w, can_shortcut
	.extCoreRegister r43, 43, r|w, can_shortcut
	.extCoreRegister r44, 44, r|w, can_shortcut
	.extCoreRegister r45, 45, r|w, can_shortcut
	.extCoreRegister r46, 46, r|w, can_shortcut
	.extCoreRegister r47, 47, r|w, can_shortcut
	.extCoreRegister r48, 48, r|w, can_shortcut
	.extCoreRegister r49, 49, r|w, can_shortcut
	.extCoreRegister r50, 50, r|w, can_shortcut
	.extCoreRegister r51, 51, r|w, can_shortcut
	.extCoreRegister r52, 52, r|w, can_shortcut
	.extCoreRegister r53, 53, r|w, can_shortcut
	.extCoreRegister r54, 54, r|w, can_shortcut
	.extCoreRegister r55, 55, r|w, can_shortcut

main:
;; ========================= PASTA TEST ==========================

	mov   r32, 1
	mov   r33, 2
	mov   r34, 3
	mov   r35, 4

	.word 0x00833800  ;; vmov vr3, (vr0:p1), 0x0000
      	
	mov	r1, r52
	mov	r2, r53
	mov	r3, r54
	mov	r4, r55
	
endtest:
	mov	r0, user_fun
	sr	r0, [0x400]	; store entry point in ERET
	mov	r0, 0x80	; STATUS32 has U-bit set for user-mode execution
	sr	r0, [0x402]	; store status to ERSTATUS
	rtie
	.size main, .-main
		
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

