// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

// Dependencies.
#include "ModuleManager.h"
#include "Core.h"

/** Struct to store information about a stream, returned from search results. */
struct FNetworkReplayStreamInfo
{
	/** The name of the stream */
	FString Name;

	/** The date and time the stream was recorded */
	FDateTime Timestamp;

	/** The size of the stream */
	int64 SizeInBytes;

	/** True if the stream is live and the game hasn't completed yet */
	bool bIsLive;
};

DECLARE_MULTICAST_DELEGATE_TwoParams( FOnStreamReady, bool, bool );
typedef FOnStreamReady::FDelegate FOnStreamReadyDelegate;

/**
 * Delegate called when DeleteFinishedStream() completes.
 *
 * @param bWasSuccessful Whether the stream was deleted.
 */
DECLARE_DELEGATE_OneParam( FOnDeleteFinishedStreamComplete, bool );

/**
 * Delegate called when EnumerateStreams() completes.
 *
 * @param Streams An array containing information about the streams that were found.
 */
DECLARE_DELEGATE_OneParam( FOnEnumerateStreamsComplete, const TArray<FNetworkReplayStreamInfo>& );

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
	
	/** Returns true if the given StreamName is a game currently in progress */
	virtual bool IsLive( const FString& StreamName ) const = 0;

	/**
	 * Attempts to delete the stream with the specified name. May execute asynchronously.
	 *
	 * @param StreamName The name of the stream to delete
	 * @param Delegate A delegate that will be executed if bound when the delete operation completes
	 */
	virtual void DeleteFinishedStream( const FString& StreamName, const FOnDeleteFinishedStreamComplete& Delegate) const = 0;

	/**
	 * Retrieves the streams that are available for viewing. May execute asynchronously.
	 *
	 * @param Delegate A delegate that will be executed if bound when the list of streams is available
	 */
	virtual void EnumerateStreams( const FOnEnumerateStreamsComplete& Delegate ) const = 0;
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
