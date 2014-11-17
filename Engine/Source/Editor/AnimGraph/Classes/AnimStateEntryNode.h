// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EdGraph/EdGraphNode.h"
#include "AnimStateEntryNode.generated.h"

UCLASS(MinimalAPI)
class UAnimStateEntryNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()


	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	// End UEdGraphNode interface
	
	ANIMGRAPH_API UEdGraphNode* GetOutputNode() const;

};
