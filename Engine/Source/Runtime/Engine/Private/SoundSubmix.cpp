// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "SoundDefinitions.h"
#include "Sound/SoundSubmix.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#include "SlateBasics.h"
#include "SNotificationList.h"
#include "NotificationManager.h"
#include "ModuleManager.h"
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
#endif
