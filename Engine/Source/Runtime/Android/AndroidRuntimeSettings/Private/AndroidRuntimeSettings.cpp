// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AndroidRuntimeSettingsPrivatePCH.h"

#include "AndroidRuntimeSettings.h"

UAndroidRuntimeSettings::UAndroidRuntimeSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Orientation(EAndroidScreenOrientation::Landscape)
	, bEnableGooglePlaySupport(false)
{

}

#if WITH_EDITOR
void UAndroidRuntimeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Ensure that at least one architecture is supported
	if (!bBuildForArmV7 && !bBuildForX86 && !bBuildForX8664)// && !bBuildForArm64)
	{
		bBuildForArmV7 = true;
	}

	// Ensure that at least one GPU architecture is supported
	if (!bBuildForES2 && !bBuildForES31)
	{
		bBuildForES2 = true;
	}
}

void UAndroidRuntimeSettings::PostInitProperties()
{
	Super::PostInitProperties();

	// If the config has an AdMobAdUnitID then we migrate it on load and clear the value
	if (!AdMobAdUnitID.IsEmpty())
	{
		AdMobAdUnitIDs.Add(AdMobAdUnitID);
		AdMobAdUnitID.Empty();
		UpdateDefaultConfigFile();
	}
}
#endif
