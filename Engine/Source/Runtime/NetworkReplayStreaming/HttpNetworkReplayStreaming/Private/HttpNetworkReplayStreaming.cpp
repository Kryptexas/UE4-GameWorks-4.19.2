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
	if ( !SessionName.IsEmpty() )
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::StartStreaming. SessionName already set." ) );
		return;
	}

	if ( IsBusy() )
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::StartStreaming. BUSY" ) );
		return;
	}

	RememberedDelegate = Delegate;

	StreamArchive.ArIsLoading = !bRecord;
	StreamArchive.ArIsSaving = !StreamArchive.ArIsLoading;

	HeaderArchive.ArIsLoading = !bRecord;
	HeaderArchive.ArIsSaving = !StreamArchive.ArIsLoading;

	LastFlushTime = FPlatformTime::Seconds();

	const bool bOverrideRecordingSession = true;//false;

	// Override session name
	if ( StreamArchive.ArIsLoading || bOverrideRecordingSession )
	{
		SessionName = StreamName;
	}

	// Create the Http request and add to pending request list
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	GConfig->GetString( TEXT( "HttpNetworkReplayStreaming" ), TEXT( "ServerURL" ), ServerURL, GEngineIni );

	if ( StreamArchive.ArIsLoading )
	{
		StreamFileCount = 0;

		// Download the demo file from the http server
		HttpRequest->SetURL( FString::Printf( TEXT( "%sstartdownloading?Version=%s&Session=%s" ), *ServerURL, TEXT( "Test" ), *SessionName ) );
		HttpRequest->SetVerb( TEXT( "POST" ) );

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpStartDownloadingFinished );

		StreamState = EStreamState::StartDownloading;
	}
	else
	{
		// Make sure this session is registered with the http server
		if ( !SessionName.IsEmpty() )
		{
			HttpRequest->SetURL( FString::Printf( TEXT( "%sstartuploading?Version=%s&Session=%s" ), *ServerURL, TEXT( "Test" ), *SessionName ) );
		}
		else
		{
			HttpRequest->SetURL( FString::Printf( TEXT( "%sstartuploading?Version=%s" ), *ServerURL, TEXT( "Test" ) ) );
		}

		HttpRequest->SetVerb( TEXT( "POST" ) );

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpStartUploadingFinished );

		SessionName.Empty();

		StreamState = EStreamState::StartUploading;
	}
	
	HttpRequest->ProcessRequest();
}

void FHttpNetworkReplayStreamer::StopStreaming()
{
	bFinalFlushNeeded = true;
}

void FHttpNetworkReplayStreamer::FlushStream()
{
	if ( SessionName.IsEmpty() || !StreamArchive.ArIsSaving )
	{
		return;
	}

	if ( IsBusy() )
	{
		return;
	}

	if ( HeaderArchive.Buffer.Num() > 0 )
	{
		// First upload the header
		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::FlushStream. Header. StreamFileCount: %i, Size: %i" ), StreamFileCount, StreamArchive.Buffer.Num() );

		// Create the Http request and add to pending request list
		TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpHeaderUploadFinished );

		StreamState = EStreamState::UploadingStream;

		HttpRequest->SetURL( FString::Printf( TEXT( "%supload?Version=%s&Session=%s&Filename=replay.header" ), *ServerURL, TEXT( "Test" ), *SessionName ) );
		HttpRequest->SetVerb( TEXT( "POST" ) );
		HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );
		HttpRequest->SetContent( HeaderArchive.Buffer );

		HeaderArchive.Buffer.Empty();
		HeaderArchive.Pos = 0;

		HttpRequest->ProcessRequest();

		LastFlushTime = FPlatformTime::Seconds();
	}
	else if ( StreamArchive.Buffer.Num() > 0 )
	{
		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::FlushStream. StreamFileCount: %i, Size: %i" ), StreamFileCount, StreamArchive.Buffer.Num() );

		// Create the Http request and add to pending request list
		TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpUploadFinished );

		StreamState = EStreamState::UploadingStream;

		HttpRequest->SetURL( FString::Printf( TEXT( "%supload?Version=%s&Session=%s&Filename=stream.%i" ), *ServerURL, TEXT( "Test" ), *SessionName, StreamFileCount ) );
		HttpRequest->SetVerb( TEXT( "POST" ) );
		HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );
		HttpRequest->SetContent( StreamArchive.Buffer );

		StreamArchive.Buffer.Empty();
		StreamArchive.Pos = 0;

		StreamFileCount++;

		HttpRequest->ProcessRequest();

		LastFlushTime = FPlatformTime::Seconds();
	}
}

void FHttpNetworkReplayStreamer::DownloadNextChunk()
{
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	if ( HeaderArchive.Buffer.Num() == 0 )
	{
		// Download header first
		HttpRequest->SetURL( FString::Printf( TEXT( "%sdownload?Version=%s&Session=%s&Filename=replay.header" ), *ServerURL, TEXT( "Test" ), *SessionName ) );
		HttpRequest->SetVerb( TEXT( "GET" ) );

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished );
	}
	else
	{
		HttpRequest->SetURL( FString::Printf( TEXT( "%sdownload?Version=%s&Session=%s&Filename=stream.%i" ), *ServerURL, TEXT( "Test" ), *SessionName, StreamFileCount ) );
		HttpRequest->SetVerb( TEXT( "GET" ) );

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpDownloadFinished );
	}

	StreamState = EStreamState::DownloadingStream;

	HttpRequest->ProcessRequest();
}

FArchive* FHttpNetworkReplayStreamer::GetHeaderArchive()
{
	return ( StreamArchive.IsSaving() || HeaderArchive.Pos < HeaderArchive.Buffer.Num() ) ? &HeaderArchive : NULL;
}

FArchive* FHttpNetworkReplayStreamer::GetStreamingArchive()
{
	return ( StreamArchive.IsSaving() || IsDataAvailable() ) ? &StreamArchive : NULL;
}

FArchive* FHttpNetworkReplayStreamer::GetMetadataArchive()
{
	return NULL;
}

bool FHttpNetworkReplayStreamer::IsDataAvailable() const
{
	//if ( StreamArchive.IsLoading() && StreamArchive.Pos < StreamArchive.Buffer.Num() && StreamFileCount == NumDownloadChunks && NumDownloadChunks > 0 )
	if ( StreamArchive.IsLoading() && StreamArchive.Pos < StreamArchive.Buffer.Num() && NumDownloadChunks > 0 )
	{
		return true;
	}
	
	return false;
}

void FHttpNetworkReplayStreamer::HttpStartUploadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( StreamState == EStreamState::StartUploading );

	StreamState = EStreamState::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		SessionName = HttpResponse->GetHeader( TEXT( "Session" ) );

		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpStartUploadingFinished. SessionName: %s" ), *SessionName );
	}
	else
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpStartUploadingFinished. FAILED" ) );
	}
}

void FHttpNetworkReplayStreamer::HttpStopUploadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( StreamState == EStreamState::StopUploading );
	check( bFinalFlushNeeded );

	StreamState = EStreamState::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpStopUploadingFinished. SessionName: %s" ), *SessionName );
	}
	else
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpStopUploadingFinished. FAILED" ) );
	}

	StreamArchive.ArIsLoading = false;
	StreamArchive.ArIsSaving = false;
	StreamArchive.Buffer.Empty();
	StreamArchive.Pos = 0;
	StreamFileCount = 0;
	bFinalFlushNeeded = false;
	SessionName.Empty();
}

void FHttpNetworkReplayStreamer::HttpHeaderUploadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( StreamState == EStreamState::UploadingStream );

	StreamState = EStreamState::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpHeaderUploadFinished." ) );
	}
	else
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpHeaderUploadFinished. FAILED" ) );
	}
}

void FHttpNetworkReplayStreamer::HttpUploadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( StreamState == EStreamState::UploadingStream );

	StreamState = EStreamState::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpUploadFinished." ) );

		if ( bFinalFlushNeeded )
		{
			// Create the Http request and add to pending request list
			TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

			HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpStopUploadingFinished );

			StreamState = EStreamState::StopUploading;

			HttpRequest->SetURL( FString::Printf( TEXT( "%sstopuploading?Version=%s&Session=%s&NumChunks=%i" ), *ServerURL, TEXT( "Test" ), *SessionName, StreamFileCount ) );
			HttpRequest->SetVerb( TEXT( "POST" ) );
			HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );

			HttpRequest->ProcessRequest();
		}
	}
	else
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpUploadFinished. FAILED" ) );
	}
}

void FHttpNetworkReplayStreamer::HttpStartDownloadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( StreamState == EStreamState::StartDownloading );

	StreamState = EStreamState::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		FString NumChunksString = HttpResponse->GetHeader( TEXT( "NumChunks" ) );
		FString State = HttpResponse->GetHeader( TEXT( "State" ) );

		NumDownloadChunks = FCString::Atoi( *NumChunksString );

		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpStartDownloadingFinished. State: %s, NumChunks: %i" ), *State, NumDownloadChunks );

		// Download the demo file from the http server
		if ( NumDownloadChunks > 0 )
		{
			DownloadNextChunk();
		}
		else
		{
			UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpStartDownloadingFinished. NO CHUNKS" ) );

			RememberedDelegate.ExecuteIfBound( false, StreamArchive.IsSaving() );

			// Reset delegate
			RememberedDelegate = FOnStreamReadyDelegate();
		}
	}
	else
	{
		// FAIL
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpStartDownloadingFinished. FAILED" ) );

		RememberedDelegate.ExecuteIfBound( false, StreamArchive.IsSaving() );

		// Reset delegate
		RememberedDelegate = FOnStreamReadyDelegate();
	}
}

void FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( StreamState == EStreamState::DownloadingStream );
	check( StreamArchive.IsLoading() );

	StreamState = EStreamState::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		HeaderArchive.Buffer.Append( HttpResponse->GetContent() );

		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished. Size: %i" ), HeaderArchive.Buffer.Num() );

		RememberedDelegate.ExecuteIfBound( true, StreamArchive.IsSaving() );
	}
	else
	{
		// FAIL
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadFinished. FAILED." ) );

		StreamArchive.Buffer.Empty();
		RememberedDelegate.ExecuteIfBound( false, StreamArchive.IsSaving() );
	}

	// Reset delegate
	RememberedDelegate = FOnStreamReadyDelegate();
}

void FHttpNetworkReplayStreamer::HttpDownloadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( StreamState == EStreamState::DownloadingStream );
	check( StreamArchive.IsLoading() );

	StreamState = EStreamState::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		StreamArchive.Buffer.Append( HttpResponse->GetContent() );
		
		StreamFileCount++;

		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadFinished. Progress: %i / %i" ), StreamFileCount, NumDownloadChunks );
	}
	else
	{
		// FAIL
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadFinished. FAILED." ) );

		StreamArchive.Buffer.Empty();
	}
}

void FHttpNetworkReplayStreamer::Tick( float DeltaTime )
{
	if ( !SessionName.IsEmpty() && !IsBusy() )
	{
		if ( StreamArchive.IsSaving() )
		{
			const double FLUSH_TIME_IN_SECONDS = 10;

			if ( bFinalFlushNeeded || HeaderArchive.Buffer.Num() > 0 || FPlatformTime::Seconds() - LastFlushTime > FLUSH_TIME_IN_SECONDS )
			{
				FlushStream();
			}
		}
		else if ( StreamArchive.IsLoading() )
		{
			if ( StreamFileCount < NumDownloadChunks && NumDownloadChunks > 0 && StreamArchive.Pos == StreamArchive.Buffer.Num() )
			{
				DownloadNextChunk();
			}
		}
	}
}

bool FHttpNetworkReplayStreamer::IsBusy()
{
	return StreamState != EStreamState::Idle;
}

IMPLEMENT_MODULE( FHttpNetworkReplayStreamingFactory, HttpNetworkReplayStreaming )

TSharedPtr< INetworkReplayStreamer > FHttpNetworkReplayStreamingFactory::CreateReplayStreamer()
{
	TSharedPtr< FHttpNetworkReplayStreamer > Streamer( new FHttpNetworkReplayStreamer );

	HttpStreamers.Add( Streamer );

	return Streamer;
}

void FHttpNetworkReplayStreamingFactory::Tick( float DeltaTime )
{
	for ( int i = HttpStreamers.Num() - 1; i >= 0; i-- )
	{
		HttpStreamers[i]->Tick( DeltaTime );
		
		// If we're the only holder of this streamer, and we're not busy, we can free it
		if ( HttpStreamers[i].IsUnique() && !HttpStreamers[i]->IsBusy() )
		{
			HttpStreamers.RemoveAt( i );
		}
	}
}

bool FHttpNetworkReplayStreamingFactory::IsTickable() const
{
	return true;
}

TStatId FHttpNetworkReplayStreamingFactory::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT( FHttpNetworkReplayStreamingFactory, STATGROUP_Tickables );
}
