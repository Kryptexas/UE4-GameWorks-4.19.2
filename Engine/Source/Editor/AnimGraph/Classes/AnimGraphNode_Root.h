// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "AnimGraphNode_Root.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_Root : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_Root Node;

	// Begin UEdGraphNode interface.
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool CanUserDeleteNode() const OVERRIDE { return false; }
	virtual bool CanDuplicateNode() const OVERRIDE { return false; }
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	// End UEdGraphNode interface.

	// UAnimGraphNode_Base interface
	virtual bool IsSinkNode() const OVERRIDE;

	// Get the link to the documentation
	virtual FString GetDocumentationLink() const OVERRIDE;

	// End of UAnimGraphNode_Base interface
};
