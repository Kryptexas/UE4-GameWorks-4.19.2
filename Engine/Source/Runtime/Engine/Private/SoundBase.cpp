// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Sound/SoundBase.h"
#include "Sound/AudioSettings.h"
#include "AudioDevice.h"

USoundBase::USoundBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIgnoreFocus(false)
	, Priority(1.0f)
{
	MaxConcurrentPlayCount_DEPRECATED = 16;
}

void USoundBase::PostInitProperties()
{
	Super::PostInitProperties();

	const FStringAssetReference DefaultSoundClassName = GetDefault<UAudioSettings>()->DefaultSoundClassName;
	if (DefaultSoundClassName.IsValid())
	{
		SoundClassObject = LoadObject<USoundClass>(nullptr, *DefaultSoundClassName.ToString());
	}

	const FStringAssetReference DefaultSoundConcurrencyName = GetDefault<UAudioSettings>()->DefaultSoundConcurrencyName;
	if (DefaultSoundConcurrencyName.IsValid())
	{
		SoundConcurrencySettings = LoadObject<USoundConcurrency>(nullptr, *DefaultSoundConcurrencyName.ToString());
	}
}

bool USoundBase::IsPlayable() const
{
	return false;
}

const FAttenuationSettings* USoundBase::GetAttenuationSettingsToApply() const
{
	if (AttenuationSettings)
	{
		return &AttenuationSettings->Attenuation;
	}
	return NULL;
}

float USoundBase::GetMaxAudibleDistance()
{
	return 0.f;
}

float USoundBase::GetDuration()
{
	return Duration;
}

float USoundBase::GetVolumeMultiplier()
{
	return 1.f;
}

float USoundBase::GetPitchMultiplier()
{
	return 1.f;
}

bool USoundBase::IsLooping()
{ 
	return (GetDuration() >= INDEFINITELY_LOOPING_DURATION); 
}


USoundClass* USoundBase::GetSoundClass() const
{
	return SoundClassObject;
}

const FSoundConcurrencySettings* USoundBase::GetSoundConcurrencySettingsToApply()
{
	if (bOverrideConcurrency)
	{
		return &ConcurrencyOverrides;
	}
	else if (SoundConcurrencySettings)
	{
		return &SoundConcurrencySettings->Concurrency;
	}
	return nullptr;
}

float USoundBase::GetPriority() const
{
	return FMath::Clamp(Priority, MIN_SOUND_PRIORITY, MAX_SOUND_PRIORITY);
}

uint32 USoundBase::GetSoundConcurrencyObjectID() const
{
	if (SoundConcurrencySettings != nullptr && !bOverrideConcurrency)
	{
		return SoundConcurrencySettings->GetUniqueID();
	}
	return 0;
}

void USoundBase::PostLoad()
{
	Super::PostLoad();

	const int32 LinkerUE4Version = GetLinkerUE4Version();

	if (LinkerUE4Version < VER_UE4_SOUND_CONCURRENCY_PACKAGE)
	{
		bOverrideConcurrency = true;
		ConcurrencyOverrides.bLimitToOwner = false;
		ConcurrencyOverrides.MaxCount = MaxConcurrentPlayCount_DEPRECATED;
		ConcurrencyOverrides.ResolutionRule = MaxConcurrentResolutionRule_DEPRECATED;
		ConcurrencyOverrides.VolumeScale = 1.0f;
	}
}

