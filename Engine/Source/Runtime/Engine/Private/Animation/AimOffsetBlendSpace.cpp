// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AimOffsetBlendSpace.cpp: AimOffsetBlendSpace functionality
=============================================================================*/ 

#include "EnginePrivate.h"
#include "Animation/BlendSpaceBase.h"
#include "Animation/AimOffsetBlendSpace.h"
#include "Animation/AnimSequence.h"

UAimOffsetBlendSpace::UAimOffsetBlendSpace(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bRotationBlendInMeshSpace = true;
}

bool UAimOffsetBlendSpace::IsValidAdditiveType(EAdditiveAnimationType AdditiveType) const
{
	return (AdditiveType == AAT_RotationOffsetMeshSpace);
}

bool UAimOffsetBlendSpace::IsValidAdditive() const
{
	return ContainsMatchingSamples(AAT_RotationOffsetMeshSpace);
}
