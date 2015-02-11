// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "NullNetworkReplayStreaming.h"
#include "Paths.h"
#include "EngineVersion.h"

/**
 * Very basic implementation of network replay streaming using the file system
 * As of now, there is just simple opening and closing of the stream, and handing off the stream for direct use
 * Eventually, we'll want to expand this interface to allow enumerating demos, support for live spectating on local machine
 * (which will require support for writing/reading at the same time)
 */

void FNullNetworkReplayStreamer::StartStreaming( const FString& StreamName, bool bRecord, const FString& VersionString, const FOnStreamReadyDelegate& Delegate )
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

	// Create a directory for this demo
	const FString DemoDir = FPaths::Combine(*FPaths::GameSavedDir(), TEXT( "Demos" ), *DemoName);

	IFileManager::Get().MakeDirectory( *DemoDir, true );

	// Demo filename without extension
	const FString FullDemoBaseName = DemoDir + TEXT("/") + DemoName;

	const FString FullDemoFilename = FullDemoBaseName + TEXT(".demo");
	const FString FullMetadataFilename = FullDemoBaseName + TEXT(".metadata");

	if ( !bRecord )
	{
		// Open file for reading
		FileAr = IFileManager::Get().CreateFileReader( *FullDemoFilename );
		MetadataFileAr = IFileManager::Get().CreateFileReader( *FullMetadataFilename );
	}
	else
	{
		// Open file for writing
		FileAr = IFileManager::Get().CreateFileWriter( *FullDemoFilename );
		MetadataFileAr = IFileManager::Get().CreateFileWriter( *FullMetadataFilename );
	}

	// Notify immediately
	Delegate.ExecuteIfBound( FileAr != NULL, bRecord );
}

void FNullNetworkReplayStreamer::StopStreaming()
{
	delete FileAr;
	FileAr = NULL;

	delete MetadataFileAr;
	MetadataFileAr = NULL;
}

FArchive* FNullNetworkReplayStreamer::GetHeaderArchive()
{
	return FileAr;
}

FArchive* FNullNetworkReplayStreamer::GetStreamingArchive()
{
	return FileAr;
}

FArchive* FNullNetworkReplayStreamer::GetMetadataArchive()
{
	return MetadataFileAr;
}

IMPLEMENT_MODULE( FNullNetworkReplayStreamingFactory, NullNetworkReplayStreaming )

TSharedPtr< INetworkReplayStreamer > FNullNetworkReplayStreamingFactory::CreateReplayStreamer() 
{
	return TSharedPtr< INetworkReplayStreamer >( new FNullNetworkReplayStreamer );
}
