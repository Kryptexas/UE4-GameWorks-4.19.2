// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_EnumLiteral.generated.h"

UCLASS(MinimalAPI)
class UK2Node_EnumLiteral : public UK2Node, public INodeDependingOnEnumInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	UEnum* Enum;

	static const FString& GetEnumInputPinName();

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool IsNodePure() const OVERRIDE { return true; }
	virtual FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End UK2Node interface

	// INodeDependingOnEnumInterface
	virtual class UEnum* GetEnum() const OVERRIDE { return Enum; }
	virtual bool ShouldBeReconstructedAfterEnumChanged() const OVERRIDE { return true; }
	// End of INodeDependingOnEnumInterface
};

