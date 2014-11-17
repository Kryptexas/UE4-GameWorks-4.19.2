// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EdGraph/EdGraphNode.h"
#include "AnimStateEntryNode.generated.h"

UCLASS(MinimalAPI)
class UAnimStateEntryNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()


	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetTooltip() const override;
	// End UEdGraphNode interface
	
	ANIMGRAPH_API UEdGraphNode* GetOutputNode() const;

};
