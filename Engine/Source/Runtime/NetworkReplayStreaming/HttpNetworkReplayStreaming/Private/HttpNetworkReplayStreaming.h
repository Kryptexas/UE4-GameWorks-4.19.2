// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NetworkReplayStreaming.h"
#include "HTTP.h"
#include "Runtime/Engine/Public/Tickable.h"

/**
 * Archive used to buffer stream over http
 */
class HttpStreamFArchive : public FArchive
{
public:
	HttpStreamFArchive() : Pos( 0 ) {}

	virtual void	Serialize( void* V, int64 Length );
	virtual int64	Tell();
	virtual int64	TotalSize();
	virtual void	Seek( int64 InPos );

	TArray< uint8 >	Buffer;
	int32			Pos;
};

/**
 * Http network replay streaming manager
 */
class FHttpNetworkReplayStreamer : public INetworkReplayStreamer
{
public:
	/** INetworkReplayStreamer implementation */
	FHttpNetworkReplayStreamer() : StreamFileCount( 0 ), LastFlushTime( 0 ), HttpState( EHttptate::Idle ), StreamerState( EStreamerState::Idle ), bStopStreamingCalled( false ), NumDownloadChunks( 0 ) {}
	virtual void StartStreaming( FString& StreamName, bool bRecord, const FOnStreamReadyDelegate& Delegate ) override;
	virtual void StopStreaming() override;
	virtual FArchive* GetHeaderArchive() override;
	virtual FArchive* GetStreamingArchive() override;
	virtual FArchive* GetMetadataArchive() override;
	virtual bool IsDataAvailable() const override;

	/** FHttpNetworkReplayStreamer */
	void UploadHeader();
	void FlushStream();
	void DownloadHeader();
	void DownloadNextChunk();

	void HttpStartDownloadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpDownloadHeaderFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpDownloadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpStartUploadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpStopUploadingFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpHeaderUploadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpUploadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );

	void Tick( float DeltaTime );

	bool IsHttpBusy() const;
	bool IsStreaming() const;

	enum class EHttptate
	{
		Idle,
		StartUploading,
		UploadingHeader,
		UploadingStream,
		StopUploading,
		StartDownloading,
		DownloadingHeader,
		DownloadingStream,
	};

	enum class EStreamerState
	{
		Idle,
		NeedToUploadHeader,
		NeedToDownloadHeader,
		StreamingUp,
		StreamingDown,
		StreamingUpFinal,
	};

	FOnStreamReadyDelegate	RememberedDelegate;		// Delegate passed in to StartStreaming
	HttpStreamFArchive		HeaderArchive;			// Archive used to buffer the header stream
	HttpStreamFArchive		StreamArchive;			// Archive used to buffer the data stream
	FString					SessionName;			// Name of the session on the http replay server
	FString					ServerURL;
	int32					StreamFileCount;		// Used as a counter to increment the stream.x extension count
	double					LastFlushTime;
	EStreamerState			StreamerState;
	EHttptate				HttpState;
	bool					bStopStreamingCalled;
	int32					NumDownloadChunks;
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
