// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimGraphNode_Fabrik.generated.h"

// Editor node for FABRIK IK skeletal controller
UCLASS(MinimalAPI)
class UAnimGraphNode_Fabrik : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_Fabrik Node;

public:
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End of UEdGraphNode interface

protected:
	// UAnimGraphNode_SkeletalControlBase interface
	virtual FText GetControllerDescription() const OVERRIDE;
	// End of UAnimGraphNode_SkeletalControlBase interface
};