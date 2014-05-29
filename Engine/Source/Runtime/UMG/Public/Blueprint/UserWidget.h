// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Geometry.h"

#include "UserWidget.generated.h"

//TODO UMG If you want to host a widget that's full screen there may need to be a SWindow equivalent that you spawn it into.

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnVisibilityChangedEvent, ESlateVisibility::Type, Visibility);

UCLASS(Abstract, editinlinenew, hideCategories=(Object, Actor, Replication, Rendering, Input))
class UMG_API UUserWidget : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	TEnumAsByte<ESlateVisibility::Type> Visiblity;

	/** Called when the visibility changes. */
	UPROPERTY(BlueprintAssignable)
	FOnVisibilityChangedEvent OnVisibilityChanged;

	/** Controls whether the cursor is automatically visible when this widget is visible. */
	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	uint32 bShowCursorWhenVisible : 1;

	UPROPERTY(EditDefaultsOnly, Category=Behavior)
	uint32 bModal : 1;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	bool bAbsoluteLayout;

	UPROPERTY(EditAnywhere, Category=Appearance, meta=( EditCondition="!bAbsoluteLayout" ))
	FMargin Padding;

	/** How much space this slot should occupy in the direction of the panel. */
	UPROPERTY(EditAnywhere, Category=Appearance, meta=( EditCondition="!bAbsoluteLayout" ))
	FSlateChildSize Size;

	/** Position. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, meta=( EditCondition="bAbsoluteLayout" ))
	FVector2D AbsolutePosition;

	/** Size. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, meta=( EditCondition="bAbsoluteLayout" ))
	FVector2D AbsoluteSize;

	/**
	* Horizontal pivot position
	*  Given a top aligned slot, where '+' represents the
	*  anchor point defined by PositionAttr.
	*
	*   Left				Center				Right
	+ _ _ _ _            _ _ + _ _          _ _ _ _ +
	|		  |		   | 		   |	  |		    |
	| _ _ _ _ |        | _ _ _ _ _ |	  | _ _ _ _ |
	*
	*  Note: FILL is NOT supported in absolute layout
	*/
	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EHorizontalAlignment> HorizontalAlignment;

	/**
	* Vertical pivot position
	*   Given a left aligned slot, where '+' represents the
	*   anchor point defined by PositionAttr.
	*
	*   Top					Center			  Bottom
	*	+_ _ _ _ _		 _ _ _ _ _		 _ _ _ _ _
	*	|         |		| 		  |		|		  |
	*	|         |     +		  |		|		  |
	*	| _ _ _ _ |		| _ _ _ _ |		+ _ _ _ _ |
	*
	*  Note: FILL is NOT supported in absolute layout
	*/
	UPROPERTY(EditAnywhere, Category=Appearance)
	TEnumAsByte<EVerticalAlignment> VerticalAlignment;

	UPROPERTY()
	TArray<USlateWrapperComponent*> Components;
	
	//UObject interface
	virtual void PostInitProperties() OVERRIDE;
	virtual class UWorld* GetWorld() const OVERRIDE;
	// End of UObject interface

	/*  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void Show();

	/*  */
	UFUNCTION(BlueprintCallable, Category="Appearance")
	void Hide();

	UFUNCTION(BlueprintPure, Category="Appearance")
	bool GetIsVisible();

	UFUNCTION(BlueprintPure, Category="Appearance")
	TEnumAsByte<ESlateVisibility::Type> GetVisiblity();

	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnKeyboardFocusReceived(FGeometry MyGeometry, FKeyboardFocusEvent InKeyboardFocusEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	void OnKeyboardFocusLost(FKeyboardFocusEvent InKeyboardFocusEvent);
	//UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	//void OnKeyboardFocusChanging(FWeakWidgetPath PreviousFocusPath, FWidgetPath NewWidgetPath);

	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnKeyChar(FGeometry MyGeometry, FCharacterEvent InCharacterEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnPreviewKeyDown(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnKeyDown(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnKeyUp(FGeometry MyGeometry, FKeyboardEvent InKeyboardEvent);

	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnPreviewMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnMouseButtonUp(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnMouseMove(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	void OnMouseEnter(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	void OnMouseLeave(const FPointerEvent& MouseEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnMouseWheel(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	//UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	//FCursorReply OnCursorQuery(FGeometry MyGeometry, const FPointerEvent& CursorEvent) const;
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnMouseButtonDoubleClick(FGeometry InMyGeometry, const FPointerEvent& InMouseEvent);

	//UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	//FSReply OnDragDetected(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
	//UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	//void OnDragEnter(FGeometry MyGeometry, FDragDropEvent DragDropEvent);
	//UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	//void OnDragLeave(FDragDropEvent DragDropEvent);
	//UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	//FSReply OnDragOver(FGeometry MyGeometry, FDragDropEvent DragDropEvent);
	//UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	//FSReply OnDrop(FGeometry MyGeometry, FDragDropEvent DragDropEvent);

	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnControllerButtonPressed(FGeometry MyGeometry, FControllerEvent ControllerEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnControllerButtonReleased(FGeometry MyGeometry, FControllerEvent ControllerEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnControllerAnalogValueChanged(FGeometry MyGeometry, FControllerEvent ControllerEvent);

	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnTouchGesture(FGeometry MyGeometry, const FPointerEvent& GestureEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnTouchStarted(FGeometry MyGeometry, const FPointerEvent& InTouchEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnTouchMoved(FGeometry MyGeometry, const FPointerEvent& InTouchEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnTouchEnded(FGeometry MyGeometry, const FPointerEvent& InTouchEvent);
	UFUNCTION(BlueprintImplementableEvent, Category="User Interface")
	FSReply OnMotionDetected(FGeometry MyGeometry, FMotionEvent InMotionEvent);


	USlateWrapperComponent* GetWidgetHandle(TSharedRef<SWidget> InWidget);

	TSharedRef<SWidget> MakeWidget();
	TSharedRef<SWidget> MakeFullScreenWidget();

	USlateWrapperComponent* GetRootWidgetComponent();

	TSharedPtr<SWidget> GetWidgetFromName(const FString& Name) const;
	USlateWrapperComponent* GetHandleFromName(const FString& Name) const;

private:
	TSharedPtr<SWidget> UserRootWidget;
	TMap< TWeakPtr<SWidget>, USlateWrapperComponent* > WidgetToComponent;

	TWeakPtr<SWidget> FullScreenWidget;

private:
	void RebuildWrapperWidget();
};
