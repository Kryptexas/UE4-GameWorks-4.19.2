// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/AITypes.h"

//----------------------------------------------------------------------//
// FAIResourceLock
//----------------------------------------------------------------------//
FAIResourceLock::FAIResourceLock()
{
	FMemory::MemZero(Locks);
}

void FAIResourceLock::ForceClearAllLocks()
{
	FMemory::MemZero(Locks);
}

FString FAIResourceLock::GetLockSourceName() const
{
	const static UEnum* SourceEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EAILockSource"));

	FString LockNames;

	for (int32 LockLevel = 0; LockLevel < int32(EAILockSource::MAX); ++LockLevel)
	{
		if (Locks[LockLevel])
		{
			LockNames += FString::Printf(TEXT("%s, "), *SourceEnum->GetEnumName(LockLevel));
		}
	}

	return LockNames;
}
