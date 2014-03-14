// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MallocProfilerEx.cpp: Extended memory profiling support.
=============================================================================*/

#include "EnginePrivate.h"

#include "MallocProfiler.h"
#include "MallocProfilerEx.h"

#if USE_MALLOC_PROFILER

// These functions are here because FMallocProfiler is in the Core
// project, and therefore can't access most of the classes needed by these functions.

/**
 * Constructor, initializing all member variables and potentially loading symbols.
 *
 * @param	InMalloc	The allocator wrapped by FMallocProfiler that will actually do the allocs/deallocs.
 */
FMallocProfilerEx::FMallocProfilerEx( FMalloc* InMalloc )
	: FMallocProfiler( InMalloc )
{}

/** 
 * Writes names of currently loaded levels. 
 * Only to be called from within the mutex / scope lock of the FMallocProfiler.
 */
void FMallocProfilerEx::WriteLoadedLevels( UWorld* InWorld )
{
	uint16 NumLoadedLevels = 0;
	int32 NumLoadedLevelsPosition = BufferedFileWriter.Tell();
	BufferedFileWriter << NumLoadedLevels;

	if (InWorld)
	{
		// Write the name of the map.
		const FString MapName = InWorld->GetCurrentLevel()->GetOutermost()->GetName();
		int32 MapNameIndex = GetNameTableIndex( MapName );
		NumLoadedLevels ++;

		BufferedFileWriter << MapNameIndex;

		// Write out all of the fully loaded levels.
		for (int32 LevelIndex = 0; LevelIndex < InWorld->StreamingLevels.Num(); LevelIndex++)
		{
			ULevelStreaming* LevelStreaming = InWorld->StreamingLevels[LevelIndex];
			if ((LevelStreaming != NULL)
				&& (LevelStreaming->PackageName != NAME_None)
				&& (LevelStreaming->PackageName != InWorld->GetOutermost()->GetFName())
				&& (LevelStreaming->GetLoadedLevel() != NULL))
			{
				NumLoadedLevels++;

				int32 LevelPackageIndex = GetNameTableIndex(LevelStreaming->PackageName);

				BufferedFileWriter << LevelPackageIndex;
			}
		}

		// Patch up the count.
		if (NumLoadedLevels > 0)
		{
			int32 EndPosition = BufferedFileWriter.Tell();
			BufferedFileWriter.Seek(NumLoadedLevelsPosition);
			BufferedFileWriter << NumLoadedLevels;
			BufferedFileWriter.Seek(EndPosition);
		}
	}
}

/** 
 * Gather texture memory stats. 
 */
void FMallocProfilerEx::GetTexturePoolSize( FMemoryAllocationStats_DEPRECATED& MemoryStats )
{
	FTextureMemoryStats Stats;

	if( GIsRHIInitialized )
	{
		RHIGetTextureMemoryStats(Stats);
	}

	MemoryStats.AllocatedTextureMemorySize = Stats.AllocatedMemorySize;
	MemoryStats.AvailableTextureMemorySize = Stats.AllocatedMemorySize;
}

#endif // USE_MALLOC_PROFILER