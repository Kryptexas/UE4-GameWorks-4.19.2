// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimationCustomTransitionSchema.generated.h"

UCLASS(MinimalAPI)
class UAnimationCustomTransitionSchema : public UAnimationGraphSchema
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphSchema interface.
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const OVERRIDE;
	void GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const OVERRIDE;
	// End UEdGraphSchema interface.
};
