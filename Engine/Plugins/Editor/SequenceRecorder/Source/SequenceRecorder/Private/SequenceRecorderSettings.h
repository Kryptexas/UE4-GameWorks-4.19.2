// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SequenceRecorderSettings.generated.h"

UCLASS(config=Editor, MinimalAPI)
class USequenceRecorderSettings : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	/** Whether to create a level sequence when recording. Actors and animations will be inserted into this sequence */
	UPROPERTY(EditAnywhere, Category = "Sequence Recording")
	bool bCreateLevelSequence;

	/** The length of the recorded sequence */
	UPROPERTY(EditAnywhere, Category = "Sequence Recording")
	float SequenceLength;

	/** Delay that we will use before starting recording */
	UPROPERTY(EditAnywhere, Category = "Sequence Recording")
	float RecordingDelay;

	/** The base name of the sequence to record to. This name will also be used to auto-generate any assets created by this recording. */
	UPROPERTY(EditAnywhere, Category = "Sequence Recording")
	FString SequenceName;

	/** Base path for this recording. Sub-assets will be created in subdirectories as specified */
	UPROPERTY(EditAnywhere, Category = "Sequence Recording", meta=(RelativeToGameContentDir))
	FDirectoryPath SequenceRecordingBasePath;

	/** The name of the subdirectory animations will be placed in. Leave this empty to place into the same directory as the sequence base path */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = "Sequence Recording")
	FString AnimationSubDirectory;

	/** When set, record a transform track and record animations in local space */
	UPROPERTY(EditAnywhere, Category = "Sequence Recording")
	bool bRecordTransformsAsSeperateTrack;
};