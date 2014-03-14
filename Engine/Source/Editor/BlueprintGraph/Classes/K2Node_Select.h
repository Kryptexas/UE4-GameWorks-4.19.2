// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_Select.generated.h"

UCLASS(MinimalAPI)
class UK2Node_Select : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** The number of selectable options this node currently has */
	UPROPERTY()
	int32 NumOptionPins;

	/** The pin type of the index pin */
	UPROPERTY()
	FEdGraphPinType IndexPinType;

	/** Name of the enum being switched on */
	UPROPERTY()
	UEnum* Enum;

	/** List of the current entries in the enum (Pin Names) */
	UPROPERTY()
	TArray<FName> EnumEntries;

	/** List of the current entries in the enum (Pin Friendly Names) */
	UPROPERTY()
	TArray<FName> EnumEntryFriendlyNames;

	/** Whether we need to reconstruct the node after the pins have changed */
	UPROPERTY()
	bool bReconstructNode;

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void NodeConnectionListChanged() OVERRIDE;
	virtual void PinTypeChanged(UEdGraphPin* Pin) OVERRIDE;
	virtual void PostPasteNode() OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.Select_16x"); }
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) OVERRIDE;
 	virtual void PostReconstructNode() OVERRIDE;
	virtual bool IsNodeSafeToIgnore() const OVERRIDE { return true; }
	virtual bool IsNodePure() const OVERRIDE { return true; }
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End UK2Node interface

	/** Get the return value pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetReturnValuePin() const;
	/** Get the condition pin */
	BLUEPRINTGRAPH_API virtual UEdGraphPin* GetIndexPin() const;
	/** Returns a list of pins that represent the selectable options */
	BLUEPRINTGRAPH_API virtual void GetOptionPins(TArray<UEdGraphPin*>& OptionPins) const;

	/** Gets the name and class of the EqualEqual_IntInt function */
	BLUEPRINTGRAPH_API void GetConditionalFunction(FName& FunctionName, UClass** FunctionClass);
	/** Gets the name and class of the PrintString function */
	BLUEPRINTGRAPH_API static void GetPrintStringFunction(FName& FunctionName, UClass** FunctionClass);

	/** Adds a new option pin to the node */
	BLUEPRINTGRAPH_API void AddOptionPinToNode();
	/** Removes the last option pin from the node */
	BLUEPRINTGRAPH_API void RemoveOptionPinToNode();
	/** Return whether an option pin can be added to the node */
	BLUEPRINTGRAPH_API virtual bool CanAddOptionPinToNode() const;
	/** Return whether an option pin can be removed to the node */
	BLUEPRINTGRAPH_API virtual bool CanRemoveOptionPinToNode() const;

	/**
	 * Notification from the editor that the user wants to change the PinType on a selected pin
	 *
	 * @param Pin	The pin the user wants to change the type for
	 */
	BLUEPRINTGRAPH_API void ChangePinType(UEdGraphPin* Pin);
	/**
	 * Whether the user can change the pintype on a selected pin
	 *
	 * @param Pin	The pin in question
	 */
	BLUEPRINTGRAPH_API bool CanChangePinType(UEdGraphPin* Pin) const;

	// Bind the options to a named enum 
	virtual void SetEnum(UEnum* InEnum);
#endif
};

