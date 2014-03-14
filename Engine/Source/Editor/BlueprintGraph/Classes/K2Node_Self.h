// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_Self.generated.h"

UCLASS(MinimalAPI)
class UK2Node_Self : public UK2Node
{
	GENERATED_UCLASS_BODY()

	/** Class that this variable is defined in.  */
	UPROPERTY(transient)
	TSubclassOf<class UObject>  SelfClass;

	

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetKeywords() const OVERRIDE;
	virtual void AllocateDefaultPins() OVERRIDE;
	// End UEdGraphNode interface

	// Begin K2Node interface
	virtual bool IsNodePure() const OVERRIDE { return true; }
	virtual bool DrawNodeAsVariable() const OVERRIDE { return true; }
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End K2Node interface
#endif
};

