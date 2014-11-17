// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_DynamicCast.h"
#include "K2Node_ClassDynamicCast.generated.h"

UCLASS(MinimalAPI)
class UK2Node_ClassDynamicCast : public UK2Node_DynamicCast
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	// End UEdGraphNode interface

	// UK2Node interface
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;
	// End of UK2Node interface

	/** Get the input object to be casted pin */
	virtual UEdGraphPin* GetCastSourcePin() const override;
};

