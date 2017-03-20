// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "HAL/ThreadSafeCounter.h"
#include "Interfaces/IHttpRequest.h"

class FHttpServiceTracker;
class IAnalyticsProvider;
struct FAnalyticsEventAttribute;

/**
 * A struct providing static access for recording analytics events.
 */
struct FBuildPatchAnalytics
{
private:
	// The analytics provider interface
	static TSharedPtr<IAnalyticsProvider> Analytics;

	// The analytics provider interface
	static TSharedPtr<FHttpServiceTracker> HttpTracker;

	// Avoid spamming download errors
	static FThreadSafeCounter DownloadErrors;

	// Avoid spamming cache errors
	static FThreadSafeCounter CacheErrors;

	// Avoid spamming construction errors
	static FThreadSafeCounter ConstructionErrors;

public:
	/**
	 * Set the analytics provider to be used when recording events
	 * @param AnalyticsProvider		Shared ptr to an analytics interface to use. If NULL analytics will be disabled.
	 */
	static void SetAnalyticsProvider(TSharedPtr< IAnalyticsProvider > AnalyticsProvider);

	/**
	 * Records an analytics event, must be called from the main thread only
	 * @param EventName		The name of the event being sent
	 * @param Attributes	The attributes for the event
	 */
	static void RecordEvent(const FString& EventName, const TArray<FAnalyticsEventAttribute>& Attributes);

	/**
	 * Resets the event counters that monitor for spamming and limit the number of events that are sent
	 */
	static void ResetCounters();

	/**
	 * @EventName Patcher.Error.Download
	 *
	 * @Trigger Fires for the first 20 chunk download errors that occur. Deprecated: Not recommenced for use.
	 *
	 * @Type Static
	 *
	 * @EventParam ChunkURL (string) The url used to request the chunk.
	 * @EventParam ResponseCode (int) The http response code received, or OS error code for file operation.
	 * @EventParam ErrorString (string) The stage at which the chunk was deemed error. "Download Fail", "Uncompress Fail", "Verify Fail", or "SaveToDisk Fail".
	 *
	 * @Owner Portal Team
	 */
	static void RecordChunkDownloadError(const FString& ChunkUrl, const int32& ResponseCode, const FString& ErrorString);

	/**
	 * @EventName Patcher.Warning.ChunkAborted
	 *
	 * @Trigger Fires for chunks that were aborted as deemed failing. This is a prediction that the chunk is suffering from 0-byte TCP window issue. Deprecated: Not recommenced for use.
	 *
	 * @Type Static
	 *
	 * @EventParam ChunkURL (string) The url used to request the chunk.
	 * @EventParam ChunkTime (double) The amount of time the chunk has been downloading for in seconds.
	 * @EventParam ChunkMean (double) The current mean average chunk download time in seconds.
	 * @EventParam ChunkStd (double) The current standard deviation of chunk download times.
	 * @EventParam BreakingPoint (double) The time at which a chunk will be aborted based on the current mean and std.
	 *
	 * @Owner Portal Team
	 */
	static void RecordChunkDownloadAborted(const FString& ChunkUrl, const double& ChunkTime, const double& ChunkMean, const double& ChunkStd, const double& BreakingPoint);

	/**
	 * @EventName Patcher.Error.Cache
	 *
	 * @Trigger Fires for the first 20 chunks which experienced a cache error. Deprecated: Not recommenced for use.
	 *
	 * @Type Static
	 *
	 * @EventParam ChunkGuid (string) The guid of the effected chunk.
	 * @EventParam Filename (string) The filename used if appropriate, or -1.
	 * @EventParam LastError (int) The OS error code if appropriate.
	 * @EventParam SystemName (string) The cache sub-system that is reporting this. "ChunkBooting", "ChunkRecycle", "DriveCache".
	 * @EventParam ErrorString (string) The stage at which the chunk was deemed error. "Hash Check Failed", "Incorrect File Size", "Datasize/Hashtype Mismatch", "Corrupt Header", "Open File Fail", "Source File Missing", "Source File Too Small", "Chunk Hash Fail", or "Chunk Save Failed".
	 *
	 * @Owner Portal Team
	 */
	static void RecordChunkCacheError(const FGuid& ChunkGuid, const FString& Filename, const int32 LastError, const FString& SystemName, const FString& ErrorString);

	/**
	 * @EventName Patcher.Error.Construction
	 *
	 * @Trigger Fires for the first 20 errors experienced while constructing an installation file. Deprecated: Not recommenced for use.
	 *
	 * @Type Static
	 *
	 * @EventParam Filename (string) The filename of the installation file.
	 * @EventParam LastError (int) The OS error code if appropriate, or -1.
	 * @EventParam ErrorString (string) The reason the error occurred. "Missing Chunk", "Not Enough Disk Space", "Could Not Create File", "Missing File Manifest", "Serialised Verify Fail", "File Data Missing", "File Data Uncompress Fail", "File Data Verify Fail", "File Data GUID Mismatch", "File Data Part OOB", "File Data Unknown Storage", or "Failed To Move".
	 *
	 * @Owner Portal Team
	 */
	static void RecordConstructionError(const FString& Filename, const int32 LastError, const FString& ErrorString);

	/**
	 * @EventName Patcher.Error.Prerequisites
	 *
	 * @Trigger Fires for errors experienced while installing prerequisites. Deprecated: Not recommenced for use.
	 *
	 * @Type Static
	 *
	 * @EventParam AppName (string) The app name for the installation that is running.
	 * @EventParam AppVersion (string) The app version for the installation that is running.
	 * @EventParam Filename (string) The filename used when running prerequisites.
	 * @EventParam CommandLine (string) The command line used when running prerequisites.
	 * @EventParam ReturnCode (int) The error or return code from running the prerequisite.
	 * @EventParam ErrorString (string) The action that failed. "Failed to start installer", or "Failed to install".
	 *
	 * @Owner Portal Team
	 */
	static void RecordPrereqInstallationError(const FString& AppName, const FString& AppVersion, const FString& Filename, const FString& CommandLine, const int32 ErrorCode, const FString& ErrorString);

	/**
	 * Set the Http Service Tracker to be used for tracking Http Service responsiveness.
	 * Will only track HTTP requests, not file requests.
	 * @param HttpTracker	Shared ptr to an Http service tracker interface to use. If NULL tracking will be disabled.
	 */
	static void SetHttpTracker(TSharedPtr< FHttpServiceTracker > HttpTracker);

	/**
	 * Tracks a specific Http request. Currently there is only one endpoint, CDN.Chunk.
	 * In the future we may decide to break up, say, file from chunk downloads, but not now.
	 *
	 * @param const FHttpRequestPtr & Request
	 */
	static void TrackRequest(const FHttpRequestPtr& Request);
};
