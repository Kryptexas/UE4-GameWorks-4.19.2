// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

/////////////////////////////////////////////////////
// FAnimSequenceEvaluatorNode

void FAnimNode_SequenceEvaluator::Initialize(const FAnimationInitializeContext& Context)
{
}

void FAnimNode_SequenceEvaluator::CacheBones(const FAnimationCacheBonesContext & Context) 
{
}

void FAnimNode_SequenceEvaluator::Update(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);
}

void FAnimNode_SequenceEvaluator::Evaluate(FPoseContext& Output)
{
	if ((Sequence != NULL) && (Output.AnimInstance->CurrentSkeleton->IsCompatible(Sequence->GetSkeleton())))
	{
		Output.AnimInstance->SequenceEvaluatePose(Sequence, Output.Pose, FAnimExtractContext(ExplicitTime, false));
	}
	else
	{
		Output.ResetToRefPose();
	}
}