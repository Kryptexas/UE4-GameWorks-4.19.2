// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Blend Space. Contains a grid of data points with weights from sample points in the space
 *
 */

#pragma once 

#include "AimOffsetBlendSpace.generated.h"


UCLASS(config=Engine, hidecategories=Object, dependson=UAnimInstance, MinimalAPI, BlueprintType)
class UAimOffsetBlendSpace : public UBlendSpace
{
	GENERATED_UCLASS_BODY()

	virtual bool IsValidAdditive() const OVERRIDE;

	/** Validate sample input. Return true if it's all good to go **/
	virtual bool ValidateSampleInput(FBlendSample & BlendSample, int32 OriginalIndex=INDEX_NONE) const OVERRIDE;
};

