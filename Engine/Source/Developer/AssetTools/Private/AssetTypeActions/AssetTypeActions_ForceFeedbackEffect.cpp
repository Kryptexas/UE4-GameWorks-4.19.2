// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "GameFramework/ForceFeedbackEffect.h"

UClass* FAssetTypeActions_ForceFeedbackEffect::GetSupportedClass() const
{
	return UForceFeedbackEffect::StaticClass();
}