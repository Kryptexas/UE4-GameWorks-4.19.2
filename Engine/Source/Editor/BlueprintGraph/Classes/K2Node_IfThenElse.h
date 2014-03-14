// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_IfThenElse.generated.h"

UCLASS(MinimalAPI)
class UK2Node_IfThenElse : public UK2Node
{
	GENERATED_UCLASS_BODY()


#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetKeywords() const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.Branch_16x"); }
	// End UEdGraphNode interface

	// Begin K2Node interface.
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End K2Node interface.

	/** Get the then output pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetThenPin() const;
	/** Get the return value pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetElsePin() const;
	/** Get the condition pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetConditionPin() const;
#endif 
};

