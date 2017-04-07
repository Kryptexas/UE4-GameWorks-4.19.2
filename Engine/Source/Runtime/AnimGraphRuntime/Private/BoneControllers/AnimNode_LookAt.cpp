// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "BoneControllers/AnimNode_LookAt.h"
#include "SceneManagement.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimationCoreLibrary.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInstanceDynamic.h"

static const FVector DefaultLookAtAxis(0.f, 1.f, 0.f);
static const FVector DefaultLookUpAxis(1.f, 0.f, 0.f);

/////////////////////////////////////////////////////
// FAnimNode_LookAt

FAnimNode_LookAt::FAnimNode_LookAt()
	: LookAtLocation(FVector(100.f, 0.f, 0.f))
	, LookAtAxis_DEPRECATED(EAxisOption::Y)
	, CustomLookAtAxis_DEPRECATED(FVector(0.f, 1.f, 0.f))
	, LookAt_Axis(DefaultLookAtAxis)
	, LookUpAxis_DEPRECATED(EAxisOption::X)
	, CustomLookUpAxis_DEPRECATED(FVector(1.f, 0.f, 0.f))
	, LookUp_Axis(DefaultLookUpAxis)
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

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	if (LookAtBone.BoneIndex != INDEX_NONE)
	{
		DebugLine += FString::Printf(TEXT(" Bone: %s, Look At Bone: %s, Look At Location: %s, Target Location : %s)"), *BoneToModify.BoneName.ToString(), *LookAtBone.BoneName.ToString(), *LookAtLocation.ToString(), *CachedCurrentTargetLocation.ToString());
	}
	else if (LookAtSocket != NAME_None)
	{
		DebugLine += FString::Printf(TEXT(" Bone: %s, Look At Socket: %s, Look At Location: %s, , Target Location : %s)"), *BoneToModify.BoneName.ToString(), *LookAtSocket.ToString(), *LookAtLocation.ToString(), *CachedCurrentTargetLocation.ToString());
	}
	else
	{
		DebugLine += FString::Printf(TEXT(" Bone: %s, Look At Location : %s, Target Location : %s)"), *BoneToModify.BoneName.ToString(), *LookAtLocation.ToString(), *CachedCurrentTargetLocation.ToString());
	}

	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

void FAnimNode_LookAt::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	check(OutBoneTransforms.Num() == 0);

	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
	const FCompactPoseBoneIndex ModifyBoneIndex = BoneToModify.GetCompactPoseIndex(BoneContainer);
	FTransform ComponentBoneTransform = Output.Pose.GetComponentSpaceTransform(ModifyBoneIndex);

	// get target location
	FVector TargetLocationInComponentSpace;
	FTransform TargetTransform = FTransform::Identity;
	if (LookAtBone.IsValid(BoneContainer))
	{
		TargetTransform  = Output.Pose.GetComponentSpaceTransform(LookAtBone.GetCompactPoseIndex(BoneContainer));
		TargetLocationInComponentSpace = TargetTransform.TransformPosition(LookAtLocation);
	}
	// if valid data is available
	else if (CachedLookAtSocketMeshBoneIndex != INDEX_NONE)
	{
		// current LOD has valid index (FCompactPoseBoneIndex is valid if current LOD supports)
		if (CachedLookAtSocketBoneIndex != INDEX_NONE)
		{
			FTransform BoneTransform = Output.Pose.GetComponentSpaceTransform(CachedLookAtSocketBoneIndex);
			FTransform SocketTransformInCS = CachedSocketLocalTransform * BoneTransform;

			TargetLocationInComponentSpace = SocketTransformInCS.TransformPosition(LookAtLocation);
			TargetTransform = SocketTransformInCS;
		}
	}
	else
	{
		TargetLocationInComponentSpace = Output.AnimInstanceProxy->GetComponentTransform().InverseTransformPosition(LookAtLocation);
		TargetTransform.SetLocation(LookAtLocation);
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

#if !UE_BUILD_SHIPPING
	CachedOriginalTransform = ComponentBoneTransform;
	CachedTargetTransform = TargetTransform;
	CachedPreviousTargetLocation = PreviousTargetLocation;
	CachedCurrentLookAtLocation = CurrentLookAtLocation;
#endif
	CachedCurrentTargetLocation = CurrentTargetLocation;

	// lookat vector
	FVector LookAtVector = LookAt_Axis.GetTransformedAxis(ComponentBoneTransform);
	// find look up vector in local space
	FVector LookUpVector = LookUp_Axis.GetTransformedAxis(ComponentBoneTransform);
	// Find new transform from look at info
	FQuat DeltaRotation = AnimationCore::SolveAim(ComponentBoneTransform, CurrentLookAtLocation, LookAtVector, bUseLookUpAxis, LookUpVector, LookAtClamp);
	ComponentBoneTransform.SetRotation(DeltaRotation * ComponentBoneTransform.GetRotation());
	// Set New Transform 
	OutBoneTransforms.Add(FBoneTransform(ModifyBoneIndex, ComponentBoneTransform));

#if !UE_BUILD_SHIPPING
	CachedLookAtTransform = ComponentBoneTransform;
#endif
}

void FAnimNode_LookAt::EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context)
{
	Super::EvaluateComponentSpaceInternal(Context);

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	/*if (bEnableDebug)
	{
		const FTransform LocalToWorld = Context.AnimInstanceProxy->GetSkelMeshCompLocalToWorld();
		FVector TargetWorldLoc = LocalToWorld.TransformPosition(CachedCurrentTargetLocation);
		FVector SourceWorldLoc = LocalToWorld.TransformPosition(CachedComponentBoneLocation);

		Context.AnimInstanceProxy->AnimDrawDebugLine(SourceWorldLoc, TargetWorldLoc, FColor::Green);
	}*/

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
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

#if WITH_EDITOR
// can't use World Draw functions because this is called from Render of viewport, AFTER ticking component, 
// which means LineBatcher already has ticked, so it won't render anymore
// to use World Draw functions, we have to call this from tick of actor
void FAnimNode_LookAt::ConditionalDebugDraw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* MeshComp) const
{
	auto CalculateLookAtMatrixFromTransform = [this](const FTransform& BaseTransform) -> FMatrix
	{
		FVector TransformedLookAtAxis = BaseTransform.TransformVector(LookAt_Axis.Axis);
		const FVector DefaultUpVector = BaseTransform.GetUnitAxis(EAxis::Z);
		FVector UpVector = (bUseLookUpAxis) ? BaseTransform.TransformVector(LookUp_Axis.Axis) : DefaultUpVector;
		// if parallel with up vector, find something else
		if (FMath::Abs(FVector::DotProduct(UpVector, TransformedLookAtAxis)) > (1.f - ZERO_ANIMWEIGHT_THRESH))
		{
			UpVector = BaseTransform.GetUnitAxis(EAxis::X);
		}

		FVector RightVector = FVector::CrossProduct(TransformedLookAtAxis, UpVector);
		FMatrix Matrix;
		FVector Location = BaseTransform.GetLocation();
		Matrix.SetAxes(&TransformedLookAtAxis, &RightVector, &UpVector, &Location);
		return Matrix;
	};

	// did not apply any of LocaltoWorld
	if(PDI && MeshComp)
	{
		FTransform LocalToWorld = MeshComp->GetComponentTransform();
		FTransform ComponentTransform = CachedOriginalTransform * LocalToWorld;
		FTransform LookAtTransform = CachedLookAtTransform * LocalToWorld;
		FTransform TargetTrasnform = CachedTargetTransform * LocalToWorld;
		FVector BoneLocation = LookAtTransform.GetLocation();

		// we're using interpolation, so print previous location
		const bool bUseInterpolation = InterpolationTime > 0.f;
		if(bUseInterpolation)
		{
			// this only will be different if we're interpolating
			DrawDashedLine(PDI, BoneLocation, LocalToWorld.TransformPosition(CachedPreviousTargetLocation), FColor(0, 255, 0), 5.f, SDPG_World);
		}

		// current look at location (can be clamped or interpolating)
		DrawDashedLine(PDI, BoneLocation, LocalToWorld.TransformPosition(CachedCurrentLookAtLocation), FColor::Yellow, 5.f, SDPG_World);
		DrawWireStar(PDI, CachedCurrentLookAtLocation, 5.0f, FColor::Yellow, SDPG_World);

		// draw current target information
		DrawDashedLine(PDI, BoneLocation, LocalToWorld.TransformPosition(CachedCurrentTargetLocation), FColor::Blue, 5.f, SDPG_World);
		DrawWireStar(PDI, CachedCurrentTargetLocation, 5.0f, FColor::Blue, SDPG_World);

		// draw the angular clamp
		if (LookAtClamp > 0.f)
		{
			float Angle = FMath::DegreesToRadians(LookAtClamp);
			float ConeSize = 30.f;
			DrawCone(PDI, FScaleMatrix(ConeSize) * CalculateLookAtMatrixFromTransform(ComponentTransform), Angle, Angle, 20, false, FLinearColor::Green, GEngine->ConstraintLimitMaterialY->GetRenderProxy(false), SDPG_World);
		}

		// draw directional  - lookat and look up
		DrawDirectionalArrow(PDI, CalculateLookAtMatrixFromTransform(LookAtTransform), FLinearColor::Red, 20, 5, SDPG_World);
		DrawCoordinateSystem(PDI, BoneLocation, LookAtTransform.GetRotation().Rotator(), 20.f, SDPG_Foreground);
		DrawCoordinateSystem(PDI, TargetTrasnform.GetLocation(), TargetTrasnform.GetRotation().Rotator(), 20.f, SDPG_Foreground);
	}
}
#endif // WITH_EDITOR

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

	// initialize
	LookUp_Axis.Initialize();
	if (LookUp_Axis.Axis.IsZero())
	{
		UE_LOG(LogAnimation, Warning, TEXT("Zero-length look-up axis specified in LookAt node. Reverting to default."));
		LookUp_Axis.Axis = DefaultLookUpAxis;
	}
	LookAt_Axis.Initialize();
	if (LookAt_Axis.Axis.IsZero())
	{
		UE_LOG(LogAnimation, Warning, TEXT("Zero-length look-at axis specified in LookAt node. Reverting to default."));
		LookAt_Axis.Axis = DefaultLookAtAxis;
	}
}
