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

void FHttpStreamFArchive::Serialize( void* V, int64 Length ) 
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

int64 FHttpStreamFArchive::Tell() 
{
	return Pos;
}

int64 FHttpStreamFArchive::TotalSize()
{
	return Buffer.Num();
}

void FHttpStreamFArchive::Seek( int64 InPos ) 
{
	check( InPos < Buffer.Num() );

	Pos = InPos;
}

bool FHttpStreamFArchive::AtEnd() 
{
	return Pos >= Buffer.Num() && bAtEndOfReplay;
}

FHttpNetworkReplayStreamer::FHttpNetworkReplayStreamer() : 
	StreamFileCount( 0 ), 
	LastChunkTime( 0 ), 
	LastRefreshViewerTime( 0 ),
	StreamerState( EStreamerState::Idle ), 
	bStopStreamingCalled( false ), 
	bNeedToUploadHeader( false ),
	bStreamIsLive( false ),
	NumDownloadChunks( 0 ),
	TotalDemoTimeInMS( 0 ),
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

	if ( IsHttpRequestInFlight() )
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::StartStreaming. IsHttpRequestInFlight == true." ) );
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

	if ( !bRecord )
	{
		// We are streaming down
		StreamerState = EStreamerState::StreamingDown;

		SessionName = StreamName;

		// Notify the http server that we want to start downloading a replay
		HttpRequest->SetURL( FString::Printf( TEXT( "%sstartdownloading?Version=%s&Session=%s" ), *ServerURL, *SessionVersion, *SessionName ) );
		HttpRequest->SetVerb( TEXT( "POST" ) );

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpStartDownloadingFinished );
	
		AddRequestToQueue( EQueuedHttpRequestType::StartDownloading, HttpRequest );

		DownloadHeader();
	}
	else
	{
		// We are streaming up
		StreamerState = EStreamerState::StreamingUp;

		SessionName.Empty();

		// We can't upload the header until we have the session name, which depends on the StartUploading task above to finish
		bNeedToUploadHeader = true;

		// Notify the http server that we want to start uploading a replay
		HttpRequest->SetURL( FString::Printf( TEXT( "%sstartuploading?Version=%s&Friendly=%s" ), *ServerURL, *SessionVersion, *StreamName ) );

		HttpRequest->SetVerb( TEXT( "POST" ) );

		HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpStartUploadingFinished );

		AddRequestToQueue( EQueuedHttpRequestType::StartUploading, HttpRequest );
	}
}

void FHttpNetworkReplayStreamer::AddRequestToQueue( const EQueuedHttpRequestType Type, TSharedPtr< class IHttpRequest >	Request )
{
	UE_LOG( LogHttpReplay, Verbose, TEXT( "FHttpNetworkReplayStreamer::AddRequestToQueue. Type: %i" ), (int)Type );

	QueuedHttpRequests.Enqueue( TSharedPtr< FQueuedHttpRequest >( new FQueuedHttpRequest( Type, Request ) ) );
}

void FHttpNetworkReplayStreamer::StopStreaming()
{
	if ( !IsStreaming() )
	{
		check( bStopStreamingCalled == false );
		return;
	}

	if ( bStopStreamingCalled )
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::StopStreaming. Already called" ) );
		return;
	}

	bStopStreamingCalled = true;

	if ( StreamerState == EStreamerState::StreamingDown )
	{
		// Refresh one last time, pasing in bFinal == true, this will notify the server we're done (remove us from viewers list)
		RefreshViewer( true );
	}
	else if ( StreamerState == EStreamerState::StreamingUp )
	{
		// Flush any final pending stream
		FlushStream();
		// Send one last http request to stop uploading		
		StopUploading();
	}
}

void FHttpNetworkReplayStreamer::UploadHeader()
{
	check( StreamArchive.ArIsSaving );

	if ( SessionName.IsEmpty() )
	{
		// IF there is no active session, or we are not recording, we don't need to flush
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::UploadHeader. No session name!" ) );
		return;
	}

	if ( HeaderArchive.Buffer.Num() == 0 )
	{
		// Header wasn't serialized
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::UploadHeader. No header to upload" ) );
		return;
	}

	check( StreamFileCount == 0 );

	// First upload the header
	UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::UploadHeader. Header. StreamFileCount: %i, Size: %i" ), StreamFileCount, StreamArchive.Buffer.Num() );

	// Create the Http request and add to pending request list
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpHeaderUploadFinished );

	HttpRequest->SetURL( FString::Printf( TEXT( "%supload?Version=%s&Session=%s&NumChunks=%i&Time=%i&Filename=replay.header" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount, TotalDemoTimeInMS ) );
	HttpRequest->SetVerb( TEXT( "POST" ) );
	HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );
	HttpRequest->SetContent( HeaderArchive.Buffer );

	// We're done with the header archive
	HeaderArchive.Buffer.Empty();
	HeaderArchive.Pos = 0;

	AddRequestToQueue( EQueuedHttpRequestType::UploadingHeader, HttpRequest );

	LastChunkTime = FPlatformTime::Seconds();
}

void FHttpNetworkReplayStreamer::FlushStream()
{
	check( StreamArchive.ArIsSaving );

	if ( bNeedToUploadHeader )
	{
		// If we haven't uploaded the header, or we are not recording, we don't need to flush
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::FlushStream. Waiting on header upload." ) );
		return;
	}

	if ( StreamArchive.Buffer.Num() == 0 )
	{
		// Nothing to flush
		return;
	}

	// Upload any new streamed data to the http server
	UE_LOG( LogHttpReplay, Verbose, TEXT( "FHttpNetworkReplayStreamer::FlushStream. StreamFileCount: %i, Size: %i" ), StreamFileCount, StreamArchive.Buffer.Num() );

	// Create the Http request and add to pending request list
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpUploadStreamFinished );

	HttpRequest->SetURL( FString::Printf( TEXT( "%supload?Version=%s&Session=%s&NumChunks=%i&Time=%i&Filename=stream.%i" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount + 1, TotalDemoTimeInMS, StreamFileCount ) );
	HttpRequest->SetVerb( TEXT( "POST" ) );
	HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );
	HttpRequest->SetContent( StreamArchive.Buffer );

	StreamArchive.Buffer.Empty();
	StreamArchive.Pos = 0;

	StreamFileCount++;

	AddRequestToQueue( EQueuedHttpRequestType::UploadingStream, HttpRequest );

	LastChunkTime = FPlatformTime::Seconds();
}

void FHttpNetworkReplayStreamer::ConditionallyFlushStream()
{
	const double FLUSH_TIME_IN_SECONDS = 10;

	if ( FPlatformTime::Seconds() - LastChunkTime > FLUSH_TIME_IN_SECONDS )
	{
		FlushStream();
	}
};

void FHttpNetworkReplayStreamer::StopUploading()
{
	// Create the Http request and add to pending request list
	TSharedRef< IHttpRequest > HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpStopUploadingFinished );

	HttpRequest->SetURL( FString::Printf( TEXT( "%sstopuploading?Version=%s&Session=%s&NumChunks=%i&Time=%i" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount, TotalDemoTimeInMS ) );
	HttpRequest->SetVerb( TEXT( "POST" ) );
	HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );

	AddRequestToQueue( EQueuedHttpRequestType::StopUploading, HttpRequest );
};

void FHttpNetworkReplayStreamer::FlushCheckpoint( const uint32 TimeInMS )
{
	if ( CheckpointArchive.Buffer.Num() == 0 )
	{
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::FlushCheckpoint. Checkpoint is empty." ) );
		return;
	}
	
	// Flush any existing stream, we need checkpoints to line up with the next chunk
	FlushStream();
	// Flush the checkpoint
	FlushCheckpointInternal( TimeInMS );
}

void FHttpNetworkReplayStreamer::GotoCheckpoint( const uint32 TimeInMS, const FOnCheckpointReadyDelegate& Delegate )
{
	if ( GotoCheckpointDelegate.IsBound() )
	{
		// If we're currently going to a checkpoint now, ignore this request
		UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::GotoCheckpoint. Busy processing another checkpoint." ) );
		return;
	}

	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	// Download the next stream chunk
	HttpRequest->SetURL( FString::Printf( TEXT( "%sdownload?Version=%s&Session=%s&Filename=checkpoint.%i" ), *ServerURL, *SessionVersion, *SessionName, TimeInMS ) );
	HttpRequest->SetVerb( TEXT( "GET" ) );

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpDownloadCheckpointFinished );

	GotoCheckpointDelegate = Delegate;

	AddRequestToQueue( EQueuedHttpRequestType::DownloadingCheckpoint, HttpRequest );
}

void FHttpNetworkReplayStreamer::FlushCheckpointInternal( uint32 TimeInMS )
{
	if ( SessionName.IsEmpty() || StreamerState != EStreamerState::StreamingUp || CheckpointArchive.Buffer.Num() == 0 )
	{
		// If there is no active session, or we are not recording, we don't need to flush
		CheckpointArchive.Buffer.Empty();
		CheckpointArchive.Pos = 0;
		return;
	}

	// Upload any new streamed data to the http server
	UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::FlushCheckpointInternal. Size: %i, StreamFileCount: %i" ), CheckpointArchive.Buffer.Num(), StreamFileCount );

	// Create the Http request and add to pending request list
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpUploadCheckpointFinished );

	// Save the chunk to the checkpoint
	CheckpointArchive << StreamFileCount;

	HttpRequest->SetURL( FString::Printf( TEXT( "%supload?Version=%s&Session=%s&NumChunks=%i&Time=%i&Filename=checkpoint.%i" ), *ServerURL, *SessionVersion, *SessionName, StreamFileCount, TotalDemoTimeInMS, TimeInMS ) );
	HttpRequest->SetVerb( TEXT( "POST" ) );
	HttpRequest->SetHeader( TEXT( "Content-Type" ), TEXT( "application/octet-stream" ) );
	HttpRequest->SetContent( CheckpointArchive.Buffer );

	CheckpointArchive.Buffer.Empty();
	CheckpointArchive.Pos = 0;

	AddRequestToQueue( EQueuedHttpRequestType::UploadingCheckpoint, HttpRequest );
}

void FHttpNetworkReplayStreamer::DownloadHeader()
{
	// Download header first
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	HttpRequest->SetURL( FString::Printf( TEXT( "%sdownload?Version=%s&Session=%s&Filename=replay.header" ), *ServerURL, *SessionVersion, *SessionName, *ViewerName ) );
	HttpRequest->SetVerb( TEXT( "GET" ) );

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished );

	AddRequestToQueue( EQueuedHttpRequestType::DownloadingHeader, HttpRequest );
}

void FHttpNetworkReplayStreamer::ConditionallyDownloadNextChunk()
{
	if ( IsHttpRequestInFlight() )
	{
		// We never download next chunk if there is an http request in flight
		return;
	}

	if ( GotoCheckpointDelegate.IsBound() )
	{
		// Don't download stream while we're waiting for a checkpoint to download
		return;
	}

	const bool bMoreChunksDefinitelyAvilable = StreamFileCount < NumDownloadChunks;
	const bool bMoreChunksMayBeAvilable = bStreamIsLive;

	if ( !bMoreChunksDefinitelyAvilable && !bMoreChunksMayBeAvilable )
	{
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

	AddRequestToQueue( EQueuedHttpRequestType::DownloadingStream, HttpRequest );

	LastChunkTime = FPlatformTime::Seconds();
}

void FHttpNetworkReplayStreamer::RefreshViewer( const bool bFinal )
{
	TSharedRef<class IHttpRequest> HttpRequest = FHttpModule::Get().CreateRequest();

	// Download the next stream chunk
	if ( bFinal )
	{
		HttpRequest->SetURL( FString::Printf( TEXT( "%srefreshviewer?Version=%s&Session=%s&Viewer=%s&Final=true" ), *ServerURL, *SessionVersion, *SessionName, *ViewerName ) );
	}
	else
	{
		HttpRequest->SetURL( FString::Printf( TEXT( "%srefreshviewer?Version=%s&Session=%s&Viewer=%s" ), *ServerURL, *SessionVersion, *SessionName, *ViewerName ) );
	}

	HttpRequest->SetVerb( TEXT( "POST" ) );

	HttpRequest->OnProcessRequestComplete().BindRaw( this, &FHttpNetworkReplayStreamer::HttpRefreshViewerFinished );

	AddRequestToQueue( EQueuedHttpRequestType::RefreshingViewer, HttpRequest );

	LastRefreshViewerTime = FPlatformTime::Seconds();
}

void FHttpNetworkReplayStreamer::ConditionallyRefreshViewer()
{
	const double REFRESH_VIEWER_IN_SECONDS = 10;

	if ( FPlatformTime::Seconds() - LastRefreshViewerTime > REFRESH_VIEWER_IN_SECONDS )
	{
		RefreshViewer( false );
	}
};

void FHttpNetworkReplayStreamer::SetLastError( const ENetworkReplayError::Type InLastError )
{
	// Cancel any in flight request
	if ( InFlightHttpRequest.IsValid() )
	{
		InFlightHttpRequest->Request->CancelRequest();
		InFlightHttpRequest = NULL;
	}

	// Empty the request queue
	TSharedPtr< FQueuedHttpRequest > QueuedRequest;
	while ( QueuedHttpRequests.Dequeue( QueuedRequest ) ) { }

	StreamerState		= EStreamerState::Idle;
	StreamerLastError	= InLastError;
}

ENetworkReplayError::Type FHttpNetworkReplayStreamer::GetLastError() const
{
	return StreamerLastError;
}

FArchive* FHttpNetworkReplayStreamer::GetHeaderArchive()
{
	return &HeaderArchive;
}

FArchive* FHttpNetworkReplayStreamer::GetStreamingArchive()
{
	return &StreamArchive;
}

FArchive* FHttpNetworkReplayStreamer::GetCheckpointArchive()
{	
	if ( bNeedToUploadHeader )
	{
		// If we need to upload the header, we're not ready to save checkpoints
		// NOTE - The code needs to be resilient to this, and keep trying!!!!
		return NULL;
	}

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
	if ( IsHttpRequestInFlight() )
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

	AddRequestToQueue( EQueuedHttpRequestType::EnumeratingSessions, HttpRequest );
}

void FHttpNetworkReplayStreamer::RequestFinished( EStreamerState ExpectedStreamerState, EQueuedHttpRequestType ExpectedType, FHttpRequestPtr HttpRequest )
{
	check( StreamerState == ExpectedStreamerState );
	check( InFlightHttpRequest.IsValid() );
	check( InFlightHttpRequest->Request == HttpRequest );
	check( InFlightHttpRequest->Type == ExpectedType );

	InFlightHttpRequest = NULL;
};

void FHttpNetworkReplayStreamer::HttpStartUploadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	RequestFinished( EStreamerState::StreamingUp, EQueuedHttpRequestType::StartUploading, HttpRequest );

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
	RequestFinished( EStreamerState::StreamingUp, EQueuedHttpRequestType::StopUploading, HttpRequest );

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
	SessionName.Empty();
}

void FHttpNetworkReplayStreamer::HttpHeaderUploadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	RequestFinished( EStreamerState::StreamingUp, EQueuedHttpRequestType::UploadingHeader, HttpRequest );

	check( StartStreamingDelegate.IsBound() );

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpHeaderUploadFinished." ) );

		StartStreamingDelegate.ExecuteIfBound( true, true );
	}
	else
	{
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpHeaderUploadFinished. FAILED" ) );
		StartStreamingDelegate.ExecuteIfBound( false, true );
		SetLastError( ENetworkReplayError::ServiceUnavailable );
	}

	// Reset delegate
	StartStreamingDelegate = FOnStreamReadyDelegate();
}

void FHttpNetworkReplayStreamer::HttpUploadStreamFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	RequestFinished( EStreamerState::StreamingUp, EQueuedHttpRequestType::UploadingStream, HttpRequest );

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		UE_LOG( LogHttpReplay, Verbose, TEXT( "FHttpNetworkReplayStreamer::HttpUploadStreamFinished." ) );
	}
	else
	{
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpUploadStreamFinished. FAILED" ) );
		SetLastError( ENetworkReplayError::ServiceUnavailable );
	}
}

void FHttpNetworkReplayStreamer::HttpUploadCheckpointFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	RequestFinished( EStreamerState::StreamingUp, EQueuedHttpRequestType::UploadingCheckpoint, HttpRequest );

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		UE_LOG( LogHttpReplay, Verbose, TEXT( "FHttpNetworkReplayStreamer::HttpUploadCheckpointFinished." ) );
	}
	else
	{
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpUploadCheckpointFinished. FAILED" ) );
		SetLastError( ENetworkReplayError::ServiceUnavailable );
	}
}

void FHttpNetworkReplayStreamer::HttpStartDownloadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	RequestFinished( EStreamerState::StreamingDown, EQueuedHttpRequestType::StartDownloading, HttpRequest );

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
			// The next queued request should be the download header request
		}
		else
		{
			UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpStartDownloadingFinished. NO CHUNKS" ) );

			StartStreamingDelegate.ExecuteIfBound( false, true );

			// Reset delegate
			StartStreamingDelegate = FOnStreamReadyDelegate();

			SetLastError( ENetworkReplayError::ServiceUnavailable );
		}
	}
	else
	{
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpStartDownloadingFinished. FAILED" ) );

		StartStreamingDelegate.ExecuteIfBound( false, true );

		// Reset delegate
		StartStreamingDelegate = FOnStreamReadyDelegate();

		SetLastError( ENetworkReplayError::ServiceUnavailable );
	}
}

void FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	RequestFinished( EStreamerState::StreamingDown, EQueuedHttpRequestType::DownloadingHeader, HttpRequest );

	check( StreamArchive.IsLoading() );
	check( StartStreamingDelegate.IsBound() );

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		HeaderArchive.Buffer.Append( HttpResponse->GetContent() );

		UE_LOG( LogHttpReplay, Log, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished. Size: %i" ), HeaderArchive.Buffer.Num() );

		StartStreamingDelegate.ExecuteIfBound( true, false );
	}
	else
	{
		// FAIL
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadHeaderFinished. FAILED." ) );

		StreamArchive.Buffer.Empty();
		StartStreamingDelegate.ExecuteIfBound( false, false );

		SetLastError( ENetworkReplayError::ServiceUnavailable );
	}

	// Reset delegate
	StartStreamingDelegate = FOnStreamReadyDelegate();
}

void FHttpNetworkReplayStreamer::HttpDownloadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	RequestFinished( EStreamerState::StreamingDown, EQueuedHttpRequestType::DownloadingStream, HttpRequest );

	check( StreamArchive.IsLoading() );

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
	RequestFinished( EStreamerState::StreamingDown, EQueuedHttpRequestType::DownloadingCheckpoint, HttpRequest );

	check( StreamArchive.IsLoading() );
	check( GotoCheckpointDelegate.IsBound() );

	if ( bSucceeded && HttpResponse->GetResponseCode() == EHttpResponseCodes::Ok )
	{
		if ( HttpResponse->GetContent().Num() == 0 )
		{
			UE_LOG( LogHttpReplay, Warning, TEXT( "FHttpNetworkReplayStreamer::HttpDownloadCheckpointFinished. Checkpoint empty." ) );
			GotoCheckpointDelegate.ExecuteIfBound( true );
			return;
		}

		StreamArchive.Buffer.Empty();
		StreamArchive.Pos = 0;
		StreamArchive.bAtEndOfReplay = false;

		CheckpointArchive.Buffer = HttpResponse->GetContent();
		CheckpointArchive.Pos = 0;

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
		//SetLastError( ENetworkReplayError::ServiceUnavailable );
		GotoCheckpointDelegate.ExecuteIfBound( false );
	}

	GotoCheckpointDelegate = FOnCheckpointReadyDelegate();
}

void FHttpNetworkReplayStreamer::HttpRefreshViewerFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	RequestFinished( EStreamerState::StreamingDown, EQueuedHttpRequestType::RefreshingViewer, HttpRequest );

	if ( !bSucceeded || HttpResponse->GetResponseCode() != EHttpResponseCodes::Ok )
	{
		UE_LOG( LogHttpReplay, Error, TEXT( "FHttpNetworkReplayStreamer::HttpRefreshViewerFinished. FAILED." ) );
		SetLastError( ENetworkReplayError::ServiceUnavailable );
	}
}

void FHttpNetworkReplayStreamer::HttpEnumerateSessionsFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded )
{
	check( InFlightHttpRequest.IsValid() );
	check( InFlightHttpRequest->Request == HttpRequest );
	check( InFlightHttpRequest->Type == EQueuedHttpRequestType::EnumeratingSessions );

	InFlightHttpRequest = NULL;

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

bool FHttpNetworkReplayStreamer::ProcessNextHttpRequest()
{
	if ( IsHttpRequestInFlight() )
	{
		// We only process one http request at a time to keep things simple
		return false;
	}

	TSharedPtr< FQueuedHttpRequest > QueuedRequest;

	if ( QueuedHttpRequests.Dequeue( QueuedRequest ) )
	{
		UE_LOG( LogHttpReplay, Verbose, TEXT( "FHttpNetworkReplayStreamer::ProcessNextHttpRequest. Dequeue Type: %i" ), (int)QueuedRequest->Type );

		check( !InFlightHttpRequest.IsValid() );

		InFlightHttpRequest = QueuedRequest;
		InFlightHttpRequest->Request->ProcessRequest();

		// We can return now since we know a http request is now in flight
		return true;
	}

	return false;
}

void FHttpNetworkReplayStreamer::Tick( const float DeltaTime )
{
	if ( IsHttpRequestInFlight() )
	{
		// We only process one http request at a time to keep things simple
		return;
	}

	// Attempt to process the next http request
	if ( ProcessNextHttpRequest() )
	{
		// We can now return since we know there is a request in flight
		check( IsHttpRequestInFlight() );
		return;
	}

	// We should have no pending or requests in flight at this point
	check( !HasPendingHttpRequests() );

	// See if we're waiting on uploading the header
	// We have to do it this way, because the header depends on the session name, and we don't have that until
	if ( bNeedToUploadHeader )
	{
		// We can't upload the header until we have the session name, so keep waiting on it
		if ( !SessionName.IsEmpty() )
		{
			UploadHeader();
			bNeedToUploadHeader = false;
		}

		return;
	}

	if ( bStopStreamingCalled )
	{
		// If we have a pending stop streaming request, we can fully stop once all http requests are processed
		check( IsStreaming() );
		StreamerState = EStreamerState::Idle;
		bStopStreamingCalled = false;
		return;
	}

	if ( StreamerState == EStreamerState::StreamingUp )
	{
		ConditionallyFlushStream();
	}
	else if ( StreamerState == EStreamerState::StreamingDown )
	{
		if ( StreamFileCount >= NumDownloadChunks && !bStreamIsLive )
		{
			StreamArchive.bAtEndOfReplay = true;
		}

		ConditionallyRefreshViewer();
		ConditionallyDownloadNextChunk();
	}
}

bool FHttpNetworkReplayStreamer::IsHttpRequestInFlight() const
{
	return InFlightHttpRequest.IsValid();
}

bool FHttpNetworkReplayStreamer::HasPendingHttpRequests() const
{
	// If there is currently one in flight, or we have more to process, return true
	return IsHttpRequestInFlight() || !QueuedHttpRequests.IsEmpty();
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
		if ( HttpStreamers[i].IsUnique() && !HttpStreamers[i]->HasPendingHttpRequests() )
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
