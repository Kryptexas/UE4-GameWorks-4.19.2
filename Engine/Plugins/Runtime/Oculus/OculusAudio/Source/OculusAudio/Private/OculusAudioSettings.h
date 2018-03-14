// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OculusAudioSettings.generated.h"


UCLASS(config = Engine, defaultconfig)
class OCULUSAUDIO_API UOculusAudioSettings : public UObject
{
    GENERATED_BODY()

public:

    UOculusAudioSettings();

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual bool CanEditChange(const UProperty* InProperty) const override;
#endif

    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Room Modeling")
    bool EarlyReflections;

    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Room Modeling")
    bool LateReverberation;
    
    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Room Modeling")
    float Width;
    
    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Room Modeling")
    float Height;

    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Room Modeling")
    float Depth;

    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Room Modeling")
    float ReflectionCoefRight;

    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Room Modeling")
    float ReflectionCoefLeft;
    
    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Room Modeling")
    float ReflectionCoefUp;
    
    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Room Modeling")
    float ReflectionCoefDown;

    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Room Modeling")
    float ReflectionCoefBack;

    UPROPERTY(GlobalConfig, EditAnywhere, Category = "Room Modeling")
    float ReflectionCoefFront;
};