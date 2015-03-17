// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimationGraphSchema.h"
#include "AnimationCustomTransitionSchema.generated.h"

UCLASS(MinimalAPI)
class UAnimationCustomTransitionSchema : public UAnimationGraphSchema
{
	GENERATED_BODY()
public:
	ANIMGRAPH_API UAnimationCustomTransitionSchema(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin UEdGraphSchema interface.
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
	void GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const override;
	// End UEdGraphSchema interface.
};
