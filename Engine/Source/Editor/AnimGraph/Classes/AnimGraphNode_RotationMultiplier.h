// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_SkeletalControlBase.h"
#include "Animation/BoneControllers/AnimNode_RotationMultiplier.h"
#include "AnimGraphNode_RotationMultiplier.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_RotationMultiplier : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_RotationMultiplier Node;

public:
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetTooltip() const override;
	// End of UEdGraphNode interface

protected:
	// UAnimGraphNode_SkeletalControlBase interface
	virtual FText GetControllerDescription() const override;
	// End of UAnimGraphNode_SkeletalControlBase interface
};
