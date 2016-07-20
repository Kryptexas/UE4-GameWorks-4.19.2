// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "HapticFeedbackEffect_Base.generated.h"

USTRUCT()
struct FActiveHapticFeedbackEffect
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	class UHapticFeedbackEffect_Base* HapticEffect;

	FActiveHapticFeedbackEffect()
		: PlayTime(0.f)
		, Scale(1.f)
	{
	}

	FActiveHapticFeedbackEffect(UHapticFeedbackEffect_Base* InEffect, float InScale)
		: HapticEffect(InEffect)
		, PlayTime(0.f)
	{
		Scale = FMath::Clamp(InScale, 0.f, 1.f);
	}

	bool Update(const float DeltaTime, struct FHapticFeedbackValues& Values);

private:
	float PlayTime;
	float Scale;

};

UCLASS(MinimalAPI, BlueprintType)
class UHapticFeedbackEffect_Base : public UObject
{
	GENERATED_UCLASS_BODY()

	virtual void GetValues(const float EvalTime, FHapticFeedbackValues& Values);

	virtual float GetDuration() const;
};
