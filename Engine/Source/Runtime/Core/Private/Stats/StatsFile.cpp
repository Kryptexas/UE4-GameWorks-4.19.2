// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StatsFile.cpp: Implements stats file related functionality.
=============================================================================*/

#include "CorePrivate.h"

#if	STATS

#include "StatsData.h"
#include "StatsFile.h"

DECLARE_CYCLE_STAT( TEXT( "Stream File" ), STAT_StreamFile, STATGROUP_StatSystem );
DECLARE_CYCLE_STAT( TEXT( "Wait For Write" ), STAT_StreamFileWaitForWrite, STATGROUP_StatSystem );

/*-----------------------------------------------------------------------------
	FStatsThreadState
-----------------------------------------------------------------------------*/

FStatsThreadState::FStatsThreadState( FString const& Filename )
	: HistoryFrames( MAX_int32 )
	, MaxFrameSeen( -1 )
	, MinFrameSeen( -1 )
	, LastFullFrameMetaAndNonFrame( -1 )
	, LastFullFrameProcessed( -1 )
	, bWasLoaded( true )
	, CurrentGameFrame( -1 )
	, CurrentRenderFrame( -1 )
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
	if( Stream.Header.bRawStatFile )
	{
		const int64 CurrentFilePos = FileReader->Tell();

		// Read metadata.
		TArray<FStatMessage> MetadataMessages;
		Stream.ReadFNamesAndMetadataMessages( *FileReader, MetadataMessages );
		ProcessMetaDataForLoad( MetadataMessages );

		// Read frames offsets.
		Stream.ReadFramesOffsets( *FileReader );

		// Verify frames offsets.
		for( int32 FrameIndex = 0; FrameIndex < Stream.FramesInfo.Num(); ++FrameIndex )
		{
			const int64 FrameOffset = Stream.FramesInfo[FrameIndex].FrameOffset;
			FileReader->Seek( FrameOffset );

			int64 TargetFrame;
			*FileReader << TargetFrame;
		}
		FileReader->Seek( Stream.FramesInfo[0].FrameOffset );

		// Read the raw stats messages.
		FStatPacketArray IncomingData;
		for( int32 FrameIndex = 0; FrameIndex < Stream.FramesInfo.Num(); ++FrameIndex )
		{
			int64 TargetFrame;
			*FileReader << TargetFrame;

			int32 NumPackets;
			*FileReader << NumPackets;

			for( int32 PacketIndex = 0; PacketIndex < NumPackets; PacketIndex++ )
			{
				FStatPacket* ToRead = new FStatPacket();
				Stream.ReadStatPacket( *FileReader, *ToRead, bIsFinalized );
				IncomingData.Packets.Add( ToRead );
			}
	
			FStatPacketArray NowData;
			// This is broken, do not use.
// 			Exchange( NowData.Packets, IncomingData.Packets );
// 			ScanForAdvance( NowData );
// 			AddToHistoryAndEmpty( NowData );
// 			check( !NowData.Packets.Num() );
		}
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

/*-----------------------------------------------------------------------------
	FStatsWriteFile
-----------------------------------------------------------------------------*/

FStatsWriteFile::FStatsWriteFile()
	: Filepos( 0 )
	, File( nullptr )
	, AsyncTask( nullptr )
{}


void FStatsWriteFile::Start( FString const& InFilename )
{
	const FString PathName = *(FPaths::ProfilingDir() + TEXT( "UnrealStats/" ));

	FString Filename = PathName + InFilename;
	FString Path = FPaths::GetPath( Filename );
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
		AddNewFrameDelegate();
		WriteHeader();
		SendTask();
		StatsMasterEnableAdd();
	}
}

void FStatsWriteFile::Stop()
{
	if( IsValid() )
	{
		StatsMasterEnableSubtract();
		RemoveNewFrameDelegate();
		SendTask();
		SendTask();
		Finalize( File );

		File->Close();
		delete File;
		File = nullptr;

		UE_LOG( LogStats, Log, TEXT( "Wrote stats file: %s" ), *ArchiveFilename );
		FCommandStatsFile::LastFileSaved = ArchiveFilename;
	}
}

void FStatsWriteFile::WriteHeader()
{
	FMemoryWriter Ar( OutData, false, true );

	uint32 Magic = EStatMagicWithHeader::MAGIC;
	// Serialize magic value.
	Ar << Magic;

	// Serialize dummy header, overwritten in Finalize.
	Header.Version = EStatMagicWithHeader::VERSION_2;
	Header.PlatformName = FPlatformProperties::PlatformName();
	Header.bRawStatFile = false;
	Ar << Header;

	// Serialize metadata.
	WriteMetadata( Ar );

	Filepos += Ar.Tell();
}

void FStatsWriteFile::WriteFrame( int64 TargetFrame, bool bNeedFullMetadata /*= false*/ )
{
	FMemoryWriter Ar( OutData, false, true );
	const int64 PrevArPos = Ar.Tell();

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
	TMap<uint32, int64> ThreadCycles;
	for( auto It = Stats.Threads.CreateConstIterator(); It; ++It )
	{
		const int64 Cycles = Stats.GetFastThreadFrameTime( TargetFrame, It.Key() );
		ThreadCycles.Add( It.Key(), Cycles );
	}

	// Serialize thread cycles. Disabled for now.
	//Ar << ThreadCycles;

	FramesInfo.Add( FStatsFrameInfo( Filepos, ThreadCycles ) );
	Filepos += Ar.Tell() - PrevArPos;
}

void FStatsWriteFile::Finalize( FArchive* File )
{
	FArchive& Ar = *File;

	// Write special marker message.
	static FStatNameAndInfo Adv( NAME_None, "", TEXT( "" ), EStatDataType::ST_int64, true, false );
	FStatMessage SpecialMessage( Adv.GetEncodedName(), EStatOperation::SpecialMessageMarker, 0LL, false );
	WriteMessage( Ar, SpecialMessage );

	// Real header, written at start of the file, but written out right before we close the file.

	// Write out frame table and update header with offset and count.
	Header.FrameTableOffset = Ar.Tell();
	Ar << FramesInfo;

	const FStatsThreadState& Stats = FStatsThreadState::GetLocalState();

	// Add FNames from the stats metadata.
	for( const auto& It : Stats.ShortNameToLongName )
	{
		const FStatMessage& StatMessage = It.Value;
		FNamesSent.Add( StatMessage.NameAndInfo.GetRawName().GetIndex() );
	}

	// Create a copy of names.
	TSet<int32> FNamesToSent = FNamesSent;
	FNamesSent.Empty( FNamesSent.Num() );

	// Serialize FNames.
	Header.FNameTableOffset = Ar.Tell();
	Header.NumFNames = FNamesToSent.Num();
	for( const int32 It : FNamesToSent )
	{
		WriteFName( Ar, FStatNameAndInfo(FName(EName(It)),false) );
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
				new(Result) FName( EName( NameIndex ) );
			}
			return Result;
		}
	};
	auto BMinANames = FLocal::GetFNameArray( BMinA );

	// Seek to the position just after a magic value of the file and write out proper header.
	Ar.Seek( sizeof(uint32) );
	Ar << Header;
}

void FStatsWriteFile::NewFrame( int64 TargetFrame )
{
	SCOPE_CYCLE_COUNTER( STAT_StreamFile );

	// @TODO yrx 2014-03-24 add -num=frames to stat startfile|startfileraw
#if	0
	if( FCommandStatsFile::FirstFrame == -1 )
	{
		FCommandStatsFile::FirstFrame = TargetFrame;
	}
	else if( TargetFrame > FCommandStatsFile::FirstFrame + 600 )
	{
		if( FCommandStatsFile::CurrentStatFile.IsValid() )
		{
			FCommandStatsFile::CurrentStatFile->Stop();
		}
		FCommandStatsFile::CurrentStatFile = nullptr;
		FCommandStatsFile::FirstFrame = -1;
		return;
	}
#endif // 0

	WriteFrame( TargetFrame );
	if( OutData.Num() > 1024 * 1024 )
	{
		SendTask();
	}
}

void FStatsWriteFile::SendTask( int64 InDataOffset /*= -1*/ )
{
	if( AsyncTask )
	{
		SCOPE_CYCLE_COUNTER( STAT_StreamFileWaitForWrite );
		AsyncTask->EnsureCompletion();
		delete AsyncTask;
		AsyncTask = NULL;
	}
	if( OutData.Num() )
	{
		AsyncTask = new FAsyncTask<FAsyncWriteWorker>( File, &OutData, InDataOffset );
		check( !OutData.Num() );
		AsyncTask->StartBackgroundTask();
	}
}

bool FStatsWriteFile::IsValid() const
{
	return !!File;
}

void FStatsWriteFile::AddNewFrameDelegate()
{
	FStatsThreadState const& Stats = FStatsThreadState::GetLocalState();
	Stats.NewFrameDelegate.AddThreadSafeSP( this->AsShared(), &FStatsWriteFile::NewFrame );
}

void FStatsWriteFile::RemoveNewFrameDelegate()
{
	FStatsThreadState const& Stats = FStatsThreadState::GetLocalState();
	Stats.NewFrameDelegate.RemoveThreadSafeSP( this->AsShared(), &FStatsWriteFile::NewFrame );
}

/*-----------------------------------------------------------------------------
	FRawStatsWriteFile
-----------------------------------------------------------------------------*/

void FRawStatsWriteFile::WriteHeader()
{
	FMemoryWriter Ar( OutData, false, true );

	uint32 Magic = EStatMagicWithHeader::MAGIC;
	// Serialize magic value.
	Ar << Magic;

	// Serialize dummy header, overwritten in Finalize.
	Header.Version = EStatMagicWithHeader::VERSION_2;
	Header.PlatformName = FPlatformProperties::PlatformName();
	Header.bRawStatFile = true;
	Ar << Header;

	// Serialize metadata messages.
	FStatsThreadState const& Stats = FStatsThreadState::GetLocalState();
	int32 NumMetadataMessages = Stats.ShortNameToLongName.Num();
	Ar << NumMetadataMessages;
	WriteMetadata( Ar );

	Filepos += Ar.Tell();
}

void FRawStatsWriteFile::WriteFrame( int64 TargetFrame, bool bNeedFullMetadata /*= false */)
{
	FMemoryWriter Ar( OutData, false, true );
	const int64 PrevArPos = Ar.Tell();
	const FStatsThreadState& Stats = FStatsThreadState::GetLocalState();

	check( Stats.IsFrameValid( TargetFrame ) );

	Ar << TargetFrame;

	// Write stat packet.
	const FStatPacketArray& Frame = Stats.GetStatPacketArray( TargetFrame );
	int32 NumPackets = Frame.Packets.Num();
	Ar << NumPackets;
	for( int32 PacketIndex = 0; PacketIndex < Frame.Packets.Num(); PacketIndex++ )
	{
		FStatPacket& StatPacket = *Frame.Packets[PacketIndex];
		WriteStatPacket( Ar, StatPacket );
	}

	// Get cycles for all threads, so we can use that data to generate the mini-view.
	TMap<uint32, int64> ThreadCycles;
	for( auto It = Stats.Threads.CreateConstIterator(); It; ++It )
	{
		const int64 Cycles = Stats.GetFastThreadFrameTime( TargetFrame, It.Key() );
		ThreadCycles.Add( It.Key(), Cycles );
	}
	
	// Serialize thread cycles.
	Ar << ThreadCycles;

	FramesInfo.Add( FStatsFrameInfo( Filepos, ThreadCycles ) );
	Filepos += Ar.Tell() - PrevArPos;
}

/*-----------------------------------------------------------------------------
	Commands functionality
-----------------------------------------------------------------------------*/

FString FCommandStatsFile::LastFileSaved;
int64 FCommandStatsFile::FirstFrame = -1;
TSharedPtr<FStatsWriteFile, ESPMode::ThreadSafe> FCommandStatsFile::CurrentStatFile = nullptr;

void FCommandStatsFile::Start( const TCHAR* Cmd )
{
	CurrentStatFile = NULL;
	FString File;
	FParse::Token( Cmd, File, false );
	TSharedPtr<FStatsWriteFile, ESPMode::ThreadSafe> StatFile = MakeShareable( new FStatsWriteFile() );
	CurrentStatFile = StatFile;
	CurrentStatFile->Start( File );
	if( !CurrentStatFile->IsValid() )
	{
		CurrentStatFile = nullptr;
	}
}

void FCommandStatsFile::StartRaw( const TCHAR* Cmd )
{
	CurrentStatFile = NULL;
	FString File;
	FParse::Token( Cmd, File, false );
	TSharedPtr<FStatsWriteFile, ESPMode::ThreadSafe> StatFile = MakeShareable( new FRawStatsWriteFile() );
	CurrentStatFile = StatFile;
	CurrentStatFile->Start( File );
	if( !CurrentStatFile->IsValid() )
	{
		CurrentStatFile = nullptr;
	}
}

void FCommandStatsFile::Stop()
{
	if( CurrentStatFile.IsValid() )
	{
		CurrentStatFile->Stop();
	}
	CurrentStatFile = nullptr;
}

#endif // STATS
