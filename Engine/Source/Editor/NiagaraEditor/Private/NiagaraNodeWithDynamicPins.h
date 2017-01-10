// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraNode.h"
#include "SlateTypes.h"
#include "EdGraph/EdGraph.h"

#include "NiagaraNodeWithDynamicPins.generated.h"

class UEdGraphPin;


/** A base node for niagara nodes with pins which can be dynamically added and removed by the user. */
UCLASS()
class UNiagaraNodeWithDynamicPins : public UNiagaraNode
{
public:
	GENERATED_BODY()

	//~ UEdGraphNode interface
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const override;

	/** Requests a new pin be added to the node with the specified direction, type, and name. */
	UEdGraphPin* RequestNewTypedPin(EEdGraphPinDirection Direction, const FNiagaraTypeDefinition& Type, FString InName);

	/** Requests a new pin be added to the node with the specified direction and type. */
	UEdGraphPin* RequestNewTypedPin(EEdGraphPinDirection Direction, const FNiagaraTypeDefinition& Type);

protected:
	/** Creates an add pin on the node for the specified direction. */
	void CreateAddPin(EEdGraphPinDirection Direction);

	/** Called when a new typed pin is added by the user. */
	virtual void OnNewTypedPinAdded(UEdGraphPin* NewPin) { }

	/** Called when a pin is renamed. */
	virtual void OnPinRenamed(UEdGraphPin* RenamedPin) { }

	/** Called to determine is a pin can be renamed by the user. */
	virtual bool CanRenamePin(const UEdGraphPin* Pin) const;

	/** Called to determine is a pin can be removed by the user. */
	virtual bool CanRemovePin(const UEdGraphPin* Pin) const;

private:
	/** Removes a pin from this node with a transaction. */
	void RemoveDynamicPin(UEdGraphPin* Pin);

	/** Gets the display text for a pin. */
	FText GetPinNameText(UEdGraphPin* Pin) const;

	/** Called when a pin's name text is committed. */
	void PinNameTextCommitted(const FText& Text, ETextCommit::Type CommitType, UEdGraphPin* Pin);

public:
	/** The sub category for add pins. */
	static const FString AddPinSubCategory;
};
