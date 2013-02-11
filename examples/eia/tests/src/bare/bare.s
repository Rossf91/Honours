;;                          Copyright Notice
;;
;;    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
;;  The University Court of the University of Edinburgh. All Rights Reserved.
;;
;; =============================================================================


;; Kernel exception vector definitions
;;
	.file	"bare.s"
	.cpu	ARC700
	
	.global	reset_handler
	.global	mem_err_handler
	.global	inst_err_handler
	.global	timer0_handler
	.global	timer1_handler
	.global	uart_handler
	.global	emac_handler
	.global	xy_mem_handler
	.global	irq_8_handler
	.global	irq_9_handler
	.global	irq_10_handler
	.global	irq_11_handler
	.global	irq_12_handler
	.global	irq_13_handler
	.global	irq_14_handler
	.global	irq_15_handler
	.global	irq_16_handler
	.global	irq_17_handler
	.global	irq_18_handler
	.global	irq_19_handler
	.global	irq_20_handler
	.global	irq_21_handler
	.global	irq_22_handler
	.global	irq_23_handler
	.global	irq_24_handler
	.global	irq_25_handler
	.global	irq_26_handler
	.global	irq_27_handler
	.global	irq_28_handler
	.global	irq_29_handler
	.global	irq_30_handler
	.global	irq_31_handler
	.global	ev_mc_check
	.global	ev_i_tlbmiss
	.global	ev_d_tlbmiss
	.global	ev_protv
	.global	ev_privilege
	.global	ev_trap
	.global	ev_extension

;; Linker script should ensure that .text is located
;; at an address above the vector table.
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
        mov     r11, r0
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
        lr      r0, [0x40c]     ; read IENABLE
        flag    0x0006 ; set E2, E1 bits
        ;mov r0, 0xffffffff  ; enable all interrupts
        sr      0xffffffff, [0x40c]
	jal	main
	rtie
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
  nop
	.size	reset_handler, .-reset_handler
	
;; Exception handlers - dafault action is to halt the CPU

	.global mem_err_handler
	.type	mem_err_handler, @function
mem_err_handler:	; memory error vector
	rtie	; return from interrupt or exception
	.size	mem_err_handler, .-mem_err_handler
	
	.global inst_err_handler
	.type	inst_err_handler, @function
inst_err_handler:	; instruction error vector
	flag 1	; halt on instruction error exception
	.size	inst_err_handler, .-inst_err_handler

	.global timer0_handler
	.type	timer0_handler, @function
timer0_handler:	; timer 0 interrupt vector
;; Timer interrupts are acknowledged automatically
;; by the timer itself. Normally we would increment
;; a timeval structure, but this is not done here.
;; 
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
;;	rtie	; return immediately from interrupt
	.size	timer0_handler, .-timer0_handler

	.global timer1_handler
	.type	timer1_handler, @function
timer1_handler:	; timer 1 interrupt vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	.size	timer1_handler, .-timer1_handler

uart_handler:	; UART interrupt vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

emac_handler:	; EMAC interrupt vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

xy_mem_handler:	; XY memory interrupt vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

	.global irq_8_handler
	.type	irq_8_handler, @function
irq_8_handler:	; interrupt 8 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
        lr      r0, [0xa]   ; read STATUS32
        lr      r0, [0x412] ; read BTA
        lr      r0, [0xc]   ; read STATUS32_L2
        lr      r0, [0x43]  ; read IRQ_LV12
        lr      r0, [0x40b] ; read ICAUSE2
        lr      r0, [0x414] ; read BTA_L2
        mov     r0, ilink2  ; read ilink2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
        rtie

	.global irq_9_handler
	.type	irq_9_handler, @function
irq_9_handler:	; interrupt 9 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
        lr      r0, [0xa]   ; read STATUS32
        lr      r0, [0x412] ; read BTA
        lr      r0, [0xb]   ; read STATUS32_L1
        lr      r0, [0x43]  ; read IRQ_LV12
        lr      r0, [0x40a] ; read ICAUSE1
        lr      r0, [0x413] ; read BTA_L1
        mov     r0, ilink1  ; read ilink1
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
        rtie

	.global irq_10_handler
	.type	irq_10_handler, @function
irq_10_handler:	; interrupt 10 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

	.global irq_11_handler
	.type	irq_11_handler, @function
irq_11_handler:	; interrupt 11 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

	.global irq_12_handler
	.type	irq_12_handler, @function
irq_12_handler:	; interrupt 12 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

	.global irq_13_handler
	.type	irq_13_handler, @function
irq_13_handler:	; interrupt 13 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

	.global irq_14_handler
	.type	irq_14_handler, @function
irq_14_handler:	; interrupt 14 ector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

	.global irq_15_handler
	.type	irq_15_handler, @function
irq_15_handler:	; interrupt 15 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

	.global irq_16_handler
	.type	irq_16_handler, @function
irq_16_handler:	; interrupt 16 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

	.global irq_17_handler
	.type	irq_17_handler, @function
irq_17_handler:	; interrupt 17 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

	.global irq_18_handler
	.type	irq_18_handler, @function
irq_18_handler:	; interrupt 18 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception

	.global irq_19_handler
	.type	irq_19_handler, @function
irq_19_handler:	; interrupt 19 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_20_handler
	.type	irq_20_handler, @function
irq_20_handler:	; interrupt 20 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_21_handler
	.type	irq_21_handler, @function
irq_21_handler:	; interrupt 21 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_22_handler
	.type	irq_22_handler, @function
irq_22_handler:	; interrupt 22 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_23_handler
	.type	irq_23_handler, @function
irq_23_handler:	; interrupt 23 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_24_handler
	.type	irq_24_handler, @function
irq_24_handler:	; interrupt 24 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_25_handler
	.type	irq_25_handler, @function
irq_25_handler:	; interrupt 25 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_26_handler
	.type	irq_26_handler, @function
irq_26_handler:	; interrupt 26 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_27_handler
	.type	irq_27_handler, @function
irq_27_handler:	; interrupt 27 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_28_handler
	.type	irq_28_handler, @function
irq_28_handler:	; interrupt 28 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_29_handler
	.type	irq_29_handler, @function
irq_29_handler:	; interrupt 29 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_30_handler
	.type	irq_30_handler, @function
irq_30_handler:	; interrupt 30 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global irq_31_handler
	.type	irq_31_handler, @function
irq_31_handler:	; interrupt 31 vector
  lr    r0,[0x40A]  ;; read ICAUSE1
  lr    r1,[0x40B]  ;; read ICAUSE2
  xor   r0,r0,r0
  sr    r0,[0x201]  ;; clear IRQ_HINT
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global ev_mc_check
	.type	ev_mc_check, @function
ev_mc_check:	; machine check exception
	rtie	; return from interrupt or exception
	
	.global ev_i_tlbmiss
	.type	ev_i_tlbmiss, @function
ev_i_tlbmiss:	; instruction tlb miss exception
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global ev_d_tlbmiss
	.type	ev_d_tlbmiss, @function
ev_d_tlbmiss:	; data tlb miss exception
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global ev_protv
	.type	ev_protv, @function
ev_protv:	; tlb protection violation exception
  .word 0x003f246f
	rtie	; return from interrupt or exception
	
	.global ev_privilege
	.type	ev_privilege, @function
ev_privilege:	; privilege violation exception
  b ap_handler
	.size	ev_privilege, .-ev_privilege
	
	.global ev_trap
	.type	ev_trap, @function
ev_trap:	; trap exception
        lr      r0, [0x403] ; read ECR
        lr      r0, [0x400] ; read ERET
        lr      r0, [0x402] ; read ERSTATUS
        lr      r0, [0x412] ; read BTA
        lr      r0, [0x404] ; read EFA
        lr      r0, [0x6]   ; read PC
        lr      r0, [0xa]   ; read STATUS32
        lr      r0, [0x401] ; read ERBTA
  .word 0x003f246f
        rtie
;	rtie	; return from interrupt or exception
	
	.global ev_extension
	.type	ev_extension, @function
ev_extension:	; extension exception
  .word 0x003f246f
	rtie	; return from interrupt or exception

	.global ap_handler
	.type	ap_handler, @function
ap_handler:
  mov   r0, 0x104    ;; Q=0, A=1, P=0, M=0, TT=00 (read), AT=0100 (aux addr)
  sr    r0, [0x225]  ;; write to AP_AC1
  .word 0x003f246f
	rtie	; return from interrupt or exception

                                           