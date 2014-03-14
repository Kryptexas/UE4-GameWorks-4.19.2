// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_TransitionResult.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_TransitionResult : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_TransitionResult Node;

	// UEdGraphNode interface
	virtual bool CanUserDeleteNode() const OVERRIDE { return false; }
	virtual bool CanDuplicateNode() const OVERRIDE { return false; }
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual bool IsSinkNode() const OVERRIDE { return true; }
	// End of UAnimGraphNode_Base interface
};
