// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	GenericPlatformMath.h: Generic platform Math classes, mostly implemented with ANSI C++
==============================================================================================*/

#pragma once


/**
 * Generic implementation for most platforms
 */
struct FGenericPlatformMath
{
	/**
	 * Converts a float to an integer with truncation towards zero.
	 * @param F		Floating point value to convert
	 * @return		Truncated integer.
	 */
	static CONSTEXPR FORCEINLINE int32 Trunc( float F )
	{
		return (int32)F;
	}

	/**
	 * Converts a float to an integer value with truncation towards zero.
	 * @param F		Floating point value to convert
	 * @return		Truncated integer value.
	 */
	static CONSTEXPR FORCEINLINE float TruncFloat( float F )
	{
		return (float)Trunc(F);
	}

	/**
	 * Converts a float to a less or equal integer.
	 * @param F		Floating point value to convert
	 * @return		An integer less or equal to 'F'.
	 */
	static FORCEINLINE int32 Floor( float F )
	{
		return Trunc(floorf(F));
	}

	/**
	 * Converts a double to a less or equal integer.
	 * @param F		Floating point value to convert
	 * @return		The nearest integer value to 'F'.
	 */
	static FORCEINLINE double FloorDouble( double F )
	{
		return floor(F);
	}

	/**
	 * Converts a float to the nearest integer. Rounds up when the fraction is .5
	 * @param F		Floating point value to convert
	 * @return		The nearest integer to 'F'.
	 */
	static FORCEINLINE int32 Round( float F )
	{
		return Floor(F+0.5f);
	}

	/**
	 * Converts a float to a greater or equal integer.
	 * @param F		Floating point value to convert
	 * @return		An integer greater or equal to 'F'.
	 */
	static FORCEINLINE int32 Ceil( float F )
	{
		return Trunc(ceilf(F));
	}
	/**
	 * Returns the fractional part of a float.
	 * @param Value	Floating point value to convert
	 * @return		A float between >=0 and < 1.
	 */
	static FORCEINLINE float Fractional( float Value ) 
	{ 
		return Value - TruncFloat( Value );
	}

	static FORCEINLINE float Exp( float Value ) { return expf(Value); }
	static FORCEINLINE float Loge( float Value ) {	return logf(Value); }
	static FORCEINLINE float LogX( float Base, float Value ) { return Loge(Value) / Loge(Base); }
	// 1.0 / Loge(2) = 1.4426950f
	static FORCEINLINE float Log2( float Value ) { return Loge(Value) * 1.4426950f; }	

	/** 
	* Returns the floating-point remainder of X / Y
	* Warning: Always returns remainder toward 0, not toward the smaller multiple of Y.
	*			So for example Fmod(2.8f, 2) gives .8f as you would expect, however, Fmod(-2.8f, 2) gives -.8f, NOT 1.2f 
	* Use Floor instead when snapping positions that can be negative to a grid
	*/
	static FORCEINLINE float Fmod( float X, float Y ) { return fmodf(X, Y); }
	static FORCEINLINE float Sin( float Value ) { return sinf(Value); }
	static FORCEINLINE float Asin( float Value ) { return asinf( (Value<-1.f) ? -1.f : ((Value<1.f) ? Value : 1.f) ); }
	static FORCEINLINE float Cos( float Value ) { return cosf(Value); }
	static FORCEINLINE float Acos( float Value ) { return acosf( (Value<-1.f) ? -1.f : ((Value<1.f) ? Value : 1.f) ); }
	static FORCEINLINE float Tan( float Value ) { return tanf(Value); }
	static FORCEINLINE float Atan( float Value ) { return atanf(Value); }
	static FORCEINLINE float Atan2( float Y, float X ) { return atan2f(Y,X); }
	static FORCEINLINE float Sqrt( float Value ) { return sqrtf(Value); }
	static FORCEINLINE float Pow( float A, float B ) { return powf(A,B); }

	/** Computes a fully accurate inverse square root */
	static FORCEINLINE float InvSqrt( float F )
	{
		return 1.0f / sqrtf( F );
	}

	/** Computes a faster but less accurate inverse square root */
	static FORCEINLINE float InvSqrtEst( float F )
	{
		return InvSqrt( F );
	}

	static FORCEINLINE bool IsNaN( float A ) 
	{
		return ((*(uint32*)&A) & 0x7FFFFFFF) > 0x7F800000;
	}
	static FORCEINLINE bool IsFinite( float A )
	{
		return ((*(uint32*)&A) & 0x7F800000) != 0x7F800000;
	}
	static FORCEINLINE bool IsNegativeFloat(const float& F1)
	{
		return ( (*(uint32*)&F1) >= (uint32)0x80000000 ); // Detects sign bit.
	}

	/** Returns a random integer between 0 and RAND_MAX, inclusive */
	static FORCEINLINE int32 Rand() { return rand(); }

	/** Seeds global random number functions Rand() and FRand() */
	static FORCEINLINE void RandInit(int32 Seed) { srand( Seed ); }

	/** Returns a random float between 0 and 1, inclusive. */
	static FORCEINLINE float FRand() { return rand() / (float)RAND_MAX; }

	/** Seeds future calls to SRand() */
	static CORE_API void SRandInit( int32 Seed );

	/** Returns a seeded random float in the range [0,1), using the seed from SRandInit(). */
	static CORE_API float SRand();

	/**
	 * Computes the base 2 logarithm for an integer value that is greater than 0.
	 * The result is rounded down to the nearest integer.
	 *
	 * @param Value		The value to compute the log of
	 * @return			Log2 of Value. 0 if Value is 0.
	 */	
	static FORCEINLINE uint32 FloorLog2(uint32 Value) 
	{
		uint32 Bit = 32;
		for (; Bit > 0;)
		{
			Bit--;
			if (Value & (1<<Bit))
			{
				break;
			}
		}
		return Bit;
	}

	/**
	 * Counts the number of leading zeros in the bit representation of the value
	 *
	 * @param Value the value to determine the number of leading zeros for
	 *
	 * @return the number of zeros before the first "on" bit
	 */
	static FORCEINLINE uint32 CountLeadingZeros(uint32 Value)
	{
		if (Value == 0) return 32;
		return 31 - FloorLog2(Value);
	}

	/**
	 * Returns smallest N such that (1<<N)>=Arg.
	 * Note: CeilLogTwo(0)=0 because (1<<0)=1 >= 0.
	 */
	static FORCEINLINE uint32 CeilLogTwo( uint32 Arg )
	{
		int32 Bitmask = ((int32)(CountLeadingZeros(Arg) << 26)) >> 31;
		return (32 - CountLeadingZeros(Arg - 1)) & (~Bitmask);
	}

	/** @return Rounds the given number up to the next highest power of two. */
	static FORCEINLINE uint32 RoundUpToPowerOfTwo(uint32 Arg)
	{
		return 1 << CeilLogTwo(Arg);
	}

	/** Spreads bits to every other. */
	static FORCEINLINE uint32 MortonCode2( uint32 x )
	{
		x &= 0x0000ffff;
		x = (x ^ (x << 8)) & 0x00ff00ff;
		x = (x ^ (x << 4)) & 0x0f0f0f0f;
		x = (x ^ (x << 2)) & 0x33333333;
		x = (x ^ (x << 1)) & 0x55555555;
		return x;
	}

	/** Reverses MortonCode2. Compacts every other bit to the right. */
	static FORCEINLINE uint32 ReverseMortonCode2( uint32 x )
	{
		x &= 0x55555555;
		x = (x ^ (x >> 1)) & 0x33333333;
		x = (x ^ (x >> 2)) & 0x0f0f0f0f;
		x = (x ^ (x >> 4)) & 0x00ff00ff;
		x = (x ^ (x >> 8)) & 0x0000ffff;
		return x;
	}

	/** Spreads bits to every 3rd. */
	static FORCEINLINE uint32 MortonCode3( uint32 x )
	{
		x &= 0x000003ff;
		x = (x ^ (x << 16)) & 0xff0000ff;
		x = (x ^ (x <<  8)) & 0x0300f00f;
		x = (x ^ (x <<  4)) & 0x030c30c3;
		x = (x ^ (x <<  2)) & 0x09249249;
		return x;
	}

	/** Reverses MortonCode3. Compacts every 3rd bit to the right. */
	static FORCEINLINE uint32 ReverseMortonCode3( uint32 x )
	{
		x &= 0x09249249;
		x = (x ^ (x >>  2)) & 0x030c30c3;
		x = (x ^ (x >>  4)) & 0x0300f00f;
		x = (x ^ (x >>  8)) & 0xff0000ff;
		x = (x ^ (x >> 16)) & 0x000003ff;
		return x;
	}

	/**
	 * Returns value based on comparand. The main purpose of this function is to avoid
	 * branching based on floating point comparison which can be avoided via compiler
	 * intrinsics.
	 *
	 * Please note that we don't define what happens in the case of NaNs as there might
	 * be platform specific differences.
	 *
	 * @param	Comparand		Comparand the results are based on
	 * @param	ValueGEZero		Return value if Comparand >= 0
	 * @param	ValueLTZero		Return value if Comparand < 0
	 *
	 * @return	ValueGEZero if Comparand >= 0, ValueLTZero otherwise
	 */
	static CONSTEXPR FORCEINLINE float FloatSelect( float Comparand, float ValueGEZero, float ValueLTZero )
	{
		return Comparand >= 0.f ? ValueGEZero : ValueLTZero;
	}

	/**
	 * Returns value based on comparand. The main purpose of this function is to avoid
	 * branching based on floating point comparison which can be avoided via compiler
	 * intrinsics.
	 *
	 * Please note that we don't define what happens in the case of NaNs as there might
	 * be platform specific differences.
	 *
	 * @param	Comparand		Comparand the results are based on
	 * @param	ValueGEZero		Return value if Comparand >= 0
	 * @param	ValueLTZero		Return value if Comparand < 0
	 *
	 * @return	ValueGEZero if Comparand >= 0, ValueLTZero otherwise
	 */
	static CONSTEXPR FORCEINLINE double FloatSelect( double Comparand, double ValueGEZero, double ValueLTZero )
	{
		return Comparand >= 0.f ? ValueGEZero : ValueLTZero;
	}

	/** Computes absolute value in a generic way */
	template< class T > 
	static CONSTEXPR FORCEINLINE T Abs( const T A )
	{
		return (A>=(T)0) ? A : -A;
	}

	/** Returns 1, 0, or -1 depending on relation of T to 0 */
	template< class T > 
	static CONSTEXPR FORCEINLINE T Sign( const T A )
	{
		return (A > (T)0) ? (T)1 : ((A < (T)0) ? (T)-1 : (T)0);
	}

	/** Returns higher value in a generic way */
	template< class T > 
	static CONSTEXPR FORCEINLINE T Max( const T A, const T B )
	{
		return (A>=B) ? A : B;
	}

	/** Returns lower value in a generic way */
	template< class T > 
	static CONSTEXPR FORCEINLINE T Min( const T A, const T B )
	{
		return (A<=B) ? A : B;
	}

	/** Test some of the tricky functions above **/
	static void AutoTest();
};

/** Float specialization */
template<>
FORCEINLINE float FGenericPlatformMath::Abs( const float A )
{
	return fabsf( A );
}

