// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "HttpNetworkReplayStreaming.h"

DEFINE_LOG_CATEGORY_STATIC( LogHttpReplay, Log, All );

void HttpStreamFArchive::Serialize( void* V, int64 Length ) 
{
	if ( IsLoading() )
	{
		if ( Pos + Length > Buffer.Num() )
		{
			ArIsError = true;
			return;
		}
			
		FMemory::Memcpy( V, Buffer.GetData() + Pos, Length );

		Pos += Length;
	}
	else
	{
		check( Pos <= Buffer.Num() );

		const int32 SpaceNeeded = Length - ( Buffer.Num() - Pos );

		if ( SpaceNeeded > 0 )
		{
			Buffer.AddZeroed( SpaceNeeded );
		}

		FMemory::Memcpy( Buffer.GetData() + Pos, V, Length );

		Pos += Length;
	}
}

int64 HttpStreamFArchive::Tell() 
{
	return Pos;
}

int64 HttpStreamFArchive::TotalSize()
{
	return Buffer.Num();
}

void HttpStreamFArchive::Seek( int64 InPos ) 
{
	Pos = InPos;
}

void FHttpNetworkReplayStreamer::StartStreaming( FString& StreamName, bool bRecord, const FOnStreamReadyDelegate& Delegate )
{
	StopStreaming();

	RememberedDelegate = Delegate;

	StreamArchive.ArIsLoading = !bRecord;
	StreamArchive.ArIsSaving = !StreamArchive.ArIsLoading;

	LastFlushTime = FPlatformTime::Seconds();

	const bool bOverrideRecordingSession = false;

	// Override session name
	if ( StreamArchive.ArIsLoading || bOverrideRecordingSession )
	{
		SessionName = StreamName;
	}

	// Create the Http request and add to pending request list
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	FString ServerURL;
	GConfig->GetString( TEXT( "HttpNetworkReplayStreaming" ), TEXT( "ServerURL" ), ServerURL, GEngineIni );

	if ( StreamArchive.ArIsLoading )
	{
		// Download the demo file from the http server
		HttpRequest->SetURL( FString::Printf( TEXT( "%sdownload?Version=%s&Session=%s&Filename=stream.%i" ), *ServerURL, TEXT( "Test" ), *SessionName, StreamFileCount ) );
		HttpRequest->SetVerb( TEXT( "GET" ) );

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpDownloadFinished );

		StreamFileCount++;
	}
	else
	{
		// Make sure this session is registered with the http server
		if ( !SessionName.IsEmpty() )
		{
			HttpRequest->SetURL( FString::Printf( TEXT( "%sstartstreamingup?Version=%s&Session=%s" ), *ServerURL, TEXT( "Test" ), *SessionName ) );
		}
		else
		{
			HttpRequest->SetURL( FString::Printf( TEXT( "%sstartstreamingup?Version=%s" ), *ServerURL, TEXT( "Test" ) ) );
		}

		HttpRequest->SetVerb( TEXT( "POST" ) );

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpStartStreamingUpFinished );

		SessionName.Empty();
	}
	
	HttpRequest->ProcessRequest();
}

void FHttpNetworkReplayStreamer::StopStreaming()
{
	FlushStream();

	StreamArchive.ArIsLoading = false;
	StreamArchive.ArIsSaving = false;
	StreamArchive.Buffer.Empty();
	StreamArchive.Pos = 0;
	StreamFileCount = 0;
}

void FHttpNetworkReplayStreamer::FlushStream()
{
	if ( StreamArchive.ArIsSaving && StreamArchive.Buffer.Num() > 0 && !SessionName.IsEmpty() )
	{
		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::FlushStream. StreamFileCount: %i, Size: %i" ), StreamFileCount, StreamArchive.Buffer.Num() );

		// Create the Http request and add to pending request list
		TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

		// Currently, the engine free's this streamer as soon as it calls this, so we can't register for a callback here right now
		// This is ok anyhow, since uploading is fire and forget (we aren't actually streaming it yet)
		//HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpRequestFinished );

		FString ServerURL;
		GConfig->GetString( TEXT( "HttpNetworkReplayStreaming" ), TEXT( "ServerURL" ), ServerURL, GEngineIni );

		HttpRequest->SetURL( FString::Printf( TEXT( "%supload?Version=%s&Session=%s&Filename=stream.%i" ), *ServerURL, TEXT( "Test" ), *SessionName, StreamFileCount ) );
		HttpRequest->SetVerb( TEXT( "POST" ) );
		HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );
		HttpRequest->SetContent( StreamArchive.Buffer );
		HttpRequest->ProcessRequest();

		StreamFileCount++;

		StreamArchive.Buffer.Empty();
		StreamArchive.Pos = 0;

		LastFlushTime = FPlatformTime::Seconds();
	}
}

FArchive* FHttpNetworkReplayStreamer::GetStreamingArchive()
{
	return ( StreamArchive.IsSaving() || StreamArchive.Buffer.Num() >  0 ) ? &StreamArchive : NULL;
}

FArchive* FHttpNetworkReplayStreamer::GetMetadataArchive()
{
	return NULL;
}

void FHttpNetworkReplayStreamer::HttpStartStreamingUpFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		SessionName = HttpResponse->GetHeader( TEXT( "Session" ) );

		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpStartStreamingUpFinished. SessionName: %s" ), *SessionName );
	}
	else
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpStartStreamingUpFinished. FAILED" ), *SessionName );
	}
}

void FHttpNetworkReplayStreamer::HttpDownloadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		if ( StreamArchive.IsLoading() )
		{
			StreamArchive.Buffer = HttpResponse->GetContent();
		}

		RememberedDelegate.ExecuteIfBound( true, StreamArchive.IsSaving() );
	}
	else
	{
		// FAIL
		StreamArchive.Buffer.Empty();
		RememberedDelegate.ExecuteIfBound( false, StreamArchive.IsSaving() );
	}

	// Reset delegate
	RememberedDelegate = FOnStreamReadyDelegate();
}

void FHttpNetworkReplayStreamer::Tick( float DeltaTime )
{
	if ( !SessionName.IsEmpty() )
	{
		const double FLUSH_TIME_IN_SECONDS = 10;

		if ( FPlatformTime::Seconds() - LastFlushTime > FLUSH_TIME_IN_SECONDS )
		{
			FlushStream();
		}
	}
}

bool FHttpNetworkReplayStreamer::IsTickable() const
{
	return true;
}

TStatId FHttpNetworkReplayStreamer::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT( FHttpNetworkReplayStreamer, STATGROUP_Tickables );
}


IMPLEMENT_MODULE( FHttpNetworkReplayStreamingFactory, HttpNetworkReplayStreaming )

TSharedPtr< INetworkReplayStreamer > FHttpNetworkReplayStreamingFactory::CreateReplayStreamer()
{
	return TSharedPtr< INetworkReplayStreamer >( new FHttpNetworkReplayStreamer );
}
