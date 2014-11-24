// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "IOSRuntimeSettingsPrivatePCH.h"

#include "IOSRuntimeSettings.h"

UIOSRuntimeSettings::UIOSRuntimeSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bEnableGameCenterSupport = true;
	bSupportsPortraitOrientation = true;
	BundleDisplayName = TEXT("UE4 Game");
	BundleName = TEXT("MyUE4Game");
	BundleIdentifier = TEXT("com.YourCompany.GameNameNoSpaces");
	VersionInfo = TEXT("1.0.0");
    FrameRateLock = EPowerUsageFrameRateLock::PUFRL_30;
	bSupportsIPad = true;
	bSupportsIPhone = true;
	MinimumiOSVersion = EIOSVersion::IOS_61;
	bDevForArmV7 = true;
	bDevForArm64 = false;
	bDevForArmV7S = false;
	bShipForArmV7 = true;
	bShipForArm64 = true;
	bShipForArmV7S = false;
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

	// Ensure that at least one API is supported
	if (!bSupportsMetal && !bSupportsOpenGLES2 && !bSupportsMetalMRT)
	{
		bSupportsOpenGLES2 = true;
	}

	// Ensure that at least armv7 is selected for shipping and dev
	if (!bDevForArmV7 && !bDevForArm64 && !bDevForArmV7S)
	{
		bDevForArmV7 = true;
	}
	if (!bShipForArmV7 && !bShipForArm64 && !bShipForArmV7S)
	{
		bShipForArmV7 = true;
	}
}
#endif
