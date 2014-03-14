// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_BaseAsyncTask.generated.h"

UCLASS(Abstract)
class BLUEPRINTGRAPH_API UK2Node_BaseAsyncTask : public UK2Node
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	virtual bool HasExternalBlueprintDependencies(TArray<class UStruct*>* OptionalOutput) const OVERRIDE;
	// End UK2Node interface

	FName	GetProxyFactoryFunctionName();
	UClass*	GetProxyFactoryClass();
	UClass* GetProxyClass();
	virtual FString GetCategoryName();

protected:
	FName ProxyFactoryFunctionName;
	UClass* ProxyFactoryClass;
	UClass* ProxyClass;
};
