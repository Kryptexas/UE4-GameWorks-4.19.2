// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealFrontendPrivatePCH.h"
#include "Profiler.h"
#include "StatsDumpMemoryCommand.h"
#include "DiagnosticTable.h"

/*-----------------------------------------------------------------------------
	Sort helpers
-----------------------------------------------------------------------------*/

/** Sorts allocations by size. */
struct FAllocationInfoSequenceTagLess
{
	FORCEINLINE bool operator()( const FAllocationInfo& A, const FAllocationInfo& B ) const
	{
		return A.SequenceTag < B.SequenceTag;
	}
};

/** Sorts allocations by size. */
struct FAllocationInfoSizeGreater
{
	FORCEINLINE bool operator()( const FAllocationInfo& A, const FAllocationInfo& B ) const
	{
		return B.Size < A.Size;
	}
};

/** Sorts combined allocations by size. */
struct FCombinedAllocationInfoSizeGreater
{
	FORCEINLINE bool operator()( const FCombinedAllocationInfo& A, const FCombinedAllocationInfo& B ) const
	{
		return B.Size < A.Size;
	}
};


/** Sorts node allocations by size. */
struct FNodeAllocationInfoSizeGreater
{
	FORCEINLINE bool operator()( const FNodeAllocationInfo& A, const FNodeAllocationInfo& B ) const
	{
		return B.Size < A.Size;
	}
};

/*-----------------------------------------------------------------------------
	Callstack decoding/encoding
-----------------------------------------------------------------------------*/

/** Helper struct used to manipulate stats based callstacks. */
struct FStatsCallstack
{
	/** Separator. */
	static const TCHAR* CallstackSeparator;

	/** Encodes decoded callstack a string, to be like '45+656+6565'. */
	static FString Encode( const TArray<FName>& Callstack )
	{
		FString Result;
		for (const auto& Name : Callstack)
		{
			Result += TTypeToString<int32>::ToString( (int32)Name.GetComparisonIndex() );
			Result += CallstackSeparator;
		}
		return Result;
	}

	/** Decodes encoded callstack to an array of FNames. */
	static void DecodeToNames( const FName& EncodedCallstack, TArray<FName>& out_DecodedCallstack )
	{
		TArray<FString> DecodedCallstack;
		DecodeToStrings( EncodedCallstack, DecodedCallstack );

		// Convert back to FNames
		for (const auto& It : DecodedCallstack)
		{
			NAME_INDEX NameIndex = 0;
			TTypeFromString<NAME_INDEX>::FromString( NameIndex, *It );
			const FName LongName = FName( NameIndex, NameIndex, 0 );

			out_DecodedCallstack.Add( LongName );
		}
	}

	/** Converts the encoded callstack into human readable callstack. */
	static FString GetHumanReadable( const FName& EncodedCallstack )
	{
		TArray<FName> DecodedCallstack;
		DecodeToNames( EncodedCallstack, DecodedCallstack );
		const FString Result = GetHumanReadable( DecodedCallstack );
		return Result;
	}

	/** Converts the encoded callstack into human readable callstack. */
	static FString GetHumanReadable( const TArray<FName>& DecodedCallstack )
	{
		FString Result;

		const int32 NumEntries = DecodedCallstack.Num();
		//for (int32 Index = DecodedCallstack.Num() - 1; Index >= 0; --Index)
		for (int32 Index = 0; Index < NumEntries; ++Index)
		{
			const FName LongName = DecodedCallstack[Index];
			const FString ShortName = FStatNameAndInfo::GetShortNameFrom( LongName ).ToString();
			//const FString Group = FStatNameAndInfo::GetGroupNameFrom( LongName ).ToString();
			FString Desc = FStatNameAndInfo::GetDescriptionFrom( LongName );
			Desc.Trim();

			if (Desc.Len() == 0)
			{
				Result += ShortName;
			}
			else
			{
				Result += Desc;
			}

			if (Index != NumEntries - 1)
			{
				Result += TEXT( " -> " );
			}
		}

		Result.ReplaceInline( TEXT( "STAT_" ), TEXT( "" ), ESearchCase::CaseSensitive );
		return Result;
	}

protected:
	/** Decodes encoded callstack to an array of strings. Where each string is the index of the FName. */
	static void DecodeToStrings( const FName& EncodedCallstack, TArray<FString>& out_DecodedCallstack )
	{
		EncodedCallstack.ToString().ParseIntoArray( out_DecodedCallstack, CallstackSeparator, true );
	}
};

const TCHAR* FStatsCallstack::CallstackSeparator = TEXT( "+" );

/*-----------------------------------------------------------------------------
	Allocation info
-----------------------------------------------------------------------------*/

FAllocationInfo::FAllocationInfo( uint64 InOldPtr, uint64 InPtr, int64 InSize, const TArray<FName>& InCallstack, uint32 InSequenceTag, EMemoryOperation InOp, bool bInHasBrokenCallstack ) 
	: OldPtr( InOldPtr )
	, Ptr( InPtr )
	, Size( InSize )
	, EncodedCallstack( *FStatsCallstack::Encode( InCallstack ) )
	, SequenceTag( InSequenceTag )
	, Op( InOp )
	, bHasBrokenCallstack( bInHasBrokenCallstack )
{

}

FAllocationInfo::FAllocationInfo( const FAllocationInfo& Other ) 
	: OldPtr( Other.OldPtr )
	, Ptr( Other.Ptr )
	, Size( Other.Size )
	, EncodedCallstack( Other.EncodedCallstack )
	, SequenceTag( Other.SequenceTag )
	, Op( Other.Op )
	, bHasBrokenCallstack( Other.bHasBrokenCallstack )
{

}

/*-----------------------------------------------------------------------------
	FNodeAllocationInfo
-----------------------------------------------------------------------------*/

void FNodeAllocationInfo::SortBySize()
{
	ChildNodes.ValueSort( FNodeAllocationInfoSizeGreater() );
	for (auto& It : ChildNodes)
	{
		It.Value->SortBySize();
	}
}


void FNodeAllocationInfo::PrepareCallstackData( const TArray<FName>& InDecodedCallstack )
{
	DecodedCallstack = InDecodedCallstack;
	EncodedCallstack = *FStatsCallstack::Encode( DecodedCallstack );
	HumanReadableCallstack = FStatsCallstack::GetHumanReadable( DecodedCallstack );
}

/*-----------------------------------------------------------------------------
	Stats stack helpers
-----------------------------------------------------------------------------*/

/** Holds stats stack state, used to preserve continuity when the game frame has changed. */
struct FStackState
{
	/** Default constructor. */
	FStackState()
		: bIsBrokenCallstack( false )
	{}

	/** Call stack. */
	TArray<FName> Stack;

	/** Current function name. */
	FName Current;

	/** Whether this callstack is marked as broken due to mismatched start and end scope cycles. */
	bool bIsBrokenCallstack;
};



/*-----------------------------------------------------------------------------
	FStatsMemoryDumpCommand
-----------------------------------------------------------------------------*/

static const double NumSecondsBetweenLogs = 5.0;

void FStatsMemoryDumpCommand::InternalRun()
{
	FString SourceFilepath;
	FParse::Value( FCommandLine::Get(), TEXT( "-INFILE=" ), SourceFilepath );

	{
		const FName NAME_ProfilerModule = TEXT( "Profiler" );
		IProfilerModule& ProfilerModule = FModuleManager::LoadModuleChecked<IProfilerModule>( NAME_ProfilerModule );
		ProfilerModule.StatsMemoryDumpCommand( *SourceFilepath );
		//FModuleManager::Get().UnloadModule( NAME_ProfilerModule );
	}

	{
		const FName NAME_ProfilerModule = TEXT( "Profiler" );
		IProfilerModule& ProfilerModule = FModuleManager::LoadModuleChecked<IProfilerModule>( NAME_ProfilerModule );
		ProfilerModule.StatsMemoryDumpCommand( *SourceFilepath );
		//FModuleManager::Get().UnloadModule( NAME_ProfilerModule );
	}


	FStatsReadFile* NewReader = FStatsReadFile::CreateReaderForRawStats( *SourceFilepath, FStatsReadFile::FOnNewCombinedHistory::CreateRaw( this, &FStatsMemoryDumpCommand::ProcessMemoryOperations ) );
	if (NewReader)
	{
		PlatformName = NewReader->Header.PlatformName;
		NewReader->ReadAndProcessAsynchronously();

		while( NewReader->IsBusy() )
		{
			FPlatformProcess::Sleep( 1.0f );

			UE_LOG( LogStats, Log, TEXT( "Async: Stage: %s / %3i%%" ), *NewReader->GetProcessingStageAsString(), NewReader->GetStageProgress() );
		}
		//NewReader->RequestStop();

		// Delete reader, we don't need the data anymore.
		delete NewReader;

		// Frame-240 Frame-120 Frame-060
		TMap<FString, FCombinedAllocationInfo> FrameBegin_Exit;
		CompareSnapshots_FString( TEXT( "BeginSnapshot" ), TEXT( "EngineLoop.Exit" ), FrameBegin_Exit );
		DumpScopedAllocations( TEXT( "Begin_Exit" ), FrameBegin_Exit );

#if	UE_BUILD_DEBUG
		TMap<FString, FCombinedAllocationInfo> Frame060_120;
		CompareSnapshots_FString( TEXT( "Frame-060" ), TEXT( "Frame-120" ), Frame060_120 );
		DumpScopedAllocations( TEXT( "Frame060_120" ), Frame060_120 );

		TMap<FString, FCombinedAllocationInfo> Frame060_240;
		CompareSnapshots_FString( TEXT( "Frame-060" ), TEXT( "Frame-240" ), Frame060_240 );
		DumpScopedAllocations( TEXT( "Frame060_240" ), Frame060_240 );

		// Generate scoped tree view.
		{
			TMap<FName, FCombinedAllocationInfo> FrameBegin_Exit_FName;
			CompareSnapshots_FName( TEXT( "BeginSnapshot" ), TEXT( "EngineLoop.Exit" ), FrameBegin_Exit_FName );

			FNodeAllocationInfo Root;
			Root.EncodedCallstack = TEXT( "ThreadRoot" );
			Root.HumanReadableCallstack = TEXT( "ThreadRoot" );
			GenerateScopedTreeAllocations( FrameBegin_Exit_FName, Root );
		}


		{
			TMap<FName, FCombinedAllocationInfo> Frame060_240_FName;
			CompareSnapshots_FName( TEXT( "Frame-060" ), TEXT( "Frame-240" ), Frame060_240_FName );

			FNodeAllocationInfo Root;
			Root.EncodedCallstack = TEXT( "ThreadRoot" );
			Root.HumanReadableCallstack = TEXT( "ThreadRoot" );
			GenerateScopedTreeAllocations( Frame060_240_FName, Root );
		}
#endif // UE_BUILD_DEBUG
	}
}


void FStatsMemoryDumpCommand::ProcessMemoryOperations( FStatsReadFile* InFile )
{
	const FStatsReadFile& File = *InFile;
	const TMap<int64, FStatPacketArray>& CombinedHistory = File.CombinedHistory;

	// This is only example code, no fully implemented, may sometimes crash.
	// This code is not optimized. 
	double PreviousSeconds = FPlatformTime::Seconds();
	int32 CurrentStatMessageIndex = 0;
	uint64 NumMemoryOperations = 0;

	// Generate frames
	TArray<int64> Frames;
	CombinedHistory.GenerateKeyArray( Frames );
	Frames.Sort();

	// Raw stats callstack for this stat packet array.
	TMap<FName, FStackState> StackStates;

	// All allocation ordered by the sequence tag.
	// There is an assumption that the sequence tag will not turn-around.
	//TMap<uint32, FAllocationInfo> SequenceAllocationMap;
	TArray<FAllocationInfo> SequenceAllocationArray;

	// Pass 1.
	// Read all stats messages, parse all memory operations and decode callstacks.
	const int64 FirstFrame = 0;
	PreviousSeconds -= NumSecondsBetweenLogs;

	// Begin marker.
	uint32 LastSequenceTagForNamedMarker = 0;
	Snapshots.Add( TPairInitializer<uint32, FName>( LastSequenceTagForNamedMarker, TEXT( "BeginSnapshot" ) ) );

	const int32 OnerPercent = FMath::Max( int32(InFile->FileInfo.TotalStatMessagesNum / 100), 65536 );

	// #YRX_Stats: 2015-07-10 This still should be generic
	for( int32 FrameIndex = 0; FrameIndex < Frames.Num(); ++FrameIndex )
	{
		const int64 TargetFrame = Frames[FrameIndex];
		const int64 Diff = TargetFrame - FirstFrame;
		const FStatPacketArray& Frame = CombinedHistory.FindChecked( TargetFrame );

		bool bAtLeastOnePacket = false;
		for( int32 PacketIndex = 0; PacketIndex < Frame.Packets.Num(); PacketIndex++ )
		{
			const FStatPacket& StatPacket = *Frame.Packets[PacketIndex];
			const FName& ThreadFName = File.State.Threads.FindChecked( StatPacket.ThreadId );

			FStackState* StackState = StackStates.Find( ThreadFName );
			if( !StackState )
			{
				StackState = &StackStates.Add( ThreadFName );
				StackState->Stack.Add( ThreadFName );
				StackState->Current = ThreadFName;
			}

			const FStatMessagesArray& Data = StatPacket.StatMessages;

			const int32 NumStatMessages = Data.Num();
			for( int32 Index = 0; Index < NumStatMessages; Index++ )
			{
				CurrentStatMessageIndex++;

				if (CurrentStatMessageIndex % OnerPercent == 0)
				{
					const double CurrentSeconds = FPlatformTime::Seconds();
					if( CurrentSeconds > PreviousSeconds + NumSecondsBetweenLogs )
					{
						const int32 PercentagePos = int32( 100.0*CurrentStatMessageIndex / InFile->FileInfo.TotalStatMessagesNum );
						InFile->StageProgress.Set( PercentagePos );
						UE_LOG( LogStats, Verbose, TEXT( "Processing %3i%% (%10i/%10i) stat messages [Frame: %3i, Packet: %2i]" ), PercentagePos, CurrentStatMessageIndex, InFile->FileInfo.TotalStatMessagesNum, FrameIndex, PacketIndex );
						PreviousSeconds = CurrentSeconds;

						// Abandon support, simply return for now.
						if (InFile->bShouldStopProcessing == true)
						{
							InFile->SetProcessingStage( EStatsProcessingStage::SPS_Stopped );
							return;
						}
					}
				}

				const FStatMessage& Item = Data[Index];
				const EStatOperation::Type Op = Item.NameAndInfo.GetField<EStatOperation>();
				const FName RawName = Item.NameAndInfo.GetRawName();

				if (Op == EStatOperation::CycleScopeStart || Op == EStatOperation::CycleScopeEnd || Op == EStatOperation::Memory || Op == EStatOperation::SpecialMessageMarker)
				{
					if( Op == EStatOperation::CycleScopeStart )
					{
						StackState->Stack.Add( RawName );
						StackState->Current = RawName;
					}
					else if( Op == EStatOperation::Memory )
					{
						// Experimental code used only to test the implementation.
						// First memory operation is Alloc or Free
						const uint64 EncodedPtr = Item.GetValue_Ptr();
						const EMemoryOperation MemOp = EMemoryOperation( EncodedPtr & (uint64)EMemoryOperation::Mask );
						const uint64 Ptr = EncodedPtr & ~(uint64)EMemoryOperation::Mask;
						if (MemOp == EMemoryOperation::Alloc)
						{
							NumMemoryOperations++;
							// @see FStatsMallocProfilerProxy::TrackAlloc
							// After AllocPtr message there is always alloc size message and the sequence tag.
							Index++; CurrentStatMessageIndex++;
							const FStatMessage& AllocSizeMessage = Data[Index];
							const int64 AllocSize = AllocSizeMessage.GetValue_int64();

							// Read OperationSequenceTag.
							Index++; CurrentStatMessageIndex++;
							const FStatMessage& SequenceTagMessage = Data[Index];
							const uint32 SequenceTag = SequenceTagMessage.GetValue_int64();

							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_AllocPtr ), (uint64)(UPTRINT)Ptr | (uint64)EMemoryOperation::Alloc );
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_AllocSize ), Size );
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_OperationSequenceTag ), (int64)SequenceTag );

							// Add a new allocation.
							SequenceAllocationArray.Add(
								FAllocationInfo(
								0,
								Ptr,
								AllocSize,
								StackState->Stack,
								SequenceTag,
								EMemoryOperation::Alloc,
								StackState->bIsBrokenCallstack
								) );
							LastSequenceTagForNamedMarker = SequenceTag;

						}
						else if (MemOp == EMemoryOperation::Realloc)
						{
							NumMemoryOperations++;
							const uint64 OldPtr = Ptr;

							// Read NewPtr
							Index++; CurrentStatMessageIndex++;
							const FStatMessage& AllocPtrMessage = Data[Index];
							const uint64 NewPtr = AllocPtrMessage.GetValue_Ptr() & ~(uint64)EMemoryOperation::Mask;

							// After AllocPtr message there is always alloc size message and the sequence tag.
							Index++; CurrentStatMessageIndex++;
							const FStatMessage& ReallocSizeMessage = Data[Index];
							const int64 ReallocSize = ReallocSizeMessage.GetValue_int64();

							// Read OperationSequenceTag.
							Index++; CurrentStatMessageIndex++;
							const FStatMessage& SequenceTagMessage = Data[Index];
							const uint32 SequenceTag = SequenceTagMessage.GetValue_int64();

							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_FreePtr ), (uint64)(UPTRINT)OldPtr | (uint64)EMemoryOperation::Realloc );
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_AllocPtr ), (uint64)(UPTRINT)NewPtr | (uint64)EMemoryOperation::Realloc );
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_AllocSize ), NewSize );
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_OperationSequenceTag ), (int64)SequenceTag );

							// Add a new realloc.
							SequenceAllocationArray.Add(
								FAllocationInfo(
								OldPtr, 
								NewPtr,
								ReallocSize,
								StackState->Stack,
								SequenceTag,
								EMemoryOperation::Realloc,
								StackState->bIsBrokenCallstack
								) );
							LastSequenceTagForNamedMarker = SequenceTag;
						}
						else if (MemOp == EMemoryOperation::Free)
						{
							NumMemoryOperations++;
							// Read OperationSequenceTag.
							Index++; CurrentStatMessageIndex++;
							const FStatMessage& SequenceTagMessage = Data[Index];
							const uint32 SequenceTag = SequenceTagMessage.GetValue_int64();

							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_FreePtr ), (uint64)(UPTRINT)Ptr | (uint64)EMemoryOperation::Free );	// 16 bytes total				
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_OperationSequenceTag ), (int64)SequenceTag );

							// Add a new free.
							SequenceAllocationArray.Add(
								FAllocationInfo(
								0,
								Ptr,		
								0,
								StackState->Stack,
								SequenceTag,
								EMemoryOperation::Free,
								StackState->bIsBrokenCallstack
								) );
						}
						else
						{
							UE_LOG( LogStats, Warning, TEXT( "Pointer from a memory operation is invalid" ) );
						}
					}
					else if( Op == EStatOperation::CycleScopeEnd )
					{
						if( StackState->Stack.Num() > 1 )
						{
							const FName ScopeStart = StackState->Stack.Pop();
							const FName ScopeEnd = Item.NameAndInfo.GetRawName();

							check( ScopeStart == ScopeEnd );

							StackState->Current = StackState->Stack.Last();

							// The stack should be ok, but it may be partially broken.
							// This will happen if memory profiling starts in the middle of executing a background thread.
							StackState->bIsBrokenCallstack = false;
						}
						else
						{
							const FName ShortName = Item.NameAndInfo.GetShortName();

							UE_LOG( LogStats, Warning, TEXT( "Broken cycle scope end %s/%s, current %s" ),
									*ThreadFName.ToString(),
									*ShortName.ToString(),
									*StackState->Current.ToString() );

							// The stack is completely broken, only has the thread name and the last cycle scope.
							// Rollback to the thread node.
							StackState->bIsBrokenCallstack = true;
							StackState->Stack.Empty();
							StackState->Stack.Add( ThreadFName );
							StackState->Current = ThreadFName;
						}
					}
					else if (Op == EStatOperation::SpecialMessageMarker)
					{
						if (RawName == FStatConstants::NAME_NamedMarker)
						{
							const FName NamedMarker = Item.GetValue_FName(); // LastSequenceTag
							Snapshots.Add( TPairInitializer<uint32, FName>( LastSequenceTagForNamedMarker, NamedMarker ) );
						}
					}
				}
			}
		}
	}

	InFile->StageProgress.Set( 100 );

	// End marker.
	Snapshots.Add( TPairInitializer<uint32, FName>( TNumericLimits<uint32>::Max(), TEXT( "EndSnapshot" ) ) );

	// Copy snapshots.
	SnapshotsToBeProcessed = Snapshots;

	UE_LOG( LogStats, Warning, TEXT( "NumMemoryOperations:   %llu" ), NumMemoryOperations );
	UE_LOG( LogStats, Warning, TEXT( "SequenceAllocationNum: %i" ), SequenceAllocationArray.Num() );

	// Pass 2.
	/*
	TMap<uint32,FAllocationInfo> UniqueSeq;
	TMultiMap<uint32,FAllocationInfo> OriginalAllocs;
	TMultiMap<uint32,FAllocationInfo> BrokenAllocs;
	for( const FAllocationInfo& Alloc : SequenceAllocationArray )
	{
		const FAllocationInfo* Found = UniqueSeq.Find(Alloc.SequenceTag);
		if( !Found )
		{
			UniqueSeq.Add(Alloc.SequenceTag,Alloc);
		}
		else
		{
			OriginalAllocs.Add(Alloc.SequenceTag, *Found);
			BrokenAllocs.Add(Alloc.SequenceTag, Alloc);
		}
	}
	*/

	InFile->SetProcessingStage( EStatsProcessingStage::SPS_SortSequence );

	// Sort all memory operation by the sequence tag, iterate through all operation and generate memory usage.
	SequenceAllocationArray.Sort( FAllocationInfoSequenceTagLess() );

	// Abandon support, simply return for now.
	if (InFile->bShouldStopProcessing == true)
	{
		InFile->SetProcessingStage( EStatsProcessingStage::SPS_Stopped );
		return;
	}

	// Named markers/snapshots

	// Alive allocations.
	TMap<uint64, FAllocationInfo> AllocationMap;
	TMultiMap<uint64, FAllocationInfo> FreeWithoutAllocMap;
	TMultiMap<uint64, FAllocationInfo> DuplicatedAllocMap;
	int32 NumDuplicatedMemoryOperations = 0;
	int32 NumFWAMemoryOperations = 0; // FreeWithoutAlloc
	int32 NumZeroAllocs = 0; // Malloc(0)

	InFile->SetProcessingStage( EStatsProcessingStage::SPS_ProcessAllocations );

	// Initialize the begin snapshot.
	auto BeginSnapshot = SnapshotsToBeProcessed[0];
	SnapshotsToBeProcessed.RemoveAt( 0 );
	PrepareSnapshot( BeginSnapshot.Value, AllocationMap );

	
	auto CurrentSnapshot = SnapshotsToBeProcessed[0];

	UE_LOG( LogStats, Warning, TEXT( "Generating memory operations map" ) );
	const int32 NumSequenceAllocations = SequenceAllocationArray.Num();
	const int32 OnePercent = FMath::Max( NumSequenceAllocations / 100, 1024 );
	for( int32 Index = 0; Index < NumSequenceAllocations; Index++ )
	{
		if( Index % OnePercent )
		{
			const double CurrentSeconds = FPlatformTime::Seconds();
			if( CurrentSeconds > PreviousSeconds + NumSecondsBetweenLogs )
			{
				const int32 PercentagePos = int32( 100.0*(Index + 1) / NumSequenceAllocations );
				InFile->StageProgress.Set( PercentagePos );
				UE_LOG( LogStats, Verbose, TEXT( "Processing allocations %3i%% (%10i/%10i)" ), PercentagePos, Index + 1, NumSequenceAllocations );
				PreviousSeconds = CurrentSeconds;

				// Abandon support, simply return for now.
				if (InFile->bShouldStopProcessing == true)
				{
					InFile->SetProcessingStage( EStatsProcessingStage::SPS_Stopped );
					return;
				}
			}
		}

		const FAllocationInfo& Alloc = SequenceAllocationArray[Index];

		// Check named marker/snapshots
		if (Alloc.SequenceTag > CurrentSnapshot.Key)
		{
			SnapshotsToBeProcessed.RemoveAt( 0 );
			PrepareSnapshot( CurrentSnapshot.Value, AllocationMap );
			CurrentSnapshot = SnapshotsToBeProcessed[0];
		}

		if (Alloc.Op == EMemoryOperation::Alloc)
		{
			if (Alloc.Size == 0)
			{
				NumZeroAllocs++;
			}

			const FAllocationInfo* Found = AllocationMap.Find( Alloc.Ptr );

			if( !Found )
			{
				AllocationMap.Add( Alloc.Ptr, Alloc );
			}
			else
			{
				const FAllocationInfo* FoundAndFreed = FreeWithoutAllocMap.Find( Found->Ptr );
				const FAllocationInfo* FoundAndAllocated = FreeWithoutAllocMap.Find( Alloc.Ptr );

#if	UE_BUILD_DEBUG
				if( FoundAndFreed )
				{
					const FString FoundAndFreedCallstack = FStatsCallstack::GetHumanReadable( FoundAndFreed->EncodedCallstack );
				}

				if( FoundAndAllocated )
				{
					const FString FoundAndAllocatedCallstack = FStatsCallstack::GetHumanReadable( FoundAndAllocated->EncodedCallstack );
				}

				NumDuplicatedMemoryOperations++;


				const FString FoundCallstack = FStatsCallstack::GetHumanReadable( Found->EncodedCallstack );
				const FString AllocCallstack = FStatsCallstack::GetHumanReadable( Alloc.EncodedCallstack );
				UE_LOG( LogStats, Warning, TEXT( "DuplicateAlloc: %s Size: %i/%i Ptr: %i/%i Tag: %i/%i" ), *AllocCallstack, Found->Size, Alloc.Size, Found->Ptr, Alloc.Ptr, Found->SequenceTag, Alloc.SequenceTag );
#endif // UE_BUILD_DEBUG

				// Replace pointer.
				AllocationMap.Add( Alloc.Ptr, Alloc );
				// Store the old pointer.
				DuplicatedAllocMap.Add( Alloc.Ptr, *Found );
			}
		}
		else if (Alloc.Op == EMemoryOperation::Realloc)
		{
			if (Alloc.Size == 0)
			{
				NumZeroAllocs++;
			}

			// Previous Alloc or Realloc
			const FAllocationInfo* FoundOld = AllocationMap.Find( Alloc.OldPtr );

			if (FoundOld)
			{
				const bool bIsValid = Alloc.SequenceTag > FoundOld->SequenceTag;
				if (!bIsValid)
				{
					UE_LOG( LogStats, Warning, TEXT( "InvalidRealloc Ptr: %llu, Seq: %i/%i" ), Alloc.Ptr, Alloc.SequenceTag, FoundOld->SequenceTag );
				}
				// Remove the old allocation.
				AllocationMap.Remove( Alloc.OldPtr );
				AllocationMap.Add( Alloc.Ptr, Alloc );
			}
			// If we have situation where the old pointer is the same as the new pointer
			// There is high change that the original call looked like this 
			// Ptr = Malloc( 40 ); Ptr = Realloc( Ptr, 40 );
			else if (Alloc.OldPtr != Alloc.Ptr)
			{
				// No OldPtr, should not happen as it means realloc without initial alloc.
				// Or Realloc after Malloc(0)

#if	UE_BUILD_DEBUG
				const FString ReallocCallstack = FStatsCallstack::GetHumanReadable( Alloc.EncodedCallstack );
				//UE_LOG( LogStats, Warning, TEXT( "ReallocWithoutAlloc: %s %i %i/%i [%i]" ), *ReallocCallstack, Alloc.Size, Alloc.OldPtr, Alloc.Ptr, Alloc.SequenceTag );
#endif // UE_BUILD_DEBUG

				AllocationMap.Add( Alloc.Ptr, Alloc );
			}

		}
		else if (Alloc.Op == EMemoryOperation::Free)
		{
			const FAllocationInfo* Found = AllocationMap.Find( Alloc.Ptr );
			if( Found )
			{
				const bool bIsValid = Alloc.SequenceTag > Found->SequenceTag;
				if( !bIsValid )
				{
					UE_LOG( LogStats, Warning, TEXT( "InvalidFree Ptr: %llu, Seq: %i/%i" ), Alloc.Ptr, Alloc.SequenceTag, Found->SequenceTag );
				}
				AllocationMap.Remove( Alloc.Ptr );
			}
			else
			{
				FreeWithoutAllocMap.Add( Alloc.Ptr, Alloc );
				NumFWAMemoryOperations++;

#if	UE_BUILD_DEBUG
				const FString FWACallstack = FStatsCallstack::GetHumanReadable( Alloc.EncodedCallstack );
				//UE_LOG( LogStats, Warning, TEXT( "FreeWithoutAllocCallstack: %s %i" ), *FWACallstack, Alloc.Ptr );
#endif // UE_BUILD_DEBUG
			}
		}
	}
	InFile->StageProgress.Set( 100 );

	auto EndSnapshot = SnapshotsToBeProcessed[0];
	SnapshotsToBeProcessed.RemoveAt( 0 );
	PrepareSnapshot( EndSnapshot.Value, AllocationMap );

	UE_LOG( LogStats, Warning, TEXT( "NumDuplicatedMemoryOperations: %i" ), NumDuplicatedMemoryOperations );
	UE_LOG( LogStats, Warning, TEXT( "NumFWAMemoryOperations:        %i" ), NumFWAMemoryOperations );
	UE_LOG( LogStats, Warning, TEXT( "NumZeroAllocs:                 %i" ), NumZeroAllocs );

	// Dump problematic allocations
	DuplicatedAllocMap.ValueSort( FAllocationInfoSizeGreater() );
	//FreeWithoutAllocMap

	uint64 TotalDuplicatedMemory = 0;
	for( const auto& It : DuplicatedAllocMap )
	{
		const FAllocationInfo& Alloc = It.Value;
		TotalDuplicatedMemory += Alloc.Size;
	}

	UE_LOG( LogStats, Warning, TEXT( "Dumping duplicated alloc map" ) );
	const float MaxPctDisplayed = 0.80f;
	uint64 DisplayedSoFar = 0;
	for( const auto& It : DuplicatedAllocMap )
	{
		const FAllocationInfo& Alloc = It.Value;
		const FString AllocCallstack = FStatsCallstack::GetHumanReadable( Alloc.EncodedCallstack );
		UE_LOG( LogStats, Log, TEXT( "%lli (%.2f MB) %s" ), Alloc.Size, Alloc.Size / 1024.0f / 1024.0f, *AllocCallstack );

		DisplayedSoFar += Alloc.Size;

		const float CurrentPct = (float)DisplayedSoFar / (float)TotalDuplicatedMemory;
		if( CurrentPct > MaxPctDisplayed )
		{
			break;
		}
	}

	InFile->SetProcessingStage( EStatsProcessingStage::SPS_Finished );
}

void FStatsMemoryDumpCommand::GenerateScopedTreeAllocations( const TMap<FName, FCombinedAllocationInfo>& ScopedAllocations, FNodeAllocationInfo& out_Root )
{
	// Experimental code, partially optimized.
	// Decode all scoped allocations.
	// Generate tree for allocations and combine them.
	
	for (const auto& It : ScopedAllocations)
	{
		const FName& EncodedCallstack = It.Key;
		const FCombinedAllocationInfo& CombinedAllocation = It.Value;

		// Decode callstack.
		TArray<FName> DecodedCallstack;
		FStatsCallstack::DecodeToNames( EncodedCallstack, DecodedCallstack );
		
		const int32 AllocationLenght = DecodedCallstack.Num();
		check( DecodedCallstack.Num() > 0 );

		FNodeAllocationInfo* CurrentNode = &out_Root;
		// Accumulate with thread root node.
		CurrentNode->Accumulate( CombinedAllocation );

		// Iterate through the callstack and prepare all nodes if needed, and accumulate memory.
		TArray<FName> CurrentCallstack;
		const int32 NumEntries = DecodedCallstack.Num();
		for (int32 Idx1 = 0; Idx1 < NumEntries; ++Idx1)
		{
			const FName NodeName = DecodedCallstack[Idx1];
			CurrentCallstack.Add( NodeName );

			FNodeAllocationInfo* Node = nullptr;
			const bool bContainsNode = CurrentNode->ChildNodes.Contains( NodeName );
			if (!bContainsNode)
			{
				Node = new FNodeAllocationInfo;
				Node->Depth = Idx1;
				Node->PrepareCallstackData( CurrentCallstack );

				CurrentNode->ChildNodes.Add( NodeName, Node );
			}
			else
			{
				Node = CurrentNode->ChildNodes.FindChecked( NodeName );
			}

			// Accumulate memory usage and num allocations for all nodes in the callstack.
			Node->Accumulate( CombinedAllocation );
			
			// Move to the next node.
			Node->Parent = CurrentNode;
			CurrentNode = Node;
		}
	}

	out_Root.SortBySize();
}


void FStatsMemoryDumpCommand::GenerateMemoryUsageReport( const TMap<uint64, FAllocationInfo>& AllocationMap, const TSet<FName>& UObjectRawNames )
{
	if( AllocationMap.Num() == 0 )
	{
		UE_LOG( LogStats, Warning, TEXT( "There are no allocations, make sure memory profiler is enabled" ) );
	}
	else
	{
		//ProcessAndDumpUObjectAllocations( AllocationMap );
		ProcessAndDumpScopedAllocations( AllocationMap );
	}
}

void FStatsMemoryDumpCommand::ProcessAndDumpScopedAllocations( const TMap<uint64, FAllocationInfo>& AllocationMap )
{
	// This code is not optimized. 
	FScopeLogTime SLT( TEXT( "ProcessingScopedAllocations" ), nullptr, FScopeLogTime::ScopeLog_Seconds );
	UE_LOG( LogStats, Warning, TEXT( "Processing scoped allocations" ) );

	const FString ReportName = FString::Printf( TEXT( "%s-Memory-Scoped" ), *GetPlatformName() );
	FDiagnosticTableViewer MemoryReport( *FDiagnosticTableViewer::GetUniqueTemporaryFilePath( *ReportName ), true );

	// Write a row of headings for the table's columns.
	MemoryReport.AddColumn( TEXT( "Size (bytes)" ) );
	MemoryReport.AddColumn( TEXT( "Size (MB)" ) );
	MemoryReport.AddColumn( TEXT( "Count" ) );
	MemoryReport.AddColumn( TEXT( "Callstack" ) );
	MemoryReport.CycleRow();

	TMap<FName, FCombinedAllocationInfo> CombinedAllocations;
	uint64 TotalAllocatedMemory = 0;
	uint64 NumAllocations = 0;
	GenerateScopedAllocations( AllocationMap, CombinedAllocations, TotalAllocatedMemory, NumAllocations );

	// Dump memory to the log.
	CombinedAllocations.ValueSort( FCombinedAllocationInfoSizeGreater() );

	const float MaxPctDisplayed = 0.90f;
	int32 CurrentIndex = 0;
	uint64 DisplayedSoFar = 0;
	UE_LOG( LogStats, Warning, TEXT( "Index, Size (Size MB), Count, Stat desc" ) );
	for( const auto& It : CombinedAllocations )
	{
		const FCombinedAllocationInfo& CombinedAllocation = It.Value;
		const FName& EncodedCallstack = It.Key;

		const FString AllocCallstack = FStatsCallstack::GetHumanReadable( EncodedCallstack );

		UE_LOG( LogStats, Log, TEXT( "%2i, %llu (%.2f MB), %llu, %s" ),
				CurrentIndex,
				CombinedAllocation.Size,
				CombinedAllocation.Size / 1024.0f / 1024.0f,
				CombinedAllocation.Count,
				*AllocCallstack );

		// Dump stats
		MemoryReport.AddColumn( TEXT( "%llu" ), CombinedAllocation.Size );
		MemoryReport.AddColumn( TEXT( "%.2f MB" ), CombinedAllocation.Size / 1024.0f / 1024.0f );
		MemoryReport.AddColumn( TEXT( "%llu" ), CombinedAllocation.Count );
		MemoryReport.AddColumn( *AllocCallstack );
		MemoryReport.CycleRow();

		CurrentIndex++;
		DisplayedSoFar += CombinedAllocation.Size;

		const float CurrentPct = (float)DisplayedSoFar / (float)TotalAllocatedMemory;
		if( CurrentPct > MaxPctDisplayed )
		{
			break;
		}
	}

	UE_LOG( LogStats, Warning, TEXT( "Allocated memory: %llu bytes (%.2f MB)" ), TotalAllocatedMemory, TotalAllocatedMemory / 1024.0f / 1024.0f );

	// Add a total row.
	MemoryReport.CycleRow();
	MemoryReport.CycleRow();
	MemoryReport.CycleRow();
	MemoryReport.AddColumn( TEXT( "%llu" ), TotalAllocatedMemory );
	MemoryReport.AddColumn( TEXT( "%.2f MB" ), TotalAllocatedMemory / 1024.0f / 1024.0f );
	MemoryReport.AddColumn( TEXT( "%llu" ), NumAllocations );
	MemoryReport.AddColumn( TEXT( "TOTAL" ) );
	MemoryReport.CycleRow();
}

void FStatsMemoryDumpCommand::ProcessAndDumpUObjectAllocations( const TMap<uint64, FAllocationInfo>& AllocationMap, const TSet<FName>& UObjectRawNames )
{
	// This code is not optimized. 
	FScopeLogTime SLT( TEXT( "ProcessingUObjectAllocations" ), nullptr, FScopeLogTime::ScopeLog_Seconds );
	UE_LOG( LogStats, Warning, TEXT( "Processing UObject allocations" ) );

	const FString ReportName = FString::Printf( TEXT( "%s-Memory-UObject" ), *GetPlatformName() );
	FDiagnosticTableViewer MemoryReport( *FDiagnosticTableViewer::GetUniqueTemporaryFilePath( *ReportName ), true );

	// Write a row of headings for the table's columns.
	MemoryReport.AddColumn( TEXT( "Size (bytes)" ) );
	MemoryReport.AddColumn( TEXT( "Size (MB)" ) );
	MemoryReport.AddColumn( TEXT( "Count" ) );
	MemoryReport.AddColumn( TEXT( "UObject class" ) );
	MemoryReport.CycleRow();

	TMap<FName, FCombinedAllocationInfo> UObjectAllocations;

	// To minimize number of calls to expensive DecodeCallstack.
	TMap<FName,FName> UObjectCallstackToClassMapping;

	uint64 NumAllocations = 0;
	uint64 TotalAllocatedMemory = 0;
	for( const auto& It : AllocationMap )
	{
		const FAllocationInfo& Alloc = It.Value;

		FName UObjectClass = UObjectCallstackToClassMapping.FindRef( Alloc.EncodedCallstack );
		if( UObjectClass == NAME_None )
		{
			TArray<FName> DecodedCallstack;
			FStatsCallstack::DecodeToNames( Alloc.EncodedCallstack, DecodedCallstack );

			for( int32 Index = DecodedCallstack.Num() - 1; Index >= 0; --Index )
			{
				const FName LongName = DecodedCallstack[Index];
				const bool bValid = UObjectRawNames.Contains( LongName );
				if( bValid )
				{
					const FString ObjectName = FStatNameAndInfo::GetShortNameFrom( LongName ).GetPlainNameString();
					UObjectClass = *ObjectName.Left( ObjectName.Find( TEXT( "//" ) ) );;
					UObjectCallstackToClassMapping.Add( Alloc.EncodedCallstack, UObjectClass );
					break;
				}
			}
		}
		
		if( UObjectClass != NAME_None )
		{
			FCombinedAllocationInfo& CombinedAllocation = UObjectAllocations.FindOrAdd( UObjectClass );
			CombinedAllocation += Alloc;

			TotalAllocatedMemory += Alloc.Size;
			NumAllocations++;
		}
	}

	// Dump memory to the log.
	UObjectAllocations.ValueSort( FCombinedAllocationInfoSizeGreater() );

	const float MaxPctDisplayed = 0.90f;
	int32 CurrentIndex = 0;
	uint64 DisplayedSoFar = 0;
	UE_LOG( LogStats, Warning, TEXT( "Index, Size (Size MB), Count, UObject class" ) );
	for( const auto& It : UObjectAllocations )
	{
		const FCombinedAllocationInfo& CombinedAllocation = It.Value;
		const FName& UObjectClass = It.Key;

		UE_LOG( LogStats, Log, TEXT( "%2i, %llu (%.2f MB), %llu, %s" ),
				CurrentIndex,
				CombinedAllocation.Size,
				CombinedAllocation.Size / 1024.0f / 1024.0f,
				CombinedAllocation.Count,
				*UObjectClass.GetPlainNameString() );

		// Dump stats
		MemoryReport.AddColumn( TEXT( "%llu" ), CombinedAllocation.Size );
		MemoryReport.AddColumn( TEXT( "%.2f MB" ), CombinedAllocation.Size / 1024.0f / 1024.0f );
		MemoryReport.AddColumn( TEXT( "%llu" ), CombinedAllocation.Count );
		MemoryReport.AddColumn( *UObjectClass.GetPlainNameString() );
		MemoryReport.CycleRow();

		CurrentIndex++;
		DisplayedSoFar += CombinedAllocation.Size;

		const float CurrentPct = (float)DisplayedSoFar / (float)TotalAllocatedMemory;
		if( CurrentPct > MaxPctDisplayed )
		{
			break;
		}
	}

	UE_LOG( LogStats, Warning, TEXT( "Allocated memory: %llu bytes (%.2f MB)" ), TotalAllocatedMemory, TotalAllocatedMemory / 1024.0f / 1024.0f );

	// Add a total row.
	MemoryReport.CycleRow();
	MemoryReport.CycleRow();
	MemoryReport.CycleRow();
	MemoryReport.AddColumn( TEXT( "%llu" ), TotalAllocatedMemory );
	MemoryReport.AddColumn( TEXT( "%.2f MB" ), TotalAllocatedMemory / 1024.0f / 1024.0f );
	MemoryReport.AddColumn( TEXT( "%llu" ), NumAllocations );
	MemoryReport.AddColumn( TEXT( "TOTAL" ) );
	MemoryReport.CycleRow();
}

void FStatsMemoryDumpCommand::DumpScopedAllocations( const TCHAR* Name, const TMap<FString, FCombinedAllocationInfo>& CombinedAllocations )
{
	if (CombinedAllocations.Num() == 0)
	{
		UE_LOG( LogStats, Warning, TEXT( "No scoped allocations: %s" ), Name );
		return;
	}

	// This code is not optimized. 
	FScopeLogTime SLT( TEXT( "ProcessingScopedAllocations" ), nullptr, FScopeLogTime::ScopeLog_Seconds );
	UE_LOG( LogStats, Warning, TEXT( "Dumping scoped allocations: %s" ), Name );

	const FString ReportName = FString::Printf( TEXT( "%s-Memory-Scoped-%s" ), *GetPlatformName(), Name );
	FDiagnosticTableViewer MemoryReport( *FDiagnosticTableViewer::GetUniqueTemporaryFilePath( *ReportName ), true );

	// Write a row of headings for the table's columns.
	MemoryReport.AddColumn( TEXT( "Size (bytes)" ) );
	MemoryReport.AddColumn( TEXT( "Size (MB)" ) );
	MemoryReport.AddColumn( TEXT( "Count" ) );
	MemoryReport.AddColumn( TEXT( "Callstack" ) );
	MemoryReport.CycleRow();

	
	FCombinedAllocationInfo Total;

	const float MaxPctDisplayed = 0.90f;
	int32 CurrentIndex = 0;
	UE_LOG( LogStats, Warning, TEXT( "Index, Size (Size MB), Count, Stat desc" ) );
	for (const auto& It : CombinedAllocations)
	{
		const FCombinedAllocationInfo& CombinedAllocation = It.Value;
		//const FName& EncodedCallstack = It.Key;
		const FString AllocCallstack = It.Key;// GetCallstack( EncodedCallstack );

		UE_LOG( LogStats, Log, TEXT( "%2i, %llu (%.2f MB), %llu, %s" ),
				CurrentIndex,
				CombinedAllocation.Size,
				CombinedAllocation.Size / 1024.0f / 1024.0f,
				CombinedAllocation.Count,
				*AllocCallstack );

		// Dump stats
		MemoryReport.AddColumn( TEXT( "%llu" ), CombinedAllocation.Size );
		MemoryReport.AddColumn( TEXT( "%.2f MB" ), CombinedAllocation.Size / 1024.0f / 1024.0f );
		MemoryReport.AddColumn( TEXT( "%llu" ), CombinedAllocation.Count );
		MemoryReport.AddColumn( *AllocCallstack );
		MemoryReport.CycleRow();

		CurrentIndex++;
		Total += CombinedAllocation;
	}

	UE_LOG( LogStats, Warning, TEXT( "Allocated memory: %llu bytes (%.2f MB)" ), Total.Size, Total.SizeMB );

	// Add a total row.
	MemoryReport.CycleRow();
	MemoryReport.CycleRow();
	MemoryReport.CycleRow();
	MemoryReport.AddColumn( TEXT( "%llu" ), Total.Size );
	MemoryReport.AddColumn( TEXT( "%.2f MB" ), Total.SizeMB );
	MemoryReport.AddColumn( TEXT( "%llu" ), Total.Count );
	MemoryReport.AddColumn( TEXT( "TOTAL" ) );
	MemoryReport.CycleRow();
}

void FStatsMemoryDumpCommand::GenerateScopedAllocations( const TMap<uint64, FAllocationInfo>& AllocationMap, TMap<FName, FCombinedAllocationInfo>& out_CombinedAllocations, uint64& TotalAllocatedMemory, uint64& NumAllocations )
{
	FScopeLogTime SLT( TEXT( "GenerateScopedAllocations" ), nullptr, FScopeLogTime::ScopeLog_Milliseconds );

	for (const auto& It : AllocationMap)
	{
		const FAllocationInfo& Alloc = It.Value;
		FCombinedAllocationInfo& CombinedAllocation = out_CombinedAllocations.FindOrAdd( Alloc.EncodedCallstack );
		CombinedAllocation += Alloc;

		TotalAllocatedMemory += Alloc.Size;
		NumAllocations++;
	}

	// Sort by size.
	out_CombinedAllocations.ValueSort( FCombinedAllocationInfoSizeGreater() );
}

void FStatsMemoryDumpCommand::PrepareSnapshot( const FName SnapshotName, const TMap<uint64, FAllocationInfo>& AllocationMap )
{
	FScopeLogTime SLT( TEXT( "PrepareSnapshot" ), nullptr, FScopeLogTime::ScopeLog_Milliseconds );

	// Make sure the snapshot name is unique.
	FName UniqueSnapshotName = SnapshotName;
	while (SnapshotNames.Contains( UniqueSnapshotName ))
	{
		UniqueSnapshotName = FName( UniqueSnapshotName, UniqueSnapshotName.GetNumber() + 1 );
	}
	SnapshotNames.Add( UniqueSnapshotName );

	SnapshotsWithAllocationMap.Add( UniqueSnapshotName, AllocationMap );

	TMap<FName, FCombinedAllocationInfo> SnapshotCombinedAllocations;
	uint64 TotalAllocatedMemory = 0;
	uint64 NumAllocations = 0;
	GenerateScopedAllocations( AllocationMap, SnapshotCombinedAllocations, TotalAllocatedMemory, NumAllocations );
	SnapshotsWithScopedAllocations.Add( UniqueSnapshotName, SnapshotCombinedAllocations );

	// Decode callstacks.
	// Replace encoded callstacks with human readable name. For easier debugging.
	TMap<FString, FCombinedAllocationInfo> SnapshotDecodedCombinedAllocations;
	for (auto& It : SnapshotCombinedAllocations)
	{
		const FString HumanReadableCallstack = FStatsCallstack::GetHumanReadable( It.Key );
		SnapshotDecodedCombinedAllocations.Add( HumanReadableCallstack, It.Value );
	}
	SnapshotsWithDecodedScopedAllocations.Add( UniqueSnapshotName, SnapshotDecodedCombinedAllocations );

	UE_LOG( LogStats, Warning, TEXT( "PrepareSnapshot: %s Alloc: %i Scoped: %i Total: %.2f MB" ), *UniqueSnapshotName.ToString(), AllocationMap.Num(), SnapshotCombinedAllocations.Num(), TotalAllocatedMemory / 1024.0f / 1024.0f );
}

void FStatsMemoryDumpCommand::CompareSnapshots_FName( const FName BeginSnaphotName, const FName EndSnaphotName, TMap<FName, FCombinedAllocationInfo>& out_Result )
{
	const auto BeginSnaphotPtr = SnapshotsWithScopedAllocations.Find( BeginSnaphotName );
	const auto EndSnapshotPtr = SnapshotsWithScopedAllocations.Find( EndSnaphotName );
	if (BeginSnaphotPtr && EndSnapshotPtr)
	{
		// Process data.
		TMap<FName, FCombinedAllocationInfo> BeginSnaphot = *BeginSnaphotPtr;
		TMap<FName, FCombinedAllocationInfo> EndSnaphot = *EndSnapshotPtr;
		TMap<FName, FCombinedAllocationInfo> Result;

		for (const auto& It : EndSnaphot)
		{
			const FName Callstack = It.Key;
			const FCombinedAllocationInfo EndCombinedAlloc = It.Value;

			const FCombinedAllocationInfo* BeginCombinedAllocPtr = BeginSnaphot.Find( Callstack );
			if (BeginCombinedAllocPtr)
			{
				FCombinedAllocationInfo CombinedAllocation;
				CombinedAllocation += EndCombinedAlloc;
				CombinedAllocation -= *BeginCombinedAllocPtr;

				if (CombinedAllocation.IsAlive())
				{
					out_Result.Add( Callstack, CombinedAllocation );
				}
			}
			else
			{
				out_Result.Add( Callstack, EndCombinedAlloc );
			}
		}

		// Sort by size.
		out_Result.ValueSort( FCombinedAllocationInfoSizeGreater() );
	}
}

void FStatsMemoryDumpCommand::CompareSnapshots_FString( const FName BeginSnaphotName, const FName EndSnaphotName, TMap<FString, FCombinedAllocationInfo>& out_Result )
{
	const auto BeginSnaphotPtr = SnapshotsWithDecodedScopedAllocations.Find( BeginSnaphotName );
	const auto EndSnapshotPtr = SnapshotsWithDecodedScopedAllocations.Find( EndSnaphotName );
	if (BeginSnaphotPtr && EndSnapshotPtr)
	{
		// Process data.
		TMap<FString, FCombinedAllocationInfo> BeginSnaphot = *BeginSnaphotPtr;
		TMap<FString, FCombinedAllocationInfo> EndSnaphot = *EndSnapshotPtr;

		for (const auto& It : EndSnaphot)
		{
			const FString& Callstack = It.Key;
			const FCombinedAllocationInfo EndCombinedAlloc = It.Value;

			const FCombinedAllocationInfo* BeginCombinedAllocPtr = BeginSnaphot.Find( Callstack );
			if (BeginCombinedAllocPtr)
			{
				FCombinedAllocationInfo CombinedAllocation;
				CombinedAllocation += EndCombinedAlloc;
				CombinedAllocation -= *BeginCombinedAllocPtr;

				if (CombinedAllocation.IsAlive())
				{
					out_Result.Add( Callstack, CombinedAllocation );
				}
			}
			else
			{
				out_Result.Add( Callstack, EndCombinedAlloc );
			}
		}

		// Sort by size.
		out_Result.ValueSort( FCombinedAllocationInfoSizeGreater() );
	}
}

