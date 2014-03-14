// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_VariableSet.generated.h"

UCLASS(MinimalAPI)
class UK2Node_VariableSet : public UK2Node_Variable
{
	GENERATED_UCLASS_BODY()


#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End UEdGraphNode interface

	// Begin K2Node interface
	virtual bool ShouldShowNodeProperties() const OVERRIDE { return true; }
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End K2Node interface


	BLUEPRINTGRAPH_API bool HasLocalRepNotify() const;
	BLUEPRINTGRAPH_API FName GetRepNotifyName() const;
	BLUEPRINTGRAPH_API bool ShouldFlushDormancyOnSet() const;


#endif
};

