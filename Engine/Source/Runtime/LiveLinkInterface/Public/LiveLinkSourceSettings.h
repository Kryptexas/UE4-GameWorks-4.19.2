// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#include "LiveLinkSourceSettings.generated.h"

USTRUCT()
struct FLiveLinkInterpolationSettings
{
	GENERATED_USTRUCT_BODY()

	FLiveLinkInterpolationSettings() : bUseInterpolation(false), InterpolationOffset(0.5f) {}

	// Should this connection use interpolation
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bUseInterpolation;

	// When interpolating: how far back from current time should we read the buffer (in seconds)
	UPROPERTY(EditAnywhere, Category = Settings)
	float InterpolationOffset;
};

// Base class for live link source settings (can be replaced by sources themselves) 
UCLASS()
class LIVELINKINTERFACE_API ULiveLinkSourceSettings : public UObject
{
public:
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Interpolation Settings")
	FLiveLinkInterpolationSettings InterpolationSettings;

};
