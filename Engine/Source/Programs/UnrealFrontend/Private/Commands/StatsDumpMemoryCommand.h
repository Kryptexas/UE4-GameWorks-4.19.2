// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StatsData.h"
#include "StatsFile.h"

/** Simple allocation info. Assumes the sequence tag are unique and doesn't wrap. */
struct FAllocationInfo
{
	uint64 OldPtr;
	uint64 Ptr;
	int64 Size;
	FName EncodedCallstack;
	uint32 SequenceTag;
	EMemoryOperation Op;
	bool bHasBrokenCallstack;

	FAllocationInfo(
		uint64 InOldPtr,
		uint64 InPtr,
		int64 InSize,
		const TArray<FName>& InCallstack,
		uint32 InSequenceTag,
		EMemoryOperation InOp,
		bool bInHasBrokenCallstack
		);

	FAllocationInfo( const FAllocationInfo& Other );

	bool operator<(const FAllocationInfo& Other) const
	{
		return SequenceTag < Other.SequenceTag;
	}
};

// #YRX_Profiler: Add TArray<FName> etc 2015-06-25 
struct FSizeAndCount
{
	int64 Size;
	int64 Count;
	double SizeMB;

	FSizeAndCount()
		: Size( 0 )
		, Count( 0 )
		, SizeMB( 0 )
	{}

	FSizeAndCount& operator+=(const FSizeAndCount& Other)
	{
		if ((UPTRINT*)this != (UPTRINT*)&Other)
		{
			AddAllocation( Other.Size, Other.Count );
		}
		return *this;
	}

	FSizeAndCount& operator-=(const FSizeAndCount& Other)
	{
		if ((UPTRINT*)this != (UPTRINT*)&Other)
		{
			RemoveAllocation( Other.Size, Other.Count );
		}
		return *this;
	}

	void AddAllocation( int64 SizeToAdd, int64 CountToAdd = 1 )
	{
		Size += SizeToAdd;
		Count += CountToAdd;

		const double InvMB = 1.0 / 1024.0 / 1024.0;
		SizeMB = Size * InvMB;
	}

	void RemoveAllocation( int64 SizeToRemove, int64 CountToRemove )
	{
		AddAllocation( -SizeToRemove, -CountToRemove );
	}

	bool IsAlive()
	{
		return Size > 0;
	}
};

struct FNodeAllocationInfo
{
	/** Size as MBs for this node and children. */
	double SizeMB;

	/** Allocations count for this node and children. */
	int64 Count;

	/** Human readable callstack like 'GameThread -> Preinit -> LoadModule */
	FString HumanReadableCallstack;

	/** Encoded callstack like '45+656+6565'. */
	FName EncodedCallstack;

	/** Callstack indices, encoded callstack parsed into an array [45,656,6565] */
	TArray<FName> DecodedCallstack;

	/** Parent node. */
	FNodeAllocationInfo* Parent;

	/** Child nodes. */
	TMap<FName, FNodeAllocationInfo*> ChildNodes;

	/** Size for this node and children. */
	int64 Size;

	/** Node's depth. */
	int32 Depth;

	FNodeAllocationInfo()
		: SizeMB( 0.0 )
		, Count( 0 )
		, Parent( nullptr )
		, Size( 0 )
		, Depth( 0 )
	{}


	~FNodeAllocationInfo()
	{
		DeleteAllChildrenNodes();
	}

	void Accumulate( const FSizeAndCount& Other )
	{
		Size += Other.Size;
		Count += Other.Count;

		const double InvMB = 1.0 / 1024.0 / 1024.0;
		SizeMB = Size * InvMB;
	}

	void DeleteAllChildrenNodes()
	{
		for (auto& It : ChildNodes)
		{
			delete It.Value;
		}
		ChildNodes.Empty();
	}

	void SortBySize();
};

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
	void ProcessAndDumpScopedAllocations( const TMap<uint64, FAllocationInfo>& AllocationMap );

	/** Generates UObject allocation statistics. */
	void ProcessAndDumpUObjectAllocations( const TMap<uint64, FAllocationInfo>& AllocationMap );

	void DumpScopedAllocations( const TCHAR* Name, const TMap<FString, FSizeAndCount>& ScopedAllocations );

	/** Generates callstack based allocation map. */
	void GenerateScopedAllocations( const TMap<uint64, FAllocationInfo>& AllocationMap, TMap<FName, FSizeAndCount>& out_ScopedAllocations, uint64& TotalAllocatedMemory, uint64& NumAllocations );

	void GenerateScopedTreeAllocations( const TMap<FName, FSizeAndCount>& ScopedAllocations, FNodeAllocationInfo& out_Root );

	/** Prepares data for a snapshot. */
	void PrepareSnapshot( const FName SnapshotName, const TMap<uint64, FAllocationInfo>& AllocationMap );

	void CompareSnapshots_FName( const FName BeginSnaphotName, const FName EndSnaphotName, TMap<FName, FSizeAndCount>& out_Result );
	void CompareSnapshots_FString( const FName BeginSnaphotName, const FName EndSnaphotName, TMap<FString, FSizeAndCount>& out_Result );

	/** Stats thread state, mostly used to manage the stats metadata. */
	FStatsThreadState StatsThreadStats;
	FStatsReadStream Stream;

	/** All names that contains a path to an UObject. */
	TSet<FName> UObjectNames;

	/** The sequence tag mapping to the named markers. */
	TArray<TPair<uint32, FName>> Snapshots;

	/** The sequence tag mapping to the named markers that need to processed. */
	TArray<TPair<uint32, FName>> SnapshotsToBeProcessed;

	/** Snapshots with allocation map. */
	TMap<FName, TMap<uint64, FAllocationInfo> > SnapshotsWithAllocationMap;

	/** Snapshots with callstack based allocation map. */
	TMap<FName, TMap<FName, FSizeAndCount> > SnapshotsWithScopedAllocations;

	TMap<FName, TMap<FString, FSizeAndCount> > SnapshotsWithDecodedScopedAllocations;

	/** Filepath to the raw stats file. */
	FString SourceFilepath;
};
