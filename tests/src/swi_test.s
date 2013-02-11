	.file	"swi_test.s"
	.cpu	ARC700

	.globl	main
	.type 	main, @function
	.section .text

main:
;; ========== Test S/W triggered interrupt =============

;; Set the AUX_IRQ_HINT register to 3, signifying a hint
;; that IRQ3 should be taken.
;;
	mov	r0, 8
	sr 	r0, [0x201] ; write 8 to AUX_IRQ_HINT
	
;; Set the AUX_IENABLE mask to enable IRQ3, but no other.
;;
	mov	r0, 0x08
	sr 	r0, [0x40C] ; write 0x08 to AUX_IENABLE
;;
;; Sample the interrupt-related aux status registers
;;
	lr 	r0, [0x043] ; read AUX_IRQ_LV12 register
	lr 	r1, [0x200] ; read AUX_IRQ_LEV
	lr 	r2, [0x40A] ; read ICAUSE1
	lr 	r3, [0x40B] ; read ICAUSE2
	lr 	r4, [0x413] ; read AUX_BTA_L1
	lr 	r5, [0x414] ; read AUX_BTA_L2
	lr 	r6, [0x416] ; read AUX_IRQ_PENDING
	lr 	r7, [0x40C] ; read AUX_IENABLE
	
;; Context-switch into a user-mode function, which also
;; enables interrupts. The user-mode function should not
;; have the chance to execute even a single instruction
;; before the interrupt handler is called. On return from
;; interrupt, the user code should be executed.
;;
	mov	r0, user_fun
	sr		r0, [0x400]	; store entry point in ERET
	mov	r0, 0x82	   ; STATUS32 has U and E1 bits set
	sr		r0, [0x402]	; store status to ERSTATUS
	rtie
	.size main, .-main
	
	
;; =================== User-mode function ==================
;; This is a trivial user-mode function which purposefully
;; raises a privilege violation exception to gracelessly 
;; halt the program after executing 16 dummy instructions.
;;
	.globl	user_fun
	.type 	user_fun, @function
user_fun:
	mov		r0, 1
	mov		r0, 2
	mov		r0, 3
	mov		r0, 4
	mov		r0, 5
	mov		r0, 6
	mov		r0, 7
	mov		r0, 8
	mov		r0, 9
	mov		r0, 10
	mov		r0, 11
	mov		r0, 12
	mov		r0, 13
	mov		r0, 14
	mov		r0, 15
	mov		r0, 16
	rtie		;; not allowed in user mode => ev_privilege
	.size user_fun, .-user_fun

