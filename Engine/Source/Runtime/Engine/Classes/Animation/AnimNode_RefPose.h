// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimNode_RefPose.generated.h"

UENUM()
enum ERefPoseType
{
	EIT_LocalSpace, 
	EIT_Additive
};

// RefPose pose nodes - ref pose or additive RefPose pose
USTRUCT()
struct ENGINE_API FAnimNode_RefPose : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TEnumAsByte<ERefPoseType> RefPoseType;
	
public:	
	FAnimNode_RefPose()
		:	FAnimNode_Base()
		, 	RefPoseType(EIT_LocalSpace)
	{
	}

	virtual void Evaluate(FPoseContext& Output) OVERRIDE;
};

USTRUCT()
struct ENGINE_API FAnimNode_MeshSpaceRefPose : public FAnimNode_Base
{
	GENERATED_USTRUCT_BODY()
public:	
	FAnimNode_MeshSpaceRefPose()
		:	FAnimNode_Base()
	{
	}

	virtual void EvaluateComponentSpace(FComponentSpacePoseContext& Output);
};
