// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_StructMemberGet.h"
#include "K2Node_BreakStruct.generated.h"

UCLASS(MinimalAPI)
class UK2Node_BreakStruct : public UK2Node_StructMemberGet
{
	GENERATED_UCLASS_BODY()

	static bool CanBeBroken(const UScriptStruct* Struct);

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FString GetTooltip() const override;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override{ return TEXT("GraphEditor.BreakStruct_16x"); }
	// End  UEdGraphNode interface

	// Begin K2Node interface
	virtual bool IsNodePure() const override { return true; }
	virtual bool DrawNodeAsVariable() const override { return false; }
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const override;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;
	// End K2Node interface
};

