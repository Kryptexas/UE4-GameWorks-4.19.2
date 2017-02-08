// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "BoneControllers/AnimNode_Ragdoll.h"
#include "AnimationRuntime.h"
#include "Animation/AnimInstanceProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

#include "ImmediatePhysicsSimulation.h"
#include "ImmediatePhysicsActorHandle.h"

#include "Logging/MessageLog.h"

using namespace ImmediatePhysics;

//#pragma optimize("", off)

/////////////////////////////////////////////////////
// FAnimNode_Ragdoll

#define LOCTEXT_NAMESPACE "ImmediatePhysics"

FAnimNode_Ragdoll::FAnimNode_Ragdoll()
{
	bNeedsInitialization = false;
	bResetSimulated = false;
	PhysicsSimulation = nullptr;
	OverridePhysicsAsset = nullptr;
	bOverrideWorldGravity = false;
	bComponentSpaceSimulation = true;
	OverrideWorldGravity = FVector::ZeroVector;
	TotalMass = 0.f;
}

FAnimNode_Ragdoll::~FAnimNode_Ragdoll()
{
	ClearHighLevelData();
	delete PhysicsSimulation;
}

void FAnimNode_Ragdoll::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);

	DebugLine += "(";
	AddDebugNodeData(DebugLine);
	DebugLine += ")";

	DebugData.AddDebugItem(DebugLine);

	ComponentPose.GatherDebugData(DebugData);
}

DECLARE_CYCLE_STAT(TEXT("FAnimNode_Ragdoll::EvaluateBoneTransforms"), STAT_ImmediateEvaluateBoneTransforms, STATGROUP_ImmediatePhysics);

void FAnimNode_Ragdoll::EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms)
{
	SCOPE_CYCLE_COUNTER(STAT_ImmediateEvaluateBoneTransforms);
	//FPlatformMisc::BeginNamedEvent(FColor::Magenta, "FAnimNode_Ragdoll::EvaluateBoneTransforms");

	if(PhysicsSimulation)
	{
		const FTransform CompWorldSpaceTM = SkelComp->GetComponentTransform();	//This is not thread safe!

		//Update physics with transforms
		const FBoneContainer& BoneContainer = MeshBases.GetPose().GetBoneContainer();
		for (const FOutputBoneData& OutputData : OutputBoneData)
		{
			int32 BodyIndex = OutputData.BodyIndex;
			if(BodyIndex != INDEX_NONE && (bResetSimulated || !IsSimulated[BodyIndex]))
			{
				FCompactPoseBoneIndex SimBoneIndex = OutputData.BoneReference.GetCompactPoseIndex(BoneContainer);
				const FTransform& ComponentSpaceTM = MeshBases.GetComponentSpaceTransform(SimBoneIndex);
				const FTransform WorldSpaceTM = bComponentSpaceSimulation ? ComponentSpaceTM : ComponentSpaceTM * CompWorldSpaceTM;

				Bodies[BodyIndex]->SetWorldTransform(WorldSpaceTM);
			}
		}

		bResetSimulated = false;

		UpdateWorldForces(CompWorldSpaceTM);

		//simulate
		PhysicsSimulation->Simulate(DeltaSeconds, Gravity);

		
		//write back to animation system
		for (const FOutputBoneData& OutputData : OutputBoneData)
		{
			int32 BodyIndex = OutputData.BodyIndex;
			if(BodyIndex != INDEX_NONE)
			{
				if(IsSimulated[BodyIndex])
				{
					FCompactPoseBoneIndex SimBoneIndex = OutputData.BoneReference.GetCompactPoseIndex(BoneContainer);
					FTransform ComponentSpaceTM;
					if(bComponentSpaceSimulation)
					{
						ComponentSpaceTM = Bodies[BodyIndex]->GetWorldTransform();
					}
					else
					{
						FTransform WorldSpaceTM = Bodies[BodyIndex]->GetWorldTransform();
						ComponentSpaceTM = WorldSpaceTM.GetRelativeTransform(CompWorldSpaceTM);	//back to component space
					}
					
					OutBoneTransforms.Add(FBoneTransform(SimBoneIndex, ComponentSpaceTM));
				}
			}
			else
			{
				//we have no body, but our ancestors are simulated so update our component space transform
			}
		}
	}
	//FPlatformMisc::EndNamedEvent();
}

void FAnimNode_Ragdoll::SwapBodyEntries(int32 BodyIdx1, int32 BodyIndx2)
{
	Swap(Bodies[BodyIdx1], Bodies[BodyIndx2]);
	Swap(BodyNames[BodyIdx1], BodyNames[BodyIndx2]);
}

void FAnimNode_Ragdoll::ClearHighLevelData()
{
	for (FConstraintInstance* CI : HighLevelConstraintInstances)
	{
		CI->TermConstraint();
		delete CI;
	}

	HighLevelConstraintInstances.Reset();

	for(FBodyInstance* BI : HighLevelBodyInstances)
	{
		BI->TermBody();
		delete BI;
	}

	HighLevelBodyInstances.Reset();
}

void FAnimNode_Ragdoll::InitPhysics(const UAnimInstance* InAnimInstance)
{
	const USkeletalMeshComponent* SkeletalMeshComp = InAnimInstance->GetSkelMeshComponent();
	const FReferenceSkeleton& RefSkel = SkeletalMeshComp->SkeletalMesh->RefSkeleton;
	UPhysicsAsset* UsePhysicsAsset = OverridePhysicsAsset ? OverridePhysicsAsset : InAnimInstance->GetSkelMeshComponent()->GetPhysicsAsset();
	if(UsePhysicsAsset)
	{
		delete PhysicsSimulation;
		PhysicsSimulation = new FSimulation();
		const int32 NumBodies = UsePhysicsAsset->SkeletalBodySetups.Num();
		Bodies.Empty(NumBodies);
		BodyBoneIndices.Empty(NumBodies);
		ComponentsInSim.Reset();
		TotalMass = 0.f;
		

		ClearHighLevelData();

		SkeletalMeshComp->InstantiatePhysicsAsset(*UsePhysicsAsset, bComponentSpaceSimulation ? FVector(1.f) : SkeletalMeshComp->GetComponentToWorld().GetScale3D(), HighLevelBodyInstances, HighLevelConstraintInstances);

		TMap<FName, FActorHandle*> NamesToHandles;

		TArray<FActorHandle*> IgnoreCollisionActors;

		//We now insert the bodies. For performance it's best to insert all the dynamic bodies first in the same order as their bone index. TODO: verify physics asset sorts bodies ascending with bone index
		//CreatePhysicsBodies<EInsertBodiesMode::SimulatedOnly>(RefSkel, PhysicsAsset, NamesToHandles);
		//CreatePhysicsBodies<EInsertBodiesMode::KinematicOnly>(RefSkel, PhysicsAsset, NamesToHandles);

#if WITH_PHYSX
		//bodies
		for(int32 BodyIdx = 0; BodyIdx < HighLevelBodyInstances.Num(); ++BodyIdx)
		{
			UBodySetup* BodySetup = UsePhysicsAsset->SkeletalBodySetups[BodyIdx];
			FBodyInstance* BodyInstance = HighLevelBodyInstances[BodyIdx];
			
			FActorHandle* NewBodyHandle;
			const bool bKinematic = BodySetup->PhysicsType != EPhysicsType::PhysType_Simulated;
			const int32 BoneIndex = RefSkel.FindBoneIndex(BodySetup->BoneName);
			if(BoneIndex == INDEX_NONE)
			{
				continue;
			}

			const FTransform& LastTransform = SkeletalMeshComp->GetComponentSpaceTransforms()[BoneIndex];	//This is out of date, but will still give our bodies an initial setup that matches the constraints (TODO: use refpose)

			if (!bKinematic)
			{
				NewBodyHandle = PhysicsSimulation->CreateDynamicActor(BodyInstance->GetPxRigidDynamic_AssumesLocked(), LastTransform);
				const float InvMass = NewBodyHandle->GetInverseMass();
				TotalMass += InvMass > 0.f ? 1.f / InvMass : 0.f;
			}
			else
			{
				NewBodyHandle = PhysicsSimulation->CreateKinematicActor(BodyInstance->GetPxRigidBody_AssumesLocked(), LastTransform);
			}

			Bodies.Add(NewBodyHandle);
			BodyBoneIndices.Add(BoneIndex);
			NamesToHandles.Add(BodySetup->BoneName, NewBodyHandle);
			IsSimulated.Add(!bKinematic);
			if(BodySetup->CollisionReponse == EBodyCollisionResponse::BodyCollision_Disabled)
			{
				IgnoreCollisionActors.Add(NewBodyHandle);
			}
		}

		if(NamesToHandles.Num() > 0)
		{
			//constraints
			for(int32 ConstraintIdx = 0; ConstraintIdx < HighLevelConstraintInstances.Num(); ++ConstraintIdx)
			{
				const UPhysicsConstraintTemplate* ConstraintTemplate = UsePhysicsAsset->ConstraintSetup[ConstraintIdx];
				FActorHandle* Body1Handle = NamesToHandles.FindRef(ConstraintTemplate->DefaultInstance.ConstraintBone1);
				FActorHandle* Body2Handle = NamesToHandles.FindRef(ConstraintTemplate->DefaultInstance.ConstraintBone2);

				if(Body1Handle && Body2Handle)
				{
					if (Body1Handle->IsSimulated() || Body2Handle->IsSimulated())
					{
						PhysicsSimulation->CreateJoint(HighLevelConstraintInstances[ConstraintIdx]->ConstraintData, Body1Handle, Body2Handle);
					}
				}
			}

			bResetSimulated = true;
		}

		TArray<FSimulation::FIgnorePair> IgnorePairs;
		const TMap<FRigidBodyIndexPair, bool>& DisableTable = UsePhysicsAsset->CollisionDisableTable;
		for(auto ConstItr = DisableTable.CreateConstIterator(); ConstItr; ++ConstItr)
		{
			FSimulation::FIgnorePair Pair;
			Pair.A = Bodies[ConstItr.Key().Indices[0]];
			Pair.B = Bodies[ConstItr.Key().Indices[1]];
			IgnorePairs.Add(Pair);
		}

		PhysicsSimulation->SetIgnoreCollisionPairTable(IgnorePairs);
		PhysicsSimulation->SetIgnoreCollisionActors(IgnoreCollisionActors);
	}
#endif
}

DECLARE_CYCLE_STAT(TEXT("FAnimNode_Ragdoll::UpdateWorldGeometry"), STAT_ImmediateUpdateWorldGeometry, STATGROUP_ImmediatePhysics);

void FAnimNode_Ragdoll::UpdateWorldGeometry(const UWorld& World, const USkeletalMeshComponent& SKC)
{
	SCOPE_CYCLE_COUNTER(STAT_ImmediateUpdateWorldGeometry);
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(TEXT("RagdollNodeFindGeometry"), /*bTraceComplex=*/false);
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

	const FSphere Bounds = SKC.CalcBounds(SKC.GetComponentToWorld()).GetSphere();
	//TODO: can probably move everything below off gt
	FPhysScene* PhysScene = World.GetPhysicsScene();

#if WITH_PHYSX
	SCOPED_SCENE_READ_LOCK(PhysScene ? PhysScene->GetPhysXScene(PST_Sync) : nullptr); //TODO: expose this part to the anim node
	World.OverlapMultiByChannel(Overlaps, Bounds.Center, FQuat::Identity,  OverlapChannel, FCollisionShape::MakeSphere(Bounds.W), QueryParams, FCollisionResponseParams(ECR_Overlap));
	for(const FOverlapResult& Overlap : Overlaps)
	{
		if(UPrimitiveComponent* OverlapComp = Overlap.GetComponent())
		{
			if (ComponentsInSim.Contains(OverlapComp)== false)
			{
				ComponentsInSim.Add(OverlapComp);
				if(PxRigidActor* RigidActor = OverlapComp->BodyInstance.GetPxRigidActor_AssumesLocked())
				{
					PhysicsSimulation->CreateStaticActor(RigidActor, P2UTransform(RigidActor->getGlobalPose()));
				}
			}
		}
		
	}
#endif
}

DECLARE_CYCLE_STAT(TEXT("FAnimNode_Ragdoll::UpdateWorldForces"), STAT_ImmediateUpdateWorldForces, STATGROUP_ImmediatePhysics);

void FAnimNode_Ragdoll::UpdateWorldForces(const FTransform& ComponentToWorld)
{
	SCOPE_CYCLE_COUNTER(STAT_ImmediateUpdateWorldForces);

	if(TotalMass > 0.f)
	{
		for (const USkeletalMeshComponent::FPendingRadialForces& PendingRadialForce : PendingRadialForces)
		{
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

					Body->AddRadialForce(bComponentSpaceSimulation ? ComponentToWorld.InverseTransformPosition(PendingRadialForce.Origin) : PendingRadialForce.Origin, StrengthPerBody, PendingRadialForce.Radius, PendingRadialForce.Falloff, ForceType);
				}
			}
		}
	}
}

void FAnimNode_Ragdoll::PreUpdate(const UAnimInstance* InAnimInstance)
{
	if(bNeedsInitialization)
	{
		bNeedsInitialization = false;
	
		InitPhysics(InAnimInstance);
	}

	UWorld* World = InAnimInstance->GetWorld();
	USkeletalMeshComponent* SKC = InAnimInstance->GetSkelMeshComponent();

#if WITH_EDITOR
	if (bEnableWorldGeometry && bComponentSpaceSimulation)
	{
		FMessageLog("PIE").Warning(FText::Format(LOCTEXT("WorldCollisionComponentSpace", "Trying to use world collision with component space simulation on ''{0}''. This is not supported, please use world space simulation"),
			FText::FromString(GetPathNameSafe(SKC))));
	}
#endif

	DeltaSeconds = World->GetDeltaSeconds();
	Gravity = bOverrideWorldGravity ? OverrideWorldGravity : FVector(0.f, 0.f, World->GetGravityZ());

	if(SKC)
	{
		if (PhysicsSimulation && bEnableWorldGeometry && !bComponentSpaceSimulation && World)
		{
			UpdateWorldGeometry(*World, *SKC);
		}

		PendingRadialForces = SKC->GetPendingRadialForces();
	}
	
}

void FAnimNode_Ragdoll::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	/** We only need to update simulated bones and children of simulated bones*/
	const int32 NumBodies = Bodies.Num();
	const TArray<FBoneIndexType>& RequiredBoneIndices = RequiredBones.GetBoneIndicesArray();
	const int32 NumRequiredBoneIndices = RequiredBoneIndices.Num();
	const FReferenceSkeleton& RefSkeleton = RequiredBones.GetReferenceSkeleton();
	
	OutputBoneData.Empty(NumRequiredBoneIndices);

	TArray<bool> OutputBonesCache;	//easy lookup into whether parent is going to be evaluated
	OutputBonesCache.AddZeroed(RefSkeleton.GetNum());

	for(int32 RequiredBoneIndexIndex = 0; RequiredBoneIndexIndex < NumRequiredBoneIndices; ++RequiredBoneIndexIndex)
	{
		const FBoneIndexType RequiredBoneIndex = RequiredBoneIndices[RequiredBoneIndexIndex];
		int32 FoundBodyIndex = INDEX_NONE;

		for(int32 BodyIndex = 0; BodyIndex < NumBodies; ++BodyIndex)	//TODO: we could sort this and avoid n^2
		{
			if(BodyBoneIndices[BodyIndex] == RequiredBoneIndex)	//If we have a body we need to save it for later
			{
				FOutputBoneData* OutputData = new (OutputBoneData) FOutputBoneData();
				OutputData->BodyIndex = BodyIndex;
				OutputData->BoneReference.BoneName = RefSkeleton.GetBoneName(RequiredBoneIndex);
				OutputData->BoneReference.Initialize(RequiredBones);

				if(IsSimulated[BodyIndex])
				{
					OutputBonesCache[RequiredBoneIndex] = true;	//children of simulated bodies need to update the component space transform
				}
		
				//++BodyIndex;	//Move on to next body
				FoundBodyIndex = BodyIndex;
				break;
			}
		}

		if(RequiredBoneIndex > 0 && FoundBodyIndex == INDEX_NONE)
		{
			//If we don't have a body, but our ancestors are simulated we will need to update the component space transform
			FBoneIndexType ParentBoneIndex = RequiredBones.GetParentBoneIndex(RequiredBoneIndex);
			if (OutputBonesCache[ParentBoneIndex])
			{
				OutputBonesCache[RequiredBoneIndex] = true;

				FOutputBoneData* OutputData = new (OutputBoneData) FOutputBoneData();
				OutputData->BodyIndex = INDEX_NONE;
				OutputData->BoneReference.BoneName = RefSkeleton.GetBoneName(RequiredBoneIndex);
				OutputData->BoneReference.Initialize(RequiredBones);
			}
		}
	}
}

void FAnimNode_Ragdoll::Initialize(const FAnimationInitializeContext& Context)
{
	bNeedsInitialization = true;

	FAnimNode_SkeletalControlBase::Initialize(Context);
}

#undef LOCTEXT_NAMESPACE