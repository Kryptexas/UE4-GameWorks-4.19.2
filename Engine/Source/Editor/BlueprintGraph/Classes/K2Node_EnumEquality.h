// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_EnumEquality.generated.h"

UCLASS(MinimalAPI)
class UK2Node_EnumEquality : public UK2Node
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetKeywords() const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
 	virtual void PostReconstructNode() OVERRIDE;
	virtual bool IsNodeSafeToIgnore() const OVERRIDE { return true; }
	virtual bool IsNodePure() const OVERRIDE { return true; }
	virtual bool ShouldDrawCompact() const OVERRIDE { return true; }
	virtual FString GetCompactNodeTitle() const OVERRIDE { return TEXT("=="); }
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End UK2Node interface

	/** Get the return value pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetReturnValuePin() const;
	/** Get the first input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetInput1Pin() const;
	/** Get the second input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetInput2Pin() const;

	/** Gets the name and class of the EqualEqual_ByteByte function */
	BLUEPRINTGRAPH_API void GetConditionalFunction(FName& FunctionName, UClass** FunctionClass);
#endif
};

