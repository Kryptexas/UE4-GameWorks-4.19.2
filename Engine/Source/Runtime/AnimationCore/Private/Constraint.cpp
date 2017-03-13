// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimationCore.h: Render core module implementation.
=============================================================================*/

#include "Constraint.h"
#include "AnimationCoreUtil.h"

void FConstraintOffset::ApplyInverseOffset(const FTransform& InTarget, FTransform& OutSource) const
{
	// in this matter, parent is accumulated first, and then individual component gets applied
	// I think that will be more consistent than going the other way
	// this parent is confusing, rename?
	OutSource = Parent.GetRelativeTransformReverse(InTarget);

	if (Translation != FVector::ZeroVector)
	{
		OutSource.AddToTranslation(Translation);
	}

	if (Rotation != FQuat::Identity)
	{
		OutSource.SetRotation(OutSource.GetRotation() * Rotation);
	}

	// I know I'm doing just != , not nearly
	if (Scale != FVector::OneVector)
	{
		OutSource.SetScale3D(OutSource.GetScale3D() * Scale);
	}
}

void FConstraintOffset::SaveInverseOffset(const FTransform& Source, const FTransform& Target, const FConstraintDescription& Operator)
{
	// override previous value, this is rule
	if (Operator.bParent)
	{
		Parent = Target.GetRelativeTransform(Source);
	}
	else
	{
		if (Operator.bTranslation)
		{
			Translation = Source.GetTranslation() - Target.GetTranslation();
		}

		if (Operator.bRotation)
		{
			Rotation = Source.GetRotation() * Target.GetRotation().Inverse();
		}

		if (Operator.bScale)
		{
			FVector RecipTarget = FTransform::GetSafeScaleReciprocal(Target.GetScale3D());
			Scale = Source.GetScale3D() * RecipTarget;
		}
	}
}