// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

#include "UMGDragDropOp.h"

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
	if ( WidgetObject )
	{
		WidgetObject->ReleaseNativeWidget();
	}
}

void SObjectWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(WidgetObject);
}

void SObjectWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	if ( WidgetObject && !WidgetObject->IsDesignTime() )
	{
		return WidgetObject->NativeTick(AllottedGeometry, InDeltaTime);
	}
}

//bool SObjectWidget::OnHitTest(const FGeometry& MyGeometry, FVector2D InAbsoluteCursorPosition)
//{
//	if ( WidgetObject && !WidgetObject->IsDesignTime() )
//	{
//	}
//}

int32 SObjectWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	int32 MaxLayer = SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if ( WidgetObject && !WidgetObject->IsDesignTime() )
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
	if ( WidgetObject && !WidgetObject->IsDesignTime() )
	{
		TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
		if ( SlateWidget.IsValid() )
		{
			return WidgetObject->OnKeyboardFocusReceived(MyGeometry, InKeyboardFocusEvent).ToReply(SlateWidget.ToSharedRef());
		}
	}

	return FReply::Unhandled();
}

void SObjectWidget::OnKeyboardFocusLost(const FKeyboardFocusEvent& InKeyboardFocusEvent)
{
	if ( WidgetObject && !WidgetObject->IsDesignTime() )
	{
		WidgetObject->OnKeyboardFocusLost(InKeyboardFocusEvent);
	}
}

void SObjectWidget::OnKeyboardFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath)
{
	// TODO UMG
}

FReply SObjectWidget::OnKeyChar(const FGeometry& MyGeometry, const FCharacterEvent& InCharacterEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnKeyChar(MyGeometry, InCharacterEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnPreviewKeyDown(MyGeometry, InKeyboardEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnKeyDown(MyGeometry, InKeyboardEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnKeyUp(const FGeometry& MyGeometry, const FKeyboardEvent& InKeyboardEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnKeyUp(MyGeometry, InKeyboardEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnMouseButtonDown(MyGeometry, MouseEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnPreviewMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnPreviewMouseButtonDown(MyGeometry, MouseEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnMouseButtonUp(MyGeometry, MouseEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnMouseMove(MyGeometry, MouseEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
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
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnMouseWheel(MyGeometry, MouseEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FCursorReply SObjectWidget::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	// TODO UMG  Allow conditional overriding of the cursor logic.
	return FCursorReply::Unhandled();
}

FReply SObjectWidget::OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnMouseButtonDoubleClick(MyGeometry, MouseEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& PointerEvent)
{
	UDragDropOperation* Operation = NULL;
	WidgetObject->OnDragDetected(MyGeometry, PointerEvent, Operation);

	if ( Operation )
	{
		FVector2D ScreenCursorPos = PointerEvent.GetScreenSpacePosition();
		FVector2D ScreenDrageePosition = MyGeometry.AbsolutePosition;

		return FReply::Handled().BeginDragDrop(FUMGDragDropOp::New(Operation, ScreenCursorPos, ScreenDrageePosition));
	}

	return FReply::Unhandled();
}

void SObjectWidget::OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FUMGDragDropOp> NativeOp = DragDropEvent.GetOperationAs<FUMGDragDropOp>();
	if ( NativeOp.IsValid() )
	{
		WidgetObject->OnDragEnter(MyGeometry, DragDropEvent, NativeOp->GetOperation());
	}
}

void SObjectWidget::OnDragLeave(const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FUMGDragDropOp> NativeOp = DragDropEvent.GetOperationAs<FUMGDragDropOp>();
	if ( NativeOp.IsValid() )
	{
		WidgetObject->OnDragLeave(DragDropEvent, NativeOp->GetOperation());
	}
}

FReply SObjectWidget::OnDragOver(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FUMGDragDropOp> NativeOp = DragDropEvent.GetOperationAs<FUMGDragDropOp>();
	if ( NativeOp.IsValid() )
	{
		if ( WidgetObject->OnDragOver(MyGeometry, DragDropEvent, NativeOp->GetOperation()) )
		{
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TSharedPtr<FUMGDragDropOp> NativeOp = DragDropEvent.GetOperationAs<FUMGDragDropOp>();
	if ( NativeOp.IsValid() )
	{
		if ( WidgetObject->OnDrop(MyGeometry, DragDropEvent, NativeOp->GetOperation()) )
		{
			return FReply::Handled();
		}
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnControllerButtonPressed(const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnControllerButtonPressed(MyGeometry, ControllerEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnControllerButtonReleased(const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnControllerButtonReleased(MyGeometry, ControllerEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnControllerAnalogValueChanged(const FGeometry& MyGeometry, const FControllerEvent& ControllerEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnControllerAnalogValueChanged(MyGeometry, ControllerEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnTouchGesture(const FGeometry& MyGeometry, const FPointerEvent& GestureEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnTouchGesture(MyGeometry, GestureEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnTouchStarted(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnTouchStarted(MyGeometry, InTouchEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnTouchMoved(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnTouchMoved(MyGeometry, InTouchEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnTouchEnded(const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnTouchEnded(MyGeometry, InTouchEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}

FReply SObjectWidget::OnMotionDetected(const FGeometry& MyGeometry, const FMotionEvent& InMotionEvent)
{
	TSharedPtr<SWidget> SlateWidget = WidgetObject->GetCachedWidget();
	if ( SlateWidget.IsValid() )
	{
		return WidgetObject->OnMotionDetected(MyGeometry, InMotionEvent).ToReply(SlateWidget.ToSharedRef());
	}

	return FReply::Unhandled();
}
