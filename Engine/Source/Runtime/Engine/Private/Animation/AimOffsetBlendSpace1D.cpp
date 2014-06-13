// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AimOffsetBlendSpace1D.cpp: AimOffsetBlendSpace functionality
=============================================================================*/ 

#include "EnginePrivate.h"
#include "Animation/BlendSpaceBase.h"
#include "Animation/AimOffsetBlendSpace1D.h"
#include "Animation/AnimSequence.h"

UAimOffsetBlendSpace1D::UAimOffsetBlendSpace1D(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

bool UAimOffsetBlendSpace1D::IsValidAdditive()  const
{
	return IsValidAdditiveInternal(AAT_RotationOffsetMeshSpace);
}

bool UAimOffsetBlendSpace1D::ValidateSampleInput(FBlendSample& BlendSample, int32 OriginalIndex) const
{
	// we only accept rotation offset additive. Otherwise it's invalid
	if (BlendSample.Animation && !BlendSample.Animation->IsValidAdditive())
	{
		return false;
	}

	return Super::ValidateSampleInput(BlendSample, OriginalIndex);
}
