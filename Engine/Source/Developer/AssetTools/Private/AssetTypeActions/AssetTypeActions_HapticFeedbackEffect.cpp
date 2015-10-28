// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "GameFramework/HapticFeedbackEffect.h"

UClass* FAssetTypeActions_HapticFeedbackEffect::GetSupportedClass() const
{
	return UHapticFeedbackEffect::StaticClass();
}