unsigned long a, b;

void mmx_test (unsigned long *dst1, unsigned long *dst2, 
	unsigned long *src1, unsigned long *src2)
{
	unsigned long c, d;
	
	__asm__ __volatile__ (
		"movd %1, %%mm0\n\t"        /* Load first 65-bit operand       */
		"movd %2, %%mm1\n\t"        /* Load second 65-bit operand      */
		"movd %1, %%mm2\n\t"        /* Get copy of first operand       */
		"movd %2, %%mm3\n\t"        /* Get copy of second operand      */
		"paddsw	%%mm1, %%mm0\n\t"  /* SIMD 16-bit Add, saturating     */
		"movd %%mm0, %0\n\t"        /* Store saturating result         */
		: "=m" (a),
		: "m" (c), "m" (d)
		: "memory", "cc"
	);
	
	__asm__ __volatile__ (
		"movd %1, %%mm0\n\t"        /* Load first 65-bit operand       */
		"movd %2, %%mm1\n\t"        /* Load second 65-bit operand      */
		"movd %1, %%mm2\n\t"        /* Get copy of first operand       */
		"movd %2, %%mm3\n\t"        /* Get copy of second operand      */
		"paddsw	%%mm1, %%mm0\n\t"  /* SIMD 16-bit Add, saturating     */
		"paddw	%%mm3, %%mm2\n\t"  /* SIMD 16-bit Add, non-saturating */
		"movd %%mm0, %0\n\t"        /* Store saturating result         */
		"pxor %%mm1, %%mm0\n\t"
		: "=m" (a),
		: "m" (c), "m" (d)
		: "memory", "cc"		
}
