// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NetworkReplayStreaming.h"
#include "Http.h"
#include "Runtime/Engine/Public/Tickable.h"

/**
 * Archive used to buffer stream over http
 */
class HttpStreamFArchive : public FArchive
{
public:
	HttpStreamFArchive() : Pos( 0 ), bAtEndOfReplay( false ) {}

	virtual void	Serialize( void* V, int64 Length );
	virtual int64	Tell();
	virtual int64	TotalSize();
	virtual void	Seek( int64 InPos );
	virtual bool	AtEnd();

	TArray< uint8 >	Buffer;
	int32			Pos;
	bool			bAtEndOfReplay;
};

/**
 * Http network replay streaming manager
 */
class FHttpNetworkReplayStreamer : public INetworkReplayStreamer
{
public:
	FHttpNetworkReplayStreamer();

	/** INetworkReplayStreamer implementation */
	virtual void		StartStreaming( const FString& StreamName, bool bRecord, const FString& VersionString, const FOnStreamReadyDelegate& Delegate ) override;
	virtual void		StopStreaming() override;
	virtual FArchive*	GetHeaderArchive() override;
	virtual FArchive*	GetStreamingArchive() override;
	virtual FArchive*	GetMetadataArchive() override;
	virtual void		UpdateTotalDemoTime( uint32 TimeInMS ) override;
	virtual uint32		GetTotalDemoTime() const override { return DemoTimeInMS; }
	virtual bool		IsDataAvailable() const override;
	virtual void		SetHighPriorityTimeRange( const uint32 StartTimeInMS, const uint32 EndTimeInMS ) override;
	virtual bool		IsDataAvailableForTimeRange( const uint32 StartTimeInMS, const uint32 EndTimeInMS ) override;
	virtual bool		IsLive( const FString& StreamName ) const override;
	virtual void		DeleteFinishedStream( const FString& StreamName, const FOnDeleteFinishedStreamComplete& Delegate ) const override;
	virtual void		EnumerateStreams( const FString& VersionString, const FOnEnumerateStreamsComplete& Delegate ) override;
	virtual ENetworkReplayError::Type GetLastError() const override;

	/** FHttpNetworkReplayStreamer */
	void UploadHeader();
	void FlushStream();
	void DownloadHeader();
	void DownloadNextChunk();
	void RefreshViewer();
	void SetLastError( const ENetworkReplayError::Type InLastError );

	/** Delegates */
	void HttpStartDownloadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpDownloadHeaderFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpDownloadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpRefreshViewerFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpStartUploadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpStopUploadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpHeaderUploadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpUploadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpEnumerateSessionsFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );

	void Tick( const float DeltaTime );
	bool IsHttpBusy() const;		// True if we are waiting on http request response
	bool IsStreaming() const;		// True if we are streaming a replay up or down

	/** EHttptate - If not idle, there is a http request in flight */
	enum class EHttptate
	{
		Idle,						// There is no http request in flight
		StartUploading,				// We have made a request to start uploading a replay
		UploadingHeader,			// We are uploading the replay header
		UploadingStream,			// We are in the process of uploading the replay stream
		StopUploading,				// We have made the request to stop uploading a live replay stream
		StartDownloading,			// We have made the request to start downloading a replay stream
		DownloadingHeader,			// We are downloading the replay header
		DownloadingStream,			// We are in the process of downloading the replay stream
		RefreshingViewer,			// We are refreshing the server to let it know we're still viewing
		EnumeratingSessions,		// We are in the process of downloading the available sessions
	};

	/** EStreamerState - Overall state of the streamer */
	enum class EStreamerState
	{
		Idle,						// The streamer is idle. Either we haven't started streaming yet, or we are done
		NeedToUploadHeader,			// We are waiting to upload the header
		NeedToDownloadHeader,		// We are waiting to download the header
		StreamingUp,				// We are in the process of streaming a replay to the http server
		StreamingDown,				// We are in the process of streaming a replay from the http server
		StreamingDownFinal,			// We are done streaming (will notify server to remove us as a viewer)
		StreamingUpFinal,			// We are uploading the final stream
	};

	HttpStreamFArchive		HeaderArchive;			// Archive used to buffer the header stream
	HttpStreamFArchive		StreamArchive;			// Archive used to buffer the data stream
	FString					SessionName;			// Name of the session on the http replay server
	FString					SessionVersion;			// Version of the session
	FString					ServerURL;				// The address of the server
	int32					StreamFileCount;		// Used as a counter to increment the stream.x extension count
	double					LastChunkTime;			// The last time we uploaded/downloaded a chunk
	double					LastRefreshViewerTime;	// The last time we refreshed ourselves as an active viewer
	EStreamerState			StreamerState;			// Overall state of the streamer
	EHttptate				HttpState;				// If not idle, there is a http request in flight
	bool					bStopStreamingCalled;
	bool					bStreamIsLive;			// If true, we are viewing a live stream
	int32					NumDownloadChunks;
	uint32					DemoTimeInMS;
	FString					ViewerName;
	uint32					HighPriorityEndTime;

	ENetworkReplayError::Type		StreamerLastError;

	FOnStreamReadyDelegate			StartStreamingDelegate;		// Delegate passed in to StartStreaming
	FOnEnumerateStreamsComplete		EnumerateStreamsDelegate;
};

class FHttpNetworkReplayStreamingFactory : public INetworkReplayStreamingFactory, public FTickableGameObject
{
public:
	/** INetworkReplayStreamingFactory */
	virtual TSharedPtr< INetworkReplayStreamer > CreateReplayStreamer();

	/** FTickableGameObject */
	virtual void Tick( float DeltaTime ) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	TArray< TSharedPtr< FHttpNetworkReplayStreamer > > HttpStreamers;
};
