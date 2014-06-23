// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_StructOperation.h"
#include "K2Node_StructMemberGet.generated.h"

// Pure kismet node that gets one or more member variables of a struct
UCLASS(MinimalAPI)
class UK2Node_StructMemberGet : public UK2Node_StructOperation
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=PinOptions, EditFixedSize)
	TArray<FOptionalPinFromProperty> ShowPinForProperties;

	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FString GetTooltip() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual bool IsNodePure() const override { return true; }
	virtual bool NodeCausesStructuralBlueprintChange() const override { return true; }
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;
	// End of UK2Node interface

	// AllocateDefaultPins with just one member set
	BLUEPRINTGRAPH_API void AllocatePinsForSingleMemberGet(FName MemberName);
};

