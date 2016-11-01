// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Animation/AnimNodeBase.h"
#include "AnimNode_PoseSnapshot.generated.h"

// RefPose pose nodes - ref pose or additive RefPose pose
USTRUCT()
struct ANIMGRAPHRUNTIME_API FAnimNode_PoseSnapshot : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

public:	

	virtual void Evaluate(FPoseContext& Output) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Copy, meta = (PinShownByDefault))
	FName SnapshotName;
};