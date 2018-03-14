// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "OculusAudioSettings.h"

UOculusAudioSettings::UOculusAudioSettings() :
    EarlyReflections(true)
    , LateReverberation(false)
    , Width(8.0f)
    , Height(3.0f)
    , Depth(5.0f)
    , ReflectionCoefRight(0.25f)
    , ReflectionCoefLeft(0.25f)
    , ReflectionCoefUp(0.5f)
    , ReflectionCoefDown(0.1f)
    , ReflectionCoefBack(0.25f)
    , ReflectionCoefFront(0.25f)
{

}

#if WITH_EDITOR
void UOculusAudioSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    // TODO notify

    Super::PostEditChangeProperty(PropertyChangedEvent);
}

bool UOculusAudioSettings::CanEditChange(const UProperty* InProperty) const
{
    // TODO disable settings when reflection engine is disabled

    return Super::CanEditChange(InProperty);
}
#endif