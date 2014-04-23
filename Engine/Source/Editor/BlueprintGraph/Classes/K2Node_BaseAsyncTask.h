// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "EdGraphSchema_K2_Actions.h"
#include "K2Node_BaseAsyncTask.generated.h"

UCLASS(Abstract)
class BLUEPRINTGRAPH_API UK2Node_BaseAsyncTask : public UK2Node
{
	GENERATED_UCLASS_BODY()

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	virtual bool HasExternalBlueprintDependencies(TArray<class UStruct*>* OptionalOutput) const OVERRIDE;
	virtual FName GetCornerIcon() const OVERRIDE;
	// End of UK2Node interface

protected:
	virtual FString GetCategoryName();

	// Creates a default menu entry
	TSharedPtr<FEdGraphSchemaAction_K2NewNode> CreateDefaultMenuEntry(UK2Node_BaseAsyncTask* NodeTemplate, FGraphContextMenuBuilder& ContextMenuBuilder) const;

	// Returns the factory function (checked)
	UFunction* GetFactoryFunction() const;

protected:
	// The name of the function to call to create a proxy object
	UPROPERTY()
	FName ProxyFactoryFunctionName;

	// The class containing the proxy object functions
	UPROPERTY()
	UClass* ProxyFactoryClass;

	// The type of proxy object that will be created
	UPROPERTY()
	UClass* ProxyClass;

	// The name of the 'go' function on the proxy object that will be called after delegates are in place, can be NAME_None
	UPROPERTY()
	FName ProxyActivateFunctionName;
};
