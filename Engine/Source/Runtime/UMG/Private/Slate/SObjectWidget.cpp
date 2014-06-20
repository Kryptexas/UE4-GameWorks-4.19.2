// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

void SObjectWidget::Construct(const FArguments& InArgs, UUserWidget* InWidgetObject)
{
	WidgetObject = InWidgetObject;

	ChildSlot
	[
		InArgs._Content.Widget
	];
}

SObjectWidget::~SObjectWidget(void)
{
	printf("");
}

void SObjectWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	printf("");
	Collector.AddReferencedObject(WidgetObject);
}

int32 SObjectWidget::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MaxLayer = SCompoundWidget::OnPaint(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (WidgetObject)
	{
		//const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled
		FPaintContext Context(AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		WidgetObject->OnPaint(Context);
		
		return FMath::Max(MaxLayer, Context.MaxLayer);
	}
	
	return MaxLayer;
}

FReply SObjectWidget::OnKeyboardFocusReceived(const FGeometry& MyGeometry, const FKeyboardFocusEvent& InKeyboardFocusEvent)
{
	return WidgetObject->OnKeyboardFocusReceived(MyGeometry, InKeyboardFocusEvent).ToReply(WidgetObject->GetWidget());
}

void SObjectWidget::OnKeyboardFocusLost(const FKeyboardFocusEvent& InKeyboardFocusEvent)
{
	WidgetObject->OnKeyboardFocusLost(InKeyboardFocusEvent);
}

void SObjectWidget::OnKeyboardFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath)
{
	// TODO UMG
}

FReply SObjectWidget::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	return WidgetObject->OnKeyChar(MyGeometry, InCharacterEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	return WidgetObject->OnPreviewKeyDown(MyGeometry, InKeyboardEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	return WidgetObject->OnKeyDown(MyGeometry, InKeyboardEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnKeyUp(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	return WidgetObject->OnKeyUp(MyGeometry, InKeyboardEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	FReply Reply = WidgetObject->OnMouseButtonDown(MyGeometry, MouseEvent).ToReply(WidgetObject->GetWidget());
	//TODO UMG Figure out how to let the user more easily manage when drags start.
	if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
	{
		return Reply.DetectDrag(WidgetObject->GetWidget(), EKeys::LeftMouseButton);
	}

	return Reply;
}

FReply SObjectWidget::OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return WidgetObject->OnPreviewMouseButtonDown(MyGeometry, MouseEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return WidgetObject->OnMouseButtonUp(MyGeometry, MouseEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return WidgetObject->OnMouseMove(MyGeometry, MouseEvent).ToReply(WidgetObject->GetWidget());
}

void SObjectWidget::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	WidgetObject->OnMouseEnter(MyGeometry, MouseEvent);
}

void SObjectWidget::OnMouseLeave(const FPointerEvent& MouseEvent)
{
	WidgetObject->OnMouseLeave(MouseEvent);
}

FReply SObjectWidget::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return WidgetObject->OnMouseWheel(MyGeometry, MouseEvent).ToReply(WidgetObject->GetWidget());
}

FCursorReply SObjectWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	// TODO UMG  Allow conditional overriding of the cursor logic.
	return FCursorReply::Unhandled();
}

FReply SObjectWidget::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	return WidgetObject->OnMouseButtonDoubleClick(MyGeometry, MouseEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	WidgetObject->OnDragDetected(MyGeometry, MouseEvent);

	//TODO UMG Need to support drag drop, instead of returning the standard FSReply, maybe something else?
	// Maybe like drag data, optional uobject + string data.

	//FUMGDragDropOp
	//.BeginDragDrop(FTrackNodeDragDropOp::New(SharedThis(this), ScreenCursorPos, ScreenNodePosition));

	//FTrackNodeDragDropOp

	// TODO UMG
	return FReply::Unhandled();
}

void SObjectWidget::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// TODO UMG
}

void SObjectWidget::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	// TODO UMG
}

FReply SObjectWidget::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// TODO UMG
	return FReply::Unhandled();
}

FReply SObjectWidget::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	// TODO UMG
	return FReply::Unhandled();
}

FReply SObjectWidget::OnControllerButtonPressed(const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent)
{
	return WidgetObject->OnControllerButtonPressed(MyGeometry, ControllerEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnControllerButtonReleased(const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent)
{
	return WidgetObject->OnControllerButtonReleased(MyGeometry, ControllerEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnControllerAnalogValueChanged(const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent)
{
	return WidgetObject->OnControllerAnalogValueChanged(MyGeometry, ControllerEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnTouchGesture(const FGeometry& MyGeometry, const FPointerEvent& GestureEvent)
{
	return WidgetObject->OnTouchGesture(MyGeometry, GestureEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnTouchStarted(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	return WidgetObject->OnTouchStarted(MyGeometry, InTouchEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnTouchMoved(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	return WidgetObject->OnTouchMoved(MyGeometry, InTouchEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnTouchEnded(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	return WidgetObject->OnTouchEnded(MyGeometry, InTouchEvent).ToReply(WidgetObject->GetWidget());
}

FReply SObjectWidget::OnMotionDetected(const FGeometry& MyGeometry, const FMotionEvent& InMotionEvent)
{
	return WidgetObject->OnMotionDetected(MyGeometry, InMotionEvent).ToReply(WidgetObject->GetWidget());
}
