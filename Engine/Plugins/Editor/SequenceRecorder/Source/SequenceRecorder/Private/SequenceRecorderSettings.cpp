// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequenceRecorderPrivatePCH.h"
#include "SequenceRecorderSettings.h"

USequenceRecorderSettings::USequenceRecorderSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateLevelSequence = true;
	SequenceLength = FAnimationRecordingSettings::DefaultMaximumLength;
	RecordingDelay = 4.0f;
	SequenceName = TEXT("RecordedSequence");
	AnimationSubDirectory = TEXT("Animations");
	SequenceRecordingBasePath.Path = TEXT("Cinematics/Sequences");
}
