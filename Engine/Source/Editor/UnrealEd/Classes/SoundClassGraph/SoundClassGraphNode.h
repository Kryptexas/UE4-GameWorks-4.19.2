// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SoundClassGraphNode.generated.h"

UCLASS(MinimalAPI)
class USoundClassGraphNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

	/** The SoundNode this represents */
	UPROPERTY(VisibleAnywhere, instanced, Category=Sound)
	USoundClass*		SoundClass;

	/** Get the Pin that connects to all children */
	UEdGraphPin* GetChildPin() const { return ChildPin; }
	/** Get the Pin that connects to its parent */
	UEdGraphPin* GetParentPin() const { return ParentPin; }
	/** Check whether the children of this node match the SoundClass it is representing */
	bool CheckRepresentsSoundClass();

	// Begin UEdGraphNode interface.
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void AutowireNewNode(UEdGraphPin* FromPin) OVERRIDE;
	virtual bool CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool CanUserDeleteNode() const OVERRIDE;
	// End UEdGraphNode interface.

private:
	/** Pin that connects to all children */
	UEdGraphPin* ChildPin;
	/** Pin that connects to its parent */
	UEdGraphPin* ParentPin;
};
