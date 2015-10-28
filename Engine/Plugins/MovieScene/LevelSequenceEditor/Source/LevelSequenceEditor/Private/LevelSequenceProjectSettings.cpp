// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "LevelSequenceEditorPCH.h"
#include "LevelSequenceProjectSettings.h"

ULevelSequenceProjectSettings::ULevelSequenceProjectSettings()
	: DefaultStartTime(1.f/30)	// Default to frame 1 of a 30fps movie
	, DefaultDuration(5.f)
{
}
