// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#include "Sound/SoundSubmix.h"
#include "AudioDeviceManager.h"
#include "EngineGlobals.h"
#include "Engine/Engine.h"
#include "UObject/UObjectIterator.h"

#if WITH_EDITOR
TSharedPtr<ISoundSubmixAudioEditor> USoundSubmix::SoundSubmixAudioEditor = nullptr;
#endif

USoundSubmix::USoundSubmix(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void USoundSubmix::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}

FString USoundSubmix::GetDesc()
{
	return FString(TEXT("Sound submix"));
}

void USoundSubmix::BeginDestroy()
{
	Super::BeginDestroy();

	// Use the main/default audio device for storing and retrieving sound class properties
	FAudioDeviceManager* AudioDeviceManager = (GEngine ? GEngine->GetAudioDeviceManager() : nullptr);

	// Force the properties to be initialized for this SoundClass on all active audio devices
	if (AudioDeviceManager)
	{
		AudioDeviceManager->UnregisterSoundSubmix(this);
	}
}

void USoundSubmix::PostLoad()
{
	Super::PostLoad();

	// Use the main/default audio device for storing and retrieving sound class properties
	FAudioDeviceManager* AudioDeviceManager = (GEngine ? GEngine->GetAudioDeviceManager() : nullptr);

	// Force the properties to be initialized for this SoundClass on all active audio devices
	if (AudioDeviceManager)
	{
		AudioDeviceManager->RegisterSoundSubmix(this);
	}
}

#if WITH_EDITOR

void USoundSubmix::PreEditChange(UProperty* PropertyAboutToChange)
{
}

void USoundSubmix::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool USoundSubmix::RecurseCheckChild(USoundSubmix* ChildSoundSubmix)
{
	for (int32 Index = 0; Index < ChildSubmixes.Num(); Index++)
	{
		if (ChildSubmixes[Index])
		{
			if (ChildSubmixes[Index] == ChildSoundSubmix)
			{
				return true;
			}

			if (ChildSubmixes[Index]->RecurseCheckChild(ChildSoundSubmix))
			{
				return true;
			}
		}
	}

	return false;
}

void USoundSubmix::SetParentSubmix(USoundSubmix* InParentSubmix)
{
	if (ParentSubmix != InParentSubmix)
	{
		if (ParentSubmix != nullptr)
		{
			ParentSubmix->Modify();
			ParentSubmix->ChildSubmixes.Remove(this);
		}

		Modify();
		ParentSubmix = InParentSubmix;
	}
}

void USoundSubmix::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	USoundSubmix* This = CastChecked<USoundSubmix>(InThis);

	Collector.AddReferencedObject(This->SoundSubmixGraph, This);

	Super::AddReferencedObjects(InThis, Collector);
}

void USoundSubmix::RefreshAllGraphs(bool bIgnoreThis)
{
	if (SoundSubmixAudioEditor.IsValid())
	{
		// Update the graph representation of every SoundClass
		for (TObjectIterator<USoundSubmix> It; It; ++It)
		{
			USoundSubmix* SoundSubmix = *It;
			if (!bIgnoreThis || SoundSubmix != this)
			{
				if (SoundSubmix->SoundSubmixGraph)
				{
					SoundSubmixAudioEditor->RefreshGraphLinks(SoundSubmix->SoundSubmixGraph);
				}
			}
		}
	}
}

void USoundSubmix::SetSoundSubmixAudioEditor(TSharedPtr<ISoundSubmixAudioEditor> InSoundSubmixAudioEditor)
{
	check(!SoundSubmixAudioEditor.IsValid());
	SoundSubmixAudioEditor = InSoundSubmixAudioEditor;
}

TSharedPtr<ISoundSubmixAudioEditor> USoundSubmix::GetSoundSubmixAudioEditor()
{
	return SoundSubmixAudioEditor;
}

#endif
