// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "NullNetworkReplayStreaming.h"
#include "Paths.h"
#include "EngineVersion.h"

DEFINE_LOG_CATEGORY_STATIC( LogNullReplay, Log, All );

/**
 * Very basic implementation of network replay streaming using the file system
 * As of now, there is just simple opening and closing of the stream, and handing off the stream for direct use
 * Eventually, we'll want to expand this interface to allow enumerating demos, support for live spectating on local machine
 * (which will require support for writing/reading at the same time)
 */

static FString GetStreamBaseFilename(const FString& StreamName)
{
	int32 Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec;
	FPlatformTime::SystemTime( Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec );

	FString DemoName = StreamName;

	DemoName.ReplaceInline( TEXT( "%td" ), *FDateTime::Now().ToString() );
	DemoName.ReplaceInline( TEXT( "%d" ), *FString::Printf( TEXT( "%i-%i-%i" ), Month, Day, Year ) );
	DemoName.ReplaceInline( TEXT( "%t" ), *FString::Printf( TEXT( "%i" ), ( ( Hour * 3600 ) + ( Min * 60 ) + Sec ) * 1000 + MSec ) );
	DemoName.ReplaceInline( TEXT( "%v" ), *FString::Printf( TEXT( "%i" ), GEngineVersion.GetChangelist() ) );

	// replace bad characters with underscores
	DemoName.ReplaceInline( TEXT( "\\" ),	TEXT( "_" ) );
	DemoName.ReplaceInline( TEXT( "/" ),	TEXT( "_" ) );
	DemoName.ReplaceInline( TEXT( "." ),	TEXT( "_" ) );
	DemoName.ReplaceInline( TEXT( " " ),	TEXT( "_" ) );
	DemoName.ReplaceInline( TEXT( "%" ),	TEXT( "_" ) );

	return DemoName;
}

static FString GetDemoPath()
{
	return FPaths::Combine(*FPaths::GameSavedDir(), TEXT( "Demos/" ));
}

static FString GetStreamDirectory(const FString& StreamName)
{
	const FString DemoName = GetStreamBaseFilename(StreamName);

	// Directory for this demo
	const FString DemoDir  = FPaths::Combine(*GetDemoPath(), *DemoName);
	
	return DemoDir;
}

static FString GetStreamFullBaseFilename(const FString& StreamName)
{
	return FPaths::Combine(*GetStreamDirectory(StreamName), *GetStreamBaseFilename(StreamName));
}

static FString GetDemoFilename(const FString& StreamName)
{
	return GetStreamFullBaseFilename(StreamName) + TEXT(".demo");
}

static FString GetMetadataFilename(const FString& StreamName)
{
	return GetStreamFullBaseFilename(StreamName) + TEXT(".metadata");
}

void FNullNetworkReplayStreamer::StartStreaming( const FString& StreamName, bool bRecord, const FNetworkReplayVersion& ReplayVersion, const FOnStreamReadyDelegate& Delegate )
{
	// Create a directory for this demo
	const FString DemoDir = GetStreamDirectory(StreamName);

	IFileManager::Get().MakeDirectory( *DemoDir, true );

	const FString FullDemoFilename = GetDemoFilename(StreamName);
	const FString FullMetadataFilename = GetMetadataFilename(StreamName);

	CurrentStreamName = StreamName;

	if ( !bRecord )
	{
		// Open file for reading
		FileAr.Reset( IFileManager::Get().CreateFileReader( *FullDemoFilename ) );
		StreamerState = EStreamerState::Playback;
	}
	else
	{
		// Open file for writing
		FileAr.Reset( IFileManager::Get().CreateFileWriter( *FullDemoFilename, FILEWRITE_AllowRead ) );
		StreamerState = EStreamerState::Recording;
	}

	// Notify immediately
	Delegate.ExecuteIfBound( FileAr.Get() != NULL, bRecord );
}

void FNullNetworkReplayStreamer::StopStreaming()
{
	FileAr.Reset();
	MetadataFileAr.Reset();

	CurrentStreamName.Empty();
	StreamerState = EStreamerState::Idle;
}

FArchive* FNullNetworkReplayStreamer::GetHeaderArchive()
{
	return FileAr.Get();
}

FArchive* FNullNetworkReplayStreamer::GetStreamingArchive()
{
	return FileAr.Get();
}

FArchive* FNullNetworkReplayStreamer::GetMetadataArchive()
{
	check( StreamerState != EStreamerState::Idle );

	// Create the metadata archive on-demand
	if (!MetadataFileAr)
	{
		switch (StreamerState)
		{
			case EStreamerState::Recording:
				MetadataFileAr.Reset( IFileManager::Get().CreateFileWriter( *GetMetadataFilename(CurrentStreamName) ) );
				break;

			case EStreamerState::Playback:
				MetadataFileAr.Reset( IFileManager::Get().CreateFileReader( *GetMetadataFilename(CurrentStreamName) ) );
				break;

			default:
				break;
		}
	}

	return MetadataFileAr.Get();
}

bool FNullNetworkReplayStreamer::IsLive( const FString& StreamName ) const
{
	// If the directory for this stream doesn't exist, it can't possibly be live.
	if (!IFileManager::Get().DirectoryExists(*GetStreamDirectory(StreamName)))
	{
		return false;
	}

	// If the metadata file doesn't exist, this is a live stream.
	const int64 MetadataFileSize = IFileManager::Get().FileSize(*GetMetadataFilename(StreamName));
	return MetadataFileSize == INDEX_NONE;
}

void FNullNetworkReplayStreamer::DeleteFinishedStream( const FString& StreamName, const FOnDeleteFinishedStreamComplete& Delegate ) const
{
	// Live streams can't be deleted
	if (IsLive(StreamName))
	{
		UE_LOG(LogNullReplay, Log, TEXT("Can't delete network replay stream %s because it is live!"), *StreamName);
		Delegate.ExecuteIfBound(false);
		return;
	}

	// Delete the directory with the specified name in the Saved/Demos directory
	const FString DemoName = GetStreamDirectory(StreamName);

	const bool DeleteSucceeded = IFileManager::Get().DeleteDirectory( *DemoName, false, true );

	Delegate.ExecuteIfBound(DeleteSucceeded);
}

void FNullNetworkReplayStreamer::EnumerateStreams( const FNetworkReplayVersion& ReplayVersion, const FOnEnumerateStreamsComplete& Delegate )
{
	// Simply returns a stream for each folder in the Saved/Demos directory
	const FString WildCardPath = GetDemoPath() + TEXT( "*" );

	TArray<FString> DirectoryNames;
	IFileManager::Get().FindFiles( DirectoryNames, *WildCardPath, false, true );

	TArray<FNetworkReplayStreamInfo> Results;

	for (const FString& Directory : DirectoryNames)
	{
		// Assume there will be one file with a .demo extension in the directory
		const FString FullDemoFilePath = GetDemoFilename(Directory);

		FNetworkReplayStreamInfo Info;
		Info.SizeInBytes = IFileManager::Get().FileSize( *FullDemoFilePath );
		
		if (Info.SizeInBytes != INDEX_NONE)
		{
			Info.Name = Directory;
			Info.Timestamp = IFileManager::Get().GetTimeStamp( *FullDemoFilePath );
			Info.bIsLive = IsLive( Directory );			

			// Live streams not supported yet
			if (!Info.bIsLive)
			{
				Results.Add(Info);
			}
		}
	}

	Delegate.ExecuteIfBound(Results);
}

IMPLEMENT_MODULE( FNullNetworkReplayStreamingFactory, NullNetworkReplayStreaming )

TSharedPtr< INetworkReplayStreamer > FNullNetworkReplayStreamingFactory::CreateReplayStreamer() 
{
	return TSharedPtr< INetworkReplayStreamer >( new FNullNetworkReplayStreamer );
}
