// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnrealTypeHash.h: Unreal type hash functions.
=============================================================================*/

#pragma once

/**
 * A hashing function that works well for pointers.
 */
inline uint32 PointerHash(const void* Key,uint32 C = 0)
{
#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}

	uint32 A;
	uint32 B;
	A = B = 0x9e3779b9;
	// Avoid LHS stalls on PS3 and Xbox 360
#if PLATFORM_64BITS
	// Ignoring the lower 4 bits since they are likely zero anyway.
	// Higher bits are more significant in 64 bit builds.
	A += (reinterpret_cast<UPTRINT>(Key) >> 4);
#else
	A += reinterpret_cast<UPTRINT>(Key);
#endif
	mix(A,B,C);
	return C;

#undef mix
}

//
// Hash functions for common types.
//

inline uint32 GetTypeHash( const uint8 A )
{
	return A;
}
inline uint32 GetTypeHash( const int8 A )
{
	return A;
}
inline uint32 GetTypeHash( const uint16 A )
{
	return A;
}
inline uint32 GetTypeHash( const int16 A )
{
	return A;
}
inline uint32 GetTypeHash( const int32 A )
{
	return A;
}
inline uint32 GetTypeHash( const uint32 A )
{
	return A;
}
inline uint32 GetTypeHash( const uint64 A )
{
	return (uint32)A+((uint32)(A>>32) * 23);
}
inline uint32 GetTypeHash( const int64 A )
{
	return (uint32)A+((uint32)(A>>32) * 23);
}
inline uint32 GetTypeHash( const float& Value )
{
	return *(uint32*)&Value;
}
inline uint32 GetTypeHash( const TCHAR* S )
{
	return FCrc::Strihash_DEPRECATED(S);
}

inline uint32 GetTypeHash( const void* A )
{
	return PointerHash(A);
}

inline uint32 GetTypeHash( void* A )
{
	return PointerHash(A);
}
