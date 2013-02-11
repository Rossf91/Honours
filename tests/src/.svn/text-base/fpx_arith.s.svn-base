	.file	"fpx_arith.s"

	.extInstruction	fmul, 0x06, 0x00, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	fadd, 0x06, 0x01, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	fsub, 0x06, 0x02, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP

	.extInstruction	dmulh11, 0x06, 0x08, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	dmulh12, 0x06, 0x09, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	dmulh21, 0x06, 0x0a, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	dmulh22, 0x06, 0x0b, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	
	.extInstruction	daddh11, 0x06, 0x0c, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	daddh12, 0x06, 0x0d, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	daddh21, 0x06, 0x0e, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	daddh22, 0x06, 0x0f, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	
	.extInstruction	dsubh11, 0x06, 0x10, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	dsubh12, 0x06, 0x11, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	dsubh21, 0x06, 0x12, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	dsubh22, 0x06, 0x13, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	
	.extInstruction	drsubh11, 0x06, 0x14, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	drsubh12, 0x06, 0x15, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	drsubh21, 0x06, 0x16, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	drsubh22, 0x06, 0x17, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	
	.extInstruction	dexcl1, 0x06, 0x18, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP
	.extInstruction	dexcl2, 0x06, 0x19, SUFFIX_COND|SUFFIX_FLAG, SYNTAX_3OP

	.extAuxRegister	fp_status,   0x300, r
	.extAuxRegister	dpfp1l,      0x301, r|w
	.extAuxRegister	dpfp12,      0x302, r|w
	.extAuxRegister	dpfp2l,      0x303, r|w
	.extAuxRegister	dpfp22,      0x304, r|w
	.extAuxRegister	dpfp_status, 0x305, r
	
	.globl	main
	.type 	main, @function
	.section .text

main:
;; ========================= FMUL ==========================
	mov	r1, 0x3fc00000 ;; 1.5
	mov	r2, 0x41480000 ;; 12.5
	fmul	r3, r2, r1     ;; r3 <= 0x41960000
;;
	
	mov	r4, 100000000
floop:
	fadd	r3, r3, r1
	sub.f r4, r4, 1
	bne	floop
	
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
	
	
;; =================== User-mode function ==================
;; This is a trivial user-mode function which purposefully
;; raises a privilege violation exception to gracelessly 
;; halt the program.
;;
	.globl	user_fun
	.type 	user_fun, @function
user_fun:
	rtie		;; not allowed in user mode => ev_privilege
	.size user_fun, .-user_fun
	
__Xspfp_check:
	j	[blink]


