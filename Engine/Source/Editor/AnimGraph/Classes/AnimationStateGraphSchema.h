// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "AnimationStateGraphSchema.generated.h"

UCLASS(MinimalAPI)
class UAnimationStateGraphSchema : public UAnimationGraphSchema
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphSchema interface.
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const OVERRIDE;
	virtual void GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const OVERRIDE;
	// End UEdGraphSchema interface.
};
