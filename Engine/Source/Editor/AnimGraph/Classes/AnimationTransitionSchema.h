// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimationTransitionSchema.generated.h"

// This class is the schema for transition rule graphs in animation state machines
UCLASS(MinimalAPI)
class UAnimationTransitionSchema : public UEdGraphSchema_K2
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphSchema interface.
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const OVERRIDE;
	virtual bool CanDuplicateGraph() const OVERRIDE { return false; }
	virtual void GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const;
	// End UEdGraphSchema interface.

private:
	void GetSourceStateActions(FGraphContextMenuBuilder& ContextMenuBuilder) const;
};
