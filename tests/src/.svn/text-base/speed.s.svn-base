	.file	"speed.s"
	.cpu	ARC700

	.globl	main
	.type 	main, @function
	.section .text

main:
;; ========================= LPCC ==========================
;;
;; First test is a simple loop containing a handful of 
;; ordinary instructions.
;;
;; This test is repeated 1,000,000 times to force translation
;; to take place:

	mov	r9, 1   ;; outer-loop iteration counter

repeat:
   mov   r3, 10000000
   b     do_loop
   
; do_loop:
; 140
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
; 135
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
; 130
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
; 125
	add r19, r19, 1
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
; 120
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
	add r4, r4, 1
	add r5, r5, 1
; 115
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
; 110
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
; 105
	add r19, r19, 1
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
; 100
do_loop:
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
	add r4, r4, 1
	add r5, r5, 1
; 95
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
; 90
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
; 85
	add r19, r19, 1
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
; 80
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
; 75
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
; 70
	add r19, r19, 1
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
; 65
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
	add r4, r4, 1
	add r5, r5, 1
; 60
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
; 55
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
; 50
	add r19, r19, 1
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
; 45
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
	add r4, r4, 1
	add r5, r5, 1
; 40
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
; 35
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
; 30
	add r19, r19, 1
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
; 25
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
	add r4, r4, 1
	add r5, r5, 1
; 20
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
	add r19, r19, 1
	add r10, r10, 1
; 15
	add r4, r4, 1
	add r5, r5, 1
	add r6, r6, 1
	add r7, r7, 1
	add r8, r8, 1
; 10
	add r19, r19, 1
	add r10, r10, 1
	sub r3, r3, 1
	add r2, r2, 1
	cmp r3, 0
; 5
	lsr r2, r2
	or  r5, r5, r2
	and r5, r3, 0x3f
	bne.d do_loop
	mov r4, r2
; 0
end1:
;;
;; outer-loop iteration control
;;
	sub.f	r9, r9, 1
	bne	repeat
	
	flag 1
	
okay:
	mov	r10,0
	b		exit_test

;;
fail:
	mov	r10,0xffffffff

exit_test:
;; EXIT THE TEST
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

