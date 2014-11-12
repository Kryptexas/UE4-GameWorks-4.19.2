// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AssetToolsPrivatePCH.h"
#include "SoundDefinitions.h"
#include "Sound/ReverbEffect.h"

UClass* FAssetTypeActions_ReverbEffect::GetSupportedClass() const
{
	return UReverbEffect::StaticClass();
}