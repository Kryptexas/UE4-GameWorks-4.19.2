// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OculusAmbisonicSettingsFactory.h"
#include "OculusAmbisonicsSettings.h"
#include "AssetTypeCategories.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

FText FAssetTypeActions_OculusAmbisonicsSettings::GetName() const
{
    return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_OculusAmbisonicsSettings", "Oculus Audio Source Settings");
}

FColor FAssetTypeActions_OculusAmbisonicsSettings::GetTypeColor() const
{
    return FColor(100, 100, 100);
}

UClass* FAssetTypeActions_OculusAmbisonicsSettings::GetSupportedClass() const
{
    return UOculusAmbisonicsSettings::StaticClass();
}

uint32 FAssetTypeActions_OculusAmbisonicsSettings::GetCategories()
{
    return EAssetTypeCategories::Sounds;
}

UOculusAmbisonicsSettingsFactory::UOculusAmbisonicsSettingsFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UOculusAmbisonicsSettings::StaticClass();

	bCreateNew = true;
	bEditorImport = true;
	bEditAfterNew = true;
}

UObject* UOculusAmbisonicsSettingsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context,
    FFeedbackContext* Warn)
{
    return NewObject<UOculusAmbisonicsSettings>(InParent, Name, Flags);
}

uint32 UOculusAmbisonicsSettingsFactory::GetMenuCategories() const
{
    return EAssetTypeCategories::Sounds;
}