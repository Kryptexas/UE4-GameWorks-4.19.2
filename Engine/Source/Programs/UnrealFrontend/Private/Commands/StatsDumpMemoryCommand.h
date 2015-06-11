// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StatsData.h"
#include "StatsFile.h"

struct FAllocationInfo;

/** An example command loading a raw stats file and dumping memory usage. */
class FStatsMemoryDumpCommand
{
public:

	/** Executes the command. */
	static void Run()
	{
		FStatsMemoryDumpCommand Instance;
		Instance.InternalRun();
	}

protected:
	/** Executes the command. */
	void InternalRun();

	/** Basic memory profiling, only for debugging purpose. */
	void ProcessMemoryOperations( const TMap<int64, FStatPacketArray>& CombinedHistory );

	/** Generates a basic memory usage report and prints it to the log. */
	void GenerateMemoryUsageReport( const TMap<uint64, FAllocationInfo>& AllocationMap );

	/** Generates scoped allocation statistics. */
	void ProcessingScopedAllocations( const TMap<uint64, FAllocationInfo>& AllocationMap );

	/** Generates UObject allocation statistics. */
	void ProcessingUObjectAllocations( const TMap<uint64, FAllocationInfo>& AllocationMap );

	/** Stats thread state, mostly used to manage the stats metadata. */
	FStatsThreadState StatsThreadStats;
	FStatsReadStream Stream;

	/** All names that containa a path to an UObject. */
	TSet<FName> UObjectNames;

	/** Filepath to the raw stats file. */
	FString SourceFilepath;
};
