// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "BoneControllers/AnimNode_RigidBody.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "GameFramework/PawnMovementComponent.h"

#include "ImmediatePhysicsSimulation.h"
#include "ImmediatePhysicsActorHandle.h"

#include "Logging/MessageLog.h"

using namespace ImmediatePhysics;

//#pragma optimize("", off)

/////////////////////////////////////////////////////
// FAnimNode_Ragdoll

#define LOCTEXT_NAMESPACE "ImmediatePhysics"

FAnimNode_RigidBody::FAnimNode_RigidBody():
	QueryParams(NAME_None, FCollisionQueryParams::GetUnknownStatId())
{
	bResetSimulated = false;
	PhysicsSimulation = nullptr;
	OverridePhysicsAsset = nullptr;
	bOverrideWorldGravity = false;
	CachedBoundsScale = 1.2f;
	SimulationSpace = ESimulationSpace::ComponentSpace;
	ExternalForce = FVector::ZeroVector;
	bComponentSpaceSimulation_DEPRECATED = true;
	OverrideWorldGravity = FVector::ZeroVector;
	TotalMass = 0.f;
	CachedBounds.W = 0;
	UnsafeWorld = nullptr;
	bSimulationStarted = false;
	bCheckForBodyTransformInit = false;
}

FAnimNode_RigidBody::~FAnimNode_RigidBody()
{
	delete PhysicsSimulation;
}

void FAnimNode_RigidBody::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += ")";

	DebugData.AddDebugItem(DebugLine);

	const bool bUsingFrozenPose = bFreezeIncomingPoseOnStart && bSimulationStarted && (CapturedFrozenPose.GetPose().GetNumBones() > 0);
	if (!bUsingFrozenPose)
	{
		ComponentPose.GatherDebugData(DebugData);
	}
}

FVector WorldVectorToSpaceNoScale(ESimulationSpace Space, const FVector& WorldDir, const FTransform& ComponentToWorld, const FTransform& RootBoneTM)
{
	switch(Space)
	{
		case ESimulationSpace::ComponentSpace: return ComponentToWorld.InverseTransformVectorNoScale(WorldDir);
		case ESimulationSpace::WorldSpace: return WorldDir;
		case ESimulationSpace::RootBoneSpace: return RootBoneTM.InverseTransformVectorNoScale(ComponentToWorld.InverseTransformVectorNoScale(WorldDir));
		default: return FVector::ZeroVector;
	}
}

FVector WorldPositionToSpace(ESimulationSpace Space, const FVector& WorldPoint, const FTransform& ComponentToWorld, const FTransform& RootBoneTM)
{
	switch (Space)
	{
		case ESimulationSpace::ComponentSpace: return ComponentToWorld.InverseTransformPosition(WorldPoint);
		case ESimulationSpace::WorldSpace: return WorldPoint;
		case ESimulationSpace::RootBoneSpace: return RootBoneTM.InverseTransformPosition(ComponentToWorld.InverseTransformPosition(WorldPoint));
		default: return FVector::ZeroVector;
	}
}

FORCEINLINE_DEBUGGABLE FTransform ConvertCSTransformToSimSpace(ESimulationSpace SimulationSpace, const FTransform& InCSTransform, const FTransform& ComponentToWorld, const FTransform& RootBoneTM)
{
	switch (SimulationSpace)
	{
		case ESimulationSpace::ComponentSpace: return InCSTransform;
		case ESimulationSpace::WorldSpace:  return InCSTransform * ComponentToWorld; 
		case ESimulationSpace::RootBoneSpace: return InCSTransform.GetRelativeTransform(RootBoneTM); break;
		default: ensureMsgf(false, TEXT("Unsupported Simulation Space")); return InCSTransform;
	}
}

void FAnimNode_RigidBody::UpdateComponentPose_AnyThread(const FAnimationUpdateContext& Context)
{
	// Only freeze update graph after initial update, as we want to get that pose through.
	if (bFreezeIncomingPoseOnStart && bSimulationStarted && !bResetSimulated)
	{
		// If we have a Frozen Pose captured, 
		// then we don't need to update the rest of the graph.
		if (CapturedFrozenPose.GetPose().GetNumBones() > 0)
		{
		}
		else
		{
			// Create a new context with zero deltatime to freeze time in rest of the graph.
			// This will be used to capture a frozen pose.
			FAnimationUpdateContext FrozenContext = Context;
			FrozenContext.FractionalWeightAndTime(1.f, 0.f);

			Super::UpdateComponentPose_AnyThread(FrozenContext);
		}
	}
	else
	{
		Super::UpdateComponentPose_AnyThread(Context);
	}
}

void FAnimNode_RigidBody::EvaluateComponentPose_AnyThread(FComponentSpacePoseContext& Output)
{
	if (bFreezeIncomingPoseOnStart && bSimulationStarted)
	{
		// If we have a Frozen Pose captured, use it.
		// Only after our intialize setup. As we need new pose for that.
		if (!bResetSimulated && (CapturedFrozenPose.GetPose().GetNumBones() > 0))
		{
			Output.Pose.CopyPose(CapturedFrozenPose);
			Output.Curve.CopyFrom(CapturedFrozenCurves);
		}
		// Otherwise eval graph to capture it.
		else
		{
			Super::EvaluateComponentPose_AnyThread(Output);
			CapturedFrozenPose.CopyPose(Output.Pose);
			CapturedFrozenCurves.CopyFrom(Output.Curve);
		}
	}
	else
	{
		Super::EvaluateComponentPose_AnyThread(Output);
	}

	// Capture incoming pose if 'bTransferBoneVelocities' is set.
	// That is, until simulation starts.
	if (bTransferBoneVelocities && !bSimulationStarted)
	{
		CapturedBoneVelocityPose.CopyPose(Output.Pose);
		CapturedBoneVelocityPose.CopyAndAssignBoneContainer(CapturedBoneVelocityBoneContainer);
	}
}

void FAnimNode_RigidBody::InitializeNewBodyTransformsDuringSimulation(FComponentSpacePoseContext& Output, const FTransform& ComponentTransform, const FTransform& RootBoneTM)
{
	for (const FOutputBoneData& OutputData : OutputBoneData)
	{
		const int32 BodyIndex = OutputData.BodyIndex;
		FBodyAnimData& BodyData = BodyAnimData[BodyIndex];
		if (!BodyData.bBodyTransformInitialized)
		{
			BodyData.bBodyTransformInitialized = true;

			// If we have a parent body, we need to grab relative transforms to it.
			if (OutputData.ParentBodyIndex != INDEX_NONE)
			{
				ensure(BodyAnimData[OutputData.ParentBodyIndex].bBodyTransformInitialized);

				FTransform BodyRelativeTransform = FTransform::Identity;
				for (const FCompactPoseBoneIndex CompactBoneIndex : OutputData.BoneIndicesToParentBody)
				{
					const FTransform& LocalSpaceTM = Output.Pose.GetLocalSpaceTransform(CompactBoneIndex);
					BodyRelativeTransform = BodyRelativeTransform * LocalSpaceTM;
				}

				const FTransform WSBodyTM = BodyRelativeTransform * Bodies[OutputData.ParentBodyIndex]->GetWorldTransform();
				Bodies[BodyIndex]->SetWorldTransform(WSBodyTM);
			}
			// If we don't have a parent body, then we can just grab the incoming pose in component space.
			else
			{
				const FTransform& ComponentSpaceTM = Output.Pose.GetComponentSpaceTransform(OutputData.CompactPoseBoneIndex);
				const FTransform BodyTM = ConvertCSTransformToSimSpace(SimulationSpace, ComponentSpaceTM, ComponentTransform, RootBoneTM);

				Bodies[BodyIndex]->SetWorldTransform(BodyTM);
			}
		}
	}
}

DECLARE_CYCLE_STAT(TEXT("RigidBody_Eval"), STAT_RigidBody_Eval, STATGROUP_Anim);
DECLARE_CYCLE_STAT(TEXT("FAnimNode_Ragdoll::EvaluateSkeletalControl_AnyThread"), STAT_ImmediateEvaluateSkeletalControl, STATGROUP_ImmediatePhysics);

void FAnimNode_RigidBody::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_RigidBody_Eval);
	SCOPE_CYCLE_COUNTER(STAT_ImmediateEvaluateSkeletalControl);
	//FPlatformMisc::BeginNamedEvent(FColor::Magenta, "FAnimNode_Ragdoll::EvaluateSkeletalControl_AnyThread");

	const float DeltaSeconds = AccumulatedDeltaTime;
	AccumulatedDeltaTime = 0.f;

	if (PhysicsSimulation)
	{
		const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
		const FTransform CompWorldSpaceTM = Output.AnimInstanceProxy->GetComponentTransform();
		const FTransform RootBoneTM = Output.Pose.GetComponentSpaceTransform(RootBoneRef.GetCompactPoseIndex(BoneContainer));

		// Initialize potential new bodies because of LOD change.
		if (!bResetSimulated && bCheckForBodyTransformInit)
		{
			bCheckForBodyTransformInit = false;
			InitializeNewBodyTransformsDuringSimulation(Output, CompWorldSpaceTM, RootBoneTM);
		}

		// If time advances, update simulation
		if (DeltaSeconds > 0)
		{
			// First update, initialize bodies from animation.
			if (bResetSimulated)
			{
				bResetSimulated = false;

				// Capture bone velocities if we have captured a bone velocity pose.
				if (bTransferBoneVelocities && (CapturedBoneVelocityPose.GetPose().GetNumBones() > 0))
				{
					for (const FOutputBoneData& OutputData : OutputBoneData)
					{
						const int32 BodyIndex = OutputData.BodyIndex;
						FBodyAnimData& BodyData = BodyAnimData[BodyIndex];

						if (BodyData.bIsSimulated)
						{
							const FCompactPoseBoneIndex NextCompactPoseBoneIndex = OutputData.CompactPoseBoneIndex;
							// Convert CompactPoseBoneIndex to SkeletonBoneIndex...
							const int32 PoseSkeletonBoneIndex = BoneContainer.GetPoseToSkeletonBoneIndexArray()[NextCompactPoseBoneIndex.GetInt()];
							// ... So we can convert to the captured pose CompactPoseBoneIndex. 
							// In case there was a LOD change, and poses are not compatible anymore.
							const FCompactPoseBoneIndex PrevCompactPoseBoneIndex = CapturedBoneVelocityBoneContainer.GetCompactPoseIndexFromSkeletonIndex(PoseSkeletonBoneIndex);

							if (PrevCompactPoseBoneIndex != FCompactPoseBoneIndex(INDEX_NONE))
							{
								const FTransform PrevCSTM = CapturedBoneVelocityPose.GetComponentSpaceTransform(PrevCompactPoseBoneIndex);
								const FTransform NextCSTM = Output.Pose.GetComponentSpaceTransform(NextCompactPoseBoneIndex);

								const FTransform PrevSSTM = ConvertCSTransformToSimSpace(SimulationSpace, PrevCSTM, CompWorldSpaceTM, RootBoneTM);
								const FTransform NextSSTM = ConvertCSTransformToSimSpace(SimulationSpace, NextCSTM, CompWorldSpaceTM, RootBoneTM);

								// Linear Velocity
								BodyData.TransferedBoneVelocity.SetTranslation((NextSSTM.GetLocation() - PrevSSTM.GetLocation()) / DeltaSeconds);

								// Angular Velocity
								const FQuat DeltaRotation = (NextSSTM.GetRotation().Inverse() * PrevSSTM.GetRotation());
								const float RotationAngle = DeltaRotation.GetAngle() / DeltaSeconds;
								BodyData.TransferedBoneVelocity.SetRotation(FQuat(DeltaRotation.GetRotationAxis(), RotationAngle)); 
							}
						}
					}
				}

				// Initialize bodies.
				{
					for (const FOutputBoneData& OutputData : OutputBoneData)
					{
						const int32 BodyIndex = OutputData.BodyIndex;
						BodyAnimData[BodyIndex].bBodyTransformInitialized = true;

						const FTransform& ComponentSpaceTM = Output.Pose.GetComponentSpaceTransform(OutputData.CompactPoseBoneIndex);
						const FTransform BodyTM = ConvertCSTransformToSimSpace(SimulationSpace, ComponentSpaceTM, CompWorldSpaceTM, RootBoneTM);
						Bodies[BodyIndex]->SetWorldTransform(BodyTM);
					}
				}
			}
			// Subsequent updates, update kinematic bodies and run simulation.
			else
			{
				// Transfer bone velocities previously captured.
				if (bTransferBoneVelocities && (CapturedBoneVelocityPose.GetPose().GetNumBones() > 0))
				{
					for (const FOutputBoneData& OutputData : OutputBoneData)
					{
						const int32 BodyIndex = OutputData.BodyIndex;
						const FBodyAnimData& BodyData = BodyAnimData[BodyIndex];

						if (BodyData.bIsSimulated)
						{
							ImmediatePhysics::FActorHandle* Body = Bodies[BodyIndex];
							Body->SetLinearVelocity(BodyData.TransferedBoneVelocity.GetTranslation());

							const FQuat AngularVelocity = BodyData.TransferedBoneVelocity.GetRotation();
							Body->SetAngularVelocity(AngularVelocity.GetRotationAxis() * AngularVelocity.GetAngle());
						}
					}

					// Free up our captured pose after it's been used.
					CapturedBoneVelocityPose.Empty();
				}

				for (const FOutputBoneData& OutputData : OutputBoneData)
				{
					const int32 BodyIndex = OutputData.BodyIndex;
					if (!BodyAnimData[BodyIndex].bIsSimulated)
					{
						const FTransform& ComponentSpaceTM = Output.Pose.GetComponentSpaceTransform(OutputData.CompactPoseBoneIndex);
						const FTransform BodyTM = ConvertCSTransformToSimSpace(SimulationSpace, ComponentSpaceTM, CompWorldSpaceTM, RootBoneTM);

						Bodies[BodyIndex]->SetKinematicTarget(BodyTM);
					}
				}

				UpdateWorldForces(CompWorldSpaceTM, RootBoneTM);
				const FVector SimSpaceGravity = WorldVectorToSpaceNoScale(SimulationSpace, WorldSpaceGravity, CompWorldSpaceTM, RootBoneTM);

				// Run simulation at a minimum of 30 FPS to prevent system from exploding.
				// DeltaTime can be higher due to URO, so take multiple iterations in that case.
				const float MaxDeltaSeconds = 1.f / 30.f;
				const int32 NumIterations = FMath::Clamp(FMath::CeilToInt(DeltaSeconds / MaxDeltaSeconds), 1, 4);
				const float StepDeltaTime = DeltaSeconds / float(NumIterations);

				for (int32 Step = 1; Step <= NumIterations; Step++)
				{
					PhysicsSimulation->Simulate(StepDeltaTime, SimSpaceGravity);
				}
			}
		}
		
		//write back to animation system
		for (const FOutputBoneData& OutputData : OutputBoneData)
		{
			const int32 BodyIndex = OutputData.BodyIndex;
			if (BodyAnimData[BodyIndex].bIsSimulated)
			{
				const FTransform BodyTM = Bodies[BodyIndex]->GetWorldTransform();
				FTransform ComponentSpaceTM;

				switch(SimulationSpace)
				{
					case ESimulationSpace::ComponentSpace: ComponentSpaceTM = BodyTM; break;
					case ESimulationSpace::WorldSpace: ComponentSpaceTM = BodyTM.GetRelativeTransform(CompWorldSpaceTM); break;
					case ESimulationSpace::RootBoneSpace: ComponentSpaceTM = BodyTM * RootBoneTM; break;
					default: ensureMsgf(false, TEXT("Unsupported Simulation Space")); ComponentSpaceTM = BodyTM;
				}
					
				OutBoneTransforms.Add(FBoneTransform(OutputData.CompactPoseBoneIndex, ComponentSpaceTM));
			}
		}
	}
	//FPlatformMisc::EndNamedEvent();
}

void ComputeBodyInsertionOrder(TArray<FBoneIndexType>& InsertionOrder, const USkeletalMeshComponent& SKC)
{
	//We want to ensure simulated bodies are sorted by LOD so that the first simulated bodies are at the highest LOD.
	//Since LOD2 is a subset of LOD1 which is a subset of LOD0 we can change the number of simulated bodies without any reordering
	//For this to work we must first insert all simulated bodies in the right order. We then insert all the kinematic bodies in the right order

	InsertionOrder.Reset();

	const int32 NumLODs = SKC.GetNumLODs();
	if(NumLODs > 0)
	{
		TArray<bool> InSortedOrder;

		TArray<FBoneIndexType> RequiredBones0;
		TArray<FBoneIndexType> ComponentSpaceTMs0;
		SKC.ComputeRequiredBones(RequiredBones0, ComponentSpaceTMs0, 0, /*bIgnorePhysicsAsset=*/ true);

		InSortedOrder.AddZeroed(RequiredBones0.Num());

		auto MergeIndices = [&InsertionOrder, &InSortedOrder](const TArray<FBoneIndexType>& RequiredBones) -> void
		{
			for (FBoneIndexType BoneIdx : RequiredBones)
			{
				if (!InSortedOrder[BoneIdx])
				{
					InsertionOrder.Add(BoneIdx);
				}

				InSortedOrder[BoneIdx] = true;
			}
		};


		for(int32 LodIdx = NumLODs - 1; LodIdx > 0; --LodIdx)
		{
			TArray<FBoneIndexType> RequiredBones;
			TArray<FBoneIndexType> ComponentSpaceTMs;
			SKC.ComputeRequiredBones(RequiredBones, ComponentSpaceTMs, LodIdx, /*bIgnorePhysicsAsset=*/ true);
			MergeIndices(RequiredBones);
		}

		MergeIndices(RequiredBones0);
	}
}

void FAnimNode_RigidBody::InitPhysics(const UAnimInstance* InAnimInstance)
{
	const USkeletalMeshComponent* SkeletalMeshComp = InAnimInstance->GetSkelMeshComponent();
	const USkeletalMesh* SkeletalMeshAsset = SkeletalMeshComp->SkeletalMesh;

	const FReferenceSkeleton& SkelMeshRefSkel = SkeletalMeshAsset->RefSkeleton;
	UPhysicsAsset* UsePhysicsAsset = OverridePhysicsAsset ? OverridePhysicsAsset : InAnimInstance->GetSkelMeshComponent()->GetPhysicsAsset();
		
	USkeleton* SkeletonAsset = InAnimInstance->CurrentSkeleton;
	ensure(SkeletonAsset == SkeletalMeshAsset->Skeleton);

	const int32 SkelMeshLinkupIndex = SkeletonAsset->GetMeshLinkupIndex(SkeletalMeshAsset);
	ensure(SkelMeshLinkupIndex != INDEX_NONE);
	const FSkeletonToMeshLinkup& SkeletonToMeshLinkupTable = SkeletonAsset->LinkupCache[SkelMeshLinkupIndex];
	const TArray<int32>& MeshToSkeletonBoneIndex = SkeletonToMeshLinkupTable.MeshToSkeletonTable;
	
	const int32 NumSkeletonBones = SkeletonAsset->GetReferenceSkeleton().GetNum();
	SkeletonBoneIndexToBodyIndex.Reset(NumSkeletonBones);
	SkeletonBoneIndexToBodyIndex.Init(INDEX_NONE, NumSkeletonBones);

	if(UsePhysicsAsset)
	{
		delete PhysicsSimulation;
		PhysicsSimulation = new FSimulation();
		const int32 NumBodies = UsePhysicsAsset->SkeletalBodySetups.Num();
		Bodies.Empty(NumBodies);
		ComponentsInSim.Reset();
		BodyAnimData.Reset(NumBodies);
		BodyAnimData.AddDefaulted(NumBodies);
		TotalMass = 0.f;
		
		TArray<FBodyInstance*> HighLevelBodyInstances;
		TArray<FConstraintInstance*> HighLevelConstraintInstances;
		SkeletalMeshComp->InstantiatePhysicsAsset(*UsePhysicsAsset, SimulationSpace == ESimulationSpace::WorldSpace ? SkeletalMeshComp->GetComponentToWorld().GetScale3D() : FVector(1.f), HighLevelBodyInstances, HighLevelConstraintInstances);

		TMap<FName, FActorHandle*> NamesToHandles;
		TArray<FActorHandle*> IgnoreCollisionActors;

		TArray<FBoneIndexType> InsertionOrder;
		ComputeBodyInsertionOrder(InsertionOrder, *SkeletalMeshComp);

		const int32 NumBonesLOD0 = InsertionOrder.Num();

		TArray<FActorHandle*> BodyIndexToActorHandle;
		BodyIndexToActorHandle.AddZeroed(NumBonesLOD0);

		TArray<FBodyInstance*> BodiesSorted;
		BodiesSorted.AddZeroed(NumBonesLOD0);

		for (FBodyInstance* BI : HighLevelBodyInstances)
		{
			if(BI->IsValidBodyInstance())
			{
				BodiesSorted[BI->InstanceBoneIndex] = BI;
			}
		}

		auto InsertBodiesHelper = [&](bool bSimulatedBodies)
		{
			for (FBoneIndexType InsertBone : InsertionOrder)
			{
				if (FBodyInstance* BodyInstance = BodiesSorted[InsertBone])
				{
					UBodySetup* BodySetup = UsePhysicsAsset->SkeletalBodySetups[BodyInstance->InstanceBodyIndex];
					const bool bKinematic = BodySetup->PhysicsType != EPhysicsType::PhysType_Simulated;
					const FTransform& LastTransform = SkeletalMeshComp->GetBoneTransform(InsertBone);	//This is out of date, but will still give our bodies an initial setup that matches the constraints (TODO: use refpose)

					FActorHandle* NewBodyHandle = nullptr;
					if (bSimulatedBodies && !bKinematic)
					{
#if WITH_PHYSX
						NewBodyHandle = PhysicsSimulation->CreateDynamicActor(BodyInstance->GetPxRigidDynamic_AssumesLocked(), LastTransform);
						checkSlow(NewBodyHandle);
						const float InvMass = NewBodyHandle->GetInverseMass();
						TotalMass += InvMass > 0.f ? 1.f / InvMass : 0.f;
#endif
					}
					else if (bSimulatedBodies && bKinematic)
					{
#if WITH_PHYSX
						NewBodyHandle = PhysicsSimulation->CreateKinematicActor(BodyInstance->GetPxRigidBody_AssumesLocked(), LastTransform);
#endif
					}

					if (NewBodyHandle)
					{
						const int32 BodyIndex = Bodies.Add(NewBodyHandle);

						const int32 SkeletonBoneIndex = MeshToSkeletonBoneIndex[InsertBone];
						SkeletonBoneIndexToBodyIndex[SkeletonBoneIndex] = BodyIndex;
						BodyAnimData[BodyIndex].bIsSimulated = !bKinematic;
						NamesToHandles.Add(BodySetup->BoneName, NewBodyHandle);
						BodyIndexToActorHandle[BodyInstance->InstanceBodyIndex] = NewBodyHandle;

						if (BodySetup->CollisionReponse == EBodyCollisionResponse::BodyCollision_Disabled)
						{
							IgnoreCollisionActors.Add(NewBodyHandle);
						}

						BodyInstance->TermBody();
						delete BodyInstance;
						BodiesSorted[InsertBone] = nullptr;
					}
				}
			}
		};

		//Insert simulated bodies first to avoid any re-ordering
		InsertBodiesHelper(/*bSimulated=*/true);
		InsertBodiesHelper(/*bSimulated=*/false);
		HighLevelBodyInstances.Empty();
		BodiesSorted.Empty();

		//Insert joints so that they coincide body order. That is, if we stop simulating all bodies past some index, we can simply ignore joints past a corresponding index without any re-order
		//For this to work we consider the most last inserted bone in each joint
		TArray<int32> InsertionOrderPerBone;
		InsertionOrderPerBone.AddUninitialized(NumBonesLOD0);

		for(int32 Position = 0; Position < NumBonesLOD0; ++Position)
		{
			InsertionOrderPerBone[InsertionOrder[Position]] = Position;
		}

		HighLevelConstraintInstances.Sort([&InsertionOrderPerBone, &SkelMeshRefSkel](const FConstraintInstance& LHS, const FConstraintInstance& RHS)
		{
			if(LHS.IsValidConstraintInstance() && RHS.IsValidConstraintInstance())
			{
				const int32 BoneIdxLHS1 = SkelMeshRefSkel.FindBoneIndex(LHS.ConstraintBone1);
				const int32 BoneIdxLHS2 = SkelMeshRefSkel.FindBoneIndex(LHS.ConstraintBone2);

				const int32 BoneIdxRHS1 = SkelMeshRefSkel.FindBoneIndex(RHS.ConstraintBone1);
				const int32 BoneIdxRHS2 = SkelMeshRefSkel.FindBoneIndex(RHS.ConstraintBone2);

				const int32 MaxPositionLHS = FMath::Max(InsertionOrderPerBone[BoneIdxLHS1], InsertionOrderPerBone[BoneIdxLHS2]);
				const int32 MaxPositionRHS = FMath::Max(InsertionOrderPerBone[BoneIdxRHS1], InsertionOrderPerBone[BoneIdxRHS2]);

				return MaxPositionLHS < MaxPositionRHS;
			}
			
			return false;
		});

#if WITH_PHYSX
		if(NamesToHandles.Num() > 0)
		{
			//constraints
			for(int32 ConstraintIdx = 0; ConstraintIdx < HighLevelConstraintInstances.Num(); ++ConstraintIdx)
			{
				FConstraintInstance* CI = HighLevelConstraintInstances[ConstraintIdx];
				FActorHandle* Body1Handle = NamesToHandles.FindRef(CI->ConstraintBone1);
				FActorHandle* Body2Handle = NamesToHandles.FindRef(CI->ConstraintBone2);

				if(Body1Handle && Body2Handle)
				{
					if (Body1Handle->IsSimulated() || Body2Handle->IsSimulated())
					{
						PhysicsSimulation->CreateJoint(CI->ConstraintData, Body1Handle, Body2Handle);
					}
				}

				CI->TermConstraint();
				delete CI;
			}

			bResetSimulated = true;
		}

		TArray<FSimulation::FIgnorePair> IgnorePairs;
		const TMap<FRigidBodyIndexPair, bool>& DisableTable = UsePhysicsAsset->CollisionDisableTable;
		for(auto ConstItr = DisableTable.CreateConstIterator(); ConstItr; ++ConstItr)
		{
			FSimulation::FIgnorePair Pair;
			Pair.A = BodyIndexToActorHandle[ConstItr.Key().Indices[0]];
			Pair.B = BodyIndexToActorHandle[ConstItr.Key().Indices[1]];
			IgnorePairs.Add(Pair);
		}

		PhysicsSimulation->SetIgnoreCollisionPairTable(IgnorePairs);
		PhysicsSimulation->SetIgnoreCollisionActors(IgnoreCollisionActors);
#endif
	}
}

DECLARE_CYCLE_STAT(TEXT("FAnimNode_Ragdoll::UpdateWorldGeometry"), STAT_ImmediateUpdateWorldGeometry, STATGROUP_ImmediatePhysics);

void FAnimNode_RigidBody::UpdateWorldGeometry(const UWorld& World, const USkeletalMeshComponent& SKC)
{
	SCOPE_CYCLE_COUNTER(STAT_ImmediateUpdateWorldGeometry);
	QueryParams = FCollisionQueryParams(SCENE_QUERY_STAT(RagdollNodeFindGeometry), /*bTraceComplex=*/false);
#if WITH_EDITOR
	if(!World.IsGameWorld())
	{
		QueryParams.MobilityType = EQueryMobilityType::Any;	//If we're in some preview world trace against everything because things like the preview floor are not static
		QueryParams.AddIgnoredComponent(&SKC);
	}
	else
#endif
	{
		QueryParams.MobilityType = EQueryMobilityType::Static;	//We only want static actors
	}

	Bounds = SKC.CalcBounds(SKC.GetComponentToWorld()).GetSphere();

	if (!Bounds.IsInside(CachedBounds))
	{
		// Since the cached bounds are no longer valid, update them.
		
		CachedBounds = Bounds;
		CachedBounds.W *= CachedBoundsScale;

		// Cache the PhysScene and World for use in UpdateWorldForces.
		PhysScene = World.GetPhysicsScene();
		UnsafeWorld = &World;
	}
}

DECLARE_CYCLE_STAT(TEXT("FAnimNode_Ragdoll::UpdateWorldForces"), STAT_ImmediateUpdateWorldForces, STATGROUP_ImmediatePhysics);

void FAnimNode_RigidBody::UpdateWorldForces(const FTransform& ComponentToWorld, const FTransform& RootBoneTM)
{
	SCOPE_CYCLE_COUNTER(STAT_ImmediateUpdateWorldForces);

	if(TotalMass > 0.f)
	{
		for (const USkeletalMeshComponent::FPendingRadialForces& PendingRadialForce : PendingRadialForces)
		{
			const FVector RadialForceOrigin = WorldPositionToSpace(SimulationSpace, PendingRadialForce.Origin, ComponentToWorld, RootBoneTM);
			for(FActorHandle* Body : Bodies)
			{
				const float InvMass = Body->GetInverseMass();
				if(InvMass > 0.f)
				{
					const float StrengthPerBody = PendingRadialForce.bIgnoreMass ? PendingRadialForce.Strength : PendingRadialForce.Strength / (TotalMass * InvMass);
					FSimulation::EForceType ForceType;
					if (PendingRadialForce.Type == USkeletalMeshComponent::FPendingRadialForces::AddImpulse)
					{
						ForceType = PendingRadialForce.bIgnoreMass ? FSimulation::EForceType::AddVelocity : FSimulation::EForceType::AddImpulse;
					}
					else
					{
						ForceType = PendingRadialForce.bIgnoreMass ? FSimulation::EForceType::AddAcceleration : FSimulation::EForceType::AddForce;
					}
					
					Body->AddRadialForce(RadialForceOrigin, StrengthPerBody, PendingRadialForce.Radius, PendingRadialForce.Falloff, ForceType);
				}
			}
		}

		if(!ExternalForce.IsNearlyZero())
		{
			const FVector ExternalForceInSimSpace = WorldVectorToSpaceNoScale(SimulationSpace, ExternalForce, ComponentToWorld, RootBoneTM);
			for (FActorHandle* Body : Bodies)
			{
				const float InvMass = Body->GetInverseMass();
				if (InvMass > 0.f)
				{
					Body->AddForce(ExternalForceInSimSpace);
				}
			}
		}
	}
}

DECLARE_CYCLE_STAT(TEXT("RigidBody_PreUpdate"), STAT_RigidBody_PreUpdate, STATGROUP_Anim);

void FAnimNode_RigidBody::PreUpdate(const UAnimInstance* InAnimInstance)
{
	SCOPE_CYCLE_COUNTER(STAT_RigidBody_PreUpdate);

	UWorld* World = InAnimInstance->GetWorld();
	USkeletalMeshComponent* SKC = InAnimInstance->GetSkelMeshComponent();
	APawn* PawnOwner = InAnimInstance->TryGetPawnOwner();
	UPawnMovementComponent* MovementComp = PawnOwner ? PawnOwner->GetMovementComponent() : nullptr;

#if WITH_EDITOR
	if (bEnableWorldGeometry && SimulationSpace != ESimulationSpace::WorldSpace)
	{
		FMessageLog("PIE").Warning(FText::Format(LOCTEXT("WorldCollisionComponentSpace", "Trying to use world collision without world space simulation for ''{0}''. This is not supported, please change SimulationSpace to WorldSpace"),
			FText::FromString(GetPathNameSafe(SKC))));
	}
#endif

	WorldSpaceGravity = bOverrideWorldGravity ? OverrideWorldGravity : (MovementComp ? FVector(0.f, 0.f, MovementComp->GetGravityZ()) : FVector(0.f, 0.f, World->GetGravityZ()));
	if(SKC)
	{
		if (PhysicsSimulation && bEnableWorldGeometry && SimulationSpace == ESimulationSpace::WorldSpace)
		{
			UpdateWorldGeometry(*World, *SKC);
		}

		PendingRadialForces = SKC->GetPendingRadialForces();
	}	
}

DECLARE_CYCLE_STAT(TEXT("RigidBody_Update"), STAT_RigidBody_Update, STATGROUP_Anim);

void FAnimNode_RigidBody::UpdateInternal(const FAnimationUpdateContext& Context)
{
	SCOPE_CYCLE_COUNTER(STAT_RigidBody_PreUpdate);

	// Accumulate deltatime elapsed during update. To be used during evaluation.
	AccumulatedDeltaTime += Context.AnimInstanceProxy->GetDeltaSeconds();

	if (UnsafeWorld != nullptr)
	{		

#if WITH_PHYSX
		// Node is valid to evaluate. Simulation is starting.
		bSimulationStarted = true;

		TArray<FOverlapResult> Overlaps;
		UnsafeWorld->OverlapMultiByChannel(Overlaps, Bounds.Center, FQuat::Identity, OverlapChannel, FCollisionShape::MakeSphere(Bounds.W), QueryParams, FCollisionResponseParams(ECR_Overlap));
		SCOPED_SCENE_READ_LOCK(PhysScene ? PhysScene->GetPhysXScene(PST_Sync) : nullptr); //TODO: expose this part to the anim node
	
		for (const FOverlapResult& Overlap : Overlaps)
		{
			if (UPrimitiveComponent* OverlapComp = Overlap.GetComponent())
			{
				if (ComponentsInSim.Contains(OverlapComp) == false)
				{
					ComponentsInSim.Add(OverlapComp);
					if (PxRigidActor* RigidActor = OverlapComp->BodyInstance.GetPxRigidActor_AssumesLocked())
					{
						PhysicsSimulation->CreateStaticActor(RigidActor, P2UTransform(RigidActor->getGlobalPose()));
					}
				}
			}
		}
#endif

		UnsafeWorld = nullptr;
		PhysScene = nullptr;
	}
}

void FAnimNode_RigidBody::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	/** We only need to update simulated bones and children of simulated bones*/
	const int32 NumBodies = Bodies.Num();
	const TArray<FBoneIndexType>& RequiredBoneIndices = RequiredBones.GetBoneIndicesArray();
	const int32 NumRequiredBoneIndices = RequiredBoneIndices.Num();
	const FReferenceSkeleton& RefSkeleton = RequiredBones.GetReferenceSkeleton();

	OutputBoneData.Empty(NumBodies);

	int32 NumSimulatedBodies = 0;

	//we always cache root bone
	RootBoneRef.BoneName = RefSkeleton.GetBoneName(0);
	RootBoneRef.Initialize(RequiredBones);

	for(int32 Index = 0; Index < NumRequiredBoneIndices; ++Index)
	{
		const FCompactPoseBoneIndex CompactPoseBoneIndex(Index);
		const FBoneIndexType SkeletonBoneIndex = RequiredBones.GetSkeletonIndex(CompactPoseBoneIndex);
		const int32 BodyIndex = SkeletonBoneIndexToBodyIndex[SkeletonBoneIndex];

		if (BodyIndex != INDEX_NONE)
		{
			//If we have a body we need to save it for later
			FOutputBoneData* OutputData = new (OutputBoneData) FOutputBoneData();
			OutputData->BodyIndex = BodyIndex;
			OutputData->CompactPoseBoneIndex = CompactPoseBoneIndex;

			if (BodyAnimData[BodyIndex].bIsSimulated)
			{
				++NumSimulatedBodies;
			}

			OutputData->BoneIndicesToParentBody.Add(CompactPoseBoneIndex);

			// Walk up parent chain until we find parent body.
			OutputData->ParentBodyIndex = INDEX_NONE;
			FCompactPoseBoneIndex CompactParentIndex = RequiredBones.GetParentBoneIndex(CompactPoseBoneIndex);
			while (CompactParentIndex != INDEX_NONE)
			{
				const FBoneIndexType SkeletonParentBoneIndex = RequiredBones.GetSkeletonIndex(CompactParentIndex);
				OutputData->ParentBodyIndex = SkeletonBoneIndexToBodyIndex[SkeletonParentBoneIndex];
				if (OutputData->ParentBodyIndex != INDEX_NONE)
				{
					break;
				}

				OutputData->BoneIndicesToParentBody.Add(CompactParentIndex);
				CompactParentIndex = RequiredBones.GetParentBoneIndex(CompactParentIndex);
			}
		}
	}

	// New bodies protentially introduced with new LOD
	// We'll have to initialize their transform.
	bCheckForBodyTransformInit = true;

	if(PhysicsSimulation)
	{
		PhysicsSimulation->SetNumActiveBodies(NumSimulatedBodies);
	}

	// We're switching to a new LOD, this invalidates our captured poses.
	CapturedFrozenPose.Empty();
	CapturedFrozenCurves.Empty();
}

void FAnimNode_RigidBody::OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance)
{
	InitPhysics(InAnimInstance);
}

void FAnimNode_RigidBody::PostSerialize(const FArchive& Ar)
{
	if(bComponentSpaceSimulation_DEPRECATED == false)
	{
		//If this is not the default value it means we have old content where we were simulating in world space
		SimulationSpace = ESimulationSpace::WorldSpace;
		bComponentSpaceSimulation_DEPRECATED = true;
	}
}

#undef LOCTEXT_NAMESPACE