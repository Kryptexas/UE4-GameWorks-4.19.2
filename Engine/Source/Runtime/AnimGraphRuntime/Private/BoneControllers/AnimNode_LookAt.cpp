// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "BoneControllers/AnimNode_LookAt.h"
#include "SceneManagement.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimationCoreLibrary.h"

FVector FAnimNode_LookAt::GetAlignVector(const FTransform& Transform, EAxisOption::Type AxisOption, const FVector& CustomAxis)
{
	switch(AxisOption)
	{
	case EAxisOption::X:
		return Transform.GetUnitAxis(EAxis::X);
	case EAxisOption::X_Neg:
		return -Transform.GetUnitAxis(EAxis::X);
	case EAxisOption::Y:
		return Transform.GetUnitAxis(EAxis::Y);
	case EAxisOption::Y_Neg:
		return -Transform.GetUnitAxis(EAxis::Y);
	case EAxisOption::Z:
		return Transform.GetUnitAxis(EAxis::Z);
	case EAxisOption::Z_Neg:
		return -Transform.GetUnitAxis(EAxis::Z);
	case EAxisOption::Custom:
		return Transform.TransformVector(CustomAxis.GetSafeNormal());
	}

	return FVector(1.f, 0.f, 0.f);
}
/////////////////////////////////////////////////////
// FAnimNode_LookAt

FAnimNode_LookAt::FAnimNode_LookAt()
	: LookAtLocation(FVector(100.f, 0.f, 0.f))
	, LookAtAxis(EAxisOption::Y)
	, CustomLookAtAxis(FVector(0.f, 1.f, 0.f))
	, LookUpAxis(EAxisOption::X)
	, CustomLookUpAxis(FVector(1.f, 0.f, 0.f))
	, LookAtClamp(0.f)
	, InterpolationTime(0.f)
	, InterpolationTriggerThreashold(0.f)
	, CurrentLookAtLocation(FVector::ZeroVector)
	, AccumulatedInterpoolationTime(0.f)
	, CachedLookAtSocketMeshBoneIndex(INDEX_NONE)
	, CachedLookAtSocketBoneIndex(INDEX_NONE)
{
}

void FAnimNode_LookAt::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	if (bEnableDebug)
	{
		DebugLine += "(";
		AddDebugNodeData(DebugLine);
		if (LookAtBone.BoneIndex != INDEX_NONE)
		{
			DebugLine += FString::Printf(TEXT(" Target: %s, Look At Bone: %s, Location : %s)"), *BoneToModify.BoneName.ToString(), *LookAtBone.BoneName.ToString(), *CurrentLookAtLocation.ToString());
		}
		else
		{
			DebugLine += FString::Printf(TEXT(" Target: %s, Look At Location : %s, Location : %s)"), *BoneToModify.BoneName.ToString(), *LookAtLocation.ToString(), *CurrentLookAtLocation.ToString());
		}
	}
	else
	{
		DebugLine += FString::Printf(TEXT("Target: %s"), *BoneToModify.BoneName.ToString());
	}

	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void DrawDebugData(FPrimitiveDrawInterface* PDI, const FTransform& TransformVector, const FVector& StartLoc, const FVector& TargetLoc, FColor Color)
{
	FVector Start = TransformVector.TransformPosition(StartLoc);
	FVector End = TransformVector.TransformPosition(TargetLoc);

	PDI->DrawLine(Start, End, Color, SDPG_Foreground);
}

void FAnimNode_LookAt::EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();
	const FCompactPoseBoneIndex ModifyBoneIndex = BoneToModify.GetCompactPoseIndex(BoneContainer);
	FTransform ComponentBoneTransform = MeshBases.GetComponentSpaceTransform(ModifyBoneIndex);

	// get target location
	FVector TargetLocationInComponentSpace;
	if (LookAtBone.IsValid(BoneContainer))
	{
		const FTransform& LookAtTransform  = MeshBases.GetComponentSpaceTransform(LookAtBone.GetCompactPoseIndex(BoneContainer));
		TargetLocationInComponentSpace = LookAtTransform.GetLocation();
	}
	// if valid data is available
	else if (CachedLookAtSocketMeshBoneIndex != INDEX_NONE)
	{
		// current LOD has valid index (FCompactPoseBoneIndex is valid if current LOD supports)
		if (CachedLookAtSocketBoneIndex != INDEX_NONE)
		{
			FTransform BoneTransform = MeshBases.GetComponentSpaceTransform(CachedLookAtSocketBoneIndex);
			FTransform SocketTransformInCS = CachedSocketLocalTransform * BoneTransform;

			TargetLocationInComponentSpace = SocketTransformInCS.GetLocation();
		}
	}
	else
	{
		TargetLocationInComponentSpace = SkelComp->ComponentToWorld.InverseTransformPosition(LookAtLocation);
	}
	
	FVector OldCurrentTargetLocation = CurrentTargetLocation;
	FVector NewCurrentTargetLocation = TargetLocationInComponentSpace;

	if ((NewCurrentTargetLocation - OldCurrentTargetLocation).SizeSquared() > InterpolationTriggerThreashold*InterpolationTriggerThreashold)
	{
		if (AccumulatedInterpoolationTime >= InterpolationTime)
		{
			// reset current Alpha, we're starting to move
			AccumulatedInterpoolationTime = 0.f;
		}

		PreviousTargetLocation = OldCurrentTargetLocation;
		CurrentTargetLocation = NewCurrentTargetLocation;
	}
	else if (InterpolationTriggerThreashold == 0.f)
	{
		CurrentTargetLocation = NewCurrentTargetLocation;
	}

	if (InterpolationTime > 0.f)
	{
		float CurrentAlpha = AccumulatedInterpoolationTime/InterpolationTime;

		if (CurrentAlpha < 1.f)
		{
			float BlendAlpha = AlphaToBlendType(CurrentAlpha, GetInterpolationType());

			CurrentLookAtLocation = FMath::Lerp(PreviousTargetLocation, CurrentTargetLocation, BlendAlpha);
		}
	}
	else
	{
		CurrentLookAtLocation = CurrentTargetLocation;
	}

#if WITH_EDITOR
	if (bEnableDebug)
	{
		CachedComponentBoneLocation = ComponentBoneTransform.GetLocation();
		CachedPreviousTargetLocation = PreviousTargetLocation;
		CachedCurrentTargetLocation = CurrentTargetLocation;
		CachedCurrentLookAtLocation = CurrentLookAtLocation;
	}
#endif

	// lookat vector
	FVector LookAtVector = GetAlignVector(ComponentBoneTransform, LookAtAxis, CustomLookAtAxis);
	// find look up vector in local space
	FVector LookUpVector = GetAlignVector(ComponentBoneTransform, LookUpAxis, CustomLookUpAxis);
	// Find new transform from look at info
	FQuat DeltaRotation = AnimationCore::SolveAim(ComponentBoneTransform, CurrentLookAtLocation, LookAtVector, bUseLookUpAxis, LookUpVector, LookAtClamp);
	ComponentBoneTransform.SetRotation(DeltaRotation * ComponentBoneTransform.GetRotation());
	// Set New Transform 
	OutBoneTransforms.Add(FBoneTransform(ModifyBoneIndex, ComponentBoneTransform));
}

bool FAnimNode_LookAt::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	// if both bones are valid
	return (BoneToModify.IsValid(RequiredBones) && 
		// or if name isn't set (use Look At Location) or Look at bone is valid 
		// do not call isValid since that means if look at bone isn't in LOD, we won't evaluate
		// we still should evaluate as long as the BoneToModify is valid even LookAtBone isn't included in required bones
		(LookAtBone.BoneName == NAME_None || LookAtBone.BoneIndex != INDEX_NONE) );
}

void FAnimNode_LookAt::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp)
{
#if WITH_EDITOR
	if(bEnableDebug)
	{
		if(UWorld* World = MeshComp->GetWorld())
		{
			if(CachedPreviousTargetLocation != FVector::ZeroVector)
			{
				DrawDebugData(PDI, FTransform::Identity, CachedComponentBoneLocation, CachedPreviousTargetLocation, FColor(0, 255, 0));
			}

			DrawDebugData(PDI, FTransform::Identity, CachedComponentBoneLocation, CachedCurrentTargetLocation, FColor(255, 0, 0));
			DrawDebugData(PDI, FTransform::Identity, CachedComponentBoneLocation, CachedCurrentLookAtLocation, FColor(0, 0, 255));

			DrawWireStar(PDI, CachedCurrentTargetLocation, 5.0f, FLinearColor(1, 0, 0), SDPG_Foreground);
		}
	}
#endif
}

void FAnimNode_LookAt::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	BoneToModify.Initialize(RequiredBones);
	LookAtBone.Initialize(RequiredBones);

	// this should be called whenver LOD changes and so on
	if (CachedLookAtSocketMeshBoneIndex != INDEX_NONE)
	{
		const int32 SocketBoneSkeletonIndex = RequiredBones.GetPoseToSkeletonBoneIndexArray()[CachedLookAtSocketMeshBoneIndex];
		CachedLookAtSocketBoneIndex = RequiredBones.GetCompactPoseIndexFromSkeletonIndex(SocketBoneSkeletonIndex);
	}
}

void FAnimNode_LookAt::UpdateInternal(const FAnimationUpdateContext& Context)
{
	FAnimNode_SkeletalControlBase::UpdateInternal(Context);

	AccumulatedInterpoolationTime = FMath::Clamp(AccumulatedInterpoolationTime+Context.GetDeltaTime(), 0.f, InterpolationTime);;
}

void FAnimNode_LookAt::Initialize(const FAnimationInitializeContext& Context)
{
	FAnimNode_SkeletalControlBase::Initialize(Context);

	CachedLookAtSocketMeshBoneIndex = INDEX_NONE;

	if (LookAtSocket != NAME_None)
	{
		const USkeletalMeshComponent* OwnerMeshComponent = Context.AnimInstanceProxy->GetSkelMeshComponent();
		if (OwnerMeshComponent && OwnerMeshComponent->DoesSocketExist(LookAtSocket))
		{
			USkeletalMeshSocket const* const Socket = OwnerMeshComponent->GetSocketByName(LookAtSocket);
			if (Socket)
			{
				CachedSocketLocalTransform = Socket->GetSocketLocalTransform();
				// cache mesh bone index, so that we know this is valid information to follow
				CachedLookAtSocketMeshBoneIndex = OwnerMeshComponent->GetBoneIndex(Socket->BoneName);

				ensureMsgf(CachedLookAtSocketMeshBoneIndex != INDEX_NONE, TEXT("%s : socket has invalid bone."), *LookAtSocket.ToString());
			}
		}
		else
		{
			// @todo : move to graph node warning
			UE_LOG(LogAnimation, Warning, TEXT("%s: socket doesn't exist"), *LookAtSocket.ToString());
		}
	}
}
