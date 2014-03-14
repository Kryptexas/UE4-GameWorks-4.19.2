// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================================
	MacPlatformMath.h: Mac platform Math functions
==============================================================================================*/

#pragma once

/**
* Mac implementation of the Math OS functions
**/
struct FMacPlatformMath : public FGenericPlatformMath
{
	static FORCEINLINE int32 Trunc( float F )
	{
		return _mm_cvtt_ss2si( _mm_set_ss( F ) );
	}
	static FORCEINLINE float TruncFloat( float F )
	{
		return (float)Trunc(F); // same as generic implementation, but this will call the faster trunc
	}
	static FORCEINLINE int32 Round( float F )
	{
		// Note: the x2 is to workaround the rounding-to-nearest-even-number issue when the fraction is .5
		return _mm_cvt_ss2si( _mm_set_ss(F + F + 0.5f) ) >> 1;
	}
	static FORCEINLINE int32 Floor( float F )
	{
		return _mm_cvt_ss2si( _mm_set_ss(F + F - 0.5f) ) >> 1;
	}
	static FORCEINLINE int32 Ceil( float F )
	{
		// Note: the x2 is to workaround the rounding-to-nearest-even-number issue when the fraction is .5
		return -(_mm_cvt_ss2si( _mm_set_ss(-0.5f - (F + F))) >> 1);
	}
	static FORCEINLINE bool IsNaN( float A ) { return isnan(A) != 0; }
	static FORCEINLINE bool IsFinite( float A ) { return isfinite(A); }

	//
	// MSM: Fast float inverse square root using SSE.
	// Accurate to within 1 LSB.
	//
#if PLATFORM_ENABLE_VECTORINTRINSICS
	static FORCEINLINE float InvSqrt( float F )
	{
		static const __m128 fThree = _mm_set_ss( 3.0f );
		static const __m128 fOneHalf = _mm_set_ss( 0.5f );
		__m128 Y0, X0, Temp;
		float temp;

		Y0 = _mm_set_ss( F );
		X0 = _mm_rsqrt_ss( Y0 );	// 1/sqrt estimate (12 bits)

		// Newton-Raphson iteration (X1 = 0.5*X0*(3-(Y*X0)*X0))
		Temp = _mm_mul_ss( _mm_mul_ss(Y0, X0), X0 );	// (Y*X0)*X0
		Temp = _mm_sub_ss( fThree, Temp );				// (3-(Y*X0)*X0)
		Temp = _mm_mul_ss( X0, Temp );					// X0*(3-(Y*X0)*X0)
		Temp = _mm_mul_ss( fOneHalf, Temp );			// 0.5*X0*(3-(Y*X0)*X0)
		_mm_store_ss( &temp, Temp );

		return temp;
	}
#endif

};

typedef FMacPlatformMath FPlatformMath;

