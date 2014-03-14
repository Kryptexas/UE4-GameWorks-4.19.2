// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AimOffsetBlendSpace.cpp: AimOffsetBlendSpace functionality
=============================================================================*/ 

#include "EnginePrivate.h"

UAimOffsetBlendSpace::UAimOffsetBlendSpace(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool UAimOffsetBlendSpace::IsValidAdditive()  const
{
	return IsValidAdditiveInternal(AAT_RotationOffsetMeshSpace);
}

bool UAimOffsetBlendSpace::ValidateSampleInput(FBlendSample& BlendSample, int32 OriginalIndex) const
{
	// we only accept rotation offset additive. Otherwise it's invalid
	if (BlendSample.Animation && !BlendSample.Animation->IsValidAdditive())
	{
		return false;
	}

	return Super::ValidateSampleInput(BlendSample, OriginalIndex);
}
