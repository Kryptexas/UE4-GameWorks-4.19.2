// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

// Dependencies.
#include "ModuleManager.h"
#include "Core.h"

DECLARE_MULTICAST_DELEGATE_TwoParams( FOnStreamReady, bool, bool );
typedef FOnStreamReady::FDelegate FOnStreamReadyDelegate;

/** Generic interface for network replay streaming */
class INetworkReplayStreamer 
{
public:
	virtual ~INetworkReplayStreamer() {}

	virtual void StartStreaming( const FString& StreamName, bool bRecord, const FString& VersionString, const FOnStreamReadyDelegate& Delegate ) = 0;
	virtual void StopStreaming() = 0;
	virtual FArchive* GetHeaderArchive() = 0;
	virtual FArchive* GetStreamingArchive() = 0;
	virtual FArchive* GetMetadataArchive() = 0;
	virtual bool IsDataAvailable() const = 0;
};

/** Replay streamer factory */
class INetworkReplayStreamingFactory : public IModuleInterface
{
public:
	virtual TSharedPtr< INetworkReplayStreamer > CreateReplayStreamer() = 0;
};

/** Replay streaming factory manager */
class FNetworkReplayStreaming : public IModuleInterface
{
public:
	static inline FNetworkReplayStreaming& Get()
	{
		return FModuleManager::LoadModuleChecked< FNetworkReplayStreaming >( "NetworkReplayStreaming" );
	}

	virtual INetworkReplayStreamingFactory& GetFactory();
};
