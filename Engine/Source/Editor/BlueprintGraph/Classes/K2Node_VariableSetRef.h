// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_VariableSetRef.generated.h"

UCLASS(MinimalAPI)
class UK2Node_VariableSetRef : public UK2Node
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End UK2Node interface
#endif

	/** 
	 * Changes the type of variable set by this node, based on the specified pin
	 *
	 * @param Pin				The pin to gather type information from.  If NULL, node will reset to wildcards
	 */
	void CoerceTypeFromPin(const UEdGraphPin* Pin);

	/** Returns the pin that specifies which by-ref variable to set */
	BLUEPRINTGRAPH_API UEdGraphPin* GetTargetPin() const;

	/** Returns the pin that specifies the value to set */
	BLUEPRINTGRAPH_API UEdGraphPin* GetValuePin() const;
};

