/*
xash_altivec.h - PowerPC AltiVec helpers
*/
#ifndef XASH_ALTIVEC_H
#define XASH_ALTIVEC_H

#if XASH_ALTIVEC

#include <stdint.h>
#include <altivec.h>

/*
 * GCC's altivec.h exposes bool, vector, and pixel as macros. Keep those
 * keywords local to this header so engine code that uses those names still
 * compiles after inclusion.
 */
#ifdef bool
#undef bool
#endif
#ifdef vector
#undef vector
#endif
#ifdef pixel
#undef pixel
#endif

#define XASH_ALIGNED16 __attribute__(( aligned( 16 )))

static inline int XASH_AltivecAligned16( const void *ptr )
{
	return (((uintptr_t)ptr) & 15) == 0;
}

static inline float XASH_AltivecDot4( __vector float a, __vector float b )
{
	union
	{
		__vector float v;
		float f[4];
	} result;
	__vector float zero = { 0.0f, 0.0f, 0.0f, 0.0f };

	__vector float sum = vec_madd( a, b, zero );

	sum = vec_add( sum, vec_sld( sum, sum, 8 ) );
	sum = vec_add( sum, vec_sld( sum, sum, 4 ) );
	result.v = sum;

	return result.f[0];
}

#endif // XASH_ALTIVEC

#ifndef XASH_ALIGNED16
#define XASH_ALIGNED16
#endif

#endif // XASH_ALTIVEC_H
