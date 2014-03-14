// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
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
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.Macro.Loop_16x"); }
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool IsNodePure() const OVERRIDE { return false; }
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	// End UK2Node interface
};

