// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.




#pragma once
#include "PoseableMeshComponent.generated.h"

UENUM()
namespace EBoneSpaces
{
	enum Type
	{
		/** Set absolute position of bone in world space */
		WorldSpace		UMETA( DisplayName = "World Space" ),
		/** Set position of bone in components reference frame */
		ComponentSpace	UMETA( DisplayName = "Component Space" ),
		/** Set position of bone relative to parent bone */
		//LocalSpace		UMETA( DisplayName = "Parent Bone Space" ),
	};
}

/**
 *	UPoseableMeshComponent that allows bone transforms to be driven by blueprint.
 */
UCLASS(HeaderGroup=Component, ClassGroup=Rendering, hidecategories=(Object,Physics), config=Engine, editinlinenew, meta=(BlueprintSpawnableComponent),MinimalAPI)
class UPoseableMeshComponent : public USkinnedMeshComponent
{
	GENERATED_UCLASS_BODY()

	/** Temporary array of local-space (ie relative to parent bone) rotation/translation/scale for each bone. */
	TArray<FTransform> LocalAtoms;

	FBoneContainer RequiredBones;

	UFUNCTION(BlueprintCallable, Category="Components|PoseableMesh")
	void SetBoneTransformByName(FName BoneName, const FTransform& InTransform, EBoneSpaces::Type BoneSpace);

	UFUNCTION(BlueprintCallable, Category="Components|PoseableMesh")
	void SetBoneLocationByName(FName BoneName, FVector InLocation, EBoneSpaces::Type BoneSpace);

	UFUNCTION(BlueprintCallable, Category="Components|PoseableMesh")
	void SetBoneRotationByName(FName BoneName, FRotator InRotation, EBoneSpaces::Type BoneSpace);

	UFUNCTION(BlueprintCallable, Category="Components|PoseableMesh")
	void SetBoneScaleByName(FName BoneName, FVector InScale3D, EBoneSpaces::Type BoneSpace);

	UFUNCTION(BlueprintCallable, Category="Components|PoseableMesh") 
	FTransform GetBoneTransformByName(FName BoneName, EBoneSpaces::Type BoneSpace);

	UFUNCTION(BlueprintCallable, Category="Components|PoseableMesh")
	FVector GetBoneLocationByName(FName BoneName, EBoneSpaces::Type BoneSpace);

	UFUNCTION(BlueprintCallable, Category="Components|PoseableMesh")
	FRotator GetBoneRotationByName(FName BoneName, EBoneSpaces::Type BoneSpace);

	UFUNCTION(BlueprintCallable, Category="Components|PoseableMesh")
	FVector GetBoneScaleByName(FName BoneName, EBoneSpaces::Type BoneSpace);

	UFUNCTION(BlueprintCallable, Category="Components|PoseableMesh")
	void ResetBoneTransformByName(FName BoneName);

	// Begin USkinnedMeshComponent Interface
	virtual void RefreshBoneTransforms() OVERRIDE;
	virtual bool AllocateTransformData() OVERRIDE;
	// End USkinnedMeshComponent Interface

	/**
	 * Take the LocalAtoms array (translation vector, rotation quaternion and scale vector) and update the array of component-space bone transformation matrices (SpaceBases).
	 * It will work down hierarchy multiplying the component-space transform of the parent by the relative transform of the child.
	 * This code also applies any per-bone rotators etc. as part of the composition process
	 */
	void FillSpaceBases();
};



