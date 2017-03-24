// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_SkeletalControlBase.h"
#include "AnimNode_ControlRig.generated.h"

class UControlRig;

/**
 * Animation node that allows animation ControlRig output to be used in an animation graph
 */
USTRUCT()
struct CONTROLRIG_API FAnimNode_ControlRig : public FAnimNode_Base
{
	GENERATED_BODY()

	FAnimNode_ControlRig();

	void SetControlRig(UControlRig* InControlRig);
	UControlRig* GetControlRig() const;

	// FAnimNode_Base interface
	virtual void RootInitialize(const FAnimInstanceProxy* InProxy) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	virtual void Update(const FAnimationUpdateContext& Context) override;
	virtual void Evaluate(FPoseContext& Output) override;
	virtual void CacheBones(const FAnimationCacheBonesContext& Context) override;

private:
	/** Cached ControlRig */
	TWeakObjectPtr<UControlRig> CachedControlRig;
	TArray<FName> NodeNames;

public:
	/** Should we apply this rig pose additively? */
	UPROPERTY()
	bool bAdditive;
};
