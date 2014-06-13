// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node.h"
#include "K2Node_ForEachElementInEnum.generated.h"

UCLASS(MinimalAPI)
class UK2Node_ForEachElementInEnum : public UK2Node
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	UEnum* Enum;

	static const FString InsideLoopPinName;
	static const FString EnumOuputPinName;

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FString GetTooltip() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetNodeNativeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override{ return TEXT("GraphEditor.Macro.Loop_16x"); }
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool IsNodePure() const override { return false; }
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	// End UK2Node interface
};

