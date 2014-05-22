// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimGraphNode_HandIKRetargeting.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_HandIKRetargeting : public UAnimGraphNode_SkeletalControlBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Settings)
	FAnimNode_HandIKRetargeting Node;

public:
	// UEdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetNodeNativeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	// End of UEdGraphNode interface

protected:
	// UAnimGraphNode_SkeletalControlBase interface
	virtual FText GetControllerDescription() const OVERRIDE;
	// End of UAnimGraphNode_SkeletalControlBase interface
};
