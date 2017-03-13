// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/**
 *
 * Used by Preview in PhAT, allows us to switch between immediate mode and vanilla physx
 */

#pragma once
#include "Animation/AnimInstance.h"
#include "PhATAnimInstance.generated.h"

class UAnimSequence;

UCLASS(transient, NotBlueprintable)
class UPhATAnimInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

public:
	bool IsPlayingAnimation() const;
	void SetAnimationSequence(UAnimSequence* AnimSequence);
	void SetPlayRate(float PlayRate);
protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};



