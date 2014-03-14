// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

/////////////////////////////////////////////////////
// FAnimationNode_TwoWayBlend

void FAnimationNode_TwoWayBlend::Initialize(const FAnimationInitializeContext& Context)
{
	A.Initialize(Context);
	B.Initialize(Context);
}

void FAnimationNode_TwoWayBlend::CacheBones(const FAnimationCacheBonesContext & Context) 
{
	A.CacheBones(Context);
	B.CacheBones(Context);
}

void FAnimationNode_TwoWayBlend::Update(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);

	const float ActualAlpha = AlphaScaleBias.ApplyTo(Alpha);
	if (ActualAlpha > ZERO_ANIMWEIGHT_THRESH)
	{
		if (ActualAlpha < 1.0f - ZERO_ANIMWEIGHT_THRESH)
		{
			// Blend A and B together
			A.Update(Context.FractionalWeight(1.0f - ActualAlpha));
			B.Update(Context.FractionalWeight(ActualAlpha));
		}
		else
		{
			// Take all of B
			B.Update(Context);
		}
	}
	else
	{
		// Take all of A
		A.Update(Context);
	}
}

void FAnimationNode_TwoWayBlend::Evaluate(FPoseContext& Output)
{
	const float ActualAlpha = AlphaScaleBias.ApplyTo(Alpha);
	if (ActualAlpha > ZERO_ANIMWEIGHT_THRESH)
	{
		if (ActualAlpha < 1.0f - ZERO_ANIMWEIGHT_THRESH)
		{
			FPoseContext Pose1(Output);
			FPoseContext Pose2(Output);

			A.Evaluate(Pose1);
			B.Evaluate(Pose2);

			Output.AnimInstance->BlendSequences(Pose1.Pose, Pose2.Pose, ActualAlpha, Output.Pose);
		}
		else
		{
			B.Evaluate(Output);
		}
	}
	else
	{
		A.Evaluate(Output);
	}
}
