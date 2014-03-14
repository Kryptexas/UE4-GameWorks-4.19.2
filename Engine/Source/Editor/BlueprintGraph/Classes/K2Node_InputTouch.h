// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_InputTouch.generated.h"

UCLASS(MinimalAPI)
class UK2Node_InputTouch : public UK2Node
{
	GENERATED_UCLASS_BODY()

	// Prevents actors with lower priority from handling this input
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bConsumeInput:1;

	// Should the binding execute even when the game is paused
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bExecuteWhenPaused:1;

	// Should any bindings to this event in parent classes be removed
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bOverrideParentBinding:1;

	// Begin UObject interface
	virtual void PostLoad() OVERRIDE;
	// End UObject interface

	// Begin UK2Node interface.
	virtual bool ShouldShowNodeProperties() const OVERRIDE { return true; }
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) OVERRIDE;
	// End UK2Node interface

	// Begin UEdGraphNode interface.
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE{ return TEXT("GraphEditor.TouchEvent_16x"); }
	// End UEdGraphNode interface.

	BLUEPRINTGRAPH_API static UEnum* GetTouchIndexEnum();

	/** Get the 'pressed' input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetPressedPin() const;

	/** Get the 'released' input pin */
	BLUEPRINTGRAPH_API UEdGraphPin* GetReleasedPin() const;

	BLUEPRINTGRAPH_API UEdGraphPin* GetLocationPin() const;
	BLUEPRINTGRAPH_API UEdGraphPin* GetFingerIndexPin() const;
	
};