// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PhononReverbSettingsFactory.h"
#include "PhononReverbSourceSettings.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

UClass* FAssetTypeActions_PhononReverbSettings::GetSupportedClass() const
{
	return UPhononReverbSourceSettings::StaticClass();
}

UPhononReverbSettingsFactory::UPhononReverbSettingsFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UPhononReverbSourceSettings::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UPhononReverbSettingsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UPhononReverbSourceSettings* NewPresetBank = NewObject<UPhononReverbSourceSettings>(InParent, InName, Flags);

	return NewPresetBank;
}