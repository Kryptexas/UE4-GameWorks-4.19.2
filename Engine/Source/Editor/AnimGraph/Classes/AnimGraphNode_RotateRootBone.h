// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "AnimGraphNode_RotateRootBone.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_RotateRootBone : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_RotateRootBone Node;

	// Begin UEdGraphNode interface.
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End UEdGraphNode interface.

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const OVERRIDE;
	// End of UAnimGraphNode_Base interface
};
