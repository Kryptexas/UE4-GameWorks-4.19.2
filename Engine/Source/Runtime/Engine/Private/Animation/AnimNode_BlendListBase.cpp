// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Animation/AnimNode_BlendListBase.h"
#include "AnimationRuntime.h"
#include "Animation/AnimStats.h"

/////////////////////////////////////////////////////
// FAnimNode_BlendListBase

void FAnimNode_BlendListBase::Initialize(const FAnimationInitializeContext& Context)
{
	const int NumPoses = BlendPose.Num();
	checkSlow(BlendTime.Num() == NumPoses);

	BlendWeights.Empty(NumPoses);
	if (NumPoses > 0)
	{
		BlendWeights.AddZeroed(NumPoses);
		BlendWeights[0] = 1.0f;

		for (int32 ChildIndex = 0; ChildIndex < NumPoses; ++ChildIndex)
		{
			BlendPose[ChildIndex].Initialize(Context);
		}
	}

	RemainingBlendTimes.Empty(NumPoses);
	RemainingBlendTimes.AddZeroed(NumPoses);
	Blends.Empty(NumPoses);
	Blends.AddZeroed(NumPoses);

	LastActiveChildIndex = INDEX_NONE;

	for(int32 i = 0 ; i < Blends.Num() ; ++i)
	{
		FAlphaBlend& Blend = Blends[i];

		Blend.SetBlendTime(0.0f);
		Blend.SetBlendOption(BlendType);
		Blend.CustomCurve = CustomBlendCurve;
	}
	Blends[0].SetAlpha(1.0f);
}

void FAnimNode_BlendListBase::CacheBones(const FAnimationCacheBonesContext& Context) 
{
	for(int32 ChildIndex=0; ChildIndex<BlendPose.Num(); ChildIndex++)
	{
		BlendPose[ChildIndex].CacheBones(Context);
	}
}

void FAnimNode_BlendListBase::Update(const FAnimationUpdateContext& Context)
{
	EvaluateGraphExposedInputs.Execute(Context);

	const int NumPoses = BlendPose.Num();
	checkSlow((BlendTime.Num() == NumPoses) && (BlendWeights.Num() == NumPoses));

	PosesToEvaluate.Empty(NumPoses);

	if (NumPoses > 0)
	{
		// Handle a change in the active child index; adjusting the target weights
		const int32 ChildIndex = GetActiveChildIndex();

		if (ChildIndex != LastActiveChildIndex)
		{
			bool LastChildIndexIsInvalid = (LastActiveChildIndex == INDEX_NONE);
			LastActiveChildIndex = ChildIndex;

			const float CurrentWeight = BlendWeights[ChildIndex];
			const float DesiredWeight = 1.0f;
			const float WeightDifference = FMath::Clamp<float>(FMath::Abs<float>(DesiredWeight - CurrentWeight), 0.0f, 1.0f);

			// scale by the weight difference since we want always consistency:
			// - if you're moving from 0 to full weight 1, it will use the normal blend time
			// - if you're moving from 0.5 to full weight 1, it will get there in half the time
			const float RemainingBlendTime = LastChildIndexIsInvalid ? 0.0f : ( BlendTime[ChildIndex] * WeightDifference );

			for (int32 i = 0; i < RemainingBlendTimes.Num(); ++i)
			{
				RemainingBlendTimes[i] = RemainingBlendTime;
			}

			for(int32 i = 0; i < Blends.Num(); ++i)
			{
				FAlphaBlend& Blend = Blends[i];

				Blend.SetBlendTime(RemainingBlendTime);

				if(i == ChildIndex)
				{
					Blend.SetValueRange(BlendWeights[i], 1.0f);
				}
				else
				{
					Blend.SetValueRange(BlendWeights[i], 0.0f);
				}

				Blend.Reset();
			}
		}

		// Advance the weights
		//@TODO: This means we advance even in a frame where the target weights/times just got modified; is that desirable?
		float SumWeight = 0.0f;
		for (int32 i = 0; i < Blends.Num(); ++i)
		{
			float& BlendWeight = BlendWeights[i];

			FAlphaBlend& Blend = Blends[i];
			Blend.Update(Context.GetDeltaTime());
			BlendWeight = Blend.GetBlendedValue();

			SumWeight += BlendWeight;
		}

		// Renormalize the weights
		if ((SumWeight > ZERO_ANIMWEIGHT_THRESH) && (FMath::Abs<float>(SumWeight - 1.0f) > ZERO_ANIMWEIGHT_THRESH))
		{
			float ReciprocalSum = 1.0f / SumWeight;
			for (int32 i = 0; i < BlendWeights.Num(); ++i)
			{
				BlendWeights[i] *= ReciprocalSum;
			}
		}

		// Update our active children
		for (int32 i = 0; i < BlendPose.Num(); ++i)
		{
			const float BlendWeight = BlendWeights[i];
			if (BlendWeight > ZERO_ANIMWEIGHT_THRESH)
			{
				BlendPose[i].Update(Context.FractionalWeight(BlendWeight));
				PosesToEvaluate.Add(i);
			}
		}
	}
}

void FAnimNode_BlendListBase::Evaluate(FPoseContext& Output)
{
	ANIM_MT_SCOPE_CYCLE_COUNTER(BlendPosesInGraph, Output.AnimInstance->IsRunningParallelEvaluation());

	const int32 NumPoses = PosesToEvaluate.Num();

	if ((NumPoses > 0) && (BlendPose.Num() == BlendWeights.Num()))
	{
		TArray<FCompactPose> FilteredPoses;
		FilteredPoses.SetNum(NumPoses);

		TArray<FBlendedCurve> FilteredCurve;
		FilteredCurve.SetNum(NumPoses);

		TArray<float> FilteredWeights;
		FilteredWeights.AddUninitialized(NumPoses);

		int32 NumActivePoses = 0;
		for (int32 i = 0; i < PosesToEvaluate.Num(); ++i)
		{
			int32 PoseIndex = PosesToEvaluate[i];

			const float BlendWeight = BlendWeights[PoseIndex];
			FPoseContext EvaluateContext(Output);

			FPoseLink& CurrentPose = BlendPose[PoseIndex];
			CurrentPose.Evaluate(EvaluateContext);

			FilteredPoses[i].MoveBonesFrom(EvaluateContext.Pose);
			FilteredCurve[i] = EvaluateContext.Curve;
			FilteredWeights[i] = BlendWeight;
		}

		FAnimationRuntime::BlendPosesTogether(FilteredPoses, FilteredCurve, FilteredWeights, Output.Pose, Output.Curve);
	}
	else
	{
		Output.ResetToRefPose();
	}
}

void FAnimNode_BlendListBase::GatherDebugData(FNodeDebugData& DebugData)
{
	const int NumPoses = BlendPose.Num();
	const int32 ChildIndex = GetActiveChildIndex();

	FString DebugLine = GetNodeName(DebugData);
	DebugLine += FString::Printf(TEXT("(Active: (%i/%i) BlendWeight: %.1f%% BlendTime %.3f)"), ChildIndex+1, NumPoses, BlendWeights[ChildIndex]*100.f, BlendTime[ChildIndex]);

	DebugData.AddDebugItem(DebugLine);
	
	for(int32 Pose = 0; Pose < NumPoses; ++Pose)
	{
		BlendPose[Pose].GatherDebugData(DebugData.BranchFlow(BlendWeights[Pose]));
	}
}