// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AimOffsetBlendSpace1D.cpp: AimOffsetBlendSpace functionality
=============================================================================*/ 

#include "EnginePrivate.h"
#include "Animation/BlendSpaceBase.h"
#include "Animation/AimOffsetBlendSpace1D.h"
#include "Animation/AnimSequence.h"

UAimOffsetBlendSpace1D::UAimOffsetBlendSpace1D(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bRotationBlendInMeshSpace = true;
}

bool UAimOffsetBlendSpace1D::IsValidAdditiveType(EAdditiveAnimationType AdditiveType) const
{
	return (AdditiveType == AAT_RotationOffsetMeshSpace);
}

bool UAimOffsetBlendSpace1D::IsValidAdditive() const
{
	return ContainsMatchingSamples(AAT_RotationOffsetMeshSpace);
}
