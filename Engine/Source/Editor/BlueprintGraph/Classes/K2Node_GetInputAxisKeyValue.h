// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_GetInputAxisKeyValue.generated.h"

UCLASS(MinimalAPI)
class UK2Node_GetInputAxisKeyValue : public UK2Node_CallFunction
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FKey InputAxisKey;

	// Prevents actors with lower priority from handling this input
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bConsumeInput:1;

	// Should the binding gather input even when the game is paused
	UPROPERTY(EditAnywhere, Category="Input")
	uint32 bExecuteWhenPaused:1;

	// Begin EdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const OVERRIDE;
	// End EdGraphNode interface

	// Begin UK2Node interface
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const OVERRIDE;
	virtual bool ShouldShowNodeProperties() const OVERRIDE { return true; }
	virtual UClass* GetDynamicBindingClass() const OVERRIDE;
	virtual void RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const OVERRIDE;
	// End UK2Node interface
	
	void Initialize(const FKey AxisKey);
};