// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"

#if USE_MALLOC_PROFILER && WITH_ENGINE && IS_MONOLITHIC
	#include "Runtime/Engine/Public/MallocProfilerEx.h"
#endif

/*-----------------------------------------------------------------------------
	Memory functions.
-----------------------------------------------------------------------------*/

#include "MallocDebug.h"
#include "MallocProfiler.h"
#include "MallocThreadSafeProxy.h"
#include "MallocVerify.h"
#include "MallocLeakDetection.h"
#include "PlatformMallocCrash.h"


class FMallocPurgatoryProxy : public FMalloc
{
	/** Malloc we're based on, aka using under the hood							*/
	FMalloc*			UsedMalloc;
	enum
	{
		PURGATORY_STOMP_CHECKS_FRAMES = 4, // we want to allow several frames since it is theoretically possible for something to be blocked mid-allocation for many frames.
		PURGATORY_STOMP_MAX_PURGATORY_MEM = 100000000, // for loading and whatnot, we really can't keep this stuff forever
		PURGATORY_STOMP_CHECKS_CANARYBYTE = 0xdc,
	};
	uint32 LastCheckFrame; // unitialized; won't matter
	FThreadSafeCounter OutstandingSizeInKB;
	FThreadSafeCounter NextOversizeClear;
	TLockFreePointerListUnordered<void, PLATFORM_CACHE_LINE_SIZE> Purgatory[PURGATORY_STOMP_CHECKS_FRAMES];

public:
	/**
	* Constructor for thread safe proxy malloc that takes a malloc to be used and a
	* synchronization object used via FScopeLock as a parameter.
	*
	* @param	InMalloc					FMalloc that is going to be used for actual allocations
	*/
	FMallocPurgatoryProxy(FMalloc* InMalloc)
		: UsedMalloc(InMalloc)
	{}

	virtual void InitializeStatsMetadata() override
	{
		UsedMalloc->InitializeStatsMetadata();
	}

	/**
	* Malloc
	*/
	virtual void* Malloc(SIZE_T Size, uint32 Alignment) override
	{
		return UsedMalloc->Malloc(Size, Alignment);
	}

	/**
	* Realloc
	*/
	virtual void* Realloc(void* Ptr, SIZE_T NewSize, uint32 Alignment) override
	{
		return UsedMalloc->Realloc(Ptr, NewSize, Alignment);
	}

	/**
	* Free
	*/
	virtual void Free(void* Ptr) override
	{
		if (!Ptr)// || !GIsGameThreadIdInitialized)
		{
			UsedMalloc->Free(Ptr);
			return;
		}
		{
			SIZE_T Size = 0;
			verify(GetAllocationSize(Ptr, Size) && Size);
			FMemory::Memset(Ptr, uint8(PURGATORY_STOMP_CHECKS_CANARYBYTE), Size);
			Purgatory[GFrameNumber % PURGATORY_STOMP_CHECKS_FRAMES].Push(Ptr);
			OutstandingSizeInKB.Add((Size + 1023) / 1024);
		}
		FPlatformMisc::MemoryBarrier();
		uint32 LocalLastCheckFrame = LastCheckFrame;
		uint32 LocalGFrameNumber = GFrameNumber;

		bool bFlushAnyway = OutstandingSizeInKB.GetValue() > PURGATORY_STOMP_MAX_PURGATORY_MEM / 1024;

		if (bFlushAnyway || LocalLastCheckFrame != LocalGFrameNumber)
		{
			if (bFlushAnyway || FPlatformAtomics::InterlockedCompareExchange((volatile int32*)&LastCheckFrame, LocalGFrameNumber, LocalLastCheckFrame) == LocalLastCheckFrame)
			{
				int32 FrameToPop = ((bFlushAnyway ? NextOversizeClear.Increment() : LocalGFrameNumber) + PURGATORY_STOMP_CHECKS_FRAMES - 1) % PURGATORY_STOMP_CHECKS_FRAMES;
				//FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Pop %d at %d %d\r\n"), FrameToPop, LocalGFrameNumber, LocalGFrameNumber % PURGATORY_STOMP_CHECKS_FRAMES);
				while (true)
				{
					uint8* Pop = (uint8*)Purgatory[FrameToPop].Pop();
					if (!Pop)
					{
						break;
					}
					SIZE_T Size = 0;
					verify(GetAllocationSize(Pop, Size) && Size);
					for (SIZE_T At = 0; At < Size; At++)
					{
						if (Pop[At] != PURGATORY_STOMP_CHECKS_CANARYBYTE)
						{
							FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Freed memory at %p + %d == %x (should be %x)\r\n"), Pop, At, (int32)Pop[At], (int32)PURGATORY_STOMP_CHECKS_CANARYBYTE);
							UE_LOG(LogMemory, Fatal, TEXT("Freed memory at %p + %d == %x (should be %x)"), Pop, At, (int32)Pop[At], (int32)PURGATORY_STOMP_CHECKS_CANARYBYTE);
						}
					}
					UsedMalloc->Free(Pop);
					OutstandingSizeInKB.Subtract((Size + 1023) / 1024);
				}
			}
		}
	}

	/** Writes allocator stats from the last update into the specified destination. */
	virtual void GetAllocatorStats(FGenericMemoryStats& out_Stats) override
	{
		UsedMalloc->GetAllocatorStats(out_Stats);
	}

	/** Dumps allocator stats to an output device. */
	virtual void DumpAllocatorStats(class FOutputDevice& Ar) override
	{
		UsedMalloc->DumpAllocatorStats(Ar);
	}

	/**
	* Validates the allocator's heap
	*/
	virtual bool ValidateHeap() override
	{
		return(UsedMalloc->ValidateHeap());
	}

	virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
		return UsedMalloc->Exec(InWorld, Cmd, Ar);
	}

	/**
	* If possible determine the size of the memory allocated at the given address
	*
	* @param Original - Pointer to memory we are checking the size of
	* @param SizeOut - If possible, this value is set to the size of the passed in pointer
	* @return true if succeeded
	*/
	virtual bool GetAllocationSize(void *Original, SIZE_T &SizeOut) override
	{
		return UsedMalloc->GetAllocationSize(Original, SizeOut);
	}

	virtual const TCHAR* GetDescriptiveName() override
	{
		return UsedMalloc->GetDescriptiveName();
	}
};

void FMemory::EnablePurgatoryTests()
{
	static bool bOnce = false;
	if (bOnce)
	{
		UE_LOG(LogMemory, Error, TEXT("Purgatory proxy was already turned on."));
		return;
	}
	bOnce = true;
	while (true)
	{
		FMalloc* LocalGMalloc = GMalloc;
		FMalloc* Proxy = new FMallocPurgatoryProxy(LocalGMalloc);
		if (FPlatformAtomics::InterlockedCompareExchangePointer((void**)&GMalloc, Proxy, LocalGMalloc) == LocalGMalloc)
		{
			UE_LOG(LogConsoleResponse, Display, TEXT("Purgatory proxy is now on."));
			return;
		}
		delete Proxy;
	}
}

#if !UE_BUILD_SHIPPING
#include "TaskGraphInterfaces.h"
static void FMallocBinnedOverrunTest()
{
	const uint32 ArraySize = 64;
	uint8* Pointer = new uint8[ArraySize];
	delete[] Pointer;
	FFunctionGraphTask::CreateAndDispatchWhenReady(
		[Pointer, ArraySize]()
	{
		//FPlatformProcess::Sleep(.01f);
		Pointer[ArraySize / 2] = 0xcc;
	},
		TStatId()
		);
}

FAutoConsoleCommand FMallocBinnedTestCommand
(
TEXT("Memory.StaleTest"),
TEXT("Test for Memory.UsePurgatory. *** Will crash the game!"),
FConsoleCommandDelegate::CreateStatic(&FMallocBinnedOverrunTest)
);

FAutoConsoleCommand FMallocUsePurgatoryCommand
(
TEXT("Memory.UsePurgatory"),
TEXT("Uses the purgatory malloc proxy to check if things are writing to stale pointers."),
FConsoleCommandDelegate::CreateStatic(&FMemory::EnablePurgatoryTests)
);
#endif



/** Helper function called on first allocation to create and initialize GMalloc */
void GCreateMalloc()
{
	GMalloc = FPlatformMemory::BaseAllocator();
	// Setup malloc crash as soon as possible.
	FPlatformMallocCrash::Get( GMalloc );

// so now check to see if we are using a Mem Profiler which wraps the GMalloc
#if USE_MALLOC_PROFILER
	#if WITH_ENGINE && IS_MONOLITHIC
		GMallocProfiler = new FMallocProfilerEx( GMalloc );
	#else
		GMallocProfiler = new FMallocProfiler( GMalloc );
	#endif
	GMallocProfiler->BeginProfiling();
	GMalloc = GMallocProfiler;
#endif

	// if the allocator is already thread safe, there is no need for the thread safe proxy
	if (!GMalloc->IsInternallyThreadSafe())
	{
		GMalloc = new FMallocThreadSafeProxy( GMalloc );
	}

#if MALLOC_VERIFY
	// Add the verifier
	GMalloc = new FMallocVerifyProxy( GMalloc );
#endif

#if MALLOC_LEAKDETECTION
	GMalloc = new FMallocLeakDetectionProxy(GMalloc);
#endif
}


#if STATS
	#define MALLOC_GT_HOOKS 1
#else
	#define MALLOC_GT_HOOKS 0
#endif

#if MALLOC_GT_HOOKS

// This code is used to find memory allocations, you set up the lambda in the section of the code you are interested in and add a breakpoint to your lambda to see who is allocating memory

//An example:
//static TFunction<void(int32)> AnimHook(
//	[](int32 Index)
//{
//	TickAnimationMallocStats[Index]++;
//	if (++AllCount % 337 == 0)
//	{
//		BreakMe();
//	}
//}
//);
//extern CORE_API TFunction<void(int32)>* GGameThreadMallocHook;
//TGuardValue<TFunction<void(int32)>*> RestoreHook(GGameThreadMallocHook, &AnimHook);


CORE_API TFunction<void(int32)>* GGameThreadMallocHook = nullptr;

FORCEINLINE static void DoGamethreadHook(int32 Index)
{
	if (GIsRunning && GGameThreadMallocHook // otherwise our hook might not be initialized yet
		&& IsInGameThread() ) 
	{
		(*GGameThreadMallocHook)(Index);
	}
}
#else
FORCEINLINE static void DoGamethreadHook(int32 Index)
{
}
#endif

void* FMemory::Malloc( SIZE_T Count, uint32 Alignment ) 
{ 
	if( !GMalloc )
	{
		GCreateMalloc();
		CA_ASSUME( GMalloc != NULL );	// Don't want to assert, but suppress static analysis warnings about potentially NULL GMalloc
	}
	DoGamethreadHook(0);
	return GMalloc->Malloc( Count, Alignment );
}

void* FMemory::Realloc( void* Original, SIZE_T Count, uint32 Alignment ) 
{ 
	if( !GMalloc )
	{
		GCreateMalloc();	
		CA_ASSUME( GMalloc != NULL );	// Don't want to assert, but suppress static analysis warnings about potentially NULL GMalloc
	}
	DoGamethreadHook(1);
	return GMalloc->Realloc( Original, Count, Alignment );
}	

void FMemory::Free( void* Original )
{
	if( !GMalloc )
	{
		GCreateMalloc();
		CA_ASSUME( GMalloc != NULL );	// Don't want to assert, but suppress static analysis warnings about potentially NULL GMalloc
	}
	DoGamethreadHook(2);
	return GMalloc->Free( Original );
}

void* FMemory::GPUMalloc(SIZE_T Count, uint32 Alignment /* = DEFAULT_ALIGNMENT */)
{
	return FPlatformMemory::GPUMalloc(Count, Alignment);
}

void* FMemory::GPURealloc(void* Original, SIZE_T Count, uint32 Alignment /* = DEFAULT_ALIGNMENT */)
{
	return FPlatformMemory::GPURealloc(Original, Count, Alignment);
}

void FMemory::GPUFree(void* Original)
{
	return FPlatformMemory::GPUFree(Original);
}

SIZE_T FMemory::GetAllocSize( void* Original )
{
	if( !GMalloc )
	{
		GCreateMalloc();	
		CA_ASSUME( GMalloc != NULL );	// Don't want to assert, but suppress static analysis warnings about potentially NULL GMalloc
	}
	
	SIZE_T Size = 0;
	return GMalloc->GetAllocationSize( Original, Size ) ? Size : 0;
}

void FMemory::TestMemory()
{
#if !UE_BUILD_SHIPPING
	if( !GMalloc )
	{
		GCreateMalloc();	
		CA_ASSUME( GMalloc != NULL );	// Don't want to assert, but suppress static analysis warnings about potentially NULL GMalloc
	}

	// track the pointers to free next call to the function
	static TArray<void*> LeakedPointers;
	TArray<void*> SavedLeakedPointers = LeakedPointers;


	// note that at the worst case, there will be NumFreedAllocations + 2 * NumLeakedAllocations allocations alive
	static const int NumFreedAllocations = 1000;
	static const int NumLeakedAllocations = 100;
	static const int MaxAllocationSize = 128 * 1024;

	TArray<void*> FreedPointers;
	// allocate pointers that will be freed later
	for (int32 Index = 0; Index < NumFreedAllocations; Index++)
	{
		FreedPointers.Add(FMemory::Malloc(FMath::RandHelper(MaxAllocationSize)));
	}


	// allocate pointers that will be leaked until the next call
	LeakedPointers.Empty();
	for (int32 Index = 0; Index < NumLeakedAllocations; Index++)
	{
		LeakedPointers.Add(FMemory::Malloc(FMath::RandHelper(MaxAllocationSize)));
	}

	// free the leaked pointers from _last_ call to this function
	for (int32 Index = 0; Index < SavedLeakedPointers.Num(); Index++)
	{
		FMemory::Free(SavedLeakedPointers[Index]);
	}

	// free the non-leaked pointers from this call to this function
	for (int32 Index = 0; Index < FreedPointers.Num(); Index++)
	{
		FMemory::Free(FreedPointers[Index]);
	}
#endif
}
