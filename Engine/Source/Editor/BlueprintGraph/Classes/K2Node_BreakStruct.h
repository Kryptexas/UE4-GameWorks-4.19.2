// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "K2Node_BreakStruct.generated.h"

UCLASS(MinimalAPI)
class UK2Node_BreakStruct : public UK2Node_StructMemberGet
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	static bool CanBeBroken(const UScriptStruct* Struct);

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.BreakStruct_16x"); }
	// End  UEdGraphNode interface

	// Begin K2Node interface
	virtual bool IsNodePure() const OVERRIDE { return true; }
	virtual bool DrawNodeAsVariable() const OVERRIDE { return false; }
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const OVERRIDE;
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	// End K2Node interface

#endif //WITH_EDITOR
};

