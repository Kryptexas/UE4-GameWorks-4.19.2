// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node.h"
#include "K2Node_CreateWidget.generated.h"

UCLASS()
class UMGEDITOR_API UK2Node_CreateWidget : public UK2Node_ConstructObjectFromClass
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	// End UEdGraphNode interface.

	// Begin UK2Node interface
	void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const;
	virtual FText GetMenuCategory() const override;
	// End UK2Node interface.

	/** Get the owning player pin */
	UEdGraphPin* GetOwningPlayerPin() const;

protected:
	/** Gets the default node title when no class is selected */
	virtual FText GetBaseNodeTitle() const;
	/** Gets the node title when a class has been selected. */
	virtual FText GetNodeTitleFormat() const;
	/** Gets base class to use for the 'class' pin.  UObject by default. */
	virtual UClass* GetClassPinBaseClass() const;
	/**  */
	virtual bool IsSpawnVarPin(UEdGraphPin* Pin) override;
};
