// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimNodeBase.h"
#include "AnimNode_CopyPoseFromMesh.generated.h"

class USkeletalMeshComponent;
struct FAnimInstanceProxy;

/**
 *	Simple controller to copy a bone's transform to another one.
 */

USTRUCT(BlueprintInternalUseOnly)
struct ANIMGRAPHRUNTIME_API FAnimNode_CopyPoseFromMesh : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	/*  This is used by default if it's valid */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Copy, meta=(PinShownByDefault))
	USkeletalMeshComponent* SourceMeshComponent;

	/* If SourceMeshComponent is not valid, and if this is true, it will look for attahced parent as a source */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Copy, meta = (NeverAsPin))
	bool bUseAttachedParent;

	FAnimNode_CopyPoseFromMesh();

	// FAnimNode_Base interface
	virtual void Initialize(const FAnimationInitializeContext& Context) override;
	virtual void CacheBones(const FAnimationCacheBonesContext& Context) override;
	virtual void Update(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	// End of FAnimNode_Base interface

private:
	TWeakObjectPtr<USkeletalMeshComponent> CurrentlyUsedSourceMeshComponent;
	// cache of target space bases to source space bases
	TMap<int32, int32> BoneMapToSource;

	// reinitialize mesh component 
	void ReinitializeMeshComponent(USkeletalMeshComponent* NewSkeletalMeshComponent, FAnimInstanceProxy* AnimInstanceProxy);
	void RefreshMeshComponent(FAnimInstanceProxy* AnimInstanceProxy);
};
