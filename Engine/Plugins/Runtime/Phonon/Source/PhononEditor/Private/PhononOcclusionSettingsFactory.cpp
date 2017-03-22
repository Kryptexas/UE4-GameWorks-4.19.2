// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PhononOcclusionSettingsFactory.h"
#include "PhononOcclusionSourceSettings.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

UClass* FAssetTypeActions_PhononOcclusionSettings::GetSupportedClass() const
{
	return UPhononOcclusionSourceSettings::StaticClass();
}

UPhononOcclusionSettingsFactory::UPhononOcclusionSettingsFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UPhononOcclusionSourceSettings::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UPhononOcclusionSettingsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UPhononOcclusionSourceSettings* NewPresetBank = NewObject<UPhononOcclusionSourceSettings>(InParent, InName, Flags);

	return NewPresetBank;
}