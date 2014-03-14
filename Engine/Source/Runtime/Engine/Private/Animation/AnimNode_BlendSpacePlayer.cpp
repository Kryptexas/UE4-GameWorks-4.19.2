// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

/////////////////////////////////////////////////////
// FAnimNode_BlendSpacePlayer

FAnimNode_BlendSpacePlayer::FAnimNode_BlendSpacePlayer()
	: X(0.0f)
	, Y(0.0f)
	, Z(0.0f)
	, PlayRate(1.0f)
	, bLoop(true)
	, BlendSpace(NULL)
	, GroupIndex(INDEX_NONE)
	, InternalTimeAccumulator(0.0f)
{
}

void FAnimNode_BlendSpacePlayer::Initialize(const FAnimationInitializeContext& Context)
{
	BlendSampleDataCache.Empty();
	
	EvaluateGraphExposedInputs.Execute(Context);
	if(PlayRate >= 0.0f)
	{
		InternalTimeAccumulator = 0.0f;
	}
	else
	{
		// Blend spaces run between 0 and 1
		InternalTimeAccumulator = 1.0f;
	}

	if (BlendSpace != NULL)
	{
		BlendSpace->InitializeFilter(&BlendFilter);
	}
}

void FAnimNode_BlendSpacePlayer::CacheBones(const FAnimationCacheBonesContext & Context) 
{
}

void FAnimNode_BlendSpacePlayer::Update(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);

	if ((BlendSpace != NULL) && (Context.AnimInstance->CurrentSkeleton->IsCompatible(BlendSpace->GetSkeleton())))
	{
		// Create a tick record and fill it out
		FAnimGroupInstance* SyncGroup;
		FAnimTickRecord& TickRecord = Context.AnimInstance->CreateUninitializedTickRecord(GroupIndex, /*out*/ SyncGroup);

		const FVector BlendInput(X, Y, Z);
	
		Context.AnimInstance->MakeBlendSpaceTickRecord(TickRecord, BlendSpace, BlendInput, BlendSampleDataCache, BlendFilter, bLoop, PlayRate, Context.GetFinalBlendWeight(), /*inout*/ InternalTimeAccumulator);

		// Update the sync group if it exists
		if (SyncGroup != NULL)
		{
			SyncGroup->TestTickRecordForLeadership(GroupRole);
		}
	}
}

void FAnimNode_BlendSpacePlayer::Evaluate(FPoseContext& Output)
{
	if ((BlendSpace != NULL) && (Output.AnimInstance->CurrentSkeleton->IsCompatible(BlendSpace->GetSkeleton())))
	{
		Output.AnimInstance->BlendSpaceEvaluatePose(BlendSpace, BlendSampleDataCache, Output.Pose, bLoop);
	}
	else
	{
		Output.ResetToRefPose();
	}
}
