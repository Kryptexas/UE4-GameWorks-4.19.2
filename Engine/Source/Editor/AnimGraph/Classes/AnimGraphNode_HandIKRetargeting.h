// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_SkeletalControlBase.h"
#include "Animation/BoneControllers/AnimNode_HandIKRetargeting.h"
#include "AnimGraphNode_HandIKRetargeting.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_HandIKRetargeting : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_BODY()
public:
	ANIMGRAPH_API UAnimGraphNode_HandIKRetargeting(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_HandIKRetargeting Node;

public:
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	// End of UEdGraphNode interface

protected:
	// UAnimGraphNode_SkeletalControlBase interface
	virtual FText GetControllerDescription() const override;
	// End of UAnimGraphNode_SkeletalControlBase interface
};
