// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_SkeletalControlBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "AnimNode_RigidBody.generated.h"

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
struct IMMEDIATEPHYSICS_API FAnimNode_RigidBody : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	FAnimNode_RigidBody();
	~FAnimNode_RigidBody();

	// FAnimNode_Base interface
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	// FAnimNode_SkeletalControlBase interface
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
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

	UPROPERTY(EditAnywhere, Category = Settings, meta=(InlineEditConditionToggle))
	bool bEnableWorldGeometry;

	/** When true, simulation is done in component space. This means velocity is only inherited by animated bodies */
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bComponentSpaceSimulation;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (InlineEditConditionToggle))
	bool bOverrideWorldGravity;

	/**
	 * Scale of cached bounds (vs. actual bounds).
	 * Increasing this may improve performance, but overlaps may not work as well.
	 * (A value of 1.0 effectively disables cached bounds).
	 */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (ClampMin="1.0", ClampMax="2.0"))
	float CachedBoundsScale;

private:
	// FAnimNode_SkeletalControlBase interface
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface

	void InitPhysics(const UAnimInstance* InAnimInstance);
	void UpdateWorldGeometry(const UWorld& World, const USkeletalMeshComponent& SKC);
	void UpdateWorldForces(const FTransform& ComponentToWorld);
private:

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
	
	TArray<struct FPhysicsConstraintHandle*> Constraints;
	TArray<USkeletalMeshComponent::FPendingRadialForces> PendingRadialForces;

	TSet<UPrimitiveComponent*> ComponentsInSim;

	FVector Gravity;
	float DeltaSeconds;
	float TotalMass;

	FSphere Bounds;
	FSphere CachedBounds;

	FCollisionQueryParams QueryParams;

	FPhysScene* PhysScene;

	// Typically, World should never be accessed off the Game Thread.
	// However, since we're just doing overlaps this should be OK.
	const UWorld* UnsafeWorld;
};
