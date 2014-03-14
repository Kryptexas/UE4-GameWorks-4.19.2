// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_ExecutionSequence.generated.h"

UCLASS(MinimalAPI)
class UK2Node_ExecutionSequence : public UK2Node
{
	GENERATED_UCLASS_BODY()


#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.Sequence_16x"); }
	// End UEdGraphNode interface

	// Begin UK2Node interface
	void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End UK2Node interface

	// K2Node_ExecutionSequence interface

	/** Gets a unique pin name, the next in the sequence */
	FString GetUniquePinName();

	/**
	 * Adds a new execution pin to an execution node
	 */
	BLUEPRINTGRAPH_API void AddPinToExecutionNode();

	/**
	 * Removes the specified execution pin from an execution node
	 *
	 * @param	TargetPin	The pin to remove from the node
	 */
	BLUEPRINTGRAPH_API void RemovePinFromExecutionNode(UEdGraphPin* TargetPin);

	/** Whether an execution pin can be removed from the node or not */
	BLUEPRINTGRAPH_API bool CanRemoveExecutionPin() const;

	// @todo document
	BLUEPRINTGRAPH_API UEdGraphPin * GetThenPinGivenIndex(int32 Index);

private:
	// Returns the exec output pin name for a given 0-based index
	virtual FString GetPinNameGivenIndex(int32 Index) const;
#endif 
};

