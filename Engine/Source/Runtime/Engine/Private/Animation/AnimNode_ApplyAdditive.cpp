// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

/////////////////////////////////////////////////////
// FAnimNode_ApplyAdditive

void FAnimNode_ApplyAdditive::Initialize(const FAnimationInitializeContext& Context)
{
	Base.Initialize(Context);
	Additive.Initialize(Context);
}

void FAnimNode_ApplyAdditive::CacheBones(const FAnimationCacheBonesContext & Context) 
{
	Base.CacheBones(Context);
	Additive.CacheBones(Context);
}

void FAnimNode_ApplyAdditive::Update(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);

	Base.Update(Context);
	const float ActualAlpha = AlphaScaleBias.ApplyTo(Alpha);
	if (ActualAlpha > ZERO_ANIMWEIGHT_THRESH)
	{
		Additive.Update(Context.FractionalWeight(ActualAlpha));
	}
}

void FAnimNode_ApplyAdditive::Evaluate(FPoseContext& Output)
{
	//@TODO: Could evaluate Base into Output and save a copy
	const float ActualAlpha = AlphaScaleBias.ApplyTo(Alpha);
	if (ActualAlpha > ZERO_ANIMWEIGHT_THRESH)
	{
		FPoseContext BaseEvalContext(Output);
		FPoseContext AdditiveEvalContext(Output);

		Base.Evaluate(BaseEvalContext);
		Additive.Evaluate(AdditiveEvalContext);

		Output.AnimInstance->ApplyAdditiveSequence(BaseEvalContext.Pose, AdditiveEvalContext.Pose, ActualAlpha, Output.Pose);
	}
	else
	{
		Base.Evaluate(Output);
	}
}

FAnimNode_ApplyAdditive::FAnimNode_ApplyAdditive()
	: Alpha(1.0f)
{
}
