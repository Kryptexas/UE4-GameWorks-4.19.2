// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "Sound/SoundEffectBase.h"
#include "UObject/UnrealType.h"
#include "Sound/SoundEffectPreset.h"

/*-----------------------------------------------------------------------------
	FSoundEffectBase Implementation
-----------------------------------------------------------------------------*/

FSoundEffectBase::FSoundEffectBase()
	: bIsRunning(false)
	, bIsActive(false)
{}

void FSoundEffectBase::SetPreset(USoundEffectPreset* InPreset)
{
	const FPresetSettings& PresetSettings = InPreset->GetPresetSettings();

	// Reset the preset data scratch buffer to the size of the settings
	RawPresetDataScratchInputBuffer.Init(0, PresetSettings.Size);

	// Copy the data to the raw data array
	UProperty* Property = PresetSettings.Struct->PropertyLink;
	while (Property)
	{
		uint32 PropertyOffset = Property->GetOffset_ForInternal();
		Property->CopyCompleteValue((uint8*)RawPresetDataScratchInputBuffer.GetData() + PropertyOffset, (const uint8*)PresetSettings.Data + PropertyOffset);
		Property = Property->PropertyLinkNext;
	}

	// Queuing the data will copy the data to the queue
	EffectSettingsQueue.Enqueue(RawPresetDataScratchInputBuffer);
}

