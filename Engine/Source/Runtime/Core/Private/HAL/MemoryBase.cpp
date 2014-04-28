// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CorePrivate.h"
#include "MemoryMisc.h"

/** Memory allocator base stats. */
DECLARE_DWORD_COUNTER_STAT(TEXT("Malloc calls"),			STAT_MallocCalls,STATGROUP_MemoryAllocator);
DECLARE_DWORD_COUNTER_STAT(TEXT("Free calls"),				STAT_FreeCalls,STATGROUP_MemoryAllocator);
DECLARE_DWORD_COUNTER_STAT(TEXT("Realloc calls"),			STAT_ReallocCalls,STATGROUP_MemoryAllocator);
DECLARE_DWORD_COUNTER_STAT(TEXT("Total Allocator calls"),	STAT_TotalAllocatorCalls,STATGROUP_MemoryAllocator);

uint32 FMalloc::TotalMallocCalls = 0;
uint32 FMalloc::TotalFreeCalls = 0;
uint32 FMalloc::TotalReallocCalls = 0;

struct FCurrentFrameCalls
{
	uint32 LastMallocCalls;
	uint32 LastReallocCalls;
	uint32 LastFreeCalls;
	
	uint32 MallocCalls;
	uint32 ReallocCalls;
	uint32 FreeCalls;
	uint32 AllocatorCalls;

	FCurrentFrameCalls()
		: LastMallocCalls(0)
		, LastReallocCalls(0)
		, LastFreeCalls(0)
		, MallocCalls(0)
		, ReallocCalls(0)
		, FreeCalls(0)
		, AllocatorCalls(0)
	{}

	void Update()
	{
		MallocCalls      = FMalloc::TotalMallocCalls - LastMallocCalls;
		ReallocCalls     = FMalloc::TotalReallocCalls - LastReallocCalls;
		FreeCalls        = FMalloc::TotalFreeCalls - LastFreeCalls;
		AllocatorCalls   = MallocCalls + ReallocCalls + FreeCalls;

		LastMallocCalls  = FMalloc::TotalMallocCalls;
		LastReallocCalls = FMalloc::TotalReallocCalls;
		LastFreeCalls    = FMalloc::TotalFreeCalls;
	}
};

static FCurrentFrameCalls& GetCurrentFrameCalls()
{
	static FCurrentFrameCalls CurrentFrameCalls;
	return CurrentFrameCalls;
}

void FMalloc::UpdateStats()
{
	GetCurrentFrameCalls().Update();

	SET_DWORD_STAT( STAT_MallocCalls, GetCurrentFrameCalls().MallocCalls );
	SET_DWORD_STAT( STAT_ReallocCalls, GetCurrentFrameCalls().ReallocCalls );
	SET_DWORD_STAT( STAT_FreeCalls, GetCurrentFrameCalls().FreeCalls );
	SET_DWORD_STAT( STAT_TotalAllocatorCalls, GetCurrentFrameCalls().AllocatorCalls );
}


void FMalloc::GetAllocatorStats( FGenericMemoryStats& out_Stats )
{
#if	STATS
	out_Stats.Add( GET_STATFNAME( STAT_MallocCalls ), GetCurrentFrameCalls().MallocCalls );
	out_Stats.Add( GET_STATFNAME( STAT_ReallocCalls ), GetCurrentFrameCalls().ReallocCalls );
	out_Stats.Add( GET_STATFNAME( STAT_FreeCalls ), GetCurrentFrameCalls().FreeCalls );
	out_Stats.Add( GET_STATFNAME( STAT_TotalAllocatorCalls ), GetCurrentFrameCalls().AllocatorCalls );
#endif // STATS
}