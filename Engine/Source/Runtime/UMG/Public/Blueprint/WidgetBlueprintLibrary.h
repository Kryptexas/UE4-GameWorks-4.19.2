// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "WidgetBlueprintLibrary.generated.h"

UCLASS(MinimalAPI)
class UWidgetBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	/** Creates a widget */
	UFUNCTION(BlueprintCallable, meta=( HidePin="WorldContextObject", DefaultToSelf="WorldContextObject", FriendlyName = "Create Widget" ), Category="User Interface|Widget")
	static class UUserWidget* Create(UObject* WorldContextObject, TSubclassOf<class UUserWidget> WidgetType, APlayerController* OwningPlayer);

	UFUNCTION(BlueprintCallable, Category="Focus")
	static void SetFocusToGameViewport();

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

	UFUNCTION(BlueprintPure, Category="Widget|Event Reply")
	static FEventReply Handled();

	UFUNCTION(BlueprintPure, Category="Widget|Event Reply")
	static FEventReply Unhandled();

	UFUNCTION(BlueprintPure, meta=( HidePin="CapturingWidget", DefaultToSelf="CapturingWidget" ), Category="Widget|Event Reply")
	static FEventReply CaptureMouse(UPARAM(ref) FEventReply& Reply, UWidget* CapturingWidget);

	UFUNCTION(BlueprintPure, Category="Widget|Event Reply")
	static FEventReply ReleaseMouseCapture(UPARAM(ref) FEventReply& Reply);

	UFUNCTION(BlueprintPure, meta=( HidePin="CapturingWidget", DefaultToSelf="CapturingWidget" ), Category="Widget|Event Reply")
	static FEventReply CaptureJoystick(UPARAM(ref) FEventReply& Reply, UWidget* CapturingWidget, bool bInAllJoysticks = false);

	UFUNCTION(BlueprintPure, Category="Widget|Event Reply")
	static FEventReply ReleaseJoystickCapture(UPARAM(ref) FEventReply& Reply, bool bInAllJoysticks = false);

	/**
	 * Ask Slate to detect if a user started dragging in this widget.
	 * If a drag is detected, Slate will send an OnDragDetected event.
	 *
	 * @param WidgetDetectingDrag  Detect dragging in this widget
	 * @param DragKey		       This button should be pressed to detect the drag
	 */
	UFUNCTION(BlueprintPure, meta=( HidePin="WidgetDetectingDrag", DefaultToSelf="WidgetDetectingDrag" ), Category="Widget|Event Reply")
	static FEventReply DetectDrag(UPARAM(ref) FEventReply& Reply, UWidget* WidgetDetectingDrag, FKey DragKey);

	UFUNCTION(BlueprintCallable, meta=( HidePin="WidgetDetectingDrag", DefaultToSelf="WidgetDetectingDrag" ), Category="Widget|Event Reply")
	static FEventReply IfKeyPressedDetectDrag(const FPointerEvent& PointerEvent, UWidget* WidgetDetectingDrag, FKey DragKey);

	/**
	 * An event should return FReply::Handled().EndDragDrop() to request that the current drag/drop operation be terminated.
	 */
	UFUNCTION(BlueprintPure, Category="Widget|Event Reply")
	static FEventReply EndDragDrop(UPARAM(ref) FEventReply& Reply);
};
