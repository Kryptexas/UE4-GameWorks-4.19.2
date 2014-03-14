// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SoundNodeParamCrossFade.generated.h"

/** 
 * Crossfades between different sounds based on a parameter
 */
UCLASS(editinlinenew, MinimalAPI, meta=( DisplayName="Crossfade by Param" ))
class USoundNodeParamCrossFade : public USoundNodeDistanceCrossFade
{
	GENERATED_UCLASS_BODY()

public:

	/* Parameter controlling cross fades. */
	UPROPERTY(EditAnywhere, Category=CrossFade )
	FName ParamName;

	virtual FString GetUniqueString() const OVERRIDE;
	virtual float GetCurrentDistance(FAudioDevice* AudioDevice, FActiveSound& ActiveSound, const FSoundParseParameters& ParseParams) const OVERRIDE;
};
