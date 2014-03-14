// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

/////////////////////////////////////////////////////
// FAnimRefPoseNode

void FAnimNode_RefPose::Evaluate(FPoseContext& Output)
{
	// I don't have anything to evaluate. Should this be even here?
	// EvaluateGraphExposedInputs.Execute(Context);

	switch (RefPoseType) 
	{
	case EIT_LocalSpace:
		Output.ResetToRefPose();
		break;

	case EIT_Additive:
	default:
		Output.ResetToIdentity();
		break;
	}
}

void FAnimNode_MeshSpaceRefPose::EvaluateComponentSpace(FComponentSpacePoseContext& Output)
{
	Output.ResetToRefPose();
}