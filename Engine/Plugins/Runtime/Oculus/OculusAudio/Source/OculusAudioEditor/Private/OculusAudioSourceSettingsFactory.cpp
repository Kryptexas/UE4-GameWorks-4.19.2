// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OculusAudioSourceSettingsFactory.h"
#include "OculusAudioSourceSettings.h"
#include "AssetTypeCategories.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

FText FAssetTypeActions_OculusAudioSourceSettings::GetName() const
{
    return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_OculusAudioSpatializationSettings", "Oculus Audio Source Settings");
}

FColor FAssetTypeActions_OculusAudioSourceSettings::GetTypeColor() const
{
    return FColor(100, 100, 100);
}

UClass* FAssetTypeActions_OculusAudioSourceSettings::GetSupportedClass() const
{
    return UOculusAudioSourceSettings::StaticClass();
}

uint32 FAssetTypeActions_OculusAudioSourceSettings::GetCategories()
{
    return EAssetTypeCategories::Sounds;
}

UOculusAudioSourceSettingsFactory::UOculusAudioSourceSettingsFactory(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SupportedClass = UOculusAudioSourceSettings::StaticClass();

    bCreateNew = true;
    bEditorImport = true;
    bEditAfterNew = true;
}

UObject* UOculusAudioSourceSettingsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context,
    FFeedbackContext* Warn)
{
    return NewObject<UOculusAudioSourceSettings>(InParent, Name, Flags);
}

uint32 UOculusAudioSourceSettingsFactory::GetMenuCategories() const
{
    return EAssetTypeCategories::Sounds;
}