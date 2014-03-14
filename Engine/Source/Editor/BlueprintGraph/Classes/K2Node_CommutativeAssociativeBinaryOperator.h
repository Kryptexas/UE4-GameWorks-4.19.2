// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node_CommutativeAssociativeBinaryOperator.generated.h"

UCLASS(MinimalAPI)
class UK2Node_CommutativeAssociativeBinaryOperator : public UK2Node_CallFunction
{
	GENERATED_UCLASS_BODY()

	/** The number of additional input pins to generate for this node (2 base pins are not included) */
	UPROPERTY()
	int32 NumAdditionalInputs;

private:
	const static int32 BinaryOperatorInputsNum = 2;

	static int32 GetMaxInputPinsNum();
	static FString GetNameForPin(int32 PinIndex);

	FEdGraphPinType GetType() const;

	void AddInputPinInner(int32 AdditionalPinIndex);
	bool CanAddPin() const;
	bool CanRemovePin(const UEdGraphPin* Pin) const;
public:
	BLUEPRINTGRAPH_API UEdGraphPin* FindOutPin() const;
	BLUEPRINTGRAPH_API UEdGraphPin* FindSelfPin() const;

	/** Get TRUE input type (self, etc.. are skipped) */
	BLUEPRINTGRAPH_API UEdGraphPin* GetInputPin(int32 InputPinIndex);

	BLUEPRINTGRAPH_API void AddInputPin();
	BLUEPRINTGRAPH_API void RemoveInputPin(UEdGraphPin* Pin);

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End of UK2Node interface
};