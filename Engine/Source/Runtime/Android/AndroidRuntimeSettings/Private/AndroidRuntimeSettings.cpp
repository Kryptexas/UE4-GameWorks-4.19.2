// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AndroidRuntimeSettingsPrivatePCH.h"

#include "AndroidRuntimeSettings.h"

UAndroidRuntimeSettings::UAndroidRuntimeSettings(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, Orientation(EAndroidScreenOrientation::Landscape)
{

}
