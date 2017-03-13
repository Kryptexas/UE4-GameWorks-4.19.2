// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimNode_ControlRig.h"
#include "AnimGraphNode_ControlRig.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_ControlRig : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_ControlRig Node;

public:
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;

	// UAnimGraphNode_SkeletalControlBase interface
	virtual FText GetControllerDescription() const override;
};

