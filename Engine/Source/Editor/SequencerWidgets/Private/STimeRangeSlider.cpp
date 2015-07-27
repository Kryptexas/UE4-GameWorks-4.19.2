// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerWidgetsPrivatePCH.h"
#include "STimeRangeSlider.h"
#include "SlateStyle.h"
#include "EditorStyle.h"

#define LOCTEXT_NAMESPACE "STimeRangeSlider"

namespace TimeRangeSliderConstants
{
	const int32 HandleSize = 14;
}

void STimeRangeSlider::Construct( const FArguments& InArgs, TSharedRef<ITimeSliderController> InTimeSliderController)
{
	TimeSliderController = InTimeSliderController;
	LastViewRange.Empty();

	ResetState();
	ResetHoveredState();
}

float STimeRangeSlider::ComputeDragDelta(const FPointerEvent& MouseEvent, int32 GeometryWidth) const
{
	float StartTime = 0;
	float EndTime = 0;

	if (TimeSliderController.IsValid())
	{
		StartTime = TimeSliderController.Get()->GetClampRange().GetLowerBoundValue();
		EndTime = TimeSliderController.Get()->GetClampRange().GetUpperBoundValue();
	}
	float DragDistance = (MouseEvent.GetScreenSpacePosition() - MouseDownPosition).X;

	return DragDistance / (float)GeometryWidth * (EndTime - StartTime);
}

void STimeRangeSlider::ComputeHandleOffsets(float& LeftHandleOffset, float& HandleOffset, float& RightHandleOffset, int32 GeometryWidth) const
{
	float StartTime = 0;
	float InTime = 0;
	float OutTime = 0;
	float EndTime = 0;

	if (TimeSliderController.IsValid())
	{
		StartTime = TimeSliderController.Get()->GetClampRange().GetLowerBoundValue();
		InTime = TimeSliderController.Get()->GetViewRange().GetLowerBoundValue();
		OutTime = TimeSliderController.Get()->GetViewRange().GetUpperBoundValue();
		EndTime = TimeSliderController.Get()->GetClampRange().GetUpperBoundValue();
	}

	LeftHandleOffset = (InTime - StartTime) / (EndTime - StartTime) * (float)GeometryWidth;
	HandleOffset = LeftHandleOffset + TimeRangeSliderConstants::HandleSize;
	RightHandleOffset = (OutTime - StartTime) / (EndTime - StartTime) * (float)GeometryWidth - TimeRangeSliderConstants::HandleSize;
}

int32 STimeRangeSlider::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	const int32 BackgroundLayer = LayerId+1;
	const int32 SliderBoxLayer = BackgroundLayer+1;
	const int32 HandleLayer = SliderBoxLayer+1;

	static const FSlateBrush* RangeHandleLeft = FEditorStyle::GetBrush( TEXT( "Sequencer.Timeline.RangeHandleLeft" ) ); 
	static const FSlateBrush* RangeHandleRight = FEditorStyle::GetBrush( TEXT( "Sequencer.Timeline.RangeHandleRight" ) ); 
	static const FSlateBrush* RangeHandle = FEditorStyle::GetBrush( TEXT( "Sequencer.Timeline.RangeHandle" ) ); 

	float LeftHandleOffset = 0.f;
	float HandleOffset = 0.f;
	float RightHandleOffset = 0.f;
	ComputeHandleOffsets(LeftHandleOffset, HandleOffset, RightHandleOffset, AllottedGeometry.Size.X);

	static const FName SelectionColorName("SelectionColor");
	FLinearColor SelectionColor = FEditorStyle::GetSlateColor(SelectionColorName).GetColor(FWidgetStyle());

	// Draw the left handle box
	FSlateDrawElement::MakeBox( 
		OutDrawElements,
		LayerId, 
		AllottedGeometry.ToPaintGeometry(FVector2D(LeftHandleOffset, 0.0f), FVector2D(TimeRangeSliderConstants::HandleSize, TimeRangeSliderConstants::HandleSize)),
		RangeHandleLeft,
		MyClippingRect,
		ESlateDrawEffect::None,
		(bLeftHandleDragged || bLeftHandleHovered) ? SelectionColor : FLinearColor::Gray);

	// Draw the right handle box
	FSlateDrawElement::MakeBox( 
		OutDrawElements,
		LayerId, 
		AllottedGeometry.ToPaintGeometry(FVector2D(RightHandleOffset, 0.0f), FVector2D(TimeRangeSliderConstants::HandleSize, TimeRangeSliderConstants::HandleSize)),
		RangeHandleRight,
		MyClippingRect,
		ESlateDrawEffect::None,
		(bRightHandleDragged || bRightHandleHovered) ? SelectionColor : FLinearColor::Gray);

	// Draw the handle box
	FSlateDrawElement::MakeBox( 
		OutDrawElements,
		LayerId, 
		AllottedGeometry.ToPaintGeometry(FVector2D(HandleOffset, 0.0f), FVector2D(RightHandleOffset-LeftHandleOffset-TimeRangeSliderConstants::HandleSize, TimeRangeSliderConstants::HandleSize)),
		RangeHandle,
		MyClippingRect,
		ESlateDrawEffect::None,
		(bHandleDragged || bHandleHovered) ? SelectionColor : FLinearColor::Gray);

	SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, ShouldBeEnabled( bParentEnabled ));

	return LayerId;
}

FReply STimeRangeSlider::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	MouseDownPosition = MouseEvent.GetScreenSpacePosition();
	if (TimeSliderController.IsValid())
	{		
		MouseDownViewRange = TimeSliderController.Get()->GetViewRange();
	}

	if (bHandleHovered)
	{
		bHandleDragged = true;
		return FReply::Handled().CaptureMouse(AsShared());
	}
	else if (bLeftHandleHovered)
	{
		bLeftHandleDragged = true;
		return FReply::Handled().CaptureMouse(AsShared());
	}
	else if (bRightHandleHovered)
	{
		bRightHandleDragged = true;
		return FReply::Handled().CaptureMouse(AsShared());
	}

	return FReply::Unhandled();
}

FReply STimeRangeSlider::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	ResetState();

	return FReply::Handled().ReleaseMouseCapture();
}

FReply STimeRangeSlider::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if (HasMouseCapture())
	{
		float DragDelta = ComputeDragDelta(MouseEvent, MyGeometry.Size.X);

		if (bHandleDragged)
		{
			if (TimeSliderController.IsValid())
			{
				float NewIn = MouseDownViewRange.GetLowerBoundValue() + DragDelta;
				float NewOut = MouseDownViewRange.GetUpperBoundValue() + DragDelta;

				TRange<float> ClampRange = TimeSliderController.Get()->GetClampRange();
				if (NewIn < ClampRange.GetLowerBoundValue())
				{
					NewIn = ClampRange.GetLowerBoundValue();
					NewOut = NewIn + (MouseDownViewRange.GetUpperBoundValue() - MouseDownViewRange.GetLowerBoundValue());
				}				
				else if (NewOut > ClampRange.GetUpperBoundValue())
				{
					NewOut = ClampRange.GetUpperBoundValue();
					NewIn = NewOut - (MouseDownViewRange.GetUpperBoundValue() - MouseDownViewRange.GetLowerBoundValue());
				}
				
				TimeSliderController.Get()->SetViewRange(NewIn, NewOut, EViewRangeInterpolation::Immediate);
			}
		}
		else if (bLeftHandleDragged)
		{
			if (TimeSliderController.IsValid())
			{
				float NewIn = MouseDownViewRange.GetLowerBoundValue() + DragDelta;
				TRange<float> ClampRange = TimeSliderController.Get()->GetClampRange();
				NewIn = FMath::Clamp(NewIn, ClampRange.GetLowerBoundValue(), ClampRange.GetUpperBoundValue());
				TimeSliderController.Get()->SetViewRange(NewIn, TimeSliderController.Get()->GetViewRange().GetUpperBoundValue(), EViewRangeInterpolation::Immediate);
			}
		}
		else if (bRightHandleDragged)
		{
			if (TimeSliderController.IsValid())
			{
				float NewOut = MouseDownViewRange.GetUpperBoundValue() + DragDelta;
				TRange<float> ClampRange = TimeSliderController.Get()->GetClampRange();
				NewOut = FMath::Clamp(NewOut, ClampRange.GetLowerBoundValue(), ClampRange.GetUpperBoundValue());
				TimeSliderController.Get()->SetViewRange(TimeSliderController.Get()->GetViewRange().GetLowerBoundValue(), NewOut, EViewRangeInterpolation::Immediate);
			}
		}

		return FReply::Handled();
	}
	else
	{
		ResetHoveredState();

		float LeftHandleOffset = 0.f;
		float HandleOffset = 0.f;
		float RightHandleOffset = 0.f;
		ComputeHandleOffsets(LeftHandleOffset, HandleOffset, RightHandleOffset, MyGeometry.Size.X);
		
		FGeometry LeftHandleRect = MyGeometry.MakeChild(FVector2D(LeftHandleOffset, 0.f), FVector2D(TimeRangeSliderConstants::HandleSize, TimeRangeSliderConstants::HandleSize));
		FGeometry RightHandleRect = MyGeometry.MakeChild(FVector2D(RightHandleOffset, 0.f), FVector2D(TimeRangeSliderConstants::HandleSize, TimeRangeSliderConstants::HandleSize));
		FGeometry HandleRect = MyGeometry.MakeChild(FVector2D(HandleOffset, 0.0f), FVector2D(RightHandleOffset-LeftHandleOffset-TimeRangeSliderConstants::HandleSize, TimeRangeSliderConstants::HandleSize));

		FVector2D LocalMousePosition = MouseEvent.GetScreenSpacePosition();

		if (HandleRect.IsUnderLocation(LocalMousePosition))
		{
			bHandleHovered = true;
		}
		else if (LeftHandleRect.IsUnderLocation(LocalMousePosition))
		{
			bLeftHandleHovered = true;
		}
		else if (RightHandleRect.IsUnderLocation(LocalMousePosition))
		{
			bRightHandleHovered = true;
		}
	}
	return FReply::Unhandled();
}

void STimeRangeSlider::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	SCompoundWidget::OnMouseLeave(MouseEvent);

	if (!HasMouseCapture())
	{
		ResetHoveredState();
	}
}

FReply STimeRangeSlider::OnMouseButtonDoubleClick( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	ResetState();

	OnMouseMove(MyGeometry, MouseEvent);

	if (bHandleHovered)
	{
		if (TimeSliderController.IsValid())
		{
			if (FMath::IsNearlyEqual(TimeSliderController.Get()->GetViewRange().GetLowerBoundValue(), TimeSliderController.Get()->GetClampRange().GetLowerBoundValue()) &&
				FMath::IsNearlyEqual(TimeSliderController.Get()->GetViewRange().GetUpperBoundValue(), TimeSliderController.Get()->GetClampRange().GetUpperBoundValue()))
			{
				if (!LastViewRange.IsEmpty())
				{
					TimeSliderController.Get()->SetViewRange(LastViewRange.GetLowerBoundValue(), LastViewRange.GetUpperBoundValue(), EViewRangeInterpolation::Immediate);
				}
			}
			else
			{
				LastViewRange = TRange<float>(TimeSliderController.Get()->GetViewRange().GetLowerBoundValue(), TimeSliderController.Get()->GetViewRange().GetUpperBoundValue());
				TimeSliderController.Get()->SetViewRange(TimeSliderController.Get()->GetClampRange().GetLowerBoundValue(), TimeSliderController.Get()->GetClampRange().GetUpperBoundValue(), EViewRangeInterpolation::Immediate);
			}
		}

		ResetState();
		return FReply::Handled();
	}
	ResetState();
	return FReply::Unhandled();
}

void STimeRangeSlider::ResetState()
{
	bHandleDragged = false;
	bLeftHandleDragged = false;
	bRightHandleDragged = false;
	ResetHoveredState();
}

void STimeRangeSlider::ResetHoveredState()
{
	bHandleHovered = false;
	bLeftHandleHovered = false;
	bRightHandleHovered = false;
}

#undef LOCTEXT_NAMESPACE
