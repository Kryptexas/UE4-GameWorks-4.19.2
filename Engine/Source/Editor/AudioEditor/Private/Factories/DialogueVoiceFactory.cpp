// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AudioEditorPrivatePCH.h"
#include "Factories/DialogueVoiceFactory.h"

UDialogueVoiceFactory::UDialogueVoiceFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UDialogueVoice::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDialogueVoiceFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	return NewObject<UObject>(InParent, Class, Name, Flags);
}