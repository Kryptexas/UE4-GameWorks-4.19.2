// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphRuntimePrivatePCH.h"
#include "AnimNode_AnimDynamics.h"

TAutoConsoleVariable<int32> CVarEnableDynamics(TEXT("p.AnimDynamics"), 1, TEXT("Enables/Disables anim dynamics node updates."));
TAutoConsoleVariable<int32> CVarEnableAdaptiveSubstep(TEXT("p.AnimDynamicsAdaptiveSubstep"), 0, TEXT("Enables/disables adaptive substepping. Adaptive substepping will substep the simulation when it is necessary and maintain a debt buffer for time, always trying to utilise as much time as possible."));
TAutoConsoleVariable<int32> CVarAdaptiveSubstepNumDebtFrames(TEXT("p.AnimDynamicsNumDebtFrames"), 5, TEXT("Number of frames to maintain as time debt when using adaptive substepping, this should be at least 1 or the time debt will never be cleared."));

const float FAnimNode_AnimDynamics::MaxTimeDebt = (1.0f / 60.0f) * 5.0f; // 5 frames max debt

FAnimNode_AnimDynamics::FAnimNode_AnimDynamics()
: BoxExtents(0.0f)
, LocalJointOffset(0.0f)
, GravityScale(1.0f)
, bLinearSpring(false)
, bAngularSpring(false)
, LinearSpringConstant(0.0f)
, AngularSpringConstant(0.0f)
, bOverrideLinearDamping(false)
, LinearDampingOverride(0.0f)
, bOverrideAngularDamping(false)
, AngularDampingOverride(0.0f)
, bDoUpdate(true)
, bDoEval(true)
, NumSolverIterationsPreUpdate(4)
, NumSolverIterationsPostUpdate(1)
{

}

void FAnimNode_AnimDynamics::Initialize(const FAnimationInitializeContext& Context)
{
	FAnimNode_SkeletalControlBase::Initialize(Context);

	USkeletalMeshComponent* SkelMeshComponent = Context.AnimInstance->GetSkelMeshComponent();
	FBoneContainer& RequiredBones = Context.AnimInstance->RequiredBones;

	BoundBone.Initialize(RequiredBones);

	if (bChain)
	{
		ChainEnd.Initialize(RequiredBones);
	}

	if(BoundBone.IsValid(RequiredBones))
	{
		RequestInitialise();
	}

	NextTimeStep = 0.0f;
	TimeDebt = 0.0f;
}

void FAnimNode_AnimDynamics::Update(const FAnimationUpdateContext& Context)
{
	FAnimNode_SkeletalControlBase::Update(Context);

	NextTimeStep = Context.GetDeltaTime();
}

void FAnimNode_AnimDynamics::EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	if (CVarEnableDynamics.GetValueOnAnyThread() == 1)
	{
		// Pretty nasty - but there isn't really a good way to get clean bone transforms (without the modification from
		// previous runs) so we have to initialize here, checking often so we can restart a simulation in the editor.
		if (bRequiresInit)
		{
			InitPhysics(SkelComp, MeshBases);
			bRequiresInit = false;
		}

		TArray<FAnimPhysRigidBody*> BodyPtrs;

		for (FAnimPhysLinkedBody& Body : Bodies)
		{
			BodyPtrs.Add(&Body.RigidBody.PhysBody);
		}

		if (bDoUpdate && NextTimeStep > 0.0f)
		{
			if (CVarEnableAdaptiveSubstep.GetValueOnAnyThread() == 1)
			{
				float FixedTimeStep = UPhysicsSettings::Get()->MaxSubstepDeltaTime;
				int32 MaxSubsteps = UPhysicsSettings::Get()->MaxSubsteps;

				// Calculate number of substeps we should do.
				int32 NumIters = FMath::TruncToInt((NextTimeStep + TimeDebt) / FixedTimeStep);
				NumIters = FMath::Clamp(NumIters, 0, MaxSubsteps);

				// Store the remaining time as debt for later frames
				TimeDebt = (NextTimeStep + TimeDebt) - (NumIters * FixedTimeStep);
				TimeDebt = FMath::Clamp(TimeDebt, 0.0f, MaxTimeDebt);

				for (int32 Iter = 0; Iter < NumIters; ++Iter)
				{
					NextTimeStep = FixedTimeStep;
					UpdateLimits(MeshBases);
					FAnimPhys::PhysicsUpdate(NextTimeStep, BodyPtrs, LinearLimits, AngularLimits, Springs, NumSolverIterationsPreUpdate, NumSolverIterationsPostUpdate);
				}
			}
			else
			{
				// Do variable frame-time update
				UpdateLimits(MeshBases);
				FAnimPhys::PhysicsUpdate(NextTimeStep, BodyPtrs, LinearLimits, AngularLimits, Springs, NumSolverIterationsPreUpdate, NumSolverIterationsPostUpdate);
			}
		}

		if (bDoEval)
		{
			const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();

			for (int32 Idx = 0; Idx < BoundBoneReferences.Num(); ++Idx)
			{
				FBoneReference& CurrentChainBone = BoundBoneReferences[Idx];
				FAnimPhysRigidBody& CurrentBody = Bodies[Idx].RigidBody.PhysBody;

				FCompactPoseBoneIndex BoneIndex = CurrentChainBone.GetCompactPoseIndex(BoneContainer);

				FTransform NewBoneTransform(CurrentBody.Pose.Orientation, CurrentBody.Pose.Position + CurrentBody.Pose.Orientation.RotateVector(LocalJointOffset));
				OutBoneTransforms.Add(FBoneTransform(BoneIndex, NewBoneTransform));
			}
		}
	}
}

void FAnimNode_AnimDynamics::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	
}

void FAnimNode_AnimDynamics::GatherDebugData(FNodeDebugData& DebugData)
{
	const float ActualAlpha = AlphaScaleBias.ApplyTo(Alpha);

	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += FString::Printf(TEXT("(Alpha: %.1f%%)"), ActualAlpha*100.f);

	DebugData.AddDebugItem(DebugLine);
	ComponentPose.GatherDebugData(DebugData.BranchFlow(1.f));
}

bool FAnimNode_AnimDynamics::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	bool bValid = BoundBone.IsValid(RequiredBones);

	if (bChain)
	{
		bValid = bValid && ChainEnd.IsValid(RequiredBones);
	}

	return bValid;
}

int32 FAnimNode_AnimDynamics::GetNumBodies() const
{
	return Bodies.Num();
}

const FAnimPhysRigidBody& FAnimNode_AnimDynamics::GetPhysBody(int32 BodyIndex) const
{
	return Bodies[BodyIndex].RigidBody.PhysBody;
}

void FAnimNode_AnimDynamics::InitPhysics(USkeletalMeshComponent* Component, FCSPose<FCompactPose>& MeshBases)
{
	// Clear up any existing physics data
	TermPhysics();

	const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();

	if (bChain)
	{
		// List of bone indices in the chain.
		TArray<int32> ChainBoneIndices;
		TArray<FName> ChainBoneNames;

		// Add the end of the chain. We have to walk from the bottom upwards to find a chain
		// as walking downwards doesn't guarantee a single end point.
		ChainBoneIndices.Add(ChainEnd.BoneIndex);
		ChainBoneNames.Add(ChainEnd.BoneName);

		int32 ParentBoneIndex = BoneContainer.GetParentBoneIndex(ChainEnd.BoneIndex);

		// Walk up the chain until we either find the top or hit the root bone
		while (ParentBoneIndex != 0)
		{
			ChainBoneIndices.Add(ParentBoneIndex);
			ChainBoneNames.Add(Component->GetBoneName(ParentBoneIndex));

			if (ParentBoneIndex == BoundBone.BoneIndex)
			{
				// Found the top of the chain
				break;
			}

			ParentBoneIndex = BoneContainer.GetParentBoneIndex(ParentBoneIndex);
		}

		// Bail if we can't find a chain, and let the user know
		if (ParentBoneIndex != BoundBone.BoneIndex)
		{
			UE_LOG(LogAnimation, Error, TEXT("AnimDynamics: Attempted to find bone chain starting at %s and ending at %s but failed."), *BoundBone.BoneName.ToString(), *ChainEnd.BoneName.ToString());
			return;
		}

		Bodies.Reserve(ChainBoneIndices.Num());
		// Walk backwards here as the chain was discovered in reverse order
		for (int32 Idx = ChainBoneIndices.Num() - 1; Idx >= 0; --Idx)
		{
			TArray<FAnimPhysShape> BodyShapes;
			BodyShapes.Add(FAnimPhysShape::MakeBox(BoxExtents));

			FBoneReference LinkBoneRef;
			LinkBoneRef.BoneName = ChainBoneNames[Idx];
			LinkBoneRef.Initialize(BoneContainer);

			// Calculate joint offsets by looking at the length of the bones and extending the provided offset
			if (BoundBoneReferences.Num() > 0)
			{
				FTransform CurrentBoneTransform = MeshBases.GetComponentSpaceTransform(LinkBoneRef.GetCompactPoseIndex(BoneContainer));
				FTransform PreviousBoneTransform = MeshBases.GetComponentSpaceTransform(BoundBoneReferences.Last().GetCompactPoseIndex(BoneContainer));

				FVector PreviousAnchor = PreviousBoneTransform.TransformPosition(-LocalJointOffset);
				float DistanceToAnchor = (PreviousAnchor - CurrentBoneTransform.GetTranslation()).Size();

				if(LocalJointOffset.SizeSquared() < SMALL_NUMBER)
				{
					// No offset, just use the position between chain links as the offset
					// This is likely to just look horrible, but at least the bodies will
					// be placed correctly and not stack up at the top of the chain.
					JointOffsets.Add(PreviousAnchor - CurrentBoneTransform.GetTranslation());
				}
				else
				{
					// Extend offset along chain.
					JointOffsets.Add(LocalJointOffset.GetSafeNormal() * DistanceToAnchor);
				}
			}
			else
			{
				// No chain to worry about, just use the specified offset.
				JointOffsets.Add(LocalJointOffset);
			}

			BoundBoneReferences.Add(LinkBoneRef);

			FTransform BodyTransform = MeshBases.GetComponentSpaceTransform(LinkBoneRef.GetCompactPoseIndex(BoneContainer));
			BodyTransform.SetTranslation(BodyTransform.GetTranslation() + BodyTransform.GetRotation().RotateVector(-LocalJointOffset));

			FAnimPhysLinkedBody NewChainBody(BodyShapes, BodyTransform.GetTranslation(), LinkBoneRef);
			FAnimPhysRigidBody& PhysicsBody = NewChainBody.RigidBody.PhysBody;
			PhysicsBody.Pose.Orientation = BodyTransform.GetRotation();
			PhysicsBody.PreviousOrientation = PhysicsBody.Pose.Orientation;
			PhysicsBody.NextOrientation = PhysicsBody.Pose.Orientation;

			if (bOverrideLinearDamping)
			{
				PhysicsBody.bLinearDampingOverriden = true;
				PhysicsBody.LinearDamping = LinearDampingOverride;
			}

			if (bOverrideAngularDamping)
			{
				PhysicsBody.bAngularDampingOverriden = true;
				PhysicsBody.AngularDamping = AngularDampingOverride;
			}

			PhysicsBody.GravityScale = GravityScale;

			// Link to parent
			if (Bodies.Num() > 0)
			{
				NewChainBody.ParentBody = &Bodies.Last().RigidBody;
			}

			Bodies.Add(NewChainBody);
		}
	}
	else
	{
		// Create body shape
		TArray<FAnimPhysShape> BodyShapes;
		BodyShapes.Add(FAnimPhysShape::MakeBox(BoxExtents));
		
		// Get joint transform
		FCompactPoseBoneIndex BoneIndex = BoundBone.GetCompactPoseIndex(BoneContainer);
		FTransform BoundBoneTransform = MeshBases.GetComponentSpaceTransform(BoneIndex);
		FTransform ShapeTransform = BoundBoneTransform;
		
		FAnimPhysLinkedBody NewChainBody(BodyShapes, ShapeTransform.GetTranslation(), BoundBone);
		FAnimPhysRigidBody& PhysicsBody = NewChainBody.RigidBody.PhysBody;
		PhysicsBody.Pose.Orientation = ShapeTransform.GetRotation();
		PhysicsBody.PreviousOrientation = PhysicsBody.Pose.Orientation;
		PhysicsBody.NextOrientation = PhysicsBody.Pose.Orientation;

		if (bOverrideLinearDamping)
		{
			PhysicsBody.bLinearDampingOverriden = true;
			PhysicsBody.LinearDamping = LinearDampingOverride;
		}

		if (bOverrideAngularDamping)
		{
			PhysicsBody.bAngularDampingOverriden = true;
			PhysicsBody.AngularDamping = AngularDampingOverride;
		}

		PhysicsBody.GravityScale = GravityScale;

		Bodies.Add(NewChainBody);
		BoundBoneReferences.Add(BoundBone);
		JointOffsets.Add(LocalJointOffset);
	}

	// Set up transient constraint data
	const bool bXAxisLocked = ConstraintSetup.LinearXLimitType != AnimPhysLinearConstraintType::Free && ConstraintSetup.LinearAxesMin.X - ConstraintSetup.LinearAxesMax.X == 0.0f;
	const bool bYAxisLocked = ConstraintSetup.LinearYLimitType != AnimPhysLinearConstraintType::Free && ConstraintSetup.LinearAxesMin.Y - ConstraintSetup.LinearAxesMax.Y == 0.0f;
	const bool bZAxisLocked = ConstraintSetup.LinearZLimitType != AnimPhysLinearConstraintType::Free && ConstraintSetup.LinearAxesMin.Z - ConstraintSetup.LinearAxesMax.Z == 0.0f;

	ConstraintSetup.bLinearFullyLocked = bXAxisLocked && bYAxisLocked && bZAxisLocked;

	bRequiresInit = false;
}

void FAnimNode_AnimDynamics::TermPhysics()
{
	Bodies.Empty();
	LinearLimits.Empty();
	AngularLimits.Empty();
	Springs.Empty();

	BoundBoneReferences.Empty();

	JointOffsets.Empty();
}


void FAnimNode_AnimDynamics::UpdateLimits(FCSPose<FCompactPose>& MeshBases)
{
	SCOPE_CYCLE_COUNTER(STAT_AnimDynamicsLimitUpdate);

	// We're always going to use the same number so don't realloc
	LinearLimits.Empty(LinearLimits.Num());
	AngularLimits.Empty(AngularLimits.Num());
	Springs.Empty(Springs.Num());

	for (int32 Idx = 0; Idx < Bodies.Num(); ++Idx)
	{
		FAnimPhysLinkedBody& ChainBody = Bodies[Idx];
		FAnimPhysRigidBody& RigidBody = Bodies[Idx].RigidBody.PhysBody;
		
		FAnimPhysRigidBody* PrevBody = nullptr;
		if (ChainBody.ParentBody)
		{
			PrevBody = &ChainBody.ParentBody->PhysBody;
		}

		// Get joint transform
		const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();
		FCompactPoseBoneIndex BoneIndex = BoundBoneReferences[Idx].GetCompactPoseIndex(BoneContainer);
		FTransform BoundBoneTransform = MeshBases.GetComponentSpaceTransform(BoneIndex);
		FTransform ShapeTransform = BoundBoneTransform;
		
		// Local offset to joint for Body1
		FVector Body1JointOffset = LocalJointOffset;

		if (PrevBody)
		{
			// Modify the shape transform to be correct in Body0 frame
			ShapeTransform = FTransform(FQuat::Identity, -LocalJointOffset);
			// Get the correct offset
			Body1JointOffset = JointOffsets[Idx];
		}
		
		if (ConstraintSetup.bLinearFullyLocked)
		{
			// Rather than calculate prismatic limits, just lock the transform (1 limit instead of 6)
			FAnimPhys::ConstrainPositionNailed(NextTimeStep, LinearLimits, PrevBody, ShapeTransform.GetTranslation(), &RigidBody, Body1JointOffset);
		}
		else
		{
			if (ConstraintSetup.LinearXLimitType != AnimPhysLinearConstraintType::Free)
			{
				FAnimPhys::ConstrainAlongDirection(NextTimeStep, LinearLimits, PrevBody, ShapeTransform.GetTranslation(), &RigidBody, Body1JointOffset, ShapeTransform.GetRotation().GetAxisX(), FVector2D(ConstraintSetup.LinearAxesMin.X, ConstraintSetup.LinearAxesMax.X));
			}

			if (ConstraintSetup.LinearYLimitType != AnimPhysLinearConstraintType::Free)
			{
				FAnimPhys::ConstrainAlongDirection(NextTimeStep, LinearLimits, PrevBody, ShapeTransform.GetTranslation(), &RigidBody, Body1JointOffset, ShapeTransform.GetRotation().GetAxisY(), FVector2D(ConstraintSetup.LinearAxesMin.Y, ConstraintSetup.LinearAxesMax.Y));
			}

			if (ConstraintSetup.LinearZLimitType != AnimPhysLinearConstraintType::Free)
			{
				FAnimPhys::ConstrainAlongDirection(NextTimeStep, LinearLimits, PrevBody, ShapeTransform.GetTranslation(), &RigidBody, Body1JointOffset, ShapeTransform.GetRotation().GetAxisZ(), FVector2D(ConstraintSetup.LinearAxesMin.Z, ConstraintSetup.LinearAxesMax.Z));
			}
		}

		if (ConstraintSetup.AngularConstraintType == AnimPhysAngularConstraintType::Angular)
		{
#if WITH_EDITOR
			// Check the ranges are valid when running in the editor, log if something is wrong
			if(ConstraintSetup.AngularLimitsMin.X > ConstraintSetup.AngularLimitsMax.X ||
			   ConstraintSetup.AngularLimitsMin.Y > ConstraintSetup.AngularLimitsMax.Y ||
			   ConstraintSetup.AngularLimitsMin.Z > ConstraintSetup.AngularLimitsMax.Z)
			{
				UE_LOG(LogAnimation, Warning, TEXT("AnimDynamics: Min/Max angular limits for bone %s incorrect, at least one min axis value is greater than the corresponding max."), *BoundBone.BoneName.ToString());
			}
#endif

			// Add angular limits. any limit with 360+ degree range is ignored and left free.
			FAnimPhys::ConstrainAngularRange(NextTimeStep, AngularLimits, PrevBody, &RigidBody, ShapeTransform.GetRotation(), ConstraintSetup.TwistAxis, ConstraintSetup.AngularLimitsMin, ConstraintSetup.AngularLimitsMax);
		}
		else
		{
			FAnimPhys::ConstrainConeAngle(NextTimeStep, AngularLimits, PrevBody, BoundBoneTransform.GetRotation().GetAxisX(), &RigidBody, FVector(1.0f, 0.0f, 0.0f), ConstraintSetup.ConeAngle);
		}

		// Add spring if we need spring forces
		if (bAngularSpring || bLinearSpring)
		{
			FAnimPhys::CreateSpring(Springs, PrevBody, ShapeTransform.GetTranslation(), &RigidBody, FVector::ZeroVector);
			FAnimPhysSpring& NewSpring = Springs.Last();
			NewSpring.SpringConstantLinear = LinearSpringConstant;
			NewSpring.SpringConstantAngular = AngularSpringConstant;
			NewSpring.AngularTarget = ConstraintSetup.AngularTarget.GetSafeNormal();
			NewSpring.AngularTargetAxis = ConstraintSetup.AngularTargetAxis;
			NewSpring.TargetOrientationOffset = ShapeTransform.GetRotation();
			NewSpring.bApplyAngular = bAngularSpring;
			NewSpring.bApplyLinear = bLinearSpring;
		}
	}
}

