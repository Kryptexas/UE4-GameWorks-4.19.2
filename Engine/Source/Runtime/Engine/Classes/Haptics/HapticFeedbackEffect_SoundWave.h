// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "HapticFeedbackEffect_SoundWave.generated.h"

UCLASS(MinimalAPI, BlueprintType)
class UHapticFeedbackEffect_SoundWave : public UHapticFeedbackEffect_Base
{
	GENERATED_UCLASS_BODY()

	void GetValues(const float EvalTime, FHapticFeedbackValues& Values) override;

	float GetDuration() const override;
};
