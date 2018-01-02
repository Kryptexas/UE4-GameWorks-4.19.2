// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimInstanceProxy.h"
#include "Animation/AnimInstance.h"
#include "AnimNode_LiveLinkPose.h"

#include "LiveLinkInstance.generated.h"

class ULiveLinkRetargetAsset;

/** Proxy override for this UAnimInstance-derived class */
USTRUCT()
struct LIVELINK_API FLiveLinkInstanceProxy : public FAnimInstanceProxy
{
public:
	friend struct FAnimNode_LiveLinkPose;

	GENERATED_BODY()

		FLiveLinkInstanceProxy()
	{
	}

	FLiveLinkInstanceProxy(UAnimInstance* InAnimInstance)
		: FAnimInstanceProxy(InAnimInstance)
	{
	}

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual bool Evaluate(FPoseContext& Output) override;
	virtual void UpdateAnimationNode(float DeltaSeconds) override;

	FAnimNode_LiveLinkPose PoseNode;
};

UCLASS(transient, NotBlueprintable)
class LIVELINK_API ULiveLinkInstance : public UAnimInstance
{
	GENERATED_UCLASS_BODY()

	void SetSubject(FName SubjectName)
	{
		GetProxyOnGameThread<FLiveLinkInstanceProxy>().PoseNode.SubjectName = SubjectName;
	}

	void SetRetargetAsset(TSubclassOf<ULiveLinkRetargetAsset> RetargetAsset)
	{
		GetProxyOnGameThread<FLiveLinkInstanceProxy>().PoseNode.RetargetAsset = RetargetAsset;
	}

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};