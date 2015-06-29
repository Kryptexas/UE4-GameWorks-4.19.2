// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "StatsData.h"
#include "StatsFile.h"

/*-----------------------------------------------------------------------------
	Basic structures
-----------------------------------------------------------------------------*/

/** Simple allocation info. Assumes the sequence tag are unique and doesn't wrap. */
struct FAllocationInfo
{
	/** Old pointer, for Realloc. */
	uint64 OldPtr;

	/** Pointer, for Alloc, Realloc and Free. */
	uint64 Ptr;

	/** Size of the allocation. */
	int64 Size;
	
	/**
	 * Encoded callstack as FNames indices separated with +
	 * 123+45+56+76
	 */
	FName EncodedCallstack;
	
	/**
	 * Index of the memory operation. 
	 * Stats are handled with packets sent from different threads.
	 * This is used to sort the memory operations to be in the sequence order.
	 */
	uint32 SequenceTag;
	
	/** Memory operation, can be Alloc, Free or Realloc. */
	EMemoryOperation Op;
	
	/** If true, the callstack is broken. Missing Start/End scope. */
	bool bHasBrokenCallstack;

	/** Initialization constructor. */
	FAllocationInfo(
		uint64 InOldPtr,
		uint64 InPtr,
		int64 InSize,
		const TArray<FName>& InCallstack,
		uint32 InSequenceTag,
		EMemoryOperation InOp,
		bool bInHasBrokenCallstack
		);

	/** Copy constructor. */
	FAllocationInfo( const FAllocationInfo& Other );
};

/** Contains data about combined allocations for an unique scope. */
struct FCombinedAllocationInfo
{
	/** Size as MB. */
	double SizeMB;

	/** Allocations count. */
	int64 Count;

	/** Human readable callstack like 'GameThread -> Preinit -> LoadModule */
	FString HumanReadableCallstack;

	/** Encoded callstack like '45+656+6565'. */
	FName EncodedCallstack;

	/** Callstack indices, encoded callstack parsed into an array [45,656,6565] */
	TArray<FName> DecodedCallstack;

	/** Size for this node and children. */
	int64 Size;

	/** Default constructor. */
	FCombinedAllocationInfo()
		: SizeMB( 0 )
		, Count( 0 )
		, Size( 0 )
	{}

	/** Operator for adding another combined allocation. */
	FCombinedAllocationInfo& operator+=(const FCombinedAllocationInfo& Other)
	{
		if ((UPTRINT*)this != (UPTRINT*)&Other)
		{
			AddAllocation( Other.Size, Other.Count );
		}
		return *this;
	}

	/** Operator for subtracting another combined allocation. */
	FCombinedAllocationInfo& operator-=(const FCombinedAllocationInfo& Other)
	{
		if ((UPTRINT*)this != (UPTRINT*)&Other)
		{
			AddAllocation( -Other.Size, -Other.Count );
		}
		return *this;
	}

	/** Operator for adding a new allocation. */
	FCombinedAllocationInfo& operator+=(const FAllocationInfo& Other)
	{
		if ((UPTRINT*)this != (UPTRINT*)&Other)
		{
			AddAllocation( Other.Size, 1 );
		}
		return *this;
	}
	
	/**
	 * @return true, if the combined allocation is alive, the allocated data is larger than 0
	 */
	bool IsAlive()
	{
		return Size > 0;
	}

protected:
	/** Adds a new allocation. */
	void AddAllocation( int64 SizeToAdd, int64 CountToAdd = 1 )
	{
		Size += SizeToAdd;
		Count += CountToAdd;

		const double InvMB = 1.0 / 1024.0 / 1024.0;
		SizeMB = Size * InvMB;
	}
};

/** Contains data about node allocations, parent and children allocations. */
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

	/** Default constructor. */
	FNodeAllocationInfo()
		: SizeMB( 0.0 )
		, Count( 0 )
		, Parent( nullptr )
		, Size( 0 )
		, Depth( 0 )
	{}

	/** Destructor. */
	~FNodeAllocationInfo()
	{
		DeleteAllChildrenNodes();
	}

	/** Accumulates this node with the other combined allocation. */
	void Accumulate( const FCombinedAllocationInfo& Other )
	{
		Size += Other.Size;
		Count += Other.Count;

		const double InvMB = 1.0 / 1024.0 / 1024.0;
		SizeMB = Size * InvMB;
	}

	/** Recursively sorts all children by size. */
	void SortBySize();

	/** Prepares all callstack data for this node. */
	void PrepareCallstackData( const TArray<FName>& InDecodedCallstack );

protected:
	/** Recursively deletes all children. */
	void DeleteAllChildrenNodes()
	{
		for (auto& It : ChildNodes)
		{
			delete It.Value;
		}
		ChildNodes.Empty();
	}
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

	void DumpScopedAllocations( const TCHAR* Name, const TMap<FString, FCombinedAllocationInfo>& CombinedAllocations );

	/** Generates callstack based allocation map. */
	void GenerateScopedAllocations( const TMap<uint64, FAllocationInfo>& AllocationMap, TMap<FName, FCombinedAllocationInfo>& out_CombinedAllocations, uint64& TotalAllocatedMemory, uint64& NumAllocations );

	void GenerateScopedTreeAllocations( const TMap<FName, FCombinedAllocationInfo>& ScopedAllocations, FNodeAllocationInfo& out_Root );

	/** Prepares data for a snapshot. */
	void PrepareSnapshot( const FName SnapshotName, const TMap<uint64, FAllocationInfo>& AllocationMap );

	void CompareSnapshots_FName( const FName BeginSnaphotName, const FName EndSnaphotName, TMap<FName, FCombinedAllocationInfo>& out_Result );
	void CompareSnapshots_FString( const FName BeginSnaphotName, const FName EndSnaphotName, TMap<FString, FCombinedAllocationInfo>& out_Result );


	/**
	 * @return Platform's name based on the loaded ue4statsraw file
	 */
	const FString& GetPlatformName() const
	{
		return PlatformName;
	}

	/** Map from a thread id to the thread name. */
	TMap<uint32, FName> ThreadIdToName;

	/** All names that contains a path to an UObject. */
	TSet<FName> UObjectNames;

	/** The sequence tag mapping to the named markers. */
	TArray<TPair<uint32, FName>> Snapshots;

	/** The sequence tag mapping to the named markers that need to processed. */
	TArray<TPair<uint32, FName>> SnapshotsToBeProcessed;

	/** Snapshots with allocation map. */
	TMap<FName, TMap<uint64, FAllocationInfo> > SnapshotsWithAllocationMap;

	/** Snapshots with callstack based allocation map. */
	TMap<FName, TMap<FName, FCombinedAllocationInfo> > SnapshotsWithScopedAllocations;

	TMap<FName, TMap<FString, FCombinedAllocationInfo> > SnapshotsWithDecodedScopedAllocations;

	/** Filepath to the raw stats file. */
	FString SourceFilepath;

	/** Platform's name based on the loaded ue4statsraw file. */
	FString PlatformName;
};
