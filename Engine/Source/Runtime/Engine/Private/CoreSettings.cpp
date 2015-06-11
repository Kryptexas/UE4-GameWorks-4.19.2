// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/CoreSettings.h"

UStreamingSettings::UStreamingSettings()
	: Super()
{
	SectionName = TEXT("Streaming");

	AsyncLoadingThreadEnabled = false;
	WarnIfTimeLimitExceeded = false;
	TimeLimitExceededMultiplier = 1.5f;
	TimeLimitExceededMinTime = 0.005f;
	MinBulkDataSizeForAsyncLoading = 131072;
	AsyncIOBandwidthLimit = 0;
	bUseBackgroundLevelStreaming = true;
	AsyncLoadingTimeLimit = 5.0f;
	bAsyncLoadingUseFullTimeLimit = true;
	PriorityAsyncLoadingExtraTime = 20.0f;
	LevelStreamingActorsUpdateTimeLimit = 5.0f;
	LevelStreamingComponentsRegistrationGranularity = 10;
}

void UStreamingSettings::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	if (IsTemplate())
	{
		ImportConsoleVariableValues();
	}
#endif // #if WITH_EDITOR
}

#if WITH_EDITOR
void UStreamingSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		ExportValuesToConsoleVariables(PropertyChangedEvent.Property);
	}
}
#endif // #if WITH_EDITOR

UGarbageCollectionSettings::UGarbageCollectionSettings()
: Super()
{
	SectionName = TEXT("Garbage Collection");

	TimeBetweenPurgingPendingKillObjects = 60.0f;
	FlushStreamingOnGC = false;
	AllowParallelGC = true;
	NumRetriesBeforeForcingGC = 0;
	MaxObjectsNotConsideredByGC = 0;
	SizeOfPermanentObjectPool = 0;
}

void UGarbageCollectionSettings::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITOR
	if (IsTemplate())
	{
		ImportConsoleVariableValues();
	}
#endif // #if WITH_EDITOR
}

#if WITH_EDITOR
void UGarbageCollectionSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		ExportValuesToConsoleVariables(PropertyChangedEvent.Property);
	}
}
#endif // #if WITH_EDITOR
