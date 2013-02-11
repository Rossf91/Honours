	.file	"loop.s"
	.cpu	ARC700

	.globl	main
	.type 	main, @function
	.section .text

main:
;; ========================= loop ==========================
;;

	mov	r9, 100   ;; outer-loop iteration counter

	mov	r1, 100
	mov	r2, 200

repeat:
        mov   r3, 100
	
do_loop:
	sub r3, r3, 1
	cmp r3, 0
	asr r5,r4,3
	add r4,r1,r2
	brne r1,0x00300000,do_loop
;;
;; outer-loop iteration control
;;
	sub.f	r9, r9, 1
	bne.d	repeat
	cmp	r0, 1     ;; Fails if I put nop_s here!!
	
	j	[blink]
