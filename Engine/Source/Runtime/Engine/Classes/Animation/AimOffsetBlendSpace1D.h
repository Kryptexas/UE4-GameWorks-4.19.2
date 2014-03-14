// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Blend Space 1D. Contains 1 axis blend 'space'
 *
 */

#pragma once 

#include "AimOffsetBlendSpace1D.generated.h"


UCLASS(config=Engine, hidecategories=Object, dependson=UAnimInstance, MinimalAPI, BlueprintType)
class UAimOffsetBlendSpace1D : public UBlendSpace1D
{
	GENERATED_UCLASS_BODY()

	virtual bool IsValidAdditive() const OVERRIDE;

	/** Validate sample input. Return true if it's all good to go **/
	virtual bool ValidateSampleInput(FBlendSample & BlendSample, int32 OriginalIndex=INDEX_NONE) const OVERRIDE;
};

