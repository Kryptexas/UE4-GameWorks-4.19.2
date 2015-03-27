// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NetworkReplayStreaming.h"
#include "Core.h"
#include "ModuleManager.h"
#include "UniquePtr.h"

/** Default streamer that goes straight to the HD */
class FNullNetworkReplayStreamer : public INetworkReplayStreamer
{
public:
	FNullNetworkReplayStreamer() :
		StreamerState( EStreamerState::Idle )
	{}
	
	/** INetworkReplayStreamer implementation */
	virtual void StartStreaming( const FString& StreamName, bool bRecord, const FString& VersionString, const FOnStreamReadyDelegate& Delegate ) override;
	virtual void StopStreaming() override;
	virtual FArchive* GetHeaderArchive() override;
	virtual FArchive* GetStreamingArchive() override;
	virtual FArchive* GetCheckpointArchive() override { return NULL; }
	virtual void FlushCheckpoint( const uint32 TimeInMS ) override { }
	virtual void GotoCheckpointIndex( const int32 TimeInMS, const FOnCheckpointReadyDelegate& Delegate ) override { }
	virtual void GotoTimeInMS( const uint32 TimeInMS, const FOnCheckpointReadyDelegate& Delegate ) override { }
	virtual FArchive* GetMetadataArchive() override;
	virtual void UpdateTotalDemoTime( uint32 TimeInMS ) override { }
	virtual uint32 GetTotalDemoTime() const override { return 0; }
	virtual bool IsDataAvailable() const override { return true; }
	virtual void SetHighPriorityTimeRange( const uint32 StartTimeInMS, const uint32 EndTimeInMS ) override { }
	virtual bool IsDataAvailableForTimeRange( const uint32 StartTimeInMS, const uint32 EndTimeInMS ) override { return true; }
	virtual bool IsLive( const FString& StreamName ) const override;
	virtual void DeleteFinishedStream( const FString& StreamName, const FOnDeleteFinishedStreamComplete& Delegate) const override;
	virtual void EnumerateStreams( const FString& VersionString, const FOnEnumerateStreamsComplete& Delegate ) override;
	virtual ENetworkReplayError::Type GetLastError() const override { return ENetworkReplayError::None; }

private:
	/** Handle to the archive that will read/write network packets */
	TUniquePtr<FArchive> FileAr;

	/* Handle to the archive that will read/write metadata */
	TUniquePtr<FArchive> MetadataFileAr;

	/** EStreamerState - Overall state of the streamer */
	enum class EStreamerState
	{
		Idle,					// The streamer is idle. Either we haven't started streaming yet, or we are done
		Recording,				// We are in the process of recording a replay to disk
		Playback,				// We are in the process of playing a replay from disk
	};

	/** Overall state of the streamer */
	EStreamerState StreamerState;

	/** Remember the name of the current stream, if any. */
	FString CurrentStreamName;
};

class FNullNetworkReplayStreamingFactory : public INetworkReplayStreamingFactory
{
public:
	virtual TSharedPtr< INetworkReplayStreamer > CreateReplayStreamer();
};
