// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_Event.h"
#include "K2Node_InputAxisKeyEvent.generated.h"

UCLASS(MinimalAPI)
class UK2Node_InputAxisKeyEvent : public UK2Node_Event
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FKey AxisKey;

	// Prevents actors with lower priority from handling this input
	UPROPERTY(EditAnywhere, Category = "Input")
	uint32 bConsumeInput : 1;

	// Should the binding execute even when the game is paused
	UPROPERTY(EditAnywhere, Category = "Input")
	uint32 bExecuteWhenPaused : 1;

	// Should any bindings to this event in parent classes be removed
	UPROPERTY(EditAnywhere, Category = "Input")
	uint32 bOverrideParentBinding : 1;

	// Begin EdGraphNode interface
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetTooltip() const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override;
	virtual bool CanPasteHere(const UEdGraph* TargetGraph, const UEdGraphSchema* Schema) const override;
	// End EdGraphNode interface

	// Begin UK2Node interface
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	virtual bool ShouldShowNodeProperties() const override{ return true; }
	virtual UClass* GetDynamicBindingClass() const override;
	virtual void RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const override;
	// End UK2Node interface

	void Initialize(const FKey AxisKey);
};