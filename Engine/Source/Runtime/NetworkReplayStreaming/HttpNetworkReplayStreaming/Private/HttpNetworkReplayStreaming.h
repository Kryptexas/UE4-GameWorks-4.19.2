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
class FHttpNetworkReplayStreamer : public INetworkReplayStreamer, public FTickableGameObject
{
public:
	/** INetworkReplayStreamer implementation */
	FHttpNetworkReplayStreamer() : StreamFileCount( 0 ), LastFlushTime( 0 ) {}
	virtual void StartStreaming( FString& StreamName, bool bRecord, const FOnStreamReadyDelegate& Delegate ) override;
	virtual void StopStreaming() override;
	virtual FArchive* GetStreamingArchive() override;
	virtual FArchive* GetMetadataArchive() override;

	/** FHttpNetworkReplayStreamer */
	void FlushStream();

	void HttpDownloadFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );
	void HttpStartStreamingUpFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );

	/** FTickableGameObject */
	virtual void Tick( float DeltaTime ) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	FOnStreamReadyDelegate	RememberedDelegate;		// Delegate passed in to StartStreaming
	HttpStreamFArchive		StreamArchive;			// Archive used to buffer the stream
	FString					SessionName;			// Name of the session on the http replay server
	int32					StreamFileCount;		// Used as a counter to increment the stream.x extension count
	double					LastFlushTime;
};

class FHttpNetworkReplayStreamingFactory : public INetworkReplayStreamingFactory
{
public:
	virtual TSharedPtr< INetworkReplayStreamer > CreateReplayStreamer();
};
