// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "HAL/ThreadSafeBool.h"
#include "Containers/Queue.h"

// The following macro code creates boiler-plate code for a sound effect preset and hides unnecessary details from user-created effects.

// Macro chain to expand "MyEffectName" to "FMyEffectNameSettings"
#define EFFECT_SETTINGS_NAME2(CLASS_NAME, SUFFIX) F ## CLASS_NAME ## SUFFIX
#define EFFECT_SETTINGS_NAME1(CLASS_NAME, SUFFIX) EFFECT_SETTINGS_NAME2(CLASS_NAME, SUFFIX)
#define EFFECT_SETTINGS_NAME(CLASS_NAME)		  EFFECT_SETTINGS_NAME1(CLASS_NAME, Settings)

#define EFFECT_PRESET_NAME2(CLASS_NAME, SUFFIX)  U ## CLASS_NAME ## SUFFIX
#define EFFECT_PRESET_NAME1(CLASS_NAME, SUFFIX)  EFFECT_PRESET_NAME2(CLASS_NAME, SUFFIX)
#define EFFECT_PRESET_NAME(CLASS_NAME)			 EFFECT_PRESET_NAME1(CLASS_NAME, Preset)

#define EFFECT_PRESET_METHODS(EFFECT_NAME) \
		virtual FText GetAssetActionName() const override { return FText::FromString(#EFFECT_NAME); } \
		virtual UClass* GetSupportedClass() const override { return EFFECT_PRESET_NAME(EFFECT_NAME)::StaticClass(); } \
		virtual USoundEffectPreset* CreateNewPreset(UObject* InParent, FName Name, EObjectFlags Flags) const override { return NewObject<EFFECT_PRESET_NAME(EFFECT_NAME)>(InParent, GetSupportedClass(), Name, Flags); } \
		virtual FSoundEffectBase* CreateNewEffect() const override { return new F##EFFECT_NAME; }

#define EFFECT_PRESET_METHODS_NO_ASSET_ACTIONS(EFFECT_NAME) \
		virtual bool HasAssetActions() const override { return false; } \
		EFFECT_PRESET_METHODS(EFFECT_NAME)

class USoundEffectPreset;

class ENGINE_API FSoundEffectBase
{
public:
	FSoundEffectBase()
		: bIsRunning(false)
		, bIsActive(false)
	{}

	virtual ~FSoundEffectBase() {}

	/** Returns if the submix is active or bypassing audio. */
	bool IsActive() const { return bIsActive; }

	/** Disables the submix effect. */
	void Disable() { bIsActive = false; }

	/** Enables the submix effect. */
	void Enable() { bIsActive = true; }

protected:

	TArray<uint8> RawPresetDataScratchInputBuffer;
	TArray<uint8> RawPresetDataScratchOutputBuffer;
	TQueue<TArray<uint8>> EffectSettingsQueue;
	TArray<uint8> CurrentAudioThreadSettingsData;

	FThreadSafeBool bIsRunning;
	FThreadSafeBool bIsActive;
};

