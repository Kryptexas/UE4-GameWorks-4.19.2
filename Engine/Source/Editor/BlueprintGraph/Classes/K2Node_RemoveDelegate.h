// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "K2Node_RemoveDelegate.generated.h"

UCLASS(MinimalAPI)
class UK2Node_RemoveDelegate : public UK2Node_BaseMCDelegate
{
	GENERATED_UCLASS_BODY()

public:
	// UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End of UEdGraphNode interface

	// Begin of UK2Node interface
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End of UK2Node interface
};
