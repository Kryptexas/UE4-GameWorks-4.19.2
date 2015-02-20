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

FHttpNetworkReplayStreamer::FHttpNetworkReplayStreamer() : 
	StreamFileCount( 0 ), 
	LastChunkTime( 0 ), 
	LastRefreshViewerTime( 0 ),
	StreamerState( EStreamerState::Idle ), 
	HttpState( EHttptate::Idle ), 
	bStopStreamingCalled( false ), 
	bStreamIsLive( false ),
	NumDownloadChunks( 0 ),
	DemoTimeInMS( 0 )
{
	// Initialize the server URL
	GConfig->GetString( TEXT( "HttpNetworkReplayStreaming" ), TEXT( "ServerURL" ), ServerURL, GEngineIni );
}

void FHttpNetworkReplayStreamer::StartStreaming( const FString& StreamName, bool bRecord, const FString& VersionString, const FOnStreamReadyDelegate& Delegate )
{
	if ( !SessionName.IsEmpty() )
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::StartStreaming. SessionName already set." ) );
		return;
	}

	if ( IsStreaming() )
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::StartStreaming. IsStreaming == true." ) );
		return;
	}

	if ( IsHttpBusy() )
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::StartStreaming. IsHttpBusy == true." ) );
		return;
	}

	SessionVersion = VersionString;

	// Remember the delegate, which we'll call as soon as the header is available
	StartStreamingDelegate = Delegate;

	// Setup the archives
	StreamArchive.ArIsLoading = !bRecord;
	StreamArchive.ArIsSaving = !StreamArchive.ArIsLoading;

	HeaderArchive.ArIsLoading = !bRecord;
	HeaderArchive.ArIsSaving = !StreamArchive.ArIsLoading;

	LastChunkTime = FPlatformTime::Seconds();

	const bool bOverrideRecordingSession = false;

	// Override session name if requested
	if ( StreamArchive.ArIsLoading || bOverrideRecordingSession )
	{
		SessionName = StreamName;
	}

	// Create the Http request and add to pending request list
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	StreamFileCount = 0;

	if ( StreamArchive.ArIsLoading )
	{
		// Notify the http server that we want to start downloading a replay
		HttpRequest->SetURL( FString::Printf( TEXT( "%sstartdownloading?Version=%s&Session=%s" ), *ServerURL, *SessionVersion, *SessionName ) );
		HttpRequest->SetVerb( TEXT( "POST" ) );

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpStartDownloadingFinished );

		HttpState = EHttptate::StartDownloading;
		
		// Set the next streamer state to download the header
		StreamerState = EStreamerState::NeedToDownloadHeader;
	}
	else
	{
		// Notify the http server that we want to start upload a replay
		if ( !SessionName.IsEmpty() )
		{
			HttpRequest->SetURL( FString::Printf( TEXT( "%sstartuploading?Version=%s&Session=%s" ), *ServerURL, *SessionVersion, *SessionName ) );
		}
		else
		{
			HttpRequest->SetURL( FString::Printf( TEXT( "%sstartuploading?Version=%s" ), *ServerURL, *SessionVersion ) );
		}

		HttpRequest->SetVerb( TEXT( "POST" ) );

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpStartUploadingFinished );

		SessionName.Empty();

		HttpState = EHttptate::StartUploading;

		// Set the next streamer state to upload the header
		StreamerState = EStreamerState::NeedToUploadHeader;
	}
	
	HttpRequest->ProcessRequest();
}

void FHttpNetworkReplayStreamer::StopStreaming()
{
	check( bStopStreamingCalled == false );

	if ( StreamerState == EStreamerState::Idle )
	{
		return;
	}

	// We need to queued this operation up, and finish it once all other operations are done
	// (wait for header to upload, and any current stream to finish uploading, then we can flush what's left in StreamArchive)
	bStopStreamingCalled = true;
}

void FHttpNetworkReplayStreamer::UploadHeader()
{
	if ( SessionName.IsEmpty() || !HeaderArchive.ArIsSaving )
	{
		// IF there is no active session, or we are not recording, we don't need to flush
		return;
	}

	if ( HeaderArchive.Buffer.Num() == 0 )
	{
		// We haven't serialized the header yet, don't do anything yet
		return;
	}

	if ( IsHttpBusy() )
	{
		// If we're currently busy we can't flush right now
		return;
	}

	check( StreamFileCount == 0 );

	// First upload the header
	UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::UploadHeader. Header. StreamFileCount: %i, Size: %i" ), StreamFileCount, StreamArchive.Buffer.Num() );

	// Create the Http request and add to pending request list
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpHeaderUploadFinished );

	HttpState = EHttptate::UploadingHeader;

	HttpRequest->SetURL( FString::Printf( TEXT( "%supload?Version=%s&Session=%s&NumChunks=%i&Time=%i&Filename=replay.header" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount, DemoTimeInMS ) );
	HttpRequest->SetVerb( TEXT( "POST" ) );
	HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );
	HttpRequest->SetContent( HeaderArchive.Buffer );

	// We're done with the header archive
	HeaderArchive.Buffer.Empty();
	HeaderArchive.Pos = 0;

	HttpRequest->ProcessRequest();

	LastChunkTime = FPlatformTime::Seconds();
}

void FHttpNetworkReplayStreamer::FlushStream()
{
	if ( SessionName.IsEmpty() || !StreamArchive.ArIsSaving )
	{
		// IF there is no active session, or we are not recording, we don't need to flush
		return;
	}

	if ( IsHttpBusy() )
	{
		// If we're currently busy we can't flush right now
		return;
	}

	// Upload any new streamed data to the http server
	UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::FlushStream. StreamFileCount: %i, Size: %i" ), StreamFileCount, StreamArchive.Buffer.Num() );

	// Create the Http request and add to pending request list
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpUploadFinished );

	HttpState = EHttptate::UploadingStream;

	HttpRequest->SetURL( FString::Printf( TEXT( "%supload?Version=%s&Session=%s&NumChunks=%i&Time=%i&Filename=stream.%i" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount + 1, DemoTimeInMS, StreamFileCount ) );
	HttpRequest->SetVerb( TEXT( "POST" ) );
	HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );
	HttpRequest->SetContent( StreamArchive.Buffer );

	StreamArchive.Buffer.Empty();
	StreamArchive.Pos = 0;

	StreamFileCount++;

	HttpRequest->ProcessRequest();

	LastChunkTime = FPlatformTime::Seconds();
}

void FHttpNetworkReplayStreamer::DownloadHeader()
{
	// Download header first
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->SetURL( FString::Printf( TEXT( "%sdownload?Version=%s&Session=%s&Filename=replay.header" ), *ServerURL, *SessionVersion, *SessionName, *ViewerName ) );
	HttpRequest->SetVerb( TEXT( "GET" ) );

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished );

	HttpState = EHttptate::DownloadingHeader;

	HttpRequest->ProcessRequest();
}

void FHttpNetworkReplayStreamer::DownloadNextChunk()
{
	if ( IsHttpBusy() )
	{
		return;
	}

	const bool bMoreChunksDefinitelyAvilable = StreamFileCount < NumDownloadChunks;
	const bool bMoreChunksMayBeAvilable = bStreamIsLive;

	if ( !bMoreChunksDefinitelyAvilable && !bMoreChunksMayBeAvilable )
	{
		//StreamerState = EStreamerState::Idle;
		//UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::DownloadNextChunk. Done streaming." ) );
		return;
	}

	// Determine if it's time to download the next chunk
	const double CHECK_FOR_NEXT_CHUNK_IN_SECONDS = 5;

	const bool bTimeToDownloadChunk = ( FPlatformTime::Seconds() - LastChunkTime > CHECK_FOR_NEXT_CHUNK_IN_SECONDS );
	const bool bReallyNeedToDownloadChunk = StreamArchive.Pos >= StreamArchive.Buffer.Num() && bMoreChunksDefinitelyAvilable;

	if ( !bTimeToDownloadChunk && !bReallyNeedToDownloadChunk )
	{
		// Not time yet
		return;
	}

	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	// Download the next stream chunk
	HttpRequest->SetURL( FString::Printf( TEXT( "%sdownload?Version=%s&Session=%s&Filename=stream.%i" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount ) );
	HttpRequest->SetVerb( TEXT( "GET" ) );

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpDownloadFinished );

	HttpState = EHttptate::DownloadingStream;

	HttpRequest->ProcessRequest();

	LastChunkTime = FPlatformTime::Seconds();
}

void FHttpNetworkReplayStreamer::RefreshViewer()
{
	if ( IsHttpBusy() )
	{
		return;
	}

	// Determine if it's time to download the next chunk
	const double REFRESH_VIEWER_IN_SECONDS = 5;

	if ( StreamerState != EStreamerState::StreamingDownFinal && FPlatformTime::Seconds() - LastRefreshViewerTime < REFRESH_VIEWER_IN_SECONDS )
	{
		// Not time yet
		return;
	}

	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	// Download the next stream chunk
	if ( StreamerState == EStreamerState::StreamingDownFinal )
	{
		HttpRequest->SetURL( FString::Printf( TEXT( "%srefreshviewer?Version=%s&Session=%s&Viewer=%s&Final=true" ), *ServerURL, *SessionVersion, *SessionName, *ViewerName ) );
		StreamerState = EStreamerState::Idle;
	}
	else
	{
		HttpRequest->SetURL( FString::Printf( TEXT( "%srefreshviewer?Version=%s&Session=%s&Viewer=%s" ), *ServerURL, *SessionVersion, *SessionName, *ViewerName ) );
	}

	HttpRequest->SetVerb( TEXT( "POST" ) );

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpRefreshViewerFinished );

	HttpState = EHttptate::RefreshingViewer;

	HttpRequest->ProcessRequest();

	LastRefreshViewerTime = FPlatformTime::Seconds();
}

FArchive* FHttpNetworkReplayStreamer::GetHeaderArchive()
{
	return ( HeaderArchive.IsSaving() || HeaderArchive.Pos < HeaderArchive.Buffer.Num() ) ? &HeaderArchive : NULL;
}

FArchive* FHttpNetworkReplayStreamer::GetStreamingArchive()
{
	return ( StreamArchive.IsSaving() || IsDataAvailable() ) ? &StreamArchive : NULL;
}

FArchive* FHttpNetworkReplayStreamer::GetMetadataArchive()
{
	return NULL;
}

void FHttpNetworkReplayStreamer::UpdateTotalDemoTime( uint32 TimeInMS )
{
	DemoTimeInMS = TimeInMS;
}

bool FHttpNetworkReplayStreamer::IsDataAvailable() const
{
	// If we are loading, and we have more data
	if ( StreamArchive.IsLoading() && StreamArchive.Pos < StreamArchive.Buffer.Num() && NumDownloadChunks > 0 )
	{
		return true;
	}
	
	return false;
}

bool FHttpNetworkReplayStreamer::IsLive(const FString& StreamName) const 
{
	return bStreamIsLive;
}

void FHttpNetworkReplayStreamer::DeleteFinishedStream( const FString& StreamName, const FOnDeleteFinishedStreamComplete& Delegate ) const
{
	// Stubbed!
	Delegate.ExecuteIfBound(false);
}

void FHttpNetworkReplayStreamer::EnumerateStreams( const FString& VersionString, const FOnEnumerateStreamsComplete& Delegate )
{
	if ( IsHttpBusy() )
	{
		Delegate.ExecuteIfBound( TArray<FNetworkReplayStreamInfo>() );
		return;
	}

	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	// Enumerate all of the sessions
	HttpRequest->SetURL( FString::Printf( TEXT( "%senumsessions?Version=%s" ), *ServerURL, *VersionString ) );
	HttpRequest->SetVerb( TEXT( "GET" ) );

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpEnumerateSessionsFinished );

	EnumerateStreamsDelegate = Delegate;
	HttpState = EHttptate::EnumeratingSessions;

	HttpRequest->ProcessRequest();
}

void FHttpNetworkReplayStreamer::HttpStartUploadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::StartUploading );
	check( StreamerState == EStreamerState::NeedToUploadHeader );

	HttpState = EHttptate::Idle;

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
	check( HttpState == EHttptate::StopUploading );
	check( StreamerState == EStreamerState::StreamingUpFinal );

	HttpState = EHttptate::Idle;
	StreamerState = EStreamerState::Idle;

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
	bStopStreamingCalled = false;
	SessionName.Empty();
}

void FHttpNetworkReplayStreamer::HttpHeaderUploadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::UploadingHeader );
	check( StreamerState == EStreamerState::NeedToUploadHeader );

	HttpState = EHttptate::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpHeaderUploadFinished." ) );

		StreamerState = EStreamerState::StreamingUp;
	}
	else
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpHeaderUploadFinished. FAILED" ) );

		StreamerState = EStreamerState::Idle;

		// FIXME: Notify demo driver with delegate
	}
}

void FHttpNetworkReplayStreamer::HttpUploadFinished( FHttpRequestPtr , FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::UploadingStream );
	check( StreamerState == EStreamerState::StreamingUp || StreamerState == EStreamerState::StreamingUpFinal );

	HttpState = EHttptate::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpUploadFinished." ) );

		if ( StreamerState == EStreamerState::StreamingUpFinal )
		{
			// Create the Http request and add to pending request list
			TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

			HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpStopUploadingFinished );

			HttpState = EHttptate::StopUploading;

			HttpRequest->SetURL( FString::Printf( TEXT( "%sstopuploading?Version=%s&Session=%s&NumChunks=%i&Time=%i" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount, DemoTimeInMS ) );
			HttpRequest->SetVerb( TEXT( "POST" ) );
			HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );

			HttpRequest->ProcessRequest();
		}
	}
	else
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpUploadFinished. FAILED" ) );
		StreamerState = EStreamerState::Idle;
	}
}

void FHttpNetworkReplayStreamer::HttpStartDownloadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::StartDownloading );
	check( StreamerState == EStreamerState::NeedToDownloadHeader );

	HttpState = EHttptate::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		FString NumChunksString = HttpResponse->GetHeader( TEXT( "NumChunks" ) );
		FString DemoTimeString = HttpResponse->GetHeader( TEXT( "Time" ) );
		FString State = HttpResponse->GetHeader( TEXT( "State" ) );

		ViewerName = HttpResponse->GetHeader( TEXT( "Viewer" ) );

		bStreamIsLive = State == TEXT( "Live" );

		NumDownloadChunks = FCString::Atoi( *NumChunksString );
		DemoTimeInMS = FCString::Atoi( *DemoTimeString );

		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpStartDownloadingFinished. Viewer: %s, State: %s, NumChunks: %i, DemoTime: %2.2f" ), *ViewerName, *State, NumDownloadChunks, (float)DemoTimeInMS / 1000 );

		// First, download the header
		if ( NumDownloadChunks > 0 )
		{			
			StreamerState = EStreamerState::NeedToDownloadHeader;
		}
		else
		{
			UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpStartDownloadingFinished. NO CHUNKS" ) );

			StartStreamingDelegate.ExecuteIfBound( false, StreamArchive.IsSaving() );

			// Reset delegate
			StartStreamingDelegate = FOnStreamReadyDelegate();

			StreamerState = EStreamerState::Idle;
		}
	}
	else
	{
		// FAIL
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpStartDownloadingFinished. FAILED" ) );

		StartStreamingDelegate.ExecuteIfBound( false, StreamArchive.IsSaving() );

		// Reset delegate
		StartStreamingDelegate = FOnStreamReadyDelegate();

		StreamerState = EStreamerState::Idle;
	}
}

void FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::DownloadingHeader );
	check( StreamerState == EStreamerState::NeedToDownloadHeader );
	check( StreamArchive.IsLoading() );

	HttpState = EHttptate::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		HeaderArchive.Buffer.Append( HttpResponse->GetContent() );

		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished. Size: %i" ), HeaderArchive.Buffer.Num() );

		StartStreamingDelegate.ExecuteIfBound( true, StreamArchive.IsSaving() );
		StreamerState = EStreamerState::StreamingDown;
	}
	else
	{
		// FAIL
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished. FAILED." ) );

		StreamArchive.Buffer.Empty();
		StartStreamingDelegate.ExecuteIfBound( false, StreamArchive.IsSaving() );

		StreamerState = EStreamerState::Idle;
	}

	// Reset delegate
	StartStreamingDelegate = FOnStreamReadyDelegate();
}

void FHttpNetworkReplayStreamer::HttpDownloadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::DownloadingStream );
	check( StreamArchive.IsLoading() );

	HttpState = EHttptate::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		FString NumChunksString = HttpResponse->GetHeader( TEXT( "NumChunks" ) );
		FString DemoTimeString = HttpResponse->GetHeader( TEXT( "Time" ) );
		FString State = HttpResponse->GetHeader( TEXT( "State" ) );

		bStreamIsLive = State == TEXT( "Live" );

		NumDownloadChunks = FCString::Atoi( *NumChunksString );
		DemoTimeInMS = FCString::Atoi( *DemoTimeString );

		if ( HttpResponse->GetContent().Num() > 0 || bStreamIsLive )
		{
			if ( HttpResponse->GetContent().Num() > 0 )
			{
				StreamArchive.Buffer.Append( HttpResponse->GetContent() );
				StreamFileCount++;
			}

			UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadFinished. State: %s, Progress: %i / %i, DemoTime: %2.2f" ), *State, StreamFileCount, NumDownloadChunks, (float)DemoTimeInMS / 1000 );
		}
		else
		{
			UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadFinished. FAILED." ) );
			StreamArchive.Buffer.Empty();
			StreamerState = EStreamerState::Idle;

			// FIXME: Report error to engine
		}		
	}
	else
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadFinished. FAILED." ) );
		StreamArchive.Buffer.Empty();
		StreamerState = EStreamerState::Idle;

		// FIXME: Report error to engine
	}
}

void FHttpNetworkReplayStreamer::HttpRefreshViewerFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::RefreshingViewer );
	check( StreamArchive.IsLoading() );

	HttpState = EHttptate::Idle;

	if ( !bSucceeded || HttpResponse->GetResponseCode() != EHttpResponseCodes::Ok )
	{
		// FAIL
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpRefreshViewerFinished. FAILED." ) );
	}
}

void FHttpNetworkReplayStreamer::HttpEnumerateSessionsFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::EnumeratingSessions );

	HttpState = EHttptate::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{		
		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpEnumerateSessionsFinished." ) );

		TArray<FNetworkReplayStreamInfo> Streams;
		FString StreamsString = HttpResponse->GetContentAsString();

		int32 Index = INDEX_NONE;

		TArray< FString > Tokens;

		// Parse the string as { token 1, token ..., token n }
		// This isn't perfect, we should convert to JSON when the dust settles
		if ( StreamsString.FindChar( '{', Index ) )
		{
			StreamsString = StreamsString.RightChop( Index + 1 );

			while ( StreamsString.FindChar( ',', Index ) )
			{
				Tokens.Add( StreamsString.Left( Index ) );
				StreamsString = StreamsString.RightChop( Index + 1 );
			}

			if ( StreamsString.FindChar( '}', Index ) )
			{
				Tokens.Add( StreamsString.Left( Index ) );
			}
			else
			{
				UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpEnumerateSessionsFinished. '}' not found." ) );
				EnumerateStreamsDelegate.ExecuteIfBound( TArray<FNetworkReplayStreamInfo>() );		// FIXME: Notify failure here
				EnumerateStreamsDelegate = FOnEnumerateStreamsComplete();
				return;
			}

			if ( Tokens.Num() > 0 )
			{
				Tokens[ Tokens.Num() - 1 ].RemoveFromStart( TEXT( " " ) );
				Tokens[ Tokens.Num() - 1 ].RemoveFromEnd( TEXT( " " ) );
			}
		}
		else
		{
			UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpEnumerateSessionsFinished. '{' not found." ) );
			EnumerateStreamsDelegate.ExecuteIfBound( TArray<FNetworkReplayStreamInfo>() );		// FIXME: Notify failure here
			EnumerateStreamsDelegate = FOnEnumerateStreamsComplete();
			return;
		}

		const int NUM_TOKENS_PER_INFO = 4;

		if ( Tokens.Num() == 0 || ( Tokens.Num() % NUM_TOKENS_PER_INFO ) != 0 )
		{
			UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpEnumerateSessionsFinished. Invalid number of tokens: %i" ), Tokens.Num() );
			EnumerateStreamsDelegate.ExecuteIfBound( TArray<FNetworkReplayStreamInfo>() );		// FIXME: Notify failure here
			EnumerateStreamsDelegate = FOnEnumerateStreamsComplete();
			return;
		}

		// Convert tokens to individual FNetworkReplayStreamInfo's
		for ( int i = 0; i < Tokens.Num(); i += NUM_TOKENS_PER_INFO )
		{
			FNetworkReplayStreamInfo NewStream;

			NewStream.Name			= Tokens[i];
			NewStream.bIsLive		= false;
			NewStream.SizeInBytes	= 0;
			NewStream.Timestamp		= 0;

			if ( i + 1 < Tokens.Num() )
			{
				// Server returns milliseconds from January 1, 1970, 00:00:00 GMT
				// We need to compensate for the fact that FDateTime starts at January 1, 0001 A.D. and is in 100 nanosecond resolution
				NewStream.Timestamp = FDateTime( FCString::Atoi64( *Tokens[ i + 1 ] ) * 1000 * 10 + FDateTime( 1970, 1, 1 ).GetTicks() );
			}

			if ( i + 2 < Tokens.Num() )
			{
				NewStream.SizeInBytes = FCString::Atoi( *Tokens[ i + 2 ] );
			}

			if ( i + 3 < Tokens.Num() )
			{
				NewStream.bIsLive = Tokens[ i + 3 ].Contains( TEXT( "true" ) );
			}

			Streams.Add( NewStream );
		}

		EnumerateStreamsDelegate.ExecuteIfBound( Streams );
	}
	else
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpEnumerateSessionsFinished. FAILED" ) );
		EnumerateStreamsDelegate.ExecuteIfBound( TArray<FNetworkReplayStreamInfo>() );		// FIXME: Notify failure here
	}

	EnumerateStreamsDelegate = FOnEnumerateStreamsComplete();
}

void FHttpNetworkReplayStreamer::Tick( float DeltaTime )
{
	if ( IsHttpBusy() )
	{
		return;
	}

	if ( SessionName.IsEmpty() )
	{
		check( StreamerState == EStreamerState::Idle );
		return;
	}

	if ( bStopStreamingCalled )
	{
		// If http isn't busy and we need to flush the last chunk, then go to that state now
		if ( StreamerState == EStreamerState::StreamingUp )
		{
			StreamerState = EStreamerState::StreamingUpFinal;
			bStopStreamingCalled = false;
		}
		else if ( StreamerState == EStreamerState::StreamingDown )
		{
			StreamerState = EStreamerState::StreamingDownFinal;
			bStopStreamingCalled = false;
		}
	}

	if ( StreamerState == EStreamerState::NeedToUploadHeader )
	{
		// If we're waiting on the header, don't do anything until the header has data and is uploaded
		UploadHeader();
	}
	else if ( StreamerState == EStreamerState::StreamingUp )
	{
		const double FLUSH_TIME_IN_SECONDS = 10;

		if ( FPlatformTime::Seconds() - LastChunkTime > FLUSH_TIME_IN_SECONDS )
		{
			FlushStream();
		}
	}
	else if ( StreamerState == EStreamerState::StreamingUpFinal )
	{
		FlushStream();
	}
	else if ( StreamerState == EStreamerState::NeedToDownloadHeader )
	{
		// If we're waiting on the header to download, don't do anything until the header has been downloaded
		DownloadHeader();
	}
	else if ( StreamerState == EStreamerState::StreamingDown )
	{
		DownloadNextChunk();
		RefreshViewer();
	}
	else if ( StreamerState == EStreamerState::StreamingDownFinal )
	{
		RefreshViewer();
	}
}

bool FHttpNetworkReplayStreamer::IsHttpBusy() const
{
	return HttpState != EHttptate::Idle;
}

bool FHttpNetworkReplayStreamer::IsStreaming() const
{
	return StreamerState != EStreamerState::Idle;
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
		
		// We can release our hold when streaming is completely done
		if ( HttpStreamers[i].IsUnique() && !HttpStreamers[i]->IsHttpBusy() && !HttpStreamers[i]->bStopStreamingCalled )
		{
			if ( HttpStreamers[i]->IsStreaming() )
			{
				UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamingFactory::Tick. Stream was stopped early." ) );
			}

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
