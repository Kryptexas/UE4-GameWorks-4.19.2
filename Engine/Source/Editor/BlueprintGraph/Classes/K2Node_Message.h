// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_Message.generated.h"

UCLASS(MinimalAPI)
class UK2Node_Message : public UK2Node_CallFunction
{
	GENERATED_UCLASS_BODY()


	// Begin UEdGraphNode interface.
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End UEdGraphNode interface.

	// Begin K2Node interface.
	virtual bool IsNodeSafeToIgnore() const OVERRIDE { return true; }
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	virtual FName GetCornerIcon() const OVERRIDE;
	// End K2Node interface.

	// Begin K2Node_CallFunction Interface.
	virtual UEdGraphPin* CreateSelfPin(const UFunction* Function) OVERRIDE;

protected:
	virtual void EnsureFunctionIsInBlueprint() OVERRIDE;
	// End K2Node_CallFunction Interface.
};

