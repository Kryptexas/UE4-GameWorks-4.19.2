// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/**
 * Blend Space. Contains a grid of data points with weights from sample points in the space
 *
 */

#pragma once 

#include "BlendSpace.h"

#include "AimOffsetBlendSpace.generated.h"

struct FBlendSample;

UCLASS(config=Engine, hidecategories=Object, MinimalAPI, BlueprintType)
class UAimOffsetBlendSpace : public UBlendSpace
{
	GENERATED_UCLASS_BODY()
		
	virtual bool IsValidAdditiveType(EAdditiveAnimationType AdditiveType) const override;
	virtual bool IsValidAdditive() const override;
};
