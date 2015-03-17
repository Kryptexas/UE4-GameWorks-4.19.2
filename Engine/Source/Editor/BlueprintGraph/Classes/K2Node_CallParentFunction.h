// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_CallFunction.h"
#include "K2Node_CallParentFunction.generated.h"

UCLASS(MinimalAPI)
class UK2Node_CallParentFunction : public UK2Node_CallFunction
{
	GENERATED_BODY()
public:
	BLUEPRINTGRAPH_API UK2Node_CallParentFunction(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin EdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void PostPlacedNewNode() override;
	// End EdGraphNode interface

	virtual void SetFromFunction(const UFunction* Function) override;
};

