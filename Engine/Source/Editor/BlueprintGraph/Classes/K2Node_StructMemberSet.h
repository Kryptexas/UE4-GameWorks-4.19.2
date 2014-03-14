// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_StructMemberSet.generated.h"

// Imperative kismet node that sets one or more member variables of a struct
UCLASS(MinimalAPI)
class UK2Node_StructMemberSet : public UK2Node_StructOperation
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=PinOptions, EditFixedSize)
	TArray<FOptionalPinFromProperty> ShowPinForProperties;

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End of UEdGraphNode interface

	// UK2Node interface
	virtual bool IsNodePure() const OVERRIDE { return false; }
	virtual bool NodeCausesStructuralBlueprintChange() const OVERRIDE { return true; }
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex)  const OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End of UK2Node interface
#endif
};

