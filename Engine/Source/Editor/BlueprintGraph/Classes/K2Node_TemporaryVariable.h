// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node_TemporaryVariable.generated.h"

UCLASS(MinimalAPI)
class UK2Node_TemporaryVariable : public UK2Node
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	struct FEdGraphPinType VariableType;

	/** Whether or not this variable should be flagged with CPF_SaveGame, and inherit its name from the GUID of the macro that that gave rise to it */
	UPROPERTY()
	bool bIsPersistent;

	// get variable pin
	BLUEPRINTGRAPH_API UEdGraphPin* GetVariablePin();

#if WITH_EDITOR
	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetDescriptiveCompiledName() const OVERRIDE;
	// End UEdGraphNode interface.

	// Begin UK2Node interface.
	virtual bool IsNodePure() const OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End UK2Node interface.
#endif
};

