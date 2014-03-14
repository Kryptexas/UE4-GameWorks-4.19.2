// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_CallMaterialParameterCollectionFunction.generated.h"

UCLASS(MinimalAPI)
class UK2Node_CallMaterialParameterCollectionFunction : public UK2Node_CallFunction
{
	GENERATED_UCLASS_BODY()

	// Begin EdGraphNode interface
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) OVERRIDE;
	// End EdGraphNode interface
};