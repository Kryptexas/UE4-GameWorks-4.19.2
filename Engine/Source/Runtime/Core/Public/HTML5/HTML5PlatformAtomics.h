// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


/*=============================================================================================
	HTML5PlatformAtomics.h: HTML5 platform Atomics functions
==============================================================================================*/

#pragma once
#include "GenericPlatform/GenericPlatformAtomics.h"

/**
 * HTML5 implementation of the Atomics OS functions (no actual atomics, as HTML5 has no threads)
 * @todo html5 threads: support this if we get threads
 */
struct CORE_API FHTML5PlatformAtomics	: public FGenericPlatformAtomics
{
	static FORCEINLINE int8 InterlockedIncrement( volatile int8* Value )
	{
		*Value += 1;
		return *Value;
	}

	static FORCEINLINE int16 InterlockedIncrement( volatile int16* Value )
	{
		*Value += 1;
		return *Value;
	}

	static FORCEINLINE int32 InterlockedIncrement( volatile int32* Value )
	{
		*Value += 1;
		return *Value;
	}

	static FORCEINLINE int64 InterlockedIncrement( volatile int64* Value )
	{
		*Value += 1;
		return *Value;
	}

	static FORCEINLINE int8 InterlockedDecrement( volatile int8* Value )
	{
		*Value -= 1;
		return *Value;
	}

	static FORCEINLINE int16 InterlockedDecrement( volatile int16* Value )
	{
		*Value -= 1;
		return *Value;
	}

	static FORCEINLINE int32 InterlockedDecrement( volatile int32* Value )
	{
		*Value -= 1;
		return *Value;
	}

	static FORCEINLINE int64 InterlockedDecrement( volatile int64* Value )
	{
		*Value -=1;
		return *Value;
	}

	static FORCEINLINE int8 InterlockedAdd( volatile int8* Value, int8 Amount )
	{
		int8 Result = *Value;
		*Value += Amount;
		return Result;
	}

	static FORCEINLINE int16 InterlockedAdd( volatile int16* Value, int16 Amount )
	{
		int16 Result = *Value;
		*Value += Amount;
		return Result;
	}

	static FORCEINLINE int32 InterlockedAdd( volatile int32* Value, int32 Amount )
	{
		int32 Result = *Value;
		*Value += Amount;
		return Result;
	}

	static FORCEINLINE int64 InterlockedAdd( volatile int64* Value, int64 Amount )
	{
		int64 Result = *Value;
		*Value += Amount;
		return Result;
	}

	static FORCEINLINE int8 InterlockedExchange( volatile int8* Value, int8 Exchange )
	{
		int8 Result = *Value;
		*Value = Exchange;
		return Result;
	}

	static FORCEINLINE int16 InterlockedExchange( volatile int16* Value, int16 Exchange )
	{
		int16 Result = *Value;
		*Value = Exchange;
		return Result;
	}

	static FORCEINLINE int32 InterlockedExchange( volatile int32* Value, int32 Exchange )
	{
		int32 Result = *Value;
		*Value = Exchange;
		return Result;
	}

	static FORCEINLINE int64 InterlockedExchange( volatile int64* Value, int64 Exchange )
	{
		int64 Result = *Value;
		*Value = Exchange;
		return Result;
	}

	static FORCEINLINE void* InterlockedExchangePtr( void** Dest, void* Exchange )
	{
		void* Result = *Dest;
		*Dest = Exchange;
		return Result;
	}

	static FORCEINLINE int8 InterlockedCompareExchange( volatile int8* Dest, int8 Exchange, int8 Comperand )
	{
		int8 Result = *Dest;
		if (Result == Comperand)
		{
			*Dest = Exchange;
		}
		return Result;
	}

	static FORCEINLINE int16 InterlockedCompareExchange( volatile int16* Dest, int16 Exchange, int16 Comperand )
	{
		int16 Result = *Dest;
		if (Result == Comperand)
		{
			*Dest = Exchange;
		}
		return Result;
	}

	static FORCEINLINE int32 InterlockedCompareExchange( volatile int32* Dest, int32 Exchange, int32 Comperand )
	{
		int32 Result = *Dest;
		if (Result == Comperand)
		{
			*Dest = Exchange;
		}
		return Result;
	}

	static FORCEINLINE int64 InterlockedCompareExchange( volatile int64* Dest, int64 Exchange, int64 Comperand )
	{
		int64 Result = *Dest;
		if (Result == Comperand)
		{
			*Dest = Exchange;
		}
		return Result;
	}

	static FORCEINLINE int8 AtomicRead(volatile const int8* Src)
	{
		return *Src;
	}

	static FORCEINLINE int16 AtomicRead(volatile const int16* Src)
	{
		return *Src;
	}

	static FORCEINLINE int32 AtomicRead(volatile const int32* Src)
	{
		return *Src;
	}

	static FORCEINLINE int64 AtomicRead(volatile const int64* Src)
	{
		return *Src;
	}

	static FORCEINLINE int8 AtomicRead_Relaxed(volatile const int8* Src)
	{
		return *Src;
	}

	static FORCEINLINE int16 AtomicRead_Relaxed(volatile const int16* Src)
	{
		return *Src;
	}

	static FORCEINLINE int32 AtomicRead_Relaxed(volatile const int32* Src)
	{
		return *Src;
	}

	static FORCEINLINE int64 AtomicRead_Relaxed(volatile const int64* Src)
	{
		return *Src;
	}

	static FORCEINLINE void AtomicStore(volatile int8* Src, int8 Val)
	{
		*Src = Val;
	}

	static FORCEINLINE void AtomicStore(volatile int16* Src, int16 Val)
	{
		*Src = Val;
	}

	static FORCEINLINE void AtomicStore(volatile int32* Src, int32 Val)
	{
		*Src = Val;
	}

	static FORCEINLINE void AtomicStore(volatile int64* Src, int64 Val)
	{
		*Src = Val;
	}

	static FORCEINLINE void AtomicStore_Relaxed(volatile int8* Src, int8 Val)
	{
		*Src = Val;
	}

	static FORCEINLINE void AtomicStore_Relaxed(volatile int16* Src, int16 Val)
	{
		*Src = Val;
	}

	static FORCEINLINE void AtomicStore_Relaxed(volatile int32* Src, int32 Val)
	{
		*Src = Val;
	}

	static FORCEINLINE void AtomicStore_Relaxed(volatile int64* Src, int64 Val)
	{
		*Src = Val;
	}

	DEPRECATED(4.19, "AtomicRead64 has been deprecated, please use AtomicRead's overload instead")
	static FORCEINLINE int64 AtomicRead64(volatile const int64* Src)
	{
		return *Src;
	}

	static FORCEINLINE void* InterlockedCompareExchangePointer( void** Dest, void* Exchange, void* Comperand )
	{
		void* Result = *Dest;
		if (Result == Comperand)
		{
			*Dest = Exchange;
		}
		return Result;
	}
};


typedef FHTML5PlatformAtomics FPlatformAtomics;