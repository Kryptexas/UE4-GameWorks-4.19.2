// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_FunctionResult.generated.h"

UCLASS(MinimalAPI)
class UK2Node_FunctionResult : public UK2Node_FunctionTerminator
{
	GENERATED_UCLASS_BODY()


#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool CanUserDeleteNode() const OVERRIDE { return false; }
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool DrawNodeAsExit() const OVERRIDE { return true; }
	virtual bool ShouldShowNodeProperties() const OVERRIDE { return true; }
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End UK2Node interface

	// Begin K2Node_FunctionTerminator interface
	virtual UEdGraphPin* CreatePinFromUserDefinition(const TSharedPtr<FUserPinInfo> NewPinInfo) OVERRIDE;
	// End K2Node_FunctionTerminator interface
#endif
};

