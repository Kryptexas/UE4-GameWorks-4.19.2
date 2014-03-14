// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNodeBase.h"
#include "AnimData/BoneMaskFilter.h"

#include "AnimNode_LayeredBoneBlend.generated.h"


// Layered blend (per bone); has dynamic number of blendposes that can blend per different bone sets
USTRUCT()
struct ENGINE_API FAnimNode_LayeredBoneBlend : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Links)
	FPoseLink BasePose;

	//@TODO: Anim: Comment these members
	UPROPERTY(EditAnywhere, BlueprintReadWrite, editfixedsize, Category=Links)
	TArray<FPoseLink> BlendPoses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, editfixedsize, Category=Config)
	TArray<FInputBlendPose> LayerSetup;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, editfixedsize, Category=Runtime, meta=(PinShownByDefault))
	TArray<float> BlendWeights;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Config)
	bool bMeshSpaceRotationBlend;


protected:
	TArray<FPerBoneBlendWeight> DesiredBoneBlendWeights;
	TArray<FPerBoneBlendWeight> CurrentBoneBlendWeights;

public:	
	FAnimNode_LayeredBoneBlend()
	{
	}

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) OVERRIDE;
	virtual void CacheBones(const FAnimationCacheBonesContext & Context) OVERRIDE;
	virtual void Update(const FAnimationUpdateContext& Context) OVERRIDE;
	virtual void Evaluate(FPoseContext& Output) OVERRIDE;
	// End of FAnimNode_Base interface

#if WITH_EDITOR
	void AddPose()
	{
		BlendWeights.Add(1.f);
		new (BlendPoses) FPoseLink();
		new (LayerSetup) FInputBlendPose();
	}

	void RemovePose(int32 PoseIndex)
	{
		BlendWeights.RemoveAt(PoseIndex);
		BlendPoses.RemoveAt(PoseIndex);
		LayerSetup.RemoveAt(PoseIndex);
	}
#endif

	void ReinitializeBoneBlendWeights(const FBoneContainer& RequiredBones, const USkeleton * Skeleton);
};
