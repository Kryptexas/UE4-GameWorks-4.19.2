// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NetworkReplayStreaming.h"
#include "Core.h"
#include "ModuleManager.h"

/** Default streamer that goes straight to the HD */
class FNullNetworkReplayStreamer : public INetworkReplayStreamer
{
public:
	FNullNetworkReplayStreamer() : FileAr( NULL ) {}
	virtual void StartStreaming( FString& StreamName, bool bRecord, const FOnStreamReadyDelegate& Delegate ) override;
	virtual void StopStreaming() override;
	virtual FArchive* GetStreamingArchive() override;

private:
	/** Handle to the archive that will read/write network packets */
	FArchive* FileAr;
};

class FNullNetworkReplayStreamingFactory : public INetworkReplayStreamingFactory
{
public:
	virtual TSharedPtr< INetworkReplayStreamer > CreateReplayStreamer();
};
