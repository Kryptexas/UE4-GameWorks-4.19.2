// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "EpicSynth1PresetBank.h"
#include "SynthComponents/EpicSynth1Component.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

UClass* FAssetTypeActions_EpicSynth1PresetBank::GetSupportedClass() const
{
	return UEpicSynth1PresetBank::StaticClass();
}

UEpicSynth1PresetBankFactory::UEpicSynth1PresetBankFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UEpicSynth1PresetBank::StaticClass();

	bCreateNew = true;
	bEditorImport = false;
	bEditAfterNew = true;
}

UObject* UEpicSynth1PresetBankFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	UEpicSynth1PresetBank* NewPresetBank = NewObject<UEpicSynth1PresetBank>(InParent, InName, Flags);

	return NewPresetBank;
}