// gcc -S -fno-strict-aliasing -msse2 -O4 sse2_test.cpp

#include "../simd_types.h"
#include <stdio.h>
	
vreg VR[64];

#if 1
static inline void vector_add (__m128i &a, __m128i &b ,__m128i &c)
{
	register __m128i d, e;
	
	d = _mm_add_epi16 (a, b);
	e = _mm_add_epi16 (a, d);
	c = _mm_add_epi16 (e, b);
}
#endif

static inline void vsplat (vreg &v, unsigned short s)
{
	unsigned long l = s | (s<<16);
	
	__asm__ __volatile__ (
		"   movd    %1, %%xmm0         \n"
		"   pshuflw $0, %%xmm0, %%xmm0 \n"
		"   pshufd  $0, %%xmm0, %%xmm0 \n"
		"   movdqa  %%xmm0, %0         \n"
		: "=m"(v.m128)
		: "m"(l)
		: "xmm0"
	);
}

short ss;
__m128i a, b, c;

int main ()
{
	
	long i, j;
	
	printf ("Checking size of vreg object: %d bytes\n", sizeof(vreg));
	for (i = 0; i < 8; i++)
		VR[0].u16[i] = 0;
	
	ss = 46;
	printf ("Testing my vsplat function\n");
	printf ("all 16-bit vector elements of VR[0] should be set to %d\n", ss);

	vsplat (VR[0], ss);
	
	for (i = 0; i < 8; i++)
		printf ("\tVR[0].u16[%d] = %d\n", i, VR[0].u16[i]);
	
	ss = 23;
	printf ("Testing splat function _mm_set1_epi16()\n");
	printf ("all 16-bit vector elements of VR[0] should be set to %d\n", ss);
	
	VR[0].m128 = _mm_set1_epi16 (ss);
	
	for (i = 0; i < 8; i++)
		printf ("\tVR[0].u16[%d] = %d\n", i, VR[0].u16[i]);

	return 0;
}

