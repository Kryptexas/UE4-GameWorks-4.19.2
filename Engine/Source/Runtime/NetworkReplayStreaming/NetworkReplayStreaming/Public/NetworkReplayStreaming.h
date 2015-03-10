// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

// Dependencies.
#include "ModuleManager.h"
#include "Core.h"

/** Struct to store information about a stream, returned from search results. */
struct FNetworkReplayStreamInfo
{
	FNetworkReplayStreamInfo() : SizeInBytes( 0 ), LengthInMS( 0 ), NumViewers( 0 ), bIsLive( false ) {}

	/** The name of the stream (generally this is auto generated, refer to friendly name for UI) */
	FString Name;

	/** The UI friendly name of the stream */
	FString FriendlyName;

	/** The date and time the stream was recorded */
	FDateTime Timestamp;

	/** The size of the stream */
	int64 SizeInBytes;
	
	/** The duration of the stream in MS */
	int32 LengthInMS;

	/** Number of viewers viewing this stream */
	int32 NumViewers;

	/** True if the stream is live and the game hasn't completed yet */
	bool bIsLive;
};

namespace ENetworkReplayError
{
	enum Type
	{
		/** There are currently no issues */
		None,

		/** The backend service supplying the stream is unavailable, or connection interrupted */
		ServiceUnavailable,
	};

	inline const TCHAR* ToString( const ENetworkReplayError::Type FailureType )
	{
		switch ( FailureType )
		{
			case None:
				return TEXT( "None" );
			case ServiceUnavailable:
				return TEXT( "ServiceUnavailable" );
		}
		return TEXT( "Unknown ENetworkReplayError error" );
	}
}

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
	virtual void UpdateTotalDemoTime( uint32 TimeInMS ) = 0;
	virtual uint32 GetTotalDemoTime() const = 0;
	virtual bool IsDataAvailable() const = 0;
	virtual void SetHighPriorityTimeRange( const uint32 StartTimeInMS, const uint32 EndTimeInMS ) = 0;
	virtual bool IsDataAvailableForTimeRange( const uint32 StartTimeInMS, const uint32 EndTimeInMS ) = 0;
	
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
	virtual void EnumerateStreams( const FString& VersionString, const FOnEnumerateStreamsComplete& Delegate ) = 0;

	/** Returns the last error that occurred while streaming replays */
	virtual ENetworkReplayError::Type GetLastError() const = 0;
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
