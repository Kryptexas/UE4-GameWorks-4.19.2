// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Animation/AnimNode_SequencePlayer.h"

/////////////////////////////////////////////////////
// FAnimSequencePlayerNode

void FAnimNode_SequencePlayer::Initialize(const FAnimationInitializeContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);
	InternalTimeAccumulator = 0.0f;

	if (Sequence != NULL)
	{
		if ((PlayRate * Sequence->RateScale) < 0.0f)
		{
			InternalTimeAccumulator = Sequence->SequenceLength;
		}
	}
}

void FAnimNode_SequencePlayer::CacheBones(const FAnimationCacheBonesContext& Context) 
{
}

void FAnimNode_SequencePlayer::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);

	if ((Sequence != NULL) && (Context.AnimInstance->CurrentSkeleton->IsCompatible(Sequence->GetSkeleton())))
	{
		const float FinalBlendWeight = Context.GetFinalBlendWeight();

		// Create a tick record and fill it out
		FAnimGroupInstance* SyncGroup;
		FAnimTickRecord& TickRecord = Context.AnimInstance->CreateUninitializedTickRecord(GroupIndex, /*out*/ SyncGroup);

		if (InternalTimeAccumulator > Sequence->SequenceLength)
		{
			InternalTimeAccumulator = 0.f;
		}
		Context.AnimInstance->MakeSequenceTickRecord(TickRecord, Sequence, bLoopAnimation, PlayRate, FinalBlendWeight, /*inout*/ InternalTimeAccumulator);

		// Update the sync group if it exists
		if (SyncGroup != NULL)
		{
			SyncGroup->TestTickRecordForLeadership(GroupRole);
		}
	}
}

void FAnimNode_SequencePlayer::Evaluate(FPoseContext& Output)
{
	if ((Sequence != NULL) && (Output.AnimInstance->CurrentSkeleton->IsCompatible(Sequence->GetSkeleton())))
	{
		Sequence->GetAnimationPose(Output.Pose, Output.Curve, FAnimExtractContext(InternalTimeAccumulator, Output.AnimInstance->ShouldExtractRootMotion()));
	}
	else
	{
		Output.ResetToRefPose();
	}
}

void FAnimNode_SequencePlayer::OverrideAsset(UAnimationAsset* NewAsset)
{
	if(UAnimSequenceBase* AnimSequence = Cast<UAnimSequenceBase>(NewAsset))
	{
		Sequence = AnimSequence;
	}
}

void FAnimNode_SequencePlayer::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	
	DebugLine += FString::Printf(TEXT("('%s' Play Time: %.3f)"), *Sequence->GetName(), InternalTimeAccumulator);
	DebugData.AddDebugItem(DebugLine, true);
}

float FAnimNode_SequencePlayer::GetTimeFromEnd(float CurrentNodeTime)
{
	return Sequence->GetMaxCurrentTime() - CurrentNodeTime;
}
