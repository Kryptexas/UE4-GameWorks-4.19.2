// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_CastByteToEnum.generated.h"

UCLASS(MinimalAPI)
class UK2Node_CastByteToEnum : public UK2Node
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	UEnum* Enum;

	/* if true, the node returns always a valid value */
	UPROPERTY()
	bool bSafe;

	static const FString ByteInputPinName;

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.Enum_16x"); }
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual FString GetCompactNodeTitle() const OVERRIDE;
	virtual bool ShouldDrawCompact() const OVERRIDE { return true; }
	virtual bool IsNodePure() const OVERRIDE { return true; }
	FNodeHandlingFunctor* CreateNodeHandler(FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End UK2Node interface

	virtual FName GetFunctionName() const;
};

