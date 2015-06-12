// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Curves/CurveFloat.h"
#include "UserInterfaceSettings.h"

#include "CoreSettings.generated.h"


/**
 * Rendering settings.
 */
UCLASS(config=Engine, defaultconfig, meta=(DisplayName="Streaming"))
class ENGINE_API UStreamingSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UStreamingSettings();

	UPROPERTY(config, EditAnywhere, Category = PackageStreaming, meta = (
		DisplayName = "Async Loading Thread Enabled",
		ToolTip = "Enables separate thread for package streaming. Requires restart to take effect."))
	uint32 AsyncLoadingThreadEnabled : 1;

	UPROPERTY(config, EditAnywhere, Category = PackageStreaming, AdvancedDisplay, meta = (
		ConsoleVariable = "WarnIfTimeLimitExceeded", DisplayName = "Warn If Time Limit Has Been Exceeded",
		ToolTip = "Enables log warning if time limit for time-sliced package streaming has been exceeded."))
	uint32 WarnIfTimeLimitExceeded : 1;

	UPROPERTY(config, EditAnywhere, Category = PackageStreaming, AdvancedDisplay, meta = (
		ConsoleVariable = "TimeLimitExceededMultiplier", DisplayName = "Time Limit Exceeded Warning Multiplier",
		ToolTip = "Multiplier for time limit exceeded warning time threshold."))
	float TimeLimitExceededMultiplier;

	UPROPERTY(config, EditAnywhere, Category = PackageStreaming, AdvancedDisplay, meta = (
		ConsoleVariable = "TimeLimitExceededMinTime", DisplayName = "Minimum Time Limit For Time Limit Exceeded Warning",
		ToolTip = "Minimum time the time limit exceeded warning will be triggered by."))
	float TimeLimitExceededMinTime;

	UPROPERTY(config, EditAnywhere, Category = PackageStreaming, AdvancedDisplay, meta = (
		ConsoleVariable = "MinBulkDataSizeForAsyncLoading", DisplayName = "Minimum Bulk Data Size For Async Loading",
		ToolTip = "Minimum time the time limit exceeded warning will be triggered by."))
	int32 MinBulkDataSizeForAsyncLoading;

	UPROPERTY(config, EditAnywhere, Category = IO, meta = (
		ConsoleVariable = "AsyncIOBandwidthLimit", DisplayName = "Asynchronous IO Bandwidth Limit",
		ToolTip = "Constrain bandwidth if wanted. Value is in MByte/ sec."))
	float AsyncIOBandwidthLimit;

	UPROPERTY(EditAnywhere, config, Category = LevelStreaming, meta = (
		DisplayName = "Use Background Level Streaming",
		ToolTip = "Whether to allow background level streaming."))
	bool bUseBackgroundLevelStreaming;

	UPROPERTY(EditAnywhere, config, Category = LevelStreaming, AdvancedDisplay, meta = (
		DisplayName = "Async Loading Time Limit",
		ToolTip = "Maximum amount of time to spend doing asynchronous loading (ms per frame)."))
	float AsyncLoadingTimeLimit;

	/** Whether to use the entire time limit even if blocked on I/O */
	UPROPERTY(EditAnywhere, config, Category = LevelStreaming, AdvancedDisplay, meta = (
		DisplayName = "Async Loading Use Full Time Limit",
		ToolTip = "Whether to use the entire time limit even if blocked on I/O."))
	bool bAsyncLoadingUseFullTimeLimit;

	UPROPERTY(EditAnywhere, config, Category = LevelStreaming, AdvancedDisplay, meta = (
		DisplayName = "Priority Async Loading Extra Time",
		ToolTip = "Additional time to spend asynchronous loading during a high priority load."))
	float PriorityAsyncLoadingExtraTime;

	/** Maximum allowed time to spend for actor registration steps during level streaming (ms per frame)*/
	UPROPERTY(EditAnywhere, config, Category = LevelStreaming, AdvancedDisplay, meta = (
		DisplayName = "Level Streaming Actors Update Time Limit",
		ToolTip = "Maximum allowed time to spend for actor registration steps during level streaming (ms per frame)."))
	float LevelStreamingActorsUpdateTimeLimit;

	/** Batching granularity used to register actor components during level streaming */
	UPROPERTY(EditAnywhere, config, Category = LevelStreaming, AdvancedDisplay, meta = (
		DisplayName = "Level Streaming Components Registration Granularity",
		ToolTip = "Batching granularity used to register actor components during level streaming."))
	int32 LevelStreamingComponentsRegistrationGranularity;

	// Begin UObject interface
	virtual void PostInitProperties() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End UObject interface
};


/**
* Implements the settings for garbage collection.
*/
UCLASS(config = Engine, defaultconfig, meta = (DisplayName = "Garbage Collection"))
class ENGINE_API UGarbageCollectionSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:

	UGarbageCollectionSettings();

	UPROPERTY(EditAnywhere, config, Category = General, meta = (
		ConsoleVariable = "TimeBetweenPurgingPendingKillObjects", DisplayName = "Time Between Purging Pending Kill Objects",
		ToolTip = "Time in seconds (game time) we should wait between purging object references to objects that are pending kill."))
	float TimeBetweenPurgingPendingKillObjects;

	UPROPERTY(EditAnywhere, config, Category = General, meta = (
		ConsoleVariable = "FlushStreamingOnGC", DisplayName = "Flush Streaming On GC",
		ToolTip = "If enabled, streaming will be flushed each time garbage collection is triggered."))
	uint32 FlushStreamingOnGC : 1;

	UPROPERTY(EditAnywhere, config, Category = Optimization, meta = (
		ConsoleVariable = "AllowParallelGC", DisplayName = "Allow Parallel GC",
		ToolTip = "If enabled, garbage collection will use multiple threads."))
	uint32 AllowParallelGC : 1;

	UPROPERTY(EditAnywhere, config, Category = General, meta = (
		ConsoleVariable = "NumRetriesBeforeForcingGC", DisplayName = "Number Of Retries Before Forcing GC",
		ToolTip = "Maximum number of times GC can be skipped if worker threads are currently modifying UObject state. 0 = never force GC"))
	int32 NumRetriesBeforeForcingGC;

	UPROPERTY(EditAnywhere, config, Category = Optimization, meta = (
		DisplayName = "Maximum Object Count Not Considered By GC",
		ToolTip = "Maximum Object Count Not Considered By GC. Works only in cooked builds."))
	int32 MaxObjectsNotConsideredByGC;

	UPROPERTY(EditAnywhere, config, Category = Optimization, meta = (
		DisplayName = "Size Of Permanent Object Pool",
		ToolTip = "Size Of Permanent Object Pool (bytes). Works only in cooked builds."))
	int32 SizeOfPermanentObjectPool;

	// Begin UObject interface
	virtual void PostInitProperties() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	// End UObject interface
};