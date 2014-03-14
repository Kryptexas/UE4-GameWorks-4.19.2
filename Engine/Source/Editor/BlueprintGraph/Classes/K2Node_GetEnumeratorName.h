// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_GetEnumeratorName.generated.h"

UCLASS(MinimalAPI)
class UK2Node_GetEnumeratorName : public UK2Node
{
	GENERATED_UCLASS_BODY()

	static FString EnumeratorPinName;

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.Enum_16x"); }
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual FString GetCompactNodeTitle() const OVERRIDE;
	virtual bool ShouldDrawCompact() const OVERRIDE { return true; }
	virtual bool IsNodePure() const OVERRIDE { return true; }
	virtual void ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	virtual bool IsConnectionDisallowed(const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const OVERRIDE;
	virtual void EarlyValidation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	virtual void PostReconstructNode() OVERRIDE;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	// End UK2Node interface

	void UpdatePinType();
	class UEnum* GetEnum() const;
	virtual FName GetFunctionName() const;
};

