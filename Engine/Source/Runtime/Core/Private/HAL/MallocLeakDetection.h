// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocLeakDetection.h: Helper class to track memory allocations
=============================================================================*/

#pragma once

#include "Allocators/AnsiAllocator.h"

#ifndef MALLOC_LEAKDETECTION
	#define MALLOC_LEAKDETECTION 0
#endif

#if MALLOC_LEAKDETECTION

/**
 * Maintains a list of all pointers to currently allocated memory.
 */
class FMallocLeakDetection
{
	struct FCallstackTrack
	{
		FCallstackTrack()
		{
			FMemory::Memzero(this, sizeof(FCallstackTrack));
		}
		static const int32 Depth = 32;
		uint64 CallStack[Depth];
		uint32 FirstFame;
		uint32 LastFrame;
		uint32 Size;
		uint32 Count;

		bool operator==(const FCallstackTrack& Other) const
		{
			bool bEqual = true;
			for (int32 i = 0; i < Depth; ++i)
			{
				if (CallStack[i] != Other.CallStack[i])
				{
					bEqual = false;
					break;
				}
			}
			return bEqual;
		}

		bool operator!=(const FCallstackTrack& Other) const
		{
			return !(*this == Other);
		}

		uint32 GetHash() const 
		{
			return FCrc::MemCrc32(CallStack, sizeof(CallStack), 0);
		}
	};

	FMallocLeakDetection();
	~FMallocLeakDetection();

	void AddCallstack(FCallstackTrack& Callstack);
	void RemoveCallstack(FCallstackTrack& Callstack);

	/** List of all currently allocated pointers */
	TMap<void*, FCallstackTrack> OpenPointers;

	/** List of all unique callstacks with allocated memory */
	TMap<uint32, FCallstackTrack> UniqueCallstacks;

	/** Set of callstacks that are known to delete memory (never cleared) */
	TSet<uint32>	KnownDeleters;

	/** Contexts that are associated with allocations */
	TMap<void*, FString>		PointerContexts;

	/** Stack of contects */
	TArray<FString>				Contexts;

	bool	bRecursive;
	bool	bCaptureAllocs;
	int32	MinAllocationSize;
	bool	bDumpOustandingAllocs;	
		
	FCriticalSection AllocatedPointersCritical;	

public:	

	static FMallocLeakDetection& Get();

	void SetAllocationCollection(bool bEnabled, int32 Size=0);
	void DumpOpenCallstacks(uint32 FilterSize = 0, const TCHAR* FileName=nullptr);
	void ClearData();

	bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar);

	/** Handles new allocated pointer */
	void Malloc(void* Ptr, SIZE_T Size);

	/** Handles reallocation */
	void Realloc(void* OldPtr, void* NewPtr, SIZE_T Size);

	/** Removes allocated pointer from list */
	void Free(void* Ptr);	

	/** Push/Pop contects that will be associated with allocations. This is very rough.
	 not thread-safe (let alone TLS), and should probably be removed. However if you really
	 need to associate state with allocations it's very useful.... Cavet Emptor! */
	void PushContext(const FString& Context);
	void PopContext();
};

/**
 * A verifying proxy malloc that takes a malloc to be used and tracks unique callstacks with outstanding allocations
 * to help identify leaks.
 */
class FMallocLeakDetectionProxy : public FMalloc
{
private:
	/** Malloc we're based on, aka using under the hood */
	FMalloc* UsedMalloc;

	/* Verifier object */
	FMallocLeakDetection& Verify;

	FCriticalSection AllocatedPointersCritical;

public:
	explicit FMallocLeakDetectionProxy(FMalloc* InMalloc);	

	virtual void* Malloc(SIZE_T Size, uint32 Alignment) override
	{
		FScopeLock SafeLock(&AllocatedPointersCritical);
		void* Result = UsedMalloc->Malloc(Size, Alignment);
		Verify.Malloc(Result, Size);
		return Result;
	}

	virtual void* Realloc(void* Ptr, SIZE_T NewSize, uint32 Alignment) override
	{
		FScopeLock SafeLock(&AllocatedPointersCritical);
		void* Result = UsedMalloc->Realloc(Ptr, NewSize, Alignment);
		Verify.Realloc(Ptr, Result, NewSize);
		return Result;
	}

	virtual void Free(void* Ptr) override
	{
		if (Ptr)
		{
			FScopeLock SafeLock(&AllocatedPointersCritical);
			Verify.Free(Ptr);
			UsedMalloc->Free(Ptr);
		}
	}

	virtual void InitializeStatsMetadata() override
	{
		UsedMalloc->InitializeStatsMetadata();
	}

	virtual void GetAllocatorStats(FGenericMemoryStats& OutStats) override
	{
		UsedMalloc->GetAllocatorStats(OutStats);
	}

	virtual void DumpAllocatorStats(FOutputDevice& Ar) override
	{
		FScopeLock Lock(&AllocatedPointersCritical);
		//Verify.DumpOpenCallstacks(1024 * 1024);
		UsedMalloc->DumpAllocatorStats(Ar);
	}

	virtual bool ValidateHeap() override
	{
		return UsedMalloc->ValidateHeap();
	}

	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
		FScopeLock Lock(&AllocatedPointersCritical);
		if (Verify.Exec(InWorld, Cmd, Ar))
		{
			return true;
		}
		return UsedMalloc->Exec(InWorld, Cmd, Ar);
	}

	virtual bool GetAllocationSize(void* Original, SIZE_T& OutSize) override
	{
		return UsedMalloc->GetAllocationSize(Original, OutSize);
	}

	virtual SIZE_T QuantizeSize(SIZE_T Count, uint32 Alignment) override
	{
		return UsedMalloc->QuantizeSize(Count, Alignment);
	}

	virtual void Trim() override
	{
		return UsedMalloc->Trim();
	}
	virtual void SetupTLSCachesOnCurrentThread() override
	{
		return UsedMalloc->SetupTLSCachesOnCurrentThread();
	}
	virtual void ClearAndDisableTLSCachesOnCurrentThread() override
	{
		return UsedMalloc->ClearAndDisableTLSCachesOnCurrentThread();
	}


	virtual const TCHAR* GetDescriptiveName() override
	{ 
		return UsedMalloc->GetDescriptiveName();
	}
};

#endif // MALLOC_LEAKDETECTION
