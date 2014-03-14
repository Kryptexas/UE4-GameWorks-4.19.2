// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_RefPoseBase.generated.h"

UCLASS(MinimalAPI, abstract)
class UAnimGraphNode_RefPoseBase : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_RefPose Node;

	// UEdGraphNode interface
	virtual FString GetNodeCategory() const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	// End of UEdGraphNode interface
};
