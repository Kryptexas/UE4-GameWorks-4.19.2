// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

/////////////////////////////////////////////////////
// FAnimNode_RotationOffsetBlendSpace

void FAnimNode_RotationOffsetBlendSpace::Initialize(const FAnimationInitializeContext& Context)
{
	FAnimNode_BlendSpacePlayer::Initialize(Context);
	BasePose.Initialize(Context);
}

void FAnimNode_RotationOffsetBlendSpace::CacheBones(const FAnimationCacheBonesContext & Context) 
{
	FAnimNode_BlendSpacePlayer::CacheBones(Context);
	BasePose.CacheBones(Context);
}

void FAnimNode_RotationOffsetBlendSpace::Update(const FAnimationUpdateContext& Context)
{
	FAnimNode_BlendSpacePlayer::Update(Context);
	BasePose.Update(Context);
}

void FAnimNode_RotationOffsetBlendSpace::Evaluate(FPoseContext& Context)
{
	FPoseContext BlendSpacePose(Context);
	FAnimNode_BlendSpacePlayer::Evaluate(BlendSpacePose);

	FPoseContext BasePoseContext(Context);
	BasePose.Evaluate(BasePoseContext);

	const float Alpha = 1.0f;
	Context.AnimInstance->BlendRotationOffset(BasePoseContext.Pose, BlendSpacePose.Pose, Alpha, Context.Pose);
}

FAnimNode_RotationOffsetBlendSpace::FAnimNode_RotationOffsetBlendSpace()
{
}
