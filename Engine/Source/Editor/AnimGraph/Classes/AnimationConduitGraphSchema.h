// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimationConduitGraphSchema.generated.h"

//@TODO: Should this derive from AnimationTransitionSchema (with appropriate suppression of state-based queries)
UCLASS(MinimalAPI)
class UAnimationConduitGraphSchema : public UEdGraphSchema_K2
{
	GENERATED_UCLASS_BODY()

	// UEdGraphSchema interface
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const OVERRIDE;
	virtual bool CanDuplicateGraph() const OVERRIDE { return false; }
	virtual void GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const OVERRIDE;
	// End of UEdGraphSchema interface
};
