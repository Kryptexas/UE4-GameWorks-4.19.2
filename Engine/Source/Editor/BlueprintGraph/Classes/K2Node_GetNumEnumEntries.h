// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_GetNumEnumEntries.generated.h"

UCLASS(MinimalAPI)
class UK2Node_GetNumEnumEntries : public UK2Node, public INodeDependingOnEnumInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	UEnum* Enum;

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.Enum_16x"); }
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual bool IsNodePure() const OVERRIDE { return true; }
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End UK2Node interface

	// INodeDependingOnEnumInterface
	virtual class UEnum* GetEnum() const OVERRIDE { return Enum; }
	virtual bool ShouldBeReconstructedAfterEnumChanged() const OVERRIDE {return false;}
	// End of INodeDependingOnEnumInterface
};

