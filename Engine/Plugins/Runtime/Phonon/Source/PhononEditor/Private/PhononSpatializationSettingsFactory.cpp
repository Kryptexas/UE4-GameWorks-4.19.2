// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "PhononSpatializationSettingsFactory.h"
#include "PhononSpatializationSourceSettings.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

UClass* FAssetTypeActions_PhononSpatializationSettings::GetSupportedClass() const
{
	return UPhononSpatializationSourceSettings::StaticClass();
}

UPhononSpatializationSettingsFactory::UPhononSpatializationSettingsFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UPhononSpatializationSourceSettings::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UPhononSpatializationSettingsFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UPhononSpatializationSourceSettings* NewPresetBank = NewObject<UPhononSpatializationSourceSettings>(InParent, InName, Flags);

	return NewPresetBank;
}