// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AudioEditorPrivatePCH.h"
#include "Factories/DialogueWaveFactory.h"

UDialogueWaveFactory::UDialogueWaveFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UDialogueWave::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDialogueWaveFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	check(Class == SupportedClass);
	UDialogueWave* DialogueWave = NewObject<UDialogueWave>(InParent, Name, Flags);

	if (InitialSoundWave)
	{
		DialogueWave->SpokenText = InitialSoundWave->SpokenText;
		DialogueWave->bMature = InitialSoundWave->bMature;
	}

	DialogueWave->UpdateContext(DialogueWave->ContextMappings[0], InitialSoundWave, InitialSpeakerVoice, InitialTargetVoices);

	return DialogueWave;
}
