// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "AnimNode_AimOffsetLookAt.generated.h"

//@TODO: Comment
USTRUCT(BlueprintInternalUseOnly)
struct ANIMGRAPHRUNTIME_API FAnimNode_AimOffsetLookAt : public FAnimNode_BlendSpacePlayer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Links)
	FPoseLink BasePose;

	/*
	* Max LOD that this node is allowed to run
	* For example if you have LODThreadhold to be 2, it will run until LOD 2 (based on 0 index)
	* when the component LOD becomes 3, it will stop update/evaluate
	* currently transition would be issue and that has to be re-visited
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta = (DisplayName = "LOD Threshold"))
	int32 LODThreshold;

	UPROPERTY(Transient)
	bool bIsLODEnabled;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAt, meta = (PinShownByDefault))
	FVector LookAtLocation;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAt, meta = (PinHiddenByDefault))
	FName SourceSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAt, meta = (PinHiddenByDefault))
	FName PivotSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LookAt, meta = (PinShownByDefault))
	float Alpha;

	UPROPERTY()
	FBoneReference SocketBoneReference;

	UPROPERTY()
	FTransform SocketLocalTransform;

	UPROPERTY()
	FBoneReference PivotSocketBoneReference;

	UPROPERTY()
	FTransform PivotSocketLocalTransform;

public:
	FAnimNode_AimOffsetLookAt();

	// FAnimNode_Base interface
	virtual void RootInitialize(const FAnimInstanceProxy* InProxy) override;
	virtual void Initialize(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones(const FAnimationCacheBonesContext& Context) override;
	virtual void UpdateAssetPlayer(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

	void UpdateFromLookAtTarget(FPoseContext& LocalPoseContext);
};
