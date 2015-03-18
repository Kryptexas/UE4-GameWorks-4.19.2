// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "HttpNetworkReplayStreaming.h"
#include "OnlineJsonSerializer.h"

DEFINE_LOG_CATEGORY_STATIC( LogHttpReplay, Log, All );

class FNetworkReplayListItem : public FOnlineJsonSerializable
{
public:
	FNetworkReplayListItem() {}
	virtual ~FNetworkReplayListItem() {}

	FString		VersionName;
	FString		SessionName;
	FString		FriendlyName;
	FDateTime	Timestamp;
	int32		SizeInBytes;
	int32		DemoTimeInMs;
	int32		NumViewers;
	bool		bIsLive;

	// FOnlineJsonSerializable
	BEGIN_ONLINE_JSON_SERIALIZER
		ONLINE_JSON_SERIALIZE( "VersionName",	VersionName );
		ONLINE_JSON_SERIALIZE( "SessionName",	SessionName );
		ONLINE_JSON_SERIALIZE( "FriendlyName",	FriendlyName );
		ONLINE_JSON_SERIALIZE( "Timestamp",		Timestamp );
		ONLINE_JSON_SERIALIZE( "SizeInBytes",	SizeInBytes );
		ONLINE_JSON_SERIALIZE( "DemoTimeInMs",	DemoTimeInMs );
		ONLINE_JSON_SERIALIZE( "NumViewers",	NumViewers );
		ONLINE_JSON_SERIALIZE( "bIsLive",		bIsLive );
	END_ONLINE_JSON_SERIALIZER
};

class FNetworkReplayList : public FOnlineJsonSerializable
{
public:
	FNetworkReplayList()
	{}
	virtual ~FNetworkReplayList() {}

	TArray< FNetworkReplayListItem > Replays;

	// FOnlineJsonSerializable
	BEGIN_ONLINE_JSON_SERIALIZER
		ONLINE_JSON_SERIALIZE_ARRAY_SERIALIZABLE( "replays", Replays, FNetworkReplayListItem );
	END_ONLINE_JSON_SERIALIZER
};

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
	check( InPos < Buffer.Num() );

	Pos = InPos;
}

bool HttpStreamFArchive::AtEnd() 
{
	return Pos >= Buffer.Num() && bAtEndOfReplay;
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
	TotalDemoTimeInMS( 0 ),
	FlushCheckpointTime( 0 ),
	HighPriorityEndTime( 0 ),
	StreamerLastError( ENetworkReplayError::None )
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
	StreamArchive.ArIsLoading		= !bRecord;
	StreamArchive.ArIsSaving		= !StreamArchive.ArIsLoading;
	StreamArchive.bAtEndOfReplay	= false;

	HeaderArchive.ArIsLoading		= StreamArchive.ArIsLoading;
	HeaderArchive.ArIsSaving		= StreamArchive.ArIsSaving;

	CheckpointArchive.ArIsLoading	= StreamArchive.ArIsLoading;
	CheckpointArchive.ArIsSaving	= StreamArchive.ArIsSaving;

	LastChunkTime = FPlatformTime::Seconds();

	// Create the Http request and add to pending request list
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	StreamFileCount = 0;

	if ( StreamArchive.ArIsLoading )
	{
		SessionName = StreamName;

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
		// Notify the http server that we want to start uploading a replay
		HttpRequest->SetURL( FString::Printf( TEXT( "%sstartuploading?Version=%s&Friendly=%s" ), *ServerURL, *SessionVersion, *StreamName ) );

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

	HttpRequest->SetURL( FString::Printf( TEXT( "%supload?Version=%s&Session=%s&NumChunks=%i&Time=%i&Filename=replay.header" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount, TotalDemoTimeInMS ) );
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

	if ( StreamArchive.Buffer.Num() == 0 )
	{
		// Nothing to flush
		return;
	}

	if ( IsHttpBusy() )
	{
		// If we're currently busy we can't flush right now
		return;
	}

	// Upload any new streamed data to the http server
	UE_LOG( LogHttpReplay, Verbose, TEXT( "FHttpNetworkReplayStreamer::FlushStream. StreamFileCount: %i, Size: %i" ), StreamFileCount, StreamArchive.Buffer.Num() );

	// Create the Http request and add to pending request list
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpUploadFinished );

	HttpState = EHttptate::UploadingStream;

	HttpRequest->SetURL( FString::Printf( TEXT( "%supload?Version=%s&Session=%s&NumChunks=%i&Time=%i&Filename=stream.%i" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount + 1, TotalDemoTimeInMS, StreamFileCount ) );
	HttpRequest->SetVerb( TEXT( "POST" ) );
	HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );
	HttpRequest->SetContent( StreamArchive.Buffer );

	StreamArchive.Buffer.Empty();
	StreamArchive.Pos = 0;

	StreamFileCount++;

	HttpRequest->ProcessRequest();

	LastChunkTime = FPlatformTime::Seconds();
}

void FHttpNetworkReplayStreamer::FlushCheckpoint( const uint32 TimeInMS )
{
	check( CheckpointArchive.Buffer.Num() > 0 );
	FlushCheckpointTime = TimeInMS;
	
	// Flush any existing stream, we need checkpoints to line up with the next chunk
	check( !IsHttpBusy() || StreamArchive.Buffer.Num() == 0 );

	FlushStream();
}

void FHttpNetworkReplayStreamer::GotoCheckpoint( const uint32 TimeInMS, const FOnCheckpointReadyDelegate& Delegate )
{
	if ( IsHttpBusy() )
	{
		return;
	}

	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	// Download the next stream chunk
	HttpRequest->SetURL( FString::Printf( TEXT( "%sdownload?Version=%s&Session=%s&Filename=checkpoint.%i" ), *ServerURL, *SessionVersion, *SessionName, 0 ) );
	HttpRequest->SetVerb( TEXT( "GET" ) );

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpDownloadCheckpointFinished );

	HttpState = EHttptate::DownloadingCheckpoint;

	HttpRequest->ProcessRequest();

	GotoCheckpointDelegate = Delegate;
}

void FHttpNetworkReplayStreamer::FlushCheckpointInternal( uint32 TimeInMS )
{
	if ( SessionName.IsEmpty() || !CheckpointArchive.ArIsSaving )
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
	UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::FlushCheckpoint. Size: %i, StreamFileCount: %i" ), CheckpointArchive.Buffer.Num(), StreamFileCount );

	// Create the Http request and add to pending request list
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpUploadFinished );

	HttpState = EHttptate::UploadingStream;

	// Save the chunk to the checkpoint
	CheckpointArchive << StreamFileCount;

	HttpRequest->SetURL( FString::Printf( TEXT( "%supload?Version=%s&Session=%s&NumChunks=%i&Time=%i&Filename=checkpoint.%i" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount, TotalDemoTimeInMS, 0 ) );
	HttpRequest->SetVerb( TEXT( "POST" ) );
	HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );
	HttpRequest->SetContent( CheckpointArchive.Buffer );

	CheckpointArchive.Buffer.Empty();
	CheckpointArchive.Pos = 0;

	HttpRequest->ProcessRequest();
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
	const bool bReallyNeedToDownloadChunk = ( StreamArchive.Pos >= StreamArchive.Buffer.Num() || HighPriorityEndTime > 0 ) && bMoreChunksDefinitelyAvilable;

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
	const double REFRESH_VIEWER_IN_SECONDS = 10;

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

void FHttpNetworkReplayStreamer::SetLastError( const ENetworkReplayError::Type InLastError )
{
	StreamerState		= EStreamerState::Idle;
	StreamerLastError	= InLastError;
}

ENetworkReplayError::Type FHttpNetworkReplayStreamer::GetLastError() const
{
	return StreamerLastError;
}

FArchive* FHttpNetworkReplayStreamer::GetHeaderArchive()
{
	return ( HeaderArchive.IsSaving() || HeaderArchive.Pos < HeaderArchive.Buffer.Num() ) ? &HeaderArchive : NULL;
}

FArchive* FHttpNetworkReplayStreamer::GetStreamingArchive()
{
	return &StreamArchive;
}

FArchive* FHttpNetworkReplayStreamer::GetCheckpointArchive()
{	
	return &CheckpointArchive;
}

FArchive* FHttpNetworkReplayStreamer::GetMetadataArchive()
{
	return NULL;
}

void FHttpNetworkReplayStreamer::UpdateTotalDemoTime( uint32 TimeInMS )
{
	TotalDemoTimeInMS = TimeInMS;
}

bool FHttpNetworkReplayStreamer::IsDataAvailable() const
{
	if ( GetLastError() != ENetworkReplayError::None )
	{
		return false;
	}

	// If we are loading, and we have more data
	if ( StreamArchive.IsLoading() && StreamArchive.Pos < StreamArchive.Buffer.Num() && NumDownloadChunks > 0 )
	{
		return true;
	}
	
	return false;
}

void FHttpNetworkReplayStreamer::SetHighPriorityTimeRange( const uint32 StartTimeInMS, const uint32 EndTimeInMS )
{
	HighPriorityEndTime = EndTimeInMS;
}

bool FHttpNetworkReplayStreamer::IsDataAvailableForTimeRange( const uint32 StartTimeInMS, const uint32 EndTimeInMS )
{
	if ( GetLastError() != ENetworkReplayError::None )
	{
		return false;
	}

	// HACK: We only support the total time range for now
	return StreamFileCount >= NumDownloadChunks;
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
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpStartUploadingFinished. FAILED" ) );
		SetLastError( ENetworkReplayError::ServiceUnavailable );
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
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpStopUploadingFinished. FAILED" ) );
		SetLastError( ENetworkReplayError::ServiceUnavailable );
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
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpHeaderUploadFinished. FAILED" ) );
		SetLastError( ENetworkReplayError::ServiceUnavailable );
	}
}

void FHttpNetworkReplayStreamer::HttpUploadFinished( FHttpRequestPtr , FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::UploadingStream );
	check( StreamerState == EStreamerState::StreamingUp || StreamerState == EStreamerState::StreamingUpFinal );

	HttpState = EHttptate::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		UE_LOG( LogHttpReplay, Verbose, TEXT( "FHttpNetworkReplayStreamer::HttpUploadFinished." ) );

		if ( StreamerState == EStreamerState::StreamingUpFinal )
		{
			// Create the Http request and add to pending request list
			TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

			HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpStopUploadingFinished );

			HttpState = EHttptate::StopUploading;

			HttpRequest->SetURL( FString::Printf( TEXT( "%sstopuploading?Version=%s&Session=%s&NumChunks=%i&Time=%i" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount, TotalDemoTimeInMS ) );
			HttpRequest->SetVerb( TEXT( "POST" ) );
			HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );

			HttpRequest->ProcessRequest();
		}
	}
	else
	{
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpUploadFinished. FAILED" ) );
		SetLastError( ENetworkReplayError::ServiceUnavailable );
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
		TotalDemoTimeInMS = FCString::Atoi( *DemoTimeString );

		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpStartDownloadingFinished. Viewer: %s, State: %s, NumChunks: %i, DemoTime: %2.2f" ), *ViewerName, *State, NumDownloadChunks, (float)TotalDemoTimeInMS / 1000 );

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

			SetLastError( ENetworkReplayError::ServiceUnavailable );
		}
	}
	else
	{
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpStartDownloadingFinished. FAILED" ) );

		StartStreamingDelegate.ExecuteIfBound( false, StreamArchive.IsSaving() );

		// Reset delegate
		StartStreamingDelegate = FOnStreamReadyDelegate();

		SetLastError( ENetworkReplayError::ServiceUnavailable );
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
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished. FAILED." ) );

		StreamArchive.Buffer.Empty();
		StartStreamingDelegate.ExecuteIfBound( false, StreamArchive.IsSaving() );

		SetLastError( ENetworkReplayError::ServiceUnavailable );
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
		TotalDemoTimeInMS = FCString::Atoi( *DemoTimeString );

		if ( HttpResponse->GetContent().Num() > 0 || bStreamIsLive )
		{
			if ( HttpResponse->GetContent().Num() > 0 )
			{
				StreamArchive.Buffer.Append( HttpResponse->GetContent() );
				StreamFileCount++;
			}

			UE_LOG( LogHttpReplay, Verbose, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadFinished. State: %s, Progress: %i / %i, DemoTime: %2.2f" ), *State, StreamFileCount, NumDownloadChunks, (float)TotalDemoTimeInMS / 1000 );
		}
		else
		{
			UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadFinished. FAILED." ) );
			StreamArchive.Buffer.Empty();
			SetLastError( ENetworkReplayError::ServiceUnavailable );
		}		
	}
	else
	{
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadFinished. FAILED." ) );
		StreamArchive.Buffer.Empty();
		SetLastError( ENetworkReplayError::ServiceUnavailable );
	}
}

void FHttpNetworkReplayStreamer::HttpDownloadCheckpointFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::DownloadingCheckpoint );
	check( StreamArchive.IsLoading() );

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		StreamArchive.Buffer.Empty();
		StreamArchive.Pos = 0;
		StreamArchive.bAtEndOfReplay = false;

		CheckpointArchive.Buffer = HttpResponse->GetContent();
		CheckpointArchive.Pos = 0;

		int32 NumValues = 0;
		CheckpointArchive << NumValues;

		// Seek to the end, where we added our internal meta data
		CheckpointArchive.Seek( CheckpointArchive.Buffer.Num() - 4 );
		
		// Set the next chunk to be the first chunk that represents the start of the stream after the checkpoint
		CheckpointArchive << StreamFileCount;

		// Make sure we read to the very end
		check( CheckpointArchive.Pos == CheckpointArchive.Buffer.Num() );

		// Remove the data we serialized from the end of the checkpoint
		CheckpointArchive.Buffer.SetNum( CheckpointArchive.Buffer.Num() - 4 );

		CheckpointArchive.Pos = 0;

		GotoCheckpointDelegate.ExecuteIfBound( true );
	}
	else
	{
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadCheckpointFinished. FAILED." ) );
		SetLastError( ENetworkReplayError::ServiceUnavailable );
		GotoCheckpointDelegate.ExecuteIfBound( false );
	}

	HttpState = EHttptate::Idle;

	GotoCheckpointDelegate = FOnCheckpointReadyDelegate();
}

void FHttpNetworkReplayStreamer::HttpRefreshViewerFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::RefreshingViewer );
	check( StreamArchive.IsLoading() );

	HttpState = EHttptate::Idle;

	if ( !bSucceeded || HttpResponse->GetResponseCode() != EHttpResponseCodes::Ok )
	{
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpRefreshViewerFinished. FAILED." ) );
		SetLastError( ENetworkReplayError::ServiceUnavailable );
	}
}

void FHttpNetworkReplayStreamer::HttpEnumerateSessionsFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( HttpState == EHttptate::EnumeratingSessions );

	HttpState = EHttptate::Idle;

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{		
		UE_LOG( LogHttpReplay, Verbose, TEXT( "FHttpNetworkReplayStreamer::HttpEnumerateSessionsFinished." ) );

		TArray<FNetworkReplayStreamInfo> Streams;
		FString JsonString = HttpResponse->GetContentAsString();

		FNetworkReplayList ReplayList;

		if ( !ReplayList.FromJson( JsonString ) )
		{
			UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpEnumerateSessionsFinished. FromJson FAILED" ) );
			EnumerateStreamsDelegate.ExecuteIfBound( TArray<FNetworkReplayStreamInfo>() );		// FIXME: Notify failure here
			return;
		}

		for ( int32 i = 0; i < ReplayList.Replays.Num(); i++ )
		{
			FNetworkReplayStreamInfo NewStream;

			NewStream.Name			= ReplayList.Replays[i].SessionName;
			NewStream.FriendlyName	= ReplayList.Replays[i].FriendlyName;
			NewStream.Timestamp		= ReplayList.Replays[i].Timestamp;
			NewStream.SizeInBytes	= ReplayList.Replays[i].SizeInBytes;
			NewStream.LengthInMS	= ReplayList.Replays[i].DemoTimeInMs;
			NewStream.NumViewers	= ReplayList.Replays[i].NumViewers;
			NewStream.bIsLive		= ReplayList.Replays[i].bIsLive;;

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

void FHttpNetworkReplayStreamer::Tick( const float DeltaTime )
{
	if ( IsHttpBusy() )
	{
		return;
	}

	if ( StreamFileCount >= NumDownloadChunks && !bStreamIsLive )
	{
		StreamArchive.bAtEndOfReplay = true;
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
		if ( FlushCheckpointTime > 0 )
		{
			FlushCheckpointInternal( FlushCheckpointTime );
			FlushCheckpointTime = 0;
		}

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
		// Give viewer refreshing priority since it's quick
		RefreshViewer();
		DownloadNextChunk();
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
