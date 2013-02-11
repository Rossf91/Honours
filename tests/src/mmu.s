	.file	"mmu.s"
	.cpu	ARC700

	.globl	main
	.type 	main, @function
	.section .text
main:
;; 
;; Write to the AUX_PID register to set PID.T and
;; to set ASID to 1.
;;
	mov	r0, 0x80000001
	sr 	r0, [0x409] ;; this will lead to machine-check
;;
;; Context-switch into a user-mode function, with L1
;; interrupts enabled. The user-mode function should be
;; interrupted regularly, once every second. On return from
;; interrupt, the user code should continue normally.
;;
	mov	r0, user_fun
	sr		r0, [0x400]	; store entry point in ERET
	mov	r0, 0x82	   ; STATUS32 has U and E1 bits set
	sr		r0, [0x402]	; store status to ERSTATUS
	rtie					; rtie into user_fun with L1 ints enabled
	.size main, .-main

;; =================== User-mode function ==================
;; This is a trivial user-mode function which loops for
;; a long time to give the timers chance to do something
;; interesting (e.g. timeout).
;;
	.globl	user_fun
	.type 	user_fun, @function
user_fun:
;; ====== loop
	mov	r9, 100000000
loop1:
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
	sub.f r9, r9, 1
	bne	loop1
;;
	rtie		;; not allowed in user mode => ev_privilege
	.size user_fun, .-user_fun

