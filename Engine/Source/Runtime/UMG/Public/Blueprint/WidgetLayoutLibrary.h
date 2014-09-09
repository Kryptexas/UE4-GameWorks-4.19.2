// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetLayoutLibrary.generated.h"

UCLASS(MinimalAPI)
class UWidgetLayoutLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/**  */
	UFUNCTION(BlueprintPure, Category="Viewport", meta=( HidePin="WorldContextObject", DefaultToSelf="WorldContextObject" ))
	static float GetViewportScale(UObject* WorldContextObject);

	/**  */
	UFUNCTION(BlueprintPure, Category="Viewport", meta=( HidePin="WorldContextObject", DefaultToSelf="WorldContextObject" ))
	static FVector2D GetViewportSize(UObject* WorldContextObject);

	/**
	 * Gets the slot object on the child widget as a Canvas Slot, allowing you to manipulate layout information.
	 * @param Widget The child widget of a canvas panel.
	 */
	UFUNCTION(BlueprintPure, Category="Slot")
	static UCanvasPanelSlot* SlotAsCanvasSlot(UWidget* Widget);

	/**
	 * Gets the slot object on the child widget as a Grid Slot, allowing you to manipulate layout information.
	 * @param Widget The child widget of a grid panel.
	 */
	UFUNCTION(BlueprintPure, Category="Slot")
	static UGridSlot* SlotAsGridSlot(UWidget* Widget);

	/**
	 * Gets the slot object on the child widget as a Uniform Grid Slot, allowing you to manipulate layout information.
	 * @param Widget The child widget of a uniform grid panel.
	 */
	UFUNCTION(BlueprintPure, Category="Slot")
	static UUniformGridSlot* SlotAsUniformGridSlot(UWidget* Widget);
};
