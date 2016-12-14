// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "SoundEffectBase.h"
#include "SoundEffectPreset.generated.h"

class FAssetData;
class FMenuBuilder;
class IToolkitHost;

/** Raw data container for arbitrary preset UStructs */
struct FPresetSettings
{
	void* Data;
	uint32 Size;
	UScriptStruct* Struct;

	FPresetSettings()
		: Data(nullptr)
		, Size(0)
		, Struct(nullptr)
	{}
};

UCLASS(config = Engine, abstract, editinlinenew, BlueprintType)
class ENGINE_API USoundEffectPreset : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	void Init();

	//~ Begin UObject
	virtual void BeginDestroy() override;
	//~ End UObject

	virtual void* GetSettings() PURE_VIRTUAL(USoundEffectPreset::GetSettings, return nullptr;);
	virtual uint32 GetSettingsSize() const PURE_VIRTUAL(USoundEffectPreset::GetSettingsSize, return 0;);
	virtual UScriptStruct* GetSettingsStruct() const PURE_VIRTUAL(USoundEffectPreset::GetSettingsStruct, return nullptr;);
	virtual FText GetAssetActionName() const PURE_VIRTUAL(USoundEffectPreset::GetAssetActionName, return FText(););
	virtual UClass* GetSupportedClass() const PURE_VIRTUAL(USoundEffectPreset::GetSupportedClass, return nullptr;);
	virtual USoundEffectPreset* CreateNewPreset(UObject* InParent, FName Name, EObjectFlags Flags) const PURE_VIRTUAL(USoundEffectPreset::CreateNewPreset, return nullptr;);
	virtual FSoundEffectBase* CreateNewEffect() const PURE_VIRTUAL(USoundEffectPreset::CreateNewEffect, return nullptr;);
	virtual bool HasAssetActions() const { return true; }


	const FPresetSettings& GetPresetSettings() const
	{
		return PresetSettings;
	}

private:
	FPresetSettings PresetSettings;
};


