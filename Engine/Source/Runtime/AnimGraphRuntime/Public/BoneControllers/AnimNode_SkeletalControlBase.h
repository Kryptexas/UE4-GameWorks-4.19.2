// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/**
 *	Abstract base class for a skeletal controller.
 *	A SkelControl is a module that can modify the position or orientation of a set of bones in a skeletal mesh in some programmatic way.
 */

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BonePose.h"
#include "Animation/AnimNodeBase.h"
#include "Animation/InputScaleBias.h"
#include "AnimNode_SkeletalControlBase.generated.h"

class USkeletalMeshComponent;

USTRUCT()
struct FSocketReference
{
	GENERATED_USTRUCT_BODY()

	/** Target socket to look at. Used if LookAtBone is empty. - You can use  LookAtLocation if you need offset from this point. That location will be used in their local space. **/
	UPROPERTY(EditAnywhere, Category = FSocketReference)
	FName SocketName;

private:
	int32 CachedSocketMeshBoneIndex;
	FCompactPoseBoneIndex CachedSocketCompactBoneIndex;
	FTransform CachedSocketLocalTransform;

public:
	FSocketReference()
		: CachedSocketMeshBoneIndex(INDEX_NONE)
		, CachedSocketCompactBoneIndex(INDEX_NONE)
	{
	}

	FSocketReference(const FName& InSocketName)
		: SocketName(InSocketName)
		, CachedSocketMeshBoneIndex(INDEX_NONE)
		, CachedSocketCompactBoneIndex(INDEX_NONE)
	{
	}

	void InitializeSocketInfo(const FAnimInstanceProxy* InAnimInstanceProxy);
	void InitialzeCompactBoneIndex(const FBoneContainer& RequiredBones);
	/* There are subtle difference between this two IsValid function
	 * First one says the configuration had a valid socket as mesh index is valid
	 * Second one says the current bonecontainer doesn't contain it, meaning the current LOD is missing the joint that is required to evaluate 
	 * Although the expected behavior is ambiguous, I'll still split these two, and use it accordingly */
	bool HasValidSetup() const
	{
		return (CachedSocketMeshBoneIndex != INDEX_NONE);
	}

	bool IsValidToEvaluate(const FBoneContainer& RequiredBones) const
	{
		return (CachedSocketCompactBoneIndex != INDEX_NONE);
	}

	FTransform GetAnimatedSocketTransform(struct FCSPose<FCompactPose>& InPose) const;
};

USTRUCT()
struct FTargetReference
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = FTargetReference)
	bool bUseSocket;

	UPROPERTY(EditAnywhere, Category = FTargetReference, meta=(EditCondition = "!bUseSocket"))
	FBoneReference BoneReference;

	UPROPERTY(EditAnywhere, Category = FTargetReference, meta = (EditCondition = "bUseSocket"))
	FSocketReference SocketReference;

	void InitializeBoneReferences(const FBoneContainer& RequiredBones)
	{
		if (bUseSocket)
		{
			SocketReference.InitialzeCompactBoneIndex(RequiredBones);
		}
		else
		{
			BoneReference.Initialize(RequiredBones);
		}
	}

	/** Initialize Bone Reference, return TRUE if success, otherwise, return false **/
	void Initialize(const FAnimInstanceProxy* InAnimInstanceProxy)
	{
		if (bUseSocket)
		{
			SocketReference.InitializeSocketInfo(InAnimInstanceProxy);
		}
	}


	/** return true if valid. Otherwise return false **/
	bool HasValidSetup() const
	{
		if (bUseSocket)
		{
			return SocketReference.HasValidSetup();
		}

		return BoneReference.BoneIndex != INDEX_NONE;
	}

	bool HasTargetSetup() const
	{
		if (bUseSocket)
		{
			return (SocketReference.SocketName != NAME_None);
		}

		return (BoneReference.BoneName != NAME_None);
	}

	FName GetTargetSetup() const
	{
		if (bUseSocket)
		{
			return (SocketReference.SocketName);
		}

		return (BoneReference.BoneName);
	}

	/** return true if valid. Otherwise return false **/
	bool IsValidToEvaluate(const FBoneContainer& RequiredBones) const
	{
		if (bUseSocket)
		{
			return SocketReference.IsValidToEvaluate(RequiredBones);
		}

		return BoneReference.IsValidToEvaluate(RequiredBones);
	}

	/** Get Target Location from current incoming pose */
	FVector GetTargetLocation(FVector TargetOffset, const FBoneContainer& BoneContainer,
		FCSPose<FCompactPose>& InPose, const FTransform& InComponentToWorld, FTransform& OutTargetTransform);
};

USTRUCT(BlueprintInternalUseOnly)
struct ANIMGRAPHRUNTIME_API FAnimNode_SkeletalControlBase : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	// Input link
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FComponentSpacePoseLink ComponentPose;

	// Current strength of the skeletal control
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings, meta=(PinShownByDefault))
	mutable float Alpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Settings)
	FInputScaleBias AlphaScaleBias;

	/*
	* Max LOD that this node is allowed to run
	* For example if you have LODThreadhold to be 2, it will run until LOD 2 (based on 0 index)
	* when the component LOD becomes 3, it will stop update/evaluate
	* currently transition would be issue and that has to be re-visited
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta = (DisplayName = "LOD Threshold"))
	int32 LODThreshold;

	UPROPERTY(Transient)
	float ActualAlpha;

public:

	FAnimNode_SkeletalControlBase()
		: Alpha(1.0f)
		, LODThreshold(INDEX_NONE)
		, ActualAlpha(0.f)
	{
	}

public:
#if WITH_EDITORONLY_DATA
	// forwarded pose data from the wired node which current node's skeletal control is not applied yet
	FCSPose<FCompactHeapPose> ForwardedPose;
#endif //#if WITH_EDITORONLY_DATA

	// FAnimNode_Base interface
	virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)  override;
	virtual void Update_AnyThread(const FAnimationUpdateContext& Context) final;
	virtual void EvaluateComponentSpace_AnyThread(FComponentSpacePoseContext& Output) final;
	// End of FAnimNode_Base interface
	
protected:
	// Interface for derived skeletal controls to implement
	// use this function to update for skeletal control base
	virtual void UpdateInternal(const FAnimationUpdateContext& Context);
	// use this function to evaluate for skeletal control base
	virtual void EvaluateComponentSpaceInternal(FComponentSpacePoseContext& Context);
	DEPRECATED(4.16, "Please use EvaluateSkeletalControl_AnyThread.")
	virtual void EvaluateBoneTransforms(USkeletalMeshComponent* SkelComp, FCSPose<FCompactPose>& MeshBases, TArray<FBoneTransform>& OutBoneTransforms) {}
	// Evaluate the new component-space transforms for the affected bones.
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms);
	// return true if it is valid to Evaluate
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) { return false; }
	// initialize any bone references you have
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones){};

	/** Allow base to add info to the node debug output */
	void AddDebugNodeData(FString& OutDebugData);
private:

	// Resused bone transform array to avoid reallocating in skeletal controls
	TArray<FBoneTransform> BoneTransforms;
};
