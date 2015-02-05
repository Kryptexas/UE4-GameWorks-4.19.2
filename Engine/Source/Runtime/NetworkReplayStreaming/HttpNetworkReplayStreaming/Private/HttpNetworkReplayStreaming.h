// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "NetworkReplayStreaming.h"
#include "HTTP.h"

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
	/** IHttpNetworkReplayStreamer implementation */
	FHttpNetworkReplayStreamer() {}
	virtual void StartStreaming( FString& StreamName, bool bRecord, const FOnStreamReadyDelegate& Delegate ) override;
	virtual void StopStreaming() override;
	virtual FArchive* GetStreamingArchive() override;

	void HttpRequestFinished( FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded );

	FOnStreamReadyDelegate	RememberedDelegate;
	HttpStreamFArchive		Archive;
	FString					DemoShortName;
};

class FHttpNetworkReplayStreamingFactory : public INetworkReplayStreamingFactory
{
public:
	virtual TSharedPtr< INetworkReplayStreamer > CreateReplayStreamer();
};
