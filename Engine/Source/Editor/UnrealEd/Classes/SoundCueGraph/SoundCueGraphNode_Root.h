// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SoundCueGraphNode_Root.generated.h"

UCLASS(MinimalAPI)
class USoundCueGraphNode_Root : public USoundCueGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool CanUserDeleteNode() const OVERRIDE { return false; }
	virtual bool CanDuplicateNode() const OVERRIDE { return false; }
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	// End of UEdGraphNode interface

	// USoundCueGraphNode_Base interface
	virtual void CreateInputPins() OVERRIDE;
	virtual bool IsRootNode() const OVERRIDE {return true;}
	// End of USoundCueGraphNode_Base interface
};
