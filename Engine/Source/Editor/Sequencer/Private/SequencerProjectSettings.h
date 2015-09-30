// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SequencerProjectSettings.generated.h"

// Settings for the Sequencer editor
UCLASS(config=EditorPerProjectUserSettings)
class USequencerProjectSettings : public UObject
{
	GENERATED_BODY()

public:
	USequencerProjectSettings();

	/** The default in frame. */
	UPROPERTY(config, EditAnywhere, Category=Timeline)
	int32 InFrame;

	/** The default duration in seconds. */
	UPROPERTY(config, EditAnywhere, Category=Timeline)
	float Duration;

};
