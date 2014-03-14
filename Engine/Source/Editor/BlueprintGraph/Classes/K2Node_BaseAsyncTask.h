// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_BaseAsyncTask.generated.h"

UCLASS(Abstract)
class BLUEPRINTGRAPH_API UK2Node_BaseAsyncTask : public UK2Node
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End UK2Node interface

	FName	GetProxyFactoryFunctionName();
	UClass*	GetProxyFactoryClass();
	UClass* GetProxyClass();
	virtual FString GetCategoryName();
#endif

protected:
	FName ProxyFactoryFunctionName;
	UClass* ProxyFactoryClass;
	UClass* ProxyClass;
};
