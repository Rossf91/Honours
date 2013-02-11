	.file	"lp_test.s"
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

	mov	r9, 200000   ;; outer-loop iteration counter

do_loop:
;;
;; Loop to write 0xa5a5a5a5 pattern into 10 word locations
;;
	mov	r0, 0xa5a5a5a5
	mov	r1, 0x02000000
	mov	r2, 0x02000200
	mov	lp_count, 100

	lp 	end1
	st 	r0,[r1,4]
	st_s	r0,[r2,4]
	add   r1,r1,4
	add	r2,r2,4
end1:
;;
;; Now read back the data
;;
	mov	r1, 0x02000000
	mov	r2, 0x02000200
	mov	lp_count, 100

	lp 	end2
	ld 	r0,[r1,4]
	ld_s	r3,[r2,4]
	cmp_s r0,r3
	bne	fail
	add   r1,r1,4
	add	r2,r2,4
end2:

;; outer-loop iteration control
;;
	sub.f	r9, r9, 1
	bne	do_loop
	
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

