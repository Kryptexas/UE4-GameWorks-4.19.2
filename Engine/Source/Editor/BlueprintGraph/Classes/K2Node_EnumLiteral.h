// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node.h"
#include "NodeDependingOnEnumInterface.h"
#include "K2Node_EnumLiteral.generated.h"

UCLASS(MinimalAPI)
class UK2Node_EnumLiteral : public UK2Node, public INodeDependingOnEnumInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	UEnum* Enum;

	static const FString& GetEnumInputPinName();

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FString GetTooltip() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool IsNodePure() const override { return true; }
	virtual FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const override;
	// End UK2Node interface

	// INodeDependingOnEnumInterface
	virtual class UEnum* GetEnum() const override { return Enum; }
	virtual bool ShouldBeReconstructedAfterEnumChanged() const override { return true; }
	// End of INodeDependingOnEnumInterface
};

