;; Kernel exception vector definitions
;;
	.file	"kernel.s"
	.cpu	ARC700
	
	.globl	reset_handler
	.globl	mem_err_handler
	.globl	inst_err_handler
	.globl	timer0_handler
	.globl	timer1_handler
	.globl	uart_handler
	.globl	emac_handler
	.globl	xy_mem_handler
	.globl	irq_8_handler
	.globl	irq_9_handler
	.globl	irq_10_handler
	.globl	irq_11_handler
	.globl	irq_12_handler
	.globl	irq_13_handler
	.globl	irq_14_handler
	.globl	irq_15_handler
	.globl	irq_16_handler
	.globl	irq_17_handler
	.globl	irq_18_handler
	.globl	irq_19_handler
	.globl	irq_20_handler
	.globl	irq_21_handler
	.globl	irq_22_handler
	.globl	irq_23_handler
	.globl	irq_24_handler
	.globl	irq_25_handler
	.globl	irq_26_handler
	.globl	irq_27_handler
	.globl	irq_28_handler
	.globl	irq_29_handler
	.globl	irq_30_handler
	.globl	irq_31_handler
	.globl	ev_mc_check
	.globl	ev_i_tlbmiss
	.globl	ev_d_tlbmiss
	.globl	ev_protv
	.globl	ev_privilege
	.globl	ev_trap
	.globl	ev_extension

;; Linker script should ensure that .text is located
;; at address 0x00000000
;;
	.section .text
	
	.global reset_handler
	.type	reset_handler, @function
reset_handler:	; reset vector
	mov	r0, 0
	mov	r1, r0
	mov	r2, r0
	mov	r3, r0
	mov	r4, r0
	mov	r5, r0
	mov	r6, r0
	mov	r7, r0
	mov	r8, r0
	mov	r9, r0
	mov	r10, r0
	mov	r12, r0
	mov	r13, r0
	mov	r14, r0
	mov	r15, r0
	mov	r16, r0
	mov	r17, r0
	mov	r18, r0
	mov	r19, r0
	mov	r20, r0
	mov	r21, r0
	mov	r22, r0
	mov	r23, r0
	mov	r24, r0
	mov	r25, r0
	mov	r26, r0
	mov	r27, r0
	mov	r28, r0
	mov	r29, r0
	mov	r30, r0
	mov	r31, r0
	mov	sp, __stack_top
	mov	r0, main
	sr	r0, [0x400]	; store entry point in ERET
;;
;; select one of the following two lines, depending on mode required
;;	mov	r0, 0x80	; STATUS32 has U-bit set for user-mode execution
	mov	r0, 0  	; STATUS32 has U-bit clearr for kernel-mode execution
;;
	sr	r0, [0x402]	; store status to ERSTATUS
	rtie
	.size	reset_handler, .-reset_handler
	
;; Exception handlers - dafault action is to halt the CPU

	.global mem_err_handler
	.type	mem_err_handler, @function
mem_err_handler:	; memory error vector
	flag	1	; halt CPU
	.size	mem_err_handler, .-mem_err_handler
	
	.global inst_err_handler
	.type	inst_err_handler, @function
inst_err_handler:	; instruction error vector
	flag	1	; halt CPU
	.size	inst_err_handler, .-inst_err_handler

	.global timer0_handler
	.type	timer0_handler, @function
timer0_handler:	; timer 0 interrupt vector
;; Timer interrupts are acknowledged automatically
;; by the timer itself. Normally we would increment
;; a timeval structure, but this is not done here.
;; 
	rtie	; return immediately from interrupt
	.size	timer0_handler, .-timer0_handler

	.global timer1_handler
	.type	timer1_handler, @function
timer1_handler:	; timer 1 interrupt vector
	flag	1	; halt CPU
	.size	timer1_handler, .-timer1_handler

uart_handler:	; UART interrupt vector
	flag	1	; halt CPU

emac_handler:	; EMAC interrupt vector
	flag	1	; halt CPU

xy_mem_handler:	; XY memory interrupt vector
	flag	1	; halt CPU

irq_8_handler:	; interrupt 8 vector
	mov	r0, 0
	sr 	r0, [0x201] ; clear AUX_IRQ_HINT
	flag	1	; halt CPU

irq_9_handler:	; interrupt 9 vector
	flag	1	; halt CPU

irq_10_handler:	; interrupt 10 vector
	flag	1	; halt CPU

irq_11_handler:	; interrupt 11 vector
	flag	1	; halt CPU

irq_12_handler:	; interrupt 12 vector
	flag	1	; halt CPU

irq_13_handler:	; interrupt 13 vector
	flag	1	; halt CPU

irq_14_handler:	; interrupt 14 ector
	flag	1	; halt CPU

irq_15_handler:	; interrupt 15 vector
	flag	1	; halt CPU

irq_16_handler:	; interrupt 16 vector
	flag	1	; halt CPU

irq_17_handler:	; interrupt 17 vector
	flag	1	; halt CPU

irq_18_handler:	; interrupt 18 vector
	flag	1	; halt CPU

irq_19_handler:	; interrupt 19 vector
	flag	1	; halt CPU
	
irq_20_handler:	; interrupt 20 vector
	flag	1	; halt CPU
	
irq_21_handler:	; interrupt 21 vector
	flag	1	; halt CPU
	
irq_22_handler:	; interrupt 22 vector
	flag	1	; halt CPU
	
irq_23_handler:	; interrupt 23 vector
	flag	1	; halt CPU
	
irq_24_handler:	; interrupt 24 vector
	flag	1	; halt CPU
	
irq_25_handler:	; interrupt 25 vector
	flag	1	; halt CPU
	
irq_26_handler:	; interrupt 26 vector
	flag	1	; halt CPU
	
irq_27_handler:	; interrupt 27 vector
	flag	1	; halt CPU
	
irq_28_handler:	; interrupt 28 vector
	flag	1	; halt CPU
	
irq_29_handler:	; interrupt 29 vector
	flag	1	; halt CPU
	
irq_30_handler:	; interrupt 30 vector
	flag	1	; halt CPU
	
irq_31_handler:	; interrupt 31 vector
	flag	1	; halt CPU
	
ev_mc_check:	; machine check exception
	flag	1	; halt CPU
	
ev_i_tlbmiss:	; instruction tlb miss exception
	flag	1	; halt CPU
	
ev_d_tlbmiss:	; data tlb miss exception
	flag	1	; halt CPU
	
ev_protv:	; tlb protection violation exception
	flag	1	; halt CPU
	
	.global ev_privilege
	.type	ev_privilege, @function
ev_privilege:	; privilege violation exception
	flag	1	; halt CPU
	.size	ev_privilege, .-ev_privilege
	
ev_trap:	; trap exception
	flag	1	; halt CPU
	
ev_extension:	; extension exception
	flag	1	; halt CPU


