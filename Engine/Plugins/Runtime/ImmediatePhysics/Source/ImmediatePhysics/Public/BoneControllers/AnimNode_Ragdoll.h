// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_SkeletalControlBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "AnimNode_Ragdoll.generated.h"

namespace ImmediatePhysics
{
	struct FSimulation;
	struct FActorHandle;
}

struct FBodyInstance;
struct FConstraintInstance;

/**
 *	Controller that simulates physics based on the physics asset of the skeletal mesh component
 */
USTRUCT()
struct IMMEDIATEPHYSICS_API FAnimNode_Ragdoll : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	FAnimNode_Ragdoll();
	~FAnimNode_Ragdoll();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void Initialize(const FAnimationInitializeContext& Context) override;
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	virtual bool HasPreUpdate() const override { return true; }
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override { return true; }
	// End of FAnimNode_SkeletalControlBase interface

	/** Physics asset to use. If empty use the skeletal mesh's default physics asset */
	UPROPERTY(EditAnywhere, Category = Settings)
	UPhysicsAsset* OverridePhysicsAsset;

	/** Override gravity*/
	UPROPERTY(EditAnywhere, Category = Settings, meta = (editcondition = "bOverrideWorldGravity"))
	FVector OverrideWorldGravity;

	/** The channel we use to find static geometry to collide with */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (editcondition = "bEnableWorldGeometry"))
	TEnumAsByte<ECollisionChannel> OverlapChannel;

	UPROPERTY()
	bool bEnableWorldGeometry;

	/** When true, simulation is done in component space. This means velocity is only inherited by animated bodies */
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bComponentSpaceSimulation;

	UPROPERTY()
	bool bOverrideWorldGravity;

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

	void InitPhysics(const UAnimInstance* InAnimInstance);
	void UpdateWorldGeometry(const UWorld& World, const USkeletalMeshComponent& SKC);
	void UpdateWorldForces(const FTransform& ComponentToWorld);
private:

	bool bNeedsInitialization;

	FDelegateHandle InitPhysHandle;
	FDelegateHandle DestroyPhysHandle;

	/** This should only be used for removing the delegate during termination. Do NOT use this for any per frame work */
	TWeakObjectPtr<USkeletalMeshComponent> SkelMeshCompWeakPtr;

	ImmediatePhysics::FSimulation* PhysicsSimulation;

	struct FOutputBoneData
	{
		FBoneReference BoneReference;
		int32 BodyIndex;
	};
	
	TArray<FOutputBoneData> OutputBoneData;
	TArray<ImmediatePhysics::FActorHandle*> Bodies;
	TArray<FName> BodyNames;
	TArray<bool> IsSimulated;
	TArray<FBoneIndexType> BodyBoneIndices;
	bool bResetSimulated;

	void SwapBodyEntries(int32 BodyIdx1, int32 BodyIndx2);

	void ClearHighLevelData();

	enum class EInsertBodiesMode
	{
		SimulatedOnly,
		KinematicOnly
	};

	TArray<struct FPhysicsConstraintHandle*> Constraints;
	
	TArray<FBodyInstance*> HighLevelBodyInstances;
	TArray<FConstraintInstance*> HighLevelConstraintInstances;

	TArray<USkeletalMeshComponent::FPendingRadialForces> PendingRadialForces;

	TSet<UPrimitiveComponent*> ComponentsInSim;

	FVector Gravity;
	float DeltaSeconds;
	float TotalMass;
};
