// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node_CallFunction.h"
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
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetTooltip() const override;
	virtual FName GetPaletteIcon(FLinearColor& OutColor) const override;
	// End EdGraphNode interface

	// Begin UK2Node interface
	virtual void ValidateNodeDuringCompilation(class FCompilerResultsLog& MessageLog) const override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual UClass* GetDynamicBindingClass() const override;
	virtual void RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const override;
	// End UK2Node interface
	
	void Initialize(const FKey AxisKey);
};