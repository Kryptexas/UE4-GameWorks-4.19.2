// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "WidgetBlueprintLibrary.generated.h"

UCLASS(MinimalAPI)
class UWidgetBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/** Creates a widget */
	UFUNCTION(BlueprintCallable, meta=( HidePin="WorldContextObject", DefaultToSelf="WorldContextObject", UnsafeDuringActorConstruction = "true", FriendlyName = "Create Widget" ), Category="User Interface|Widget")
	static class UUserWidget* Create(UObject* WorldContextObject, TSubclassOf<class UUserWidget> WidgetType);

	/** Draws a box */
	UFUNCTION(BlueprintCallable, Category="Painting")
	static void DrawBox(UPARAM(ref) FPaintContext& Context, FVector2D Position, FVector2D Size, USlateBrushAsset* Brush, FLinearColor Tint = FLinearColor::White);

	/**
	 * Draws a line.
	 *
	 * @param PositionA		Starting position of the line in local space.
	 * @param PositionB		Ending position of the line in local space.
	 * @param Thickness				How many pixels thick this line should be.
	 * @param Tint			Color to render the line.
	 */
	UFUNCTION(BlueprintCallable, meta=( AdvancedDisplay = "5" ), Category="Painting" )
	static void DrawLine(UPARAM(ref) FPaintContext& Context, FVector2D PositionA, FVector2D PositionB, float Thickness = 1.0f, FLinearColor Tint = FLinearColor::White, bool bAntiAlias = true);

	// TODO UMG DrawLines

	/** 
	 * Draws text.
	 *
	 * @param InString		The string to draw.
	 * @param Position		The starting position where the text is drawn in local space.
	 * @param Tint			Color to render the line.
	 */
	UFUNCTION(BlueprintCallable, Category="Painting")
	static void DrawText(UPARAM(ref) FPaintContext& Context, const FString& InString, FVector2D Position, FLinearColor Tint = FLinearColor::White);

};
