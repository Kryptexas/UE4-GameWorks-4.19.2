// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_CallFunction.h"
#include "K2Node_CallDataTableFunction.generated.h"

UCLASS(MinimalAPI)
class UK2Node_CallDataTableFunction : public UK2Node_CallFunction
{
	GENERATED_BODY()
public:
	BLUEPRINTGRAPH_API UK2Node_CallDataTableFunction(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin EdGraphNode interface
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	// End EdGraphNode interface
};
