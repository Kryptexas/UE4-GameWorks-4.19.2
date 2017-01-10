// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "Sound/SoundEffectPreset.h"

/*-----------------------------------------------------------------------------
	USoundEffectPresetBase Implementation
-----------------------------------------------------------------------------*/

USoundEffectPreset::USoundEffectPreset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USoundEffectPreset::Init()
{
	PresetSettings.Data = GetSettings();
	PresetSettings.Size = GetSettingsSize();
	PresetSettings.Struct = GetSettingsStruct();
}


