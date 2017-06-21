// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Animation/AnimInstanceProxy.h"
#include "Engine/SkeletalMeshSocket.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Socket Reference 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FSocketReference::InitializeSocketInfo(const FAnimInstanceProxy* InAnimInstanceProxy)
{
	CachedSocketMeshBoneIndex = INDEX_NONE;
	CachedSocketCompactBoneIndex = FCompactPoseBoneIndex(INDEX_NONE);

	if (SocketName != NAME_None)
	{
		const USkeletalMeshComponent* OwnerMeshComponent = InAnimInstanceProxy->GetSkelMeshComponent();
		if (OwnerMeshComponent && OwnerMeshComponent->DoesSocketExist(SocketName))
		{
			USkeletalMeshSocket const* const Socket = OwnerMeshComponent->GetSocketByName(SocketName);
			if (Socket)
			{
				CachedSocketLocalTransform = Socket->GetSocketLocalTransform();
				// cache mesh bone index, so that we know this is valid information to follow
				CachedSocketMeshBoneIndex = OwnerMeshComponent->GetBoneIndex(Socket->BoneName);

				ensureMsgf(CachedSocketMeshBoneIndex != INDEX_NONE, TEXT("%s : socket has invalid bone."), *SocketName.ToString());
			}
		}
		else
		{
			// @todo : move to graph node warning
			UE_LOG(LogAnimation, Warning, TEXT("%s: socket doesn't exist"), *SocketName.ToString());
		}
	}
}

void FSocketReference::InitialzeCompactBoneIndex(const FBoneContainer& RequiredBones)
{
	if (CachedSocketMeshBoneIndex != INDEX_NONE)
	{
		const int32 SocketBoneSkeletonIndex = RequiredBones.GetPoseToSkeletonBoneIndexArray()[CachedSocketMeshBoneIndex];
		CachedSocketCompactBoneIndex = RequiredBones.GetCompactPoseIndexFromSkeletonIndex(SocketBoneSkeletonIndex);
	}
}

FTransform FSocketReference::GetAnimatedSocketTransform(FCSPose<FCompactPose>&	InPose) const
{
	// current LOD has valid index (FCompactPoseBoneIndex is valid if current LOD supports)
	if (CachedSocketCompactBoneIndex != INDEX_NONE)
	{
		FTransform BoneTransform = InPose.GetComponentSpaceTransform(CachedSocketCompactBoneIndex);
		return CachedSocketLocalTransform * BoneTransform;
	}

	return FTransform::Identity;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Target Reference 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FVector FTargetReference::GetTargetLocation(FVector TargetOffset, const FBoneContainer& BoneContainer, FCSPose<FCompactPose>& InPose, const FTransform& InComponentToWorld, FTransform& OutTargetTransform)
{
	FVector TargetLocationInComponentSpace;

	auto SetWorldOffset = [](const FVector& InTargetOffset, const FTransform& LocalInComponentToWorld, FTransform& LocalOutTargetTransform, FVector& OutTargetLocationInComponentSpace)
	{
		LocalOutTargetTransform.SetIdentity();
		OutTargetLocationInComponentSpace = LocalInComponentToWorld.InverseTransformPosition(InTargetOffset);
		LocalOutTargetTransform.SetLocation(InTargetOffset);
	};

	if (bUseSocket)
	{
		if (SocketReference.IsValidToEvaluate(BoneContainer))
		{
			FTransform SocketTransformInCS = SocketReference.GetAnimatedSocketTransform(InPose);

			TargetLocationInComponentSpace = SocketTransformInCS.TransformPosition(TargetOffset);
			OutTargetTransform = SocketTransformInCS;
		}
		else
		{
			SetWorldOffset(TargetOffset, InComponentToWorld, OutTargetTransform, TargetLocationInComponentSpace);
		}
	}
	// if valid data is available
	else if (BoneReference.HasValidSetup())
	{
		if (BoneReference.IsValidToEvaluate(BoneContainer))
		{
			OutTargetTransform = InPose.GetComponentSpaceTransform(BoneReference.GetCompactPoseIndex(BoneContainer));
			TargetLocationInComponentSpace = OutTargetTransform.TransformPosition(TargetOffset);
		}
		else
		{
			SetWorldOffset(TargetOffset, InComponentToWorld, OutTargetTransform, TargetLocationInComponentSpace);
		}
	}
	else
	{
		SetWorldOffset(TargetOffset, InComponentToWorld, OutTargetTransform, TargetLocationInComponentSpace);
	}

	return TargetLocationInComponentSpace;
}

/////////////////////////////////////////////////////
// FAnimNode_SkeletalControlBase

void FAnimNode_SkeletalControlBase::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	FAnimNode_Base::Initialize_AnyThread(Context);

	ComponentPose.Initialize(Context);
}

void FAnimNode_SkeletalControlBase::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) 
{
	InitializeBoneReferences(Context.AnimInstanceProxy->GetRequiredBones());
	ComponentPose.CacheBones(Context);
}

void FAnimNode_SkeletalControlBase::UpdateInternal(const FAnimationUpdateContext& Context)
{
}

void FAnimNode_SkeletalControlBase::Update_AnyThread(const FAnimationUpdateContext& Context)
{
	ComponentPose.Update(Context);

	ActualAlpha = 0.f;
	if (IsLODEnabled(Context.AnimInstanceProxy, LODThreshold))
	{
		EvaluateGraphExposedInputs.Execute(Context);

		// Apply the skeletal control if it's valid
		ActualAlpha = AlphaScaleBias.ApplyTo(Alpha);
		if (FAnimWeight::IsRelevant(ActualAlpha) && IsValidToEvaluate(Context.AnimInstanceProxy->GetSkeleton(), Context.AnimInstanceProxy->GetRequiredBones()))
		{
			UpdateInternal(Context);
		}
	}
}

bool ContainsNaN(const TArray<FBoneTransform> & BoneTransforms)
{
	for (int32 i = 0; i < BoneTransforms.Num(); ++i)
	{
		if (BoneTransforms[i].Transform.ContainsNaN())
		{
			return true;
		}
	}

	return false;
}

void FAnimNode_SkeletalControlBase::EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context)
{
}

void FAnimNode_SkeletalControlBase::EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output)
{
	// Evaluate the input
	ComponentPose.EvaluateComponentSpace(Output);

	// Apply the skeletal control if it's valid
	if (FAnimWeight::IsRelevant(ActualAlpha) && IsValidToEvaluate(Output.AnimInstanceProxy->GetSkeleton(), Output.AnimInstanceProxy->GetRequiredBones()))
	{
		EvaluateComponentSpaceInternal(Output);

#if WITH_EDITORONLY_DATA
		// save current pose before applying skeletal control to compute the exact gizmo location in AnimGraphNode
		ForwardedPose.CopyPose(Output.Pose);
#endif // #if WITH_EDITORONLY_DATA

		USkeletalMeshComponent* Component = Output.AnimInstanceProxy->GetSkelMeshComponent();

		BoneTransforms.Reset(BoneTransforms.Num());
		EvaluateSkeletalControl_AnyThread(Output, BoneTransforms);

		checkSlow(!ContainsNaN(BoneTransforms));

		if (BoneTransforms.Num() > 0)
		{
			const float BlendWeight = FMath::Clamp<float>(ActualAlpha, 0.f, 1.f);
			Output.Pose.LocalBlendCSBoneTransforms(BoneTransforms, BlendWeight);
		}
	}
}

void FAnimNode_SkeletalControlBase::AddDebugNodeData(FString& OutDebugData)
{
	OutDebugData += FString::Printf(TEXT("Alpha: %.1f%%"), ActualAlpha*100.f);
}

void FAnimNode_SkeletalControlBase::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	// Call legacy implementation for backwards compatibility
	EvaluateBoneTransforms(Output.AnimInstanceProxy->GetSkelMeshComponent(), Output.Pose, OutBoneTransforms);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

