// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreTypes.h"
#include "Misc/AssertionMacros.h"
#include "HAL/CriticalSection.h"

//
// A scope lifetime controlled Read or Write lock of referenced mutex object
//
enum FRWScopeLockType
{
	SLT_ReadOnly = 0,
	SLT_Write,
};

/**
 * Implements a scope lock for RW locks.
 *	Notes:
 *		- PThreads and Win32 API's don't provide a mechanism for upgrading a ownership of a read lock to a write lock - to get round that this system unlocks then acquires a write lock so it can race between.
 */
class FRWScopeLock
{
public:
	FORCEINLINE FRWScopeLock(FRWLock& InLockObject,FRWScopeLockType InLockType)
	: LockObject(InLockObject)
	, LockType(InLockType)
	{
		if(LockType != SLT_ReadOnly)
		{
			LockObject.WriteLock();
		}
		else
		{
			LockObject.ReadLock();
		}
	}
	
	// NOTE: As the name suggests, this function should be used with caution. 
	// It releases the read lock _before_ acquiring a new write lock. This is not an atomic operation and the caller should 
	// not treat it as such. 
	// E.g. Pointers read from protected data structures prior to this call may be invalid after the function is called. 
	FORCEINLINE void ReleaseReadOnlyLockAndAcquireWriteLock_USE_WITH_CAUTION()
	{
		if(LockType == SLT_ReadOnly)
		{
			LockObject.ReadUnlock();
			LockObject.WriteLock();
			LockType = SLT_Write;
		}
	}
	
	FORCEINLINE ~FRWScopeLock()
	{
		if(LockType == SLT_ReadOnly)
		{
			LockObject.ReadUnlock();
		}
		else
		{
			LockObject.WriteUnlock();
		}
	}
	
private:
	FRWScopeLock();
	FRWScopeLock(const FRWScopeLock&);
	FRWScopeLock& operator=(const FRWScopeLock&);
	
private:
	FRWLock& LockObject;
	FRWScopeLockType LockType;
};
