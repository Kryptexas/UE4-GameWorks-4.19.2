// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_Variable.h"
#include "K2Node_VariableSet.generated.h"

UCLASS(MinimalAPI)
class UK2Node_VariableSet : public UK2Node_Variable
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual FString GetTooltip() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	// End UEdGraphNode interface

	// Begin K2Node interface
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;
	// End K2Node interface


	BLUEPRINTGRAPH_API bool HasLocalRepNotify() const;
	BLUEPRINTGRAPH_API FName GetRepNotifyName() const;
	BLUEPRINTGRAPH_API bool ShouldFlushDormancyOnSet() const;
};

