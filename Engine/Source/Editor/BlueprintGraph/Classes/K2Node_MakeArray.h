// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_MakeArray.generated.h"

UCLASS(MinimalAPI)
class UK2Node_MakeArray : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** The number of input pins to generate for this node */
	UPROPERTY()
	int32 NumInputs;

public:
	BLUEPRINTGRAPH_API void AddInputPin();
	BLUEPRINTGRAPH_API void RemoveInputPin(UEdGraphPin* Pin);

	/** returns a reference to the output array pin of this node, which is responsible for defining the type */
	BLUEPRINTGRAPH_API UEdGraphPin* GetOutputPin() const;

public:
	// UEdGraphNode interface
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void PostReconstructNode() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.MakeArray_16x"); }
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual bool IsNodePure() const OVERRIDE { return true; }
	void NotifyPinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const OVERRIDE;
	// End of UK2Node interface



protected:
	friend class FKismetCompilerContext;

	/** If needed, will clear all pins to be wildcards */
	void ClearPinTypeToWildcard();

	/** Propagates the pin type from the output (array) pin to the inputs, to make sure types are consistent */
	void PropagatePinType();

	/** Returns the function to be used to clear the array */
	BLUEPRINTGRAPH_API UFunction* GetArrayClearFunction() const;

	/** Returns the function to be used to add a function to the array */
	BLUEPRINTGRAPH_API UFunction* GetArrayAddFunction() const;
};

