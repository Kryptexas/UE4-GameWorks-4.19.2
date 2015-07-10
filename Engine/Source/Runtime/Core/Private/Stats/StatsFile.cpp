// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StatsFile.cpp: Implements stats file related functionality.
=============================================================================*/

#include "CorePrivatePCH.h"

#if	STATS

#include "StatsData.h"
#include "StatsFile.h"
#include "ScopeExit.h"

DECLARE_CYCLE_STAT( TEXT( "Stream File" ), STAT_StreamFile, STATGROUP_StatSystem );
DECLARE_CYCLE_STAT( TEXT( "Wait For Write" ), STAT_StreamFileWaitForWrite, STATGROUP_StatSystem );

/*-----------------------------------------------------------------------------
	FAsyncStatsWrite
-----------------------------------------------------------------------------*/

/**
*	Helper class used to save the capture stats data via the background thread.
*	!!CAUTION!! Can exist only one instance at the same time. Synchronized via EnsureCompletion.
*/
class FAsyncStatsWrite : public FNonAbandonableTask
{
public:
	/**
	 *	Pointer to the instance of the stats write file.
	 *	Generally speaking accessing this pointer by a different thread is not thread-safe.
	 *	But in this specific case it is.
	 *	@see SendTask
	 */
	IStatsWriteFile* Outer;

	/** Data for the file. Moved via Exchange. */
	TArray<uint8> Data;

	/** Constructor. */
	FAsyncStatsWrite( IStatsWriteFile* InStatsWriteFile )
		: Outer( InStatsWriteFile )
	{
		Exchange( Data, InStatsWriteFile->OutData );
	}

	/** Write compressed data to the file. */
	void DoWork()
	{
		check( Data.Num() );
		FArchive& Ar = *Outer->File;

		// Seek to the end of the file.
		Ar.Seek( Ar.TotalSize() );
		const int64 FrameFileOffset = Ar.Tell();

		FCompressedStatsData CompressedData( Data, Outer->CompressedData );
		Ar << CompressedData;

		Outer->FinalizeSavingData(FrameFileOffset);
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT( FAsyncStatsWrite, STATGROUP_ThreadPoolAsyncTasks );
	}
};

/*-----------------------------------------------------------------------------
	FStatsThreadState
-----------------------------------------------------------------------------*/

FStatsThreadState::FStatsThreadState( FString const& Filename )
	: HistoryFrames( MAX_int32 )
	, LastFullFrameMetaAndNonFrame( -1 )
	, LastFullFrameProcessed( -1 )
	, TotalNumStatMessages( 0 )
	, MaxNumStatMessages( 0 )
	, bFindMemoryExtensiveStats( false )
	, CurrentGameFrame( -1 )
	, CurrentRenderFrame( -1 )	
	, MaxFrameSeen( -1 )
	, MinFrameSeen( -1 )
	, bWasLoaded( true )
{
	const int64 Size = IFileManager::Get().FileSize( *Filename );
	if( Size < 4 )
	{
		UE_LOG( LogStats, Error, TEXT( "Could not open: %s" ), *Filename );
		return;
	}
	TAutoPtr<FArchive> FileReader( IFileManager::Get().CreateFileReader( *Filename ) );
	if( !FileReader )
	{
		UE_LOG( LogStats, Error, TEXT( "Could not open: %s" ), *Filename );
		return;
	}

	FStatsReadStream Stream;
	if( !Stream.ReadHeader( *FileReader ) )
	{
		UE_LOG( LogStats, Error, TEXT( "Could not open, bad magic: %s" ), *Filename );
		return;
	}

	// Test version only works for the finalized stats files.
	const bool bIsFinalized = Stream.Header.IsFinalized();
	check( bIsFinalized );
	
	TArray<FStatMessage> Messages;
	if( Stream.Header.bRawStatsFile )
	{
		int32 NumReadStatPackets = 0;

		// Read metadata.
		TArray<FStatMessage> MetadataMessages;
		Stream.ReadFNamesAndMetadataMessages( *FileReader, MetadataMessages );
		ProcessMetaDataForLoad( MetadataMessages );

		// Read the raw stats messages.
		const bool bHasCompressedData = Stream.Header.HasCompressedData();
		check(bHasCompressedData);

		// Buffer used to store the compressed and decompressed data.
		TArray<uint8> SrcData;
		TArray<uint8> DestData;
		FCompressedStatsData UncompressedData( SrcData, DestData );
		while( true )
		{
			// Read the compressed data.		
			*FileReader << UncompressedData;

			if( UncompressedData.HasReachedEndOfCompressedData() )
			{
				// EOF
				break;
		}

			// Select the proper archive.
			FMemoryReader MemoryReader( DestData, true );
			FArchive& Archive = bHasCompressedData ? MemoryReader : *FileReader;

			FStatPacket StatPacket;
			Stream.ReadStatPacket( MemoryReader, StatPacket );
			// Process the raw stat messages.
			NumReadStatPackets++;
			}
	
		UE_LOG( LogStats, Log, TEXT( "File: %s has %i stat packets" ), *Filename, NumReadStatPackets );
	}
	else
	{
		// Read the condensed stats messages.
		while( FileReader->Tell() < Size )
		{
			FStatMessage Read( Stream.ReadMessage( *FileReader ) );
			if( Read.NameAndInfo.GetField<EStatOperation>() == EStatOperation::SpecialMessageMarker )
			{
				// Simply break the loop.
				// The profiler supports more advanced handling of this message.
				break;
			}
			else if( Read.NameAndInfo.GetField<EStatOperation>() == EStatOperation::AdvanceFrameEventGameThread )
			{
				ProcessMetaDataForLoad( Messages );
				if( CurrentGameFrame > 0 && Messages.Num() )
				{
					check( !CondensedStackHistory.Contains( CurrentGameFrame ) );
					TArray<FStatMessage>* Save = new TArray<FStatMessage>();
					Exchange( *Save, Messages );
					CondensedStackHistory.Add( CurrentGameFrame, Save );
					GoodFrames.Add( CurrentGameFrame );
				}
			}

			new (Messages)FStatMessage( Read );
		}
		// meh, we will discard the last frame, but we will look for meta data
	}
}

void FStatsThreadState::AddMessages( TArray<FStatMessage>& InMessages )
{
	bWasLoaded = true;
	TArray<FStatMessage> Messages;
	for( int32 Index = 0; Index < InMessages.Num(); ++Index )
	{
		if( InMessages[Index].NameAndInfo.GetField<EStatOperation>() == EStatOperation::AdvanceFrameEventGameThread )
		{
			ProcessMetaDataForLoad( Messages );
			if( !CondensedStackHistory.Contains( CurrentGameFrame ) && Messages.Num() )
			{
				TArray<FStatMessage>* Save = new TArray<FStatMessage>();
				Exchange( *Save, Messages );
				if( CondensedStackHistory.Num() >= HistoryFrames )
				{
					for( auto It = CondensedStackHistory.CreateIterator(); It; ++It )
					{
						delete It.Value();
					}
					CondensedStackHistory.Reset();
				}
				CondensedStackHistory.Add( CurrentGameFrame, Save );
				GoodFrames.Add( CurrentGameFrame );
			}
		}

		new (Messages)FStatMessage( InMessages[Index] );
	}
	bWasLoaded = false;
}

void FStatsThreadState::ProcessMetaDataForLoad(TArray<FStatMessage>& Data)
{
	check(bWasLoaded);
	for (int32 Index = 0; Index < Data.Num() ; Index++)
	{
		FStatMessage& Item = Data[Index];
		EStatOperation::Type Op = Item.NameAndInfo.GetField<EStatOperation>();
		if (Op == EStatOperation::SetLongName)
		{
			FindOrAddMetaData(Item);
		}
		else if (Op == EStatOperation::AdvanceFrameEventGameThread)
		{
			check(Item.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
			if (Item.GetValue_int64() > 0)
			{
				CurrentGameFrame = Item.GetValue_int64();
				if (CurrentGameFrame > MaxFrameSeen)
				{
					MaxFrameSeen = CurrentGameFrame;
				}
				if (MinFrameSeen < 0)
				{
					MinFrameSeen = CurrentGameFrame;
				}
			}
		}
		else if (Op == EStatOperation::AdvanceFrameEventRenderThread)
		{
			check(Item.NameAndInfo.GetField<EStatDataType>() == EStatDataType::ST_int64);
			if (Item.GetValue_int64() > 0)
			{
				CurrentRenderFrame = Item.GetValue_int64();
				if (CurrentGameFrame > MaxFrameSeen)
				{
					MaxFrameSeen = CurrentGameFrame;
				}
				if (MinFrameSeen < 0)
				{
					MinFrameSeen = CurrentGameFrame;
				}
			}
		}
	}
}

/*-----------------------------------------------------------------------------
	IStatsWriteFile
-----------------------------------------------------------------------------*/

IStatsWriteFile::IStatsWriteFile() 
	: File( nullptr )
	, AsyncTask( nullptr )
{
	// Reserve 1MB.
	CompressedData.Reserve( EStatsFileConstants::MAX_COMPRESSED_SIZE );
}

void IStatsWriteFile::Start( const FString& InFilename )
{
	const FString PathName = *(FPaths::ProfilingDir() + TEXT( "UnrealStats/" ));
	const FString Filename = PathName + InFilename;
	const FString Path = FPaths::GetPath( Filename );
	IFileManager::Get().MakeDirectory( *Path, true );

	UE_LOG( LogStats, Log, TEXT( "Opening stats file: %s" ), *Filename );

	File = IFileManager::Get().CreateFileWriter( *Filename );
	if( !File )
	{
		UE_LOG( LogStats, Error, TEXT( "Could not open: %s" ), *Filename );
	}
	else
	{
		ArchiveFilename = Filename;
		WriteHeader();
		SetDataDelegate(true);
		StatsMasterEnableAdd();
	}
}


void IStatsWriteFile::Stop()
{
	if( IsValid() )
	{
		StatsMasterEnableSubtract();
		SetDataDelegate(false);
		SendTask();
		SendTask();
		Finalize();

		File->Close();
		delete File;
		File = nullptr;

		UE_LOG( LogStats, Log, TEXT( "Wrote stats file: %s" ), *ArchiveFilename );
		FCommandStatsFile::Get().LastFileSaved = ArchiveFilename;
	}
}

void IStatsWriteFile::WriteHeader()
{
	FMemoryWriter MemoryWriter( OutData, false, true );
	FArchive& Ar = File ? *File : MemoryWriter;

	uint32 Magic = EStatMagicWithHeader::MAGIC;
	// Serialize magic value.
	Ar << Magic;

	// Serialize dummy header, overwritten in Finalize.
	Header.Version = EStatMagicWithHeader::VERSION_LATEST;
	Header.PlatformName = FPlatformProperties::PlatformName();
	Ar << Header;

	// Serialize metadata.
	WriteMetadata( Ar );
	Ar.Flush();
}

void IStatsWriteFile::WriteMetadata( FArchive& Ar )
{
	const FStatsThreadState& Stats = FStatsThreadState::GetLocalState();
	for( const auto& It : Stats.ShortNameToLongName )
	{
		const FStatMessage& StatMessage = It.Value;
		WriteMessage( Ar, StatMessage );
	}
	}

void IStatsWriteFile::Finalize()
{
	FArchive& Ar = *File;

	// Write dummy compression size, so we can detect the end of the file.
	FCompressedStatsData::WriteEndOfCompressedData( Ar );

	// Real header, written at start of the file, but written out right before we close the file.

	// Write out frame table and update header with offset and count.
	Header.FrameTableOffset = Ar.Tell();
	// This is ok to access the frames info, the async write thread is dead.
	Ar << FramesInfo;

	const FStatsThreadState& Stats = FStatsThreadState::GetLocalState();

	// Add FNames from the stats metadata.
	for( const auto& It : Stats.ShortNameToLongName )
	{
		const FStatMessage& StatMessage = It.Value;
		FNamesSent.Add( StatMessage.NameAndInfo.GetRawName().GetComparisonIndex() );
	}

	// Create a copy of names.
	TSet<int32> FNamesToSent = FNamesSent;
	FNamesSent.Empty( FNamesSent.Num() );

	// Serialize FNames.
	Header.FNameTableOffset = Ar.Tell();
	Header.NumFNames = FNamesToSent.Num();
	for( const int32 It : FNamesToSent )
	{
		WriteFName( Ar, FStatNameAndInfo(FName(It, It, 0),false) );
	}

	// Serialize metadata messages.
	Header.MetadataMessagesOffset = Ar.Tell();
	Header.NumMetadataMessages = Stats.ShortNameToLongName.Num();
	WriteMetadata( Ar );

	// Verify data.
	TSet<int32> BMinA = FNamesSent.Difference( FNamesToSent );
	struct FLocal
	{
		static TArray<FName> GetFNameArray( const TSet<int32>& NameIndices )
		{
			TArray<FName> Result;
			for( const int32 NameIndex : NameIndices )
			{
				new(Result) FName( NameIndex, NameIndex, 0 );
			}
			return Result;
		}
	};
	auto BMinANames = FLocal::GetFNameArray( BMinA );

	// Seek to the position just after a magic value of the file and write out proper header.
	Ar.Seek( sizeof(uint32) );
	Ar << Header;
}

void IStatsWriteFile::SendTask()
{
	if( AsyncTask )
	{
		SCOPE_CYCLE_COUNTER( STAT_StreamFileWaitForWrite );
		AsyncTask->EnsureCompletion();
		delete AsyncTask;
		AsyncTask = nullptr;
	}
	if( OutData.Num() )
	{
		AsyncTask = new FAsyncTask<FAsyncStatsWrite>( this );
		check( !OutData.Num() );
		//check( !ThreadCycles.Num() );
		AsyncTask->StartBackgroundTask();
	}
}

/*-----------------------------------------------------------------------------
	FStatsWriteFile
-----------------------------------------------------------------------------*/


void FStatsWriteFile::SetDataDelegate( bool bSet )
{
	FStatsThreadState const& Stats = FStatsThreadState::GetLocalState();
	if (bSet)
	{
		DataDelegateHandle = Stats.NewFrameDelegate.AddRaw( this, &FStatsWriteFile::WriteFrame );
	}
	else
	{
		Stats.NewFrameDelegate.Remove( DataDelegateHandle );
	}
}


void FStatsWriteFile::WriteFrame( int64 TargetFrame, bool bNeedFullMetadata )
{
	// #YRX_STATS: 2015-06-17 Add stat startfile -num=number of frames to capture

	SCOPE_CYCLE_COUNTER( STAT_StreamFile );

	FMemoryWriter Ar( OutData, false, true );

	if( bNeedFullMetadata )
	{
		WriteMetadata( Ar );
	}

	FStatsThreadState const& Stats = FStatsThreadState::GetLocalState();
	TArray<FStatMessage> const& Data = Stats.GetCondensedHistory( TargetFrame );
	for( auto It = Data.CreateConstIterator(); It; ++It )
	{
		WriteMessage( Ar, *It );
	}

	// Get cycles for all threads, so we can use that data to generate the mini-view.
	for( auto It = Stats.Threads.CreateConstIterator(); It; ++It )
	{
		const int64 Cycles = Stats.GetFastThreadFrameTime( TargetFrame, It.Key() );
		ThreadCycles.Add( It.Key(), Cycles );
	}
	
	// Serialize thread cycles. Disabled for now.
	//Ar << ThreadCycles;
}

void FStatsWriteFile::FinalizeSavingData( int64 FrameFileOffset )
{
	// Called from the async write thread.
	FramesInfo.Add( FStatsFrameInfo( FrameFileOffset, ThreadCycles ) );
}

/*-----------------------------------------------------------------------------
	FRawStatsWriteFile
-----------------------------------------------------------------------------*/

void FRawStatsWriteFile::SetDataDelegate( bool bSet )
{
	FStatsThreadState const& Stats = FStatsThreadState::GetLocalState();
	if( bSet )
	{
		DataDelegateHandle = Stats.NewRawStatPacket.AddRaw( this, &FRawStatsWriteFile::WriteRawStatPacket );
		if( !bWrittenOffsetToData )
		{
			const int64 FrameFileOffset = File->Tell();
			FramesInfo.Add( FStatsFrameInfo( FrameFileOffset ) );
			bWrittenOffsetToData = true;
		}
	}
	else
	{
		Stats.NewRawStatPacket.Remove( DataDelegateHandle );
	}
}

void FRawStatsWriteFile::WriteRawStatPacket( const FStatPacket* StatPacket )
{
	FMemoryWriter Ar( OutData, false, true );

	// Write stat packet.
	WriteStatPacket( Ar, (FStatPacket&)*StatPacket );
	SendTask();
}

void FRawStatsWriteFile::WriteStatPacket( FArchive& Ar, FStatPacket& StatPacket )
{
	Ar << StatPacket.Frame;
	Ar << StatPacket.ThreadId;
	int32 MyThreadType = (int32)StatPacket.ThreadType;
	Ar << MyThreadType;

	Ar << StatPacket.bBrokenCallstacks;
	// We must handle stat messages in a different way.
	int32 NumMessages = StatPacket.StatMessages.Num();
	Ar << NumMessages;
	for( int32 MessageIndex = 0; MessageIndex < NumMessages; ++MessageIndex )
	{
		WriteMessage( Ar, StatPacket.StatMessages[MessageIndex] );
	}
}

/*-----------------------------------------------------------------------------
	FAsyncRawStatsReadFile
-----------------------------------------------------------------------------*/

FAsyncRawStatsFile::FAsyncRawStatsFile( FStatsReadFile* InOwner )
	: Owner( InOwner )
{}

void FAsyncRawStatsFile::DoWork()
{
	Owner->ReadAndProcessSynchronously();
}

void FAsyncRawStatsFile::Abandon()
{
	Owner->RequestStop();
}

/*-----------------------------------------------------------------------------
	FStatsReadFile
-----------------------------------------------------------------------------*/

const double FStatsReadFile::NumSecondsBetweenUpdates = 5.0;

FStatsReadFile* FStatsReadFile::CreateReaderForRegularStats( const TCHAR* Filename, FOnNewStatPacket InNewStatPacket )
{
	FStatsReadFile* StatsReadFile = new FStatsReadFile( Filename, false );
	const bool bValid = StatsReadFile->PrepareLoading();
	if (bValid)
	{
		StatsReadFile->NewStatPacket = InNewStatPacket;
	}
	else
	{
		delete StatsReadFile;
	}
	return bValid ? StatsReadFile : nullptr;
}


FStatsReadFile* FStatsReadFile::CreateReaderForRawStats( const TCHAR* Filename, FOnNewCombinedHistory InNewCombinedHistory )
{
	FStatsReadFile* StatsReadFile = new FStatsReadFile( Filename, true );
	const bool bValid = StatsReadFile->PrepareLoading();
	if (bValid)
	{
		StatsReadFile->NewCombinedHistory = InNewCombinedHistory;
	}
	else
	{
		delete StatsReadFile;
	}
	return bValid ? StatsReadFile : nullptr;
}

void FStatsReadFile::ReadAndProcessSynchronously()
{
	ReadStats();
	ProcessStats();

	if (GetProcessingStage() == EStatsProcessingStage::SPS_Stopped)
	{
		SetProcessingStage( EStatsProcessingStage::SPS_Invalid );
	}
}

void FStatsReadFile::ReadAndProcessAsynchronously()
{
	if (bRawStatsFile)
	{
		AsyncWork = new FAsyncTask<FAsyncRawStatsFile>( this );
		AsyncWork->StartBackgroundTask();
	}
}

void FStatsReadFile::ReadStats()
{
	if (bRawStatsFile)
	{
		const double StartTime = FPlatformTime::Seconds();

		// Buffer used to store the compressed and decompressed data.
		TArray<uint8> SrcArray;
		TArray<uint8> DestArray;

		// Read all packets sequentially, forced by the memory profiler which is now a part of the raw stats.
		// !!CAUTION!! Frame number in the raw stats is pointless, because it is time/cycles based, not frame based.
		// Background threads usually execute time consuming operations, so the frame number won't be valid.
		// Needs to be combined by the thread and the time, not by the frame number.

		// Update stage progress once per NumSecondsBetweenUpdates(5) seconds to avoid spamming.
		double PreviousSeconds = FPlatformTime::Seconds();
		SetProcessingStage( EStatsProcessingStage::SPS_ReadAndCombinePackets );

		while (Reader->Tell() < Reader->TotalSize())
		{
			// Read the compressed data.
			FCompressedStatsData UncompressedData( SrcArray, DestArray );
			*Reader << UncompressedData;
			if (UncompressedData.HasReachedEndOfCompressedData())
			{
				StageProgress.Set( 100 );
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

			
			if (CombinedPacket)
			{
				(*CombinedPacket)->StatMessages += StatPacket->StatMessages;
				delete StatPacket;
			}
			else
			{
				Frame.Packets.Add( StatPacket );
				FileInfo.MaximumPacketSize = FMath::Max<int64>( FileInfo.MaximumPacketSize, StatPacket->StatMessages.GetAllocatedSize() );
			}

			UpdateReadStageProgress();

			if (bShouldStopProcessing == true)
			{
				SetProcessingStage( EStatsProcessingStage::SPS_Stopped );
				break;
			}

			FileInfo.TotalPacketsNum++;
		}

		if (GetProcessingStage() != EStatsProcessingStage::SPS_Stopped)
		{
			const double TotalTime = FPlatformTime::Seconds() - StartTime;
			UE_LOG( LogStats, Log, TEXT( "Reading took %.2f sec(s)" ), TotalTime );

			UpdateCombinedHistoryStats();
		}
		else
		{
			UE_LOG( LogStats, Warning, TEXT( "Reading stopped, abandoning" ) );
			// Clear all data.
			CombinedHistory.Empty();
		}
	}
}

void FStatsReadFile::ProcessStats()
{
	if (bRawStatsFile)
	{
		if (GetProcessingStage() != EStatsProcessingStage::SPS_Stopped)
		{
			SetProcessingStage( EStatsProcessingStage::SPS_ProcessStatMessages );
			NewCombinedHistory.ExecuteIfBound( this );

			if (GetProcessingStage() == EStatsProcessingStage::SPS_Stopped)
			{
				UE_LOG( LogStats, Warning, TEXT( "Processing stopped, abandoning" ) );
			}

			// Clear all data.
			CombinedHistory.Empty();
		}
	}
}


FStatsReadFile::FStatsReadFile( const TCHAR* InFilename, bool bInRawStatsFile )
	: Header( Stream.Header )
	, Reader( nullptr )
	, AsyncWork( nullptr )
	, LastUpdateTime( 0.0 )
	, Filename( InFilename )
	, bRawStatsFile( bInRawStatsFile )
{
	
}

FStatsReadFile::~FStatsReadFile()
{
	RequestStop();

	FPlatformProcess::ConditionalSleep( [&]()
	{
		return !IsBusy();
	}, 1.0f );

	if (AsyncWork)
	{
		check( AsyncWork->IsDone() );

		delete AsyncWork;
		AsyncWork = nullptr;
	}

	delete Reader;
	Reader = nullptr;
}

bool FStatsReadFile::PrepareLoading()
{
	const double StartTime = FPlatformTime::Seconds();

	SetProcessingStage( EStatsProcessingStage::SPS_Started );

	{
		ON_SCOPE_EXIT
		{
			SetProcessingStage( EStatsProcessingStage::SPS_Invalid );
		};

		const int64 Size = IFileManager::Get().FileSize( *Filename );
		if (Size < 4)
		{
			UE_LOG( LogStats, Error, TEXT( "Could not open: %s" ), *Filename );
			return false;
		}

		Reader = IFileManager::Get().CreateFileReader( *Filename );
		if (!Reader)
		{
			UE_LOG( LogStats, Error, TEXT( "Could not open: %s" ), *Filename );
			return false;
		}

		if (!Stream.ReadHeader( *Reader ))
		{
			UE_LOG( LogStats, Error, TEXT( "Could not read, header is invalid: %s" ), *Filename );
			return false;
		}

		//UE_LOG( LogStats, Warning, TEXT( "Reading a raw stats file for memory profiling: %s" ), *Filename );

		const bool bIsFinalized = Stream.Header.IsFinalized();
		if (!bIsFinalized)
		{
			UE_LOG( LogStats, Error, TEXT( "Could not read, file is not finalized: %s" ), *Filename );
			return false;
		}

		if (Stream.Header.Version < EStatMagicWithHeader::VERSION_6)
		{
			UE_LOG( LogStats, Error, TEXT( "Could not read, invalid version: %s, expected %u, was %u" ), *Filename, (uint32)EStatMagicWithHeader::VERSION_6, Stream.Header.Version );
			return false;
		}

		const bool bHasCompressedData = Stream.Header.HasCompressedData();
		if (!bHasCompressedData)
		{
			UE_LOG( LogStats, Error, TEXT( "Could not read, required compressed data: %s" ), *Filename );
			return false;
		}
	}

	State.MarkAsLoaded();

	// Read metadata.
	TArray<FStatMessage> MetadataMessages;
	Stream.ReadFNamesAndMetadataMessages( *Reader, MetadataMessages );
	State.ProcessMetaDataOnly( MetadataMessages );

	// Find all UObject metadata messages.
	for (const auto& Meta : MetadataMessages)
	{
		const FName EncName = Meta.NameAndInfo.GetEncodedName();
		const FName RawName = Meta.NameAndInfo.GetRawName();
		const FString Desc = FStatNameAndInfo::GetShortNameFrom( RawName ).GetPlainNameString();
		const bool bContainsUObject = Desc.Contains( TEXT( "//" ) );
		if (bContainsUObject)
		{
			UObjectRawNames.Add( RawName );
		}
	}

	// Read frames offsets.
	Stream.ReadFramesOffsets( *Reader );

	// Move file pointer to the first frame or first stat packet.
	const int64 FrameOffset0 = Stream.FramesInfo[0].FrameFileOffset;
	Reader->Seek( FrameOffset0 );

	const double TotalTime = FPlatformTime::Seconds() - StartTime;
	UE_LOG( LogStats, Log, TEXT( "Prepare loading took %.2f sec(s)" ), TotalTime );

	return true;
}

void FStatsReadFile::UpdateReadStageProgress()
{
	const double CurrentSeconds = FPlatformTime::Seconds();
	if (CurrentSeconds > LastUpdateTime + NumSecondsBetweenUpdates)
	{
		const int32 PercentagePos = int32( 100.0*Reader->Tell() / Reader->TotalSize() );
		StageProgress.Set( PercentagePos );
		UE_LOG( LogStats, Verbose, TEXT( "%3i%%, current num frames %4i" ), PercentagePos, CombinedHistory.Num() );
		LastUpdateTime = CurrentSeconds;
	}
}

void FStatsReadFile::UpdateCombinedHistoryStats()
{
	// Dump frame stats
	int64 LastFrameNum = 0;
	for (const auto& It : CombinedHistory)
	{
		const int64 FrameNum = It.Key;
		int64 FramePacketsSize = 0;
		int64 FrameStatMessages = 0;
		int64 FramePackets = It.Value.Packets.Num(); // Threads
		for (const auto& It2 : It.Value.Packets)
		{
			FramePacketsSize += It2->StatMessages.GetAllocatedSize();
			FrameStatMessages += It2->StatMessages.Num();
		}

		UE_LOG( LogStats, Verbose, TEXT( "Frame: %4llu/%2lli Size: %5.1f MB / %10lli" ),
				FrameNum,
				FramePackets,
				FramePacketsSize / 1024.0f / 1024.0f,
				FrameStatMessages );

		LastFrameNum = FMath::Max( LastFrameNum, FrameNum );

		FileInfo.TotalStatMessagesNum += FrameStatMessages;
		FileInfo.TotalPacketsSize += FramePacketsSize;
	}

	UE_LOG( LogStats, Warning, TEXT( "Total PacketSize: %6.1f MB, Max: %2f MB, PacketsNum: %lli, StatMessagesNum: %lli, Frames: %i" ),
			FileInfo.TotalPacketsSize / 1024.0f / 1024.0f,
			FileInfo.MaximumPacketSize / 1024.0f / 1024.0f,
			FileInfo.TotalPacketsNum,
			FileInfo.TotalStatMessagesNum,
			CombinedHistory.Num() );

	TArray<int64> Frames;
	CombinedHistory.GenerateKeyArray( Frames );
	Frames.Sort();
	// Verify that frames are sequential.
	check( Frames[Frames.Num() - 1] == LastFrameNum );
}

/*-----------------------------------------------------------------------------
	Commands functionality
-----------------------------------------------------------------------------*/;

FCommandStatsFile& FCommandStatsFile::Get()
{
	static FCommandStatsFile CommandStatsFile;
	return CommandStatsFile;
}


void FCommandStatsFile::Start( const FString& Filename )
{
	Stop();
	CurrentStatsFile = new FStatsWriteFile();
	CurrentStatsFile->Start( Filename );

	StatFileActiveCounter.Increment();
}

void FCommandStatsFile::StartRaw( const FString& Filename )
{
	Stop();
	CurrentStatsFile = new FRawStatsWriteFile();
	CurrentStatsFile->Start( Filename );

	StatFileActiveCounter.Increment();
}

void FCommandStatsFile::Stop()
{
	if (CurrentStatsFile)
	{
		CurrentStatsFile->Stop();
		delete CurrentStatsFile;
		CurrentStatsFile = nullptr;

		StatFileActiveCounter.Decrement();
	}
}

#endif // STATS
