// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "NullNetworkReplayStreaming.h"

/**
 * Very basic implementation of network replay streaming using the file system
 * As of now, there is just simple opening and closing of the stream, and handing off the stream for direct use
 * Eventually, we'll want to expand this interface to allow enumerating demos, support for live spectating on local machine
 * (which will require support for writing/reading at the same time)
 */

void FNullNetworkReplayStreamer::StartStreaming( FString& StreamName, bool bRecord, const FOnStreamReadyDelegate& Delegate )
{
	if ( !bRecord )
	{
		// Opne file for reading
		FileAr = IFileManager::Get().CreateFileReader( *StreamName );
	}
	else
	{
		// Open file for writing
		FileAr = IFileManager::Get().CreateFileWriter( *StreamName );
	}

	// Notify immediately
	Delegate.ExecuteIfBound( FileAr != NULL, bRecord );
}

void FNullNetworkReplayStreamer::StopStreaming()
{
	delete FileAr;
	FileAr = NULL;
}

FArchive* FNullNetworkReplayStreamer::GetStreamingArchive()
{
	return FileAr;
}

IMPLEMENT_MODULE( FNullNetworkReplayStreamingFactory, NullNetworkReplayStreaming )

TSharedPtr< INetworkReplayStreamer > FNullNetworkReplayStreamingFactory::CreateReplayStreamer() 
{
	return TSharedPtr< INetworkReplayStreamer >( new FNullNetworkReplayStreamer );
}
