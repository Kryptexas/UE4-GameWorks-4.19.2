// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealFrontendPrivatePCH.h"
#include "StatsDumpMemoryCommand.h"
#include "DiagnosticTable.h"

/*-----------------------------------------------------------------------------
	Callstack decoding/encoding
-----------------------------------------------------------------------------*/

static const TCHAR* CallstackSeparator = TEXT( "+" );

static FString EncodeCallstack( const TArray<FName>& Callstack )
{
	FString Result;
	for (const auto& Name : Callstack)
	{
		Result += TTypeToString<int32>::ToString( (int32)Name.GetComparisonIndex() );
		Result += CallstackSeparator;
	}
	return Result;
}

static void DecodeCallstack( const FName& EncodedCallstack, TArray<FString>& out_DecodedCallstack )
{
	EncodedCallstack.ToString().ParseIntoArray( out_DecodedCallstack, CallstackSeparator, true );
}

static FString GetCallstack( const FName& EncodedCallstack )
{
	FString Result;

	TArray<FString> DecodedCallstack;
	DecodeCallstack( EncodedCallstack, DecodedCallstack );

	for (int32 Index = DecodedCallstack.Num() - 1; Index >= 0; --Index)
	{
		NAME_INDEX NameIndex = 0;
		TTypeFromString<NAME_INDEX>::FromString( NameIndex, *DecodedCallstack[Index] );

		const FName LongName = FName( NameIndex, NameIndex, 0 );

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

		if (Index > 0)
		{
			Result += TEXT( " <- " );
		}
	}

	Result.ReplaceInline( TEXT( "STAT_" ), TEXT( "" ), ESearchCase::CaseSensitive );
	return Result;
}

/*-----------------------------------------------------------------------------
	Allocation info
-----------------------------------------------------------------------------*/

FAllocationInfo::FAllocationInfo( uint64 InOldPtr, uint64 InPtr, int64 InSize, const TArray<FName>& InCallstack, uint32 InSequenceTag, EMemoryOperation InOp, bool bInHasBrokenCallstack ) 
	: OldPtr( InOldPtr )
	, Ptr( InPtr )
	, Size( InSize )
	, EncodedCallstack( *EncodeCallstack( InCallstack ) )
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

struct FAllocationInfoGreater
{
	FORCEINLINE bool operator()( const FAllocationInfo& A, const FAllocationInfo& B ) const
	{
		return B.Size < A.Size;
	}
};
struct FSizeAndCountGreater
{
	FORCEINLINE bool operator()( const FSizeAndCount& A, const FSizeAndCount& B ) const
	{
		return B.Size < A.Size;
	}
};

/*-----------------------------------------------------------------------------
	Stats stack helpers
-----------------------------------------------------------------------------*/

/** Holds stats stack state, used to preserve continuity when the game frame has changed. */
struct FStackState
{
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
	FParse::Value( FCommandLine::Get(), TEXT( "-INFILE=" ), SourceFilepath );

	const int64 Size = IFileManager::Get().FileSize( *SourceFilepath );
	if( Size < 4 )
	{
		UE_LOG( LogStats, Error, TEXT( "Could not open: %s" ), *SourceFilepath );
		return;
	}
	TAutoPtr<FArchive> FileReader( IFileManager::Get().CreateFileReader( *SourceFilepath ) );
	if( !FileReader )
	{
		UE_LOG( LogStats, Error, TEXT( "Could not open: %s" ), *SourceFilepath );
		return;
	}

	if( !Stream.ReadHeader( *FileReader ) )
	{
		UE_LOG( LogStats, Error, TEXT( "Could not open, bad magic: %s" ), *SourceFilepath );
		return;
	}

	UE_LOG( LogStats, Warning, TEXT( "Reading a raw stats file for memory profiling: %s" ), *SourceFilepath );

	const bool bIsFinalized = Stream.Header.IsFinalized();
	check( bIsFinalized );
	check( Stream.Header.Version >= EStatMagicWithHeader::VERSION_6 );
	StatsThreadStats.MarkAsLoaded();

	TArray<FStatMessage> Messages;
	if( Stream.Header.bRawStatsFile )
	{
		FScopeLogTime SLT( TEXT( "FStatsMemoryDumpCommand::InternalRun" ), nullptr, FScopeLogTime::ScopeLog_Seconds );

		// Read metadata.
		TArray<FStatMessage> MetadataMessages;
		Stream.ReadFNamesAndMetadataMessages( *FileReader, MetadataMessages );
		StatsThreadStats.ProcessMetaDataOnly( MetadataMessages );

		// Find all UObject metadata messages.
		for( const auto& Meta : MetadataMessages )
		{
			const FName EncName = Meta.NameAndInfo.GetEncodedName();
			const FName RawName = Meta.NameAndInfo.GetRawName();
			const FString Desc = FStatNameAndInfo::GetShortNameFrom( RawName ).GetPlainNameString();
			const bool bContainsUObject = Desc.Contains( TEXT( "//" ) );
			if( bContainsUObject )
			{
				UObjectNames.Add( RawName );
			}
		}

		const int64 CurrentFilePos = FileReader->Tell();

		// Read frames offsets.
		Stream.ReadFramesOffsets( *FileReader );

		// Buffer used to store the compressed and decompressed data.
		TArray<uint8> SrcArray;
		TArray<uint8> DestArray;
		const bool bHasCompressedData = Stream.Header.HasCompressedData();
		check( bHasCompressedData );

		TMap<int64, FStatPacketArray> CombinedHistory;
		int64 TotalDataSize = 0;
		int64 TotalStatMessagesNum = 0;
		int64 MaximumPacketSize = 0;
		int64 TotalPacketsNum = 0;
		// Read all packets sequentially, forced by the memory profiler which is now a part of the raw stats.
		// !!CAUTION!! Frame number in the raw stats is pointless, because it is time/cycles based, not frame based.
		// Background threads usually execute time consuming operations, so the frame number won't be valid.
		// Needs to be combined by the thread and the time, not by the frame number.
		{
			// Display log information once per 5 seconds to avoid spamming.
			double PreviousSeconds = FPlatformTime::Seconds();
			const int64 FrameOffset0 = Stream.FramesInfo[0].FrameFileOffset;
			FileReader->Seek( FrameOffset0 );

			const int64 FileSize = FileReader->TotalSize();

			while( FileReader->Tell() < FileSize )
			{
				// Read the compressed data.
				FCompressedStatsData UncompressedData( SrcArray, DestArray );
				*FileReader << UncompressedData;
				if( UncompressedData.HasReachedEndOfCompressedData() )
				{
					break;
				}

				FMemoryReader MemoryReader( DestArray, true );

				FStatPacket* StatPacket = new FStatPacket();
				Stream.ReadStatPacket( MemoryReader, *StatPacket );

				const int64 StatPacketFrameNum = StatPacket->Frame;
				FStatPacketArray& Frame = CombinedHistory.FindOrAdd( StatPacketFrameNum );

				// Check if we need to combine packets from the same thread.
				FStatPacket** CombinedPacket = Frame.Packets.FindByPredicate( [&]( FStatPacket* Item ) -> bool
				{
					return Item->ThreadId == StatPacket->ThreadId;
				} );

				const int64 PacketSize = StatPacket->StatMessages.GetAllocatedSize();
				TotalStatMessagesNum += StatPacket->StatMessages.Num();

				if( CombinedPacket )
				{
					TotalDataSize -= (*CombinedPacket)->StatMessages.GetAllocatedSize();
					(*CombinedPacket)->StatMessages += StatPacket->StatMessages;
					TotalDataSize += (*CombinedPacket)->StatMessages.GetAllocatedSize();

					delete StatPacket;
				}
				else
				{
					Frame.Packets.Add( StatPacket );
					TotalDataSize += PacketSize;
				}

				const double CurrentSeconds = FPlatformTime::Seconds();
				if( CurrentSeconds > PreviousSeconds + NumSecondsBetweenLogs )
				{
					const int32 PctPos = int32( 100.0*FileReader->Tell() / FileSize );
					UE_LOG( LogStats, Log, TEXT( "%3i%% %10llu (%.1f MB) read messages, last read frame %4i" ), PctPos, TotalStatMessagesNum, TotalDataSize / 1024.0f / 1024.0f, StatPacketFrameNum );
					PreviousSeconds = CurrentSeconds;
				}
			
				MaximumPacketSize = FMath::Max( MaximumPacketSize, PacketSize );			
				TotalPacketsNum++;
			}
		}

		// Dump frame stats
		for( const auto& It : CombinedHistory )
		{
			const int64 FrameNum = It.Key;
			int64 FramePacketsSize = 0;
			int64 FrameStatMessages = 0;
			int64 FramePackets = It.Value.Packets.Num(); // Threads
			for( const auto& It2 : It.Value.Packets )
			{
				FramePacketsSize += It2->StatMessages.GetAllocatedSize();
				FrameStatMessages += It2->StatMessages.Num();
			}

			UE_LOG( LogStats, Warning, TEXT( "Frame: %10llu/%3lli Size: %.1f MB / %10lli" ), 
					FrameNum, 
					FramePackets, 
					FramePacketsSize / 1024.0f / 1024.0f,
					FrameStatMessages );
		}

		UE_LOG( LogStats, Warning, TEXT( "TotalPacketSize: %.1f MB, Max: %1f MB" ),
				TotalDataSize / 1024.0f / 1024.0f,
				MaximumPacketSize / 1024.0f / 1024.0f );

		TArray<int64> Frames;
		CombinedHistory.GenerateKeyArray( Frames );
		Frames.Sort();
		const int64 MiddleFrame = Frames[Frames.Num() / 2];

		ProcessMemoryOperations( CombinedHistory );
	}
}


void FStatsMemoryDumpCommand::ProcessMemoryOperations( const TMap<int64, FStatPacketArray>& CombinedHistory )
{
	// This is only example code, no fully implemented, may sometimes crash.
	// This code is not optimized. 
	double PreviousSeconds = FPlatformTime::Seconds();
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

	for( int32 FrameIndex = 0; FrameIndex < Frames.Num(); ++FrameIndex )
	{
		{
			const double CurrentSeconds = FPlatformTime::Seconds();
			if( CurrentSeconds > PreviousSeconds + NumSecondsBetweenLogs )
			{
				UE_LOG( LogStats, Warning, TEXT( "Processing frame %i/%i" ), FrameIndex+1, Frames.Num() );
				PreviousSeconds = CurrentSeconds;
			}
		}

		const int64 TargetFrame = Frames[FrameIndex];
		const int64 Diff = TargetFrame - FirstFrame;
		const FStatPacketArray& Frame = CombinedHistory.FindChecked( TargetFrame );

		bool bAtLeastOnePacket = false;
		for( int32 PacketIndex = 0; PacketIndex < Frame.Packets.Num(); PacketIndex++ )
		{
			{
				const double CurrentSeconds = FPlatformTime::Seconds();
				if( CurrentSeconds > PreviousSeconds + NumSecondsBetweenLogs )
				{
					UE_LOG( LogStats, Log, TEXT( "Processing packet %i/%i" ), PacketIndex, Frame.Packets.Num() );
					PreviousSeconds = CurrentSeconds;
					bAtLeastOnePacket = true;
				}
			}

			const FStatPacket& StatPacket = *Frame.Packets[PacketIndex];
			const FName& ThreadFName = StatsThreadStats.Threads.FindChecked( StatPacket.ThreadId );

			FStackState* StackState = StackStates.Find( ThreadFName );
			if( !StackState )
			{
				StackState = &StackStates.Add( ThreadFName );
				StackState->Stack.Add( ThreadFName );
				StackState->Current = ThreadFName;
			}

			const FStatMessagesArray& Data = StatPacket.StatMessages;

			int32 LastPct = 0;
			const int32 NumDataElements = Data.Num();
			const int32 OnerPercent = FMath::Max( NumDataElements / 100, 1024 );
			bool bAtLeastOneMessage = false;
			for( int32 Index = 0; Index < NumDataElements; Index++ )
			{
				if( Index % OnerPercent )
				{
					const double CurrentSeconds = FPlatformTime::Seconds();
					if( CurrentSeconds > PreviousSeconds + NumSecondsBetweenLogs )
					{
						const int32 CurrentPct = int32( 100.0*(Index + 1) / NumDataElements );
						UE_LOG( LogStats, Log, TEXT( "Processing %3i%% (%i/%i) stat messages" ), CurrentPct, Index, NumDataElements );
						PreviousSeconds = CurrentSeconds;
						bAtLeastOneMessage = true;
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
							Index++;
							const FStatMessage& AllocSizeMessage = Data[Index];
							const int64 AllocSize = AllocSizeMessage.GetValue_int64();

							// Read OperationSequenceTag.
							Index++;
							const FStatMessage& SequenceTagMessage = Data[Index];
							const uint32 SequenceTag = SequenceTagMessage.GetValue_int64();

							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_AllocPtr ), (uint64)(UPTRINT)Ptr | (uint64)EMemoryOperation::Alloc );
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_AllocSize ), Size );
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_OperationSequenceTag ), (int64)SequenceTag );

							// Create a callstack.
							TArray<FName> StatsBasedCallstack;
							for (const auto& StackName : StackState->Stack)
							{
								StatsBasedCallstack.Add( StackName );
							}

							// Add a new allocation.
							SequenceAllocationArray.Add(
								FAllocationInfo(
								0,
								Ptr,
								AllocSize,
								StatsBasedCallstack,
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
							Index++;
							const FStatMessage& AllocPtrMessage = Data[Index];
							const uint64 NewPtr = AllocPtrMessage.GetValue_Ptr() & ~(uint64)EMemoryOperation::Mask;

							// After AllocPtr message there is always alloc size message and the sequence tag.
							Index++;
							const FStatMessage& ReallocSizeMessage = Data[Index];
							const int64 ReallocSize = ReallocSizeMessage.GetValue_int64();

							// Read OperationSequenceTag.
							Index++;
							const FStatMessage& SequenceTagMessage = Data[Index];
							const uint32 SequenceTag = SequenceTagMessage.GetValue_int64();

							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_FreePtr ), (uint64)(UPTRINT)OldPtr | (uint64)EMemoryOperation::Realloc );
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_AllocPtr ), (uint64)(UPTRINT)NewPtr | (uint64)EMemoryOperation::Realloc );
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_AllocSize ), NewSize );
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_OperationSequenceTag ), (int64)SequenceTag );

							// Create a callstack.
							TArray<FName> StatsBasedCallstack;
							for (const auto& StackName : StackState->Stack)
							{
								StatsBasedCallstack.Add( StackName );
							}

							// Add a new realloc.
							SequenceAllocationArray.Add(
								FAllocationInfo(
								OldPtr, 
								NewPtr,
								ReallocSize,
								StatsBasedCallstack,
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
							Index++;
							const FStatMessage& SequenceTagMessage = Data[Index];
							const uint32 SequenceTag = SequenceTagMessage.GetValue_int64();

							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_FreePtr ), (uint64)(UPTRINT)Ptr | (uint64)EMemoryOperation::Free );	// 16 bytes total				
							//ThreadStats->AddMemoryMessage( GET_STATFNAME( STAT_Memory_OperationSequenceTag ), (int64)SequenceTag );

							// Create a callstack.
							TArray<FName> StatsBasedCallstack;
							for (const auto& StackName : StackState->Stack)
							{
								StatsBasedCallstack.Add( StackName );
							}

							// Add a new free.
							SequenceAllocationArray.Add(
								FAllocationInfo(
								0,
								Ptr,		
								0,
								StatsBasedCallstack,					
								SequenceTag,
								EMemoryOperation::Free,
								StackState->bIsBrokenCallstack
								) );
							//LastSequenceTagForNamedMarker = SequenceTag;????
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
			if( bAtLeastOneMessage )
			{
				PreviousSeconds -= NumSecondsBetweenLogs;
			}
		}
		if( bAtLeastOnePacket )
		{
			PreviousSeconds -= NumSecondsBetweenLogs;
		}
	}

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

	// Sort all memory operation by the sequence tag, iterate through all operation and generate memory usage.
	SequenceAllocationArray.Sort( TLess<FAllocationInfo>() );

	// Named markers/snapshots

	// Alive allocations.
	TMap<uint64, FAllocationInfo> AllocationMap;
	TMultiMap<uint64, FAllocationInfo> FreeWithoutAllocMap;
	TMultiMap<uint64, FAllocationInfo> DuplicatedAllocMap;
	int32 NumDuplicatedMemoryOperations = 0;
	int32 NumFWAMemoryOperations = 0; // FreeWithoutAlloc
	int32 NumZeroAllocs = 0; // Malloc(0)

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
				const int32 CurrentPct = int32( 100.0*(Index + 1) / NumSequenceAllocations );
				UE_LOG( LogStats, Log, TEXT( "Processing allocations %3i%% (%10i/%10i)" ), CurrentPct, Index + 1, NumSequenceAllocations );
				PreviousSeconds = CurrentSeconds;
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
					const FString FoundAndFreedCallstack = GetCallstack( FoundAndFreed->EncodedCallstack );
				}

				if( FoundAndAllocated )
				{
					const FString FoundAndAllocatedCallstack = GetCallstack( FoundAndAllocated->EncodedCallstack );
				}

				NumDuplicatedMemoryOperations++;


				const FString FoundCallstack = GetCallstack( Found->EncodedCallstack );
				const FString AllocCallstack = GetCallstack( Alloc.EncodedCallstack );
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
				const FString ReallocCallstack = GetCallstack( Alloc.EncodedCallstack );
				UE_LOG( LogStats, Warning, TEXT( "ReallocWithoutAlloc: %s %i %i/%i [%i]" ), *ReallocCallstack, Alloc.Size, Alloc.OldPtr, Alloc.Ptr, Alloc.SequenceTag );
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
				const FString FWACallstack = GetCallstack( Alloc.EncodedCallstack );
				UE_LOG( LogStats, Warning, TEXT( "FreeWithoutAllocCallstack: %s %i" ), *FWACallstack, Alloc.Ptr );
#endif // UE_BUILD_DEBUG
			}
		}
	}

	auto EndSnapshot = SnapshotsToBeProcessed[0];
	SnapshotsToBeProcessed.RemoveAt( 0 );
	PrepareSnapshot( EndSnapshot.Value, AllocationMap );

	UE_LOG( LogStats, Warning, TEXT( "NumDuplicatedMemoryOperations: %i" ), NumDuplicatedMemoryOperations );
	UE_LOG( LogStats, Warning, TEXT( "NumFWAMemoryOperations:        %i" ), NumFWAMemoryOperations );
	UE_LOG( LogStats, Warning, TEXT( "NumZeroAllocs:                 %i" ), NumZeroAllocs );

	// Dump problematic allocations
	DuplicatedAllocMap.ValueSort( FAllocationInfoGreater() );
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
		const FString AllocCallstack = GetCallstack( Alloc.EncodedCallstack );
		UE_LOG( LogStats, Log, TEXT( "%lli (%.2f MB) %s" ), Alloc.Size, Alloc.Size / 1024.0f / 1024.0f, *AllocCallstack );

		DisplayedSoFar += Alloc.Size;

		const float CurrentPct = (float)DisplayedSoFar / (float)TotalDuplicatedMemory;
		if( CurrentPct > MaxPctDisplayed )
		{
			break;
		}
	}

	// Frame-240 Frame-120 Frame-060
	TMap<FString, FSizeAndCount> FrameBegin_Exit;
	CompareSnapshots_FString( TEXT( "BeginSnapshot" ), TEXT( "EngineLoop.Exit" ), FrameBegin_Exit );
	DumpScopedAllocations( TEXT( "FrameBegin_Exit" ), FrameBegin_Exit );

	TMap<FString, FSizeAndCount> Frame060_120;
	CompareSnapshots_FString( TEXT( "Frame-060" ), TEXT( "Frame-120" ), Frame060_120 );
	DumpScopedAllocations( TEXT( "Frame060_120" ), Frame060_120 );

	TMap<FString, FSizeAndCount> Frame060_240;
	CompareSnapshots_FString( TEXT( "Frame-060" ), TEXT( "Frame-240" ), Frame060_240 );
	DumpScopedAllocations( TEXT( "Frame060_240" ), Frame060_240 );

	// For snapshot?
	//GenerateMemoryUsageReport( AllocationMap );
}

void FStatsMemoryDumpCommand::GenerateMemoryUsageReport( const TMap<uint64, FAllocationInfo>& AllocationMap )
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

	const FString ReportName = FString::Printf( TEXT( "%s-Memory-Scoped" ), *Stream.Header.PlatformName );
	FDiagnosticTableViewer MemoryReport( *FDiagnosticTableViewer::GetUniqueTemporaryFilePath( *ReportName ), true );

	// Write a row of headings for the table's columns.
	MemoryReport.AddColumn( TEXT( "Size (bytes)" ) );
	MemoryReport.AddColumn( TEXT( "Size (MB)" ) );
	MemoryReport.AddColumn( TEXT( "Count" ) );
	MemoryReport.AddColumn( TEXT( "Callstack" ) );
	MemoryReport.CycleRow();

	TMap<FName, FSizeAndCount> ScopedAllocations;
	uint64 TotalAllocatedMemory = 0;
	uint64 NumAllocations = 0;
	GenerateScopedAllocations( AllocationMap, ScopedAllocations, TotalAllocatedMemory, NumAllocations );

	// Dump memory to the log.
	ScopedAllocations.ValueSort( FSizeAndCountGreater() );

	const float MaxPctDisplayed = 0.90f;
	int32 CurrentIndex = 0;
	uint64 DisplayedSoFar = 0;
	UE_LOG( LogStats, Warning, TEXT( "Index, Size (Size MB), Count, Stat desc" ) );
	for( const auto& It : ScopedAllocations )
	{
		const FSizeAndCount& SizeAndCount = It.Value;
		const FName& EncodedCallstack = It.Key;

		const FString AllocCallstack = GetCallstack( EncodedCallstack );

		UE_LOG( LogStats, Log, TEXT( "%2i, %llu (%.2f MB), %llu, %s" ),
				CurrentIndex,
				SizeAndCount.Size,
				SizeAndCount.Size / 1024.0f / 1024.0f,
				SizeAndCount.Count,
				*AllocCallstack );

		// Dump stats
		MemoryReport.AddColumn( TEXT( "%llu" ), SizeAndCount.Size );
		MemoryReport.AddColumn( TEXT( "%.2f MB" ), SizeAndCount.Size / 1024.0f / 1024.0f );
		MemoryReport.AddColumn( TEXT( "%llu" ), SizeAndCount.Count );
		MemoryReport.AddColumn( *AllocCallstack );
		MemoryReport.CycleRow();

		CurrentIndex++;
		DisplayedSoFar += SizeAndCount.Size;

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

void FStatsMemoryDumpCommand::ProcessAndDumpUObjectAllocations( const TMap<uint64, FAllocationInfo>& AllocationMap )
{
	// This code is not optimized. 
	FScopeLogTime SLT( TEXT( "ProcessingUObjectAllocations" ), nullptr, FScopeLogTime::ScopeLog_Seconds );
	UE_LOG( LogStats, Warning, TEXT( "Processing UObject allocations" ) );

	const FString ReportName = FString::Printf( TEXT("%s-Memory-UObject"), *Stream.Header.PlatformName );
	FDiagnosticTableViewer MemoryReport( *FDiagnosticTableViewer::GetUniqueTemporaryFilePath( *ReportName ), true );

	// Write a row of headings for the table's columns.
	MemoryReport.AddColumn( TEXT( "Size (bytes)" ) );
	MemoryReport.AddColumn( TEXT( "Size (MB)" ) );
	MemoryReport.AddColumn( TEXT( "Count" ) );
	MemoryReport.AddColumn( TEXT( "UObject class" ) );
	MemoryReport.CycleRow();

	TMap<FName, FSizeAndCount> UObjectAllocations;

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
			TArray<FString> DecodedCallstack;
			DecodeCallstack( Alloc.EncodedCallstack, DecodedCallstack );

			for( int32 Index = DecodedCallstack.Num() - 1; Index >= 0; --Index )
			{
				NAME_INDEX NameIndex = 0;
				TTypeFromString<NAME_INDEX>::FromString( NameIndex, *DecodedCallstack[Index] );

				const FName LongName = FName( NameIndex, NameIndex, 0 );
				const bool bValid = UObjectNames.Contains( LongName );
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
			FSizeAndCount& SizeAndCount = UObjectAllocations.FindOrAdd( UObjectClass );
			SizeAndCount.AddAllocation( Alloc.Size );

			TotalAllocatedMemory += Alloc.Size;
			NumAllocations++;
		}
	}

	// Dump memory to the log.
	UObjectAllocations.ValueSort( FSizeAndCountGreater() );

	const float MaxPctDisplayed = 0.90f;
	int32 CurrentIndex = 0;
	uint64 DisplayedSoFar = 0;
	UE_LOG( LogStats, Warning, TEXT( "Index, Size (Size MB), Count, UObject class" ) );
	for( const auto& It : UObjectAllocations )
	{
		const FSizeAndCount& SizeAndCount = It.Value;
		const FName& UObjectClass = It.Key;

		UE_LOG( LogStats, Log, TEXT( "%2i, %llu (%.2f MB), %llu, %s" ),
				CurrentIndex,
				SizeAndCount.Size,
				SizeAndCount.Size / 1024.0f / 1024.0f,
				SizeAndCount.Count,
				*UObjectClass.GetPlainNameString() );

		// Dump stats
		MemoryReport.AddColumn( TEXT( "%llu" ), SizeAndCount.Size );
		MemoryReport.AddColumn( TEXT( "%.2f MB" ), SizeAndCount.Size / 1024.0f / 1024.0f );
		MemoryReport.AddColumn( TEXT( "%llu" ), SizeAndCount.Count );
		MemoryReport.AddColumn( *UObjectClass.GetPlainNameString() );
		MemoryReport.CycleRow();

		CurrentIndex++;
		DisplayedSoFar += SizeAndCount.Size;

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

void FStatsMemoryDumpCommand::DumpScopedAllocations( const TCHAR* Name, const TMap<FString, FSizeAndCount>& ScopedAllocations )
{
	// This code is not optimized. 
	FScopeLogTime SLT( TEXT( "ProcessingScopedAllocations" ), nullptr, FScopeLogTime::ScopeLog_Seconds );
	UE_LOG( LogStats, Warning, TEXT( "Processing scoped allocations" ) );

	const FString ReportName = FString::Printf( TEXT( "%s-Memory-Scoped-%s" ), *Stream.Header.PlatformName, Name );
	FDiagnosticTableViewer MemoryReport( *FDiagnosticTableViewer::GetUniqueTemporaryFilePath( *ReportName ), true );

	// Write a row of headings for the table's columns.
	MemoryReport.AddColumn( TEXT( "Size (bytes)" ) );
	MemoryReport.AddColumn( TEXT( "Size (MB)" ) );
	MemoryReport.AddColumn( TEXT( "Count" ) );
	MemoryReport.AddColumn( TEXT( "Callstack" ) );
	MemoryReport.CycleRow();

	
	FSizeAndCount Total;

	const float MaxPctDisplayed = 0.90f;
	int32 CurrentIndex = 0;
	UE_LOG( LogStats, Warning, TEXT( "Index, Size (Size MB), Count, Stat desc" ) );
	for (const auto& It : ScopedAllocations)
	{
		const FSizeAndCount& SizeAndCount = It.Value;
		//const FName& EncodedCallstack = It.Key;
		const FString AllocCallstack = It.Key;// GetCallstack( EncodedCallstack );

		UE_LOG( LogStats, Log, TEXT( "%2i, %llu (%.2f MB), %llu, %s" ),
				CurrentIndex,
				SizeAndCount.Size,
				SizeAndCount.Size / 1024.0f / 1024.0f,
				SizeAndCount.Count,
				*AllocCallstack );

		// Dump stats
		MemoryReport.AddColumn( TEXT( "%llu" ), SizeAndCount.Size );
		MemoryReport.AddColumn( TEXT( "%.2f MB" ), SizeAndCount.Size / 1024.0f / 1024.0f );
		MemoryReport.AddColumn( TEXT( "%llu" ), SizeAndCount.Count );
		MemoryReport.AddColumn( *AllocCallstack );
		MemoryReport.CycleRow();

		CurrentIndex++;
		Total += SizeAndCount;
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

void FStatsMemoryDumpCommand::GenerateScopedAllocations( const TMap<uint64, FAllocationInfo>& AllocationMap, TMap<FName, FSizeAndCount>& out_ScopedAllocations, uint64& TotalAllocatedMemory, uint64& NumAllocations )
{
	FScopeLogTime SLT( TEXT( "GenerateScopedAllocations" ), nullptr, FScopeLogTime::ScopeLog_Milliseconds );

	for (const auto& It : AllocationMap)
	{
		const FAllocationInfo& Alloc = It.Value;
		FSizeAndCount& SizeAndCount = out_ScopedAllocations.FindOrAdd( Alloc.EncodedCallstack );
		SizeAndCount.AddAllocation( Alloc.Size );

		TotalAllocatedMemory += Alloc.Size;
		NumAllocations++;
	}

	// Sort by size.
	out_ScopedAllocations.ValueSort( FSizeAndCountGreater() );
}

void FStatsMemoryDumpCommand::PrepareSnapshot( const FName SnapshotName, const TMap<uint64, FAllocationInfo>& AllocationMap )
{
	FScopeLogTime SLT( TEXT( "PrepareSnapshot" ), nullptr, FScopeLogTime::ScopeLog_Milliseconds );

	SnapshotsWithAllocationMap.Add( SnapshotName, AllocationMap );

	TMap<FName, FSizeAndCount> SnapshotScopedAllocations;
	uint64 TotalAllocatedMemory = 0;
	uint64 NumAllocations = 0;
	GenerateScopedAllocations( AllocationMap, SnapshotScopedAllocations, TotalAllocatedMemory, NumAllocations );
	SnapshotsWithScopedAllocations.Add( SnapshotName, SnapshotScopedAllocations );

	// Decode callstacks.
	// Replace encoded callstacks with human readable name. For easier debugging.
	TMap<FString, FSizeAndCount> SnapshotDecodedScopedAllocations;
	for (auto& It : SnapshotScopedAllocations)
	{
		const FString DecodedCallstack = GetCallstack( It.Key );
		SnapshotDecodedScopedAllocations.Add( DecodedCallstack, It.Value );
	}
	SnapshotsWithDecodedScopedAllocations.Add( SnapshotName, SnapshotDecodedScopedAllocations );

	UE_LOG( LogStats, Warning, TEXT( "PrepareSnapshot: %s Num: %i/%i Total: %.2f MB / %llu" ), *SnapshotName.GetPlainNameString(), AllocationMap.Num(), SnapshotScopedAllocations.Num(), TotalAllocatedMemory / 1024.0f / 1024.0f, NumAllocations );
}

void FStatsMemoryDumpCommand::CompareSnapshots_FName( const FName BeginSnaphotName, const FName EndSnaphotName, TMap<FName, FSizeAndCount>& out_Result )
{
	const auto BeginSnaphotPtr = SnapshotsWithScopedAllocations.Find( BeginSnaphotName );
	const auto EndSnapshotPtr = SnapshotsWithScopedAllocations.Find( EndSnaphotName );
	if (BeginSnaphotPtr && EndSnapshotPtr)
	{
		// Process data.
		TMap<FName, FSizeAndCount> BeginSnaphot = *BeginSnaphotPtr;
		TMap<FName, FSizeAndCount> EndSnaphot = *EndSnapshotPtr;
		TMap<FName, FSizeAndCount> Result;

		for (const auto& It : EndSnaphot)
		{
			const FName Callstack = It.Key;
			const FSizeAndCount EndScopedAlloc = It.Value;

			const FSizeAndCount* BeginScopedAllocPtr = BeginSnaphot.Find( Callstack );
			if (BeginSnaphotPtr)
			{
				FSizeAndCount SizeAndCount;
				SizeAndCount += EndScopedAlloc;
				SizeAndCount -= *BeginScopedAllocPtr;

				if (SizeAndCount.IsAlive())
				{
					Result.Add( Callstack, SizeAndCount );
				}
			}
		}

		// Sort by size.
		out_Result.ValueSort( FSizeAndCountGreater() );
	}
}

void FStatsMemoryDumpCommand::CompareSnapshots_FString( const FName BeginSnaphotName, const FName EndSnaphotName, TMap<FString, FSizeAndCount>& out_Result )
{
	const auto BeginSnaphotPtr = SnapshotsWithDecodedScopedAllocations.Find( BeginSnaphotName );
	const auto EndSnapshotPtr = SnapshotsWithDecodedScopedAllocations.Find( EndSnaphotName );
	if (BeginSnaphotPtr && EndSnapshotPtr)
	{
		// Process data.
		TMap<FString, FSizeAndCount> BeginSnaphot = *BeginSnaphotPtr;
		TMap<FString, FSizeAndCount> EndSnaphot = *EndSnapshotPtr;

		for (const auto& It : EndSnaphot)
		{
			const FString& Callstack = It.Key;
			const FSizeAndCount EndScopedAlloc = It.Value;

			const FSizeAndCount* BeginScopedAllocPtr = BeginSnaphot.Find( Callstack );
			if (BeginScopedAllocPtr)
			{
				FSizeAndCount SizeAndCount;
				SizeAndCount += EndScopedAlloc;
				SizeAndCount -= *BeginScopedAllocPtr;

				if (SizeAndCount.IsAlive())
				{
					out_Result.Add( Callstack, SizeAndCount );
				}
			}
			else
			{
				out_Result.Add( Callstack, EndScopedAlloc );
			}
		}

		// Sort by size.
		out_Result.ValueSort( FSizeAndCountGreater() );
	}
}

