// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocBinned.cpp: Binned memory allocator
=============================================================================*/

#include "CorePrivate.h"

#if	!FORCE_ANSI_ALLOCATOR

#include "MallocBinned.h"
#include "MemoryMisc.h"

/** Malloc binned allocator specific stats. */
DEFINE_STAT(STAT_Binned_OsCurrent);
DEFINE_STAT(STAT_Binned_OsPeak);
DEFINE_STAT(STAT_Binned_WasteCurrent);
DEFINE_STAT(STAT_Binned_WastePeak);
DEFINE_STAT(STAT_Binned_UsedCurrent);
DEFINE_STAT(STAT_Binned_UsedPeak);
DEFINE_STAT(STAT_Binned_CurrentAllocs);
DEFINE_STAT(STAT_Binned_TotalAllocs);
DEFINE_STAT(STAT_Binned_SlackCurrent);

void FMallocBinned::GetAllocatorStats( FGenericMemoryStats& out_Stats )
{
#ifdef USE_INTERNAL_LOCKS
	FScopeLock ScopedLock(&AccessGuard);
#endif

	FMalloc::GetAllocatorStats( out_Stats );
	UpdateSlackStat();

#if	STATS
	// Malloc binned stats.
	out_Stats.Add( GET_STATFNAME( STAT_Binned_OsCurrent ), OsCurrent );
	out_Stats.Add( GET_STATFNAME( STAT_Binned_OsPeak ), OsPeak );
	out_Stats.Add( GET_STATFNAME( STAT_Binned_WasteCurrent ), WasteCurrent );
	out_Stats.Add( GET_STATFNAME( STAT_Binned_WastePeak ), WastePeak );
	out_Stats.Add( GET_STATFNAME( STAT_Binned_UsedCurrent ), UsedCurrent );
	out_Stats.Add( GET_STATFNAME( STAT_Binned_UsedPeak ), UsedPeak );
	out_Stats.Add( GET_STATFNAME( STAT_Binned_CurrentAllocs ), CurrentAllocs );
	out_Stats.Add( GET_STATFNAME( STAT_Binned_SlackCurrent ), SlackCurrent );
#endif // STATS
}

#endif // !FORCE_ANSI_ALLOCATOR