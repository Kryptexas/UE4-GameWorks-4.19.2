// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AssetTypeActions/AssetTypeActions_SoundMix.h"
#include "Sound/SoundMix.h"

UClass* FAssetTypeActions_SoundMix::GetSupportedClass() const
{
	return USoundMix::StaticClass();
}
