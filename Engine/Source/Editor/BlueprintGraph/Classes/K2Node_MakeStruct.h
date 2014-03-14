// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_MakeStruct.generated.h"

// Pure kismet node that creates a struct with specified values for each member
UCLASS(MinimalAPI)
class UK2Node_MakeStruct : public UK2Node_StructMemberSet
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	BLUEPRINTGRAPH_API static bool CanBeMade(const UScriptStruct* Struct);
	static bool CanBeExposed(const UProperty* Property);

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.MakeStruct_16x"); }
	// End  UEdGraphNode interface

	// Begin K2Node interface
	virtual bool IsNodePure() const OVERRIDE { return true; }
	virtual bool DrawNodeAsVariable() const OVERRIDE { return false; }
	virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const OVERRIDE;
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex)  const OVERRIDE;
	// End K2Node interface
#endif
};
