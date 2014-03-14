// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvironmentQueryGraphNode_Option.generated.h"

UCLASS()
class UEnvironmentQueryGraphNode_Option : public UEnvironmentQueryGraphNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TArray<class UEnvironmentQueryGraphNode_Test*> Tests;

	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetDescription() const OVERRIDE;

	void AddSubNode(class UEnvironmentQueryGraphNode_Test* NodeTemplate, class UEdGraph* ParentGraph);
	void CreateAddTestSubMenu(class FMenuBuilder& MenuBuilder, UEdGraph* Graph) const;

	void CalculateWeights();
};
