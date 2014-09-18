// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AndroidRuntimeSettingsPrivatePCH.h"

#include "AndroidRuntimeSettings.h"

UAndroidRuntimeSettings::UAndroidRuntimeSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Orientation(EAndroidScreenOrientation::Landscape)
	, bEnableGooglePlaySupport(false)
{

}

#if WITH_EDITOR
void UAndroidRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Ensure that at least one architecture is supported
	if (!bBuildForArmV7 && !bBuildForX86)// && !bBuildForArm64 && !bBuildForX8664)
	{
		bBuildForArmV7 = true;
	}
}
#endif
