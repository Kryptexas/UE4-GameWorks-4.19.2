// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IOSRuntimeSettingsPrivatePCH.h"

#include "IOSRuntimeSettings.h"

UIOSRuntimeSettings::UIOSRuntimeSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bEnableGameCenterSupport = true;
	bSupportsPortraitOrientation = true;
}

#if WITH_EDITOR
void UIOSRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Ensure that at least one orientation is supported
	if (!bSupportsPortraitOrientation && !bSupportsUpsideDownOrientation && !bSupportsLandscapeLeftOrientation && !bSupportsLandscapeRightOrientation)
	{
		bSupportsPortraitOrientation = true;
	}
}
#endif
