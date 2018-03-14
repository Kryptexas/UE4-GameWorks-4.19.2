// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimInstanceProxy.h"

#define LOCTEXT_NAMESPACE "AnimNode_SequencePlayer"

/////////////////////////////////////////////////////
// FAnimSequencePlayerNode

float FAnimNode_SequencePlayer::GetCurrentAssetTime()
{
	return InternalTimeAccumulator;
}

float FAnimNode_SequencePlayer::GetCurrentAssetTimePlayRateAdjusted()
{
	const float SequencePlayRate = (Sequence ? Sequence->RateScale : 1.f);
	const float EffectivePlayrate = FMath::IsNearlyZero(PlayRateBasis) ? 0.f : (SequencePlayRate * PlayRate / PlayRateBasis);
	return (EffectivePlayrate < 0.0f) ? GetCurrentAssetLength() - InternalTimeAccumulator : InternalTimeAccumulator;
}

float FAnimNode_SequencePlayer::GetCurrentAssetLength()
{
	return Sequence ? Sequence->SequenceLength : 0.0f;
}

void FAnimNode_SequencePlayer::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_AssetPlayerBase::Initialize_AnyThread(Context);

	EvaluateGraphExposedInputs.Execute(Context);
	InternalTimeAccumulator = StartPosition;
	if (Sequence != nullptr)
	{
		InternalTimeAccumulator = FMath::Clamp(StartPosition, 0.f, Sequence->SequenceLength);
		const float EffectivePlayrate = FMath::IsNearlyZero(PlayRateBasis) ? 0.f : (Sequence->RateScale * PlayRate / PlayRateBasis);
		if ((StartPosition == 0.f) && (EffectivePlayrate < 0.f))
		{
			InternalTimeAccumulator = Sequence->SequenceLength;
		}
	}
}

void FAnimNode_SequencePlayer::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) 
{
}

void FAnimNode_SequencePlayer::UpdateAssetPlayer(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);

	if ((Sequence != nullptr) && (Context.AnimInstanceProxy->IsSkeletonCompatible(Sequence->GetSkeleton())))
	{
		InternalTimeAccumulator = FMath::Clamp(InternalTimeAccumulator, 0.f, Sequence->SequenceLength);
		const float AdjustedPlayRate = FMath::IsNearlyZero(PlayRateBasis) ? 0.f : (PlayRate / PlayRateBasis);
		CreateTickRecordForNode(Context, Sequence, bLoopAnimation, AdjustedPlayRate);
	}
}

void FAnimNode_SequencePlayer::Evaluate_AnyThread(FPoseContext& Output)
{
	auto BoolToYesNo = [](const bool& bBool) -> const TCHAR*
	{
		return bBool ? TEXT("Yes") : TEXT("No");
	};

	if ((Sequence != nullptr) && (Output.AnimInstanceProxy->IsSkeletonCompatible(Sequence->GetSkeleton())))
	{
		const bool bExpectedAdditive = Output.ExpectsAdditivePose();
		const bool bIsAdditive = Sequence->IsValidAdditive();

		if (bExpectedAdditive && !bIsAdditive)
		{
			FText Message = FText::Format(LOCTEXT("AdditiveMismatchWarning", "Trying to play a non-additive animation '{0}' into a pose that is expected to be additive in anim instance '{1}'"), FText::FromString(Sequence->GetName()), FText::FromString(Output.AnimInstanceProxy->GetAnimInstanceName()));
			Output.LogMessage(EMessageSeverity::Warning, Message);
		}

		Sequence->GetAnimationPose(Output.Pose, Output.Curve, FAnimExtractContext(InternalTimeAccumulator, Output.AnimInstanceProxy->ShouldExtractRootMotion()));
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
	
	DebugLine += FString::Printf(TEXT("('%s' Play Time: %.3f)"), Sequence ? *Sequence->GetName() : TEXT("NULL"), InternalTimeAccumulator);
	DebugData.AddDebugItem(DebugLine, true);
}

float FAnimNode_SequencePlayer::GetTimeFromEnd(float CurrentNodeTime)
{
	return Sequence->GetMaxCurrentTime() - CurrentNodeTime;
}

#undef LOCTEXT_NAMESPACE