;;                          Copyright Notice
;;
;;    Certain materials incorporated herein are copyright (C) 2004 – 2011, 
;;  The University Court of the University of Edinburgh. All Rights Reserved.
;;
;; =============================================================================


; Start of exception vectors
		.file   "vectors.s"
		.cpu    ARC700
		.section .text

		.globl	__vectors
		.type   __vectors, @function
                
		.globl  __ivec_3
		.type   __ivec_3, @function

		.globl  __ivec_4
		.type   __ivec_4, @function

		.globl	__ivec_8
		.type   __ivec_8, @function
                
		.globl	__ivec_9
		.type   __ivec_9, @function
                
__vectors:

		.globl  __reset
                .globl  __ivec_3
                .globl  __ivec_4
                .globl  __ivec_8
                .globl  __ivec_9
	
__reset:	j	reset_handler	; reset vector
__mem_err:	j	mem_err_handler	; memory error vector
__inst_err:	j	inst_err_handler ; instruction error vector
__ivec_3:	j	timer0_handler	; timer 0 interrupt vector
__ivec_4:	j	timer1_handler	; timer 1 interrupt vector
__ivec_5:	j	uart_handler	; UART interrupt vector
__ivec_6:	j	emac_handler	; EMAC interrupt vector
__ivec_7:	j	xy_mem_handler	; XY memory interrupt vector

__ivec_8:	j	irq_8_handler	; interrupt 8 vector
__ivec_9:	j	irq_9_handler	; interrupt 9 vector
__ivec_10:	j	irq_10_handler	; interrupt 10 vector
__ivec_11:	j	irq_11_handler	; interrupt 11 vector
__ivec_12:	j	irq_12_handler	; interrupt 12 vector
__ivec_13:	j	irq_13_handler	; interrupt 13 vector
__ivec_14:	j	irq_14_handler	; interrupt 14 ector
__ivec_15:	j	irq_15_handler	; interrupt 15 vector
__ivec_16:	j	irq_16_handler	; interrupt 16 vector
__ivec_17:	j	irq_17_handler	; interrupt 17 vector
__ivec_18:	j	irq_18_handler	; interrupt 18 vector
__ivec_19:	j	irq_19_handler	; interrupt 19 vector
__ivec_20:	j	irq_20_handler	; interrupt 20 vector
__ivec_21:	j	irq_21_handler	; interrupt 21 vector
__ivec_22:	j	irq_22_handler	; interrupt 22 vector
__ivec_23:	j	irq_23_handler	; interrupt 23 vector
__ivec_24:	j	irq_24_handler	; interrupt 24 vector
__ivec_25:	j	irq_25_handler	; interrupt 25 vector
__ivec_26:	j	irq_26_handler	; interrupt 26 vector
__ivec_27:	j	irq_27_handler	; interrupt 27 vector
__ivec_28:	j	irq_28_handler	; interrupt 28 vector
__ivec_29:	j	irq_29_handler	; interrupt 29 vector
__ivec_30:	j	irq_30_handler	; interrupt 30 vector
__ivec_31:	j	irq_31_handler	; interrupt 31 vector
__ev_mcheck:	j	ev_mc_check	; machine check exception
__ev_itlbmiss:	j	ev_i_tlbmiss	; instruction tlb miss exception
__ev_dtlbmiss:	j	ev_d_tlbmiss	; data tlb miss exception
__ev_protvio:	j	ev_protv	; tlb protection violation exception
__ev_privi:	j	ev_privilege	; privilege violation exception
__ev_trap:	j 	ev_trap 	; trap exception
__ev_ext:	j	ev_extension	; extension exception
		.size	__vectors, .-__vectors
