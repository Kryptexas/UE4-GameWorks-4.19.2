// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerWidgetsPrivatePCH.h"
#include "STimeSlider.h"

#define LOCTEXT_NAMESPACE "STimeSlider"


void STimeSlider::Construct( const STimeSlider::FArguments& InArgs, TSharedRef<ITimeSliderController> InTimeSliderController )
{
	TimeSliderController = InTimeSliderController;
	bMirrorLabels = InArgs._MirrorLabels;
}

int32 STimeSlider::OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const
{
	int32 NewLayer = TimeSliderController->OnPaintTimeSlider( bMirrorLabels, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled );

	return FMath::Max( NewLayer, SCompoundWidget::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, NewLayer, InWidgetStyle, ShouldBeEnabled( bParentEnabled ) ) );
}

FReply STimeSlider::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return TimeSliderController->OnMouseButtonDown( *this, MyGeometry, MouseEvent );
}

FReply STimeSlider::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return TimeSliderController->OnMouseButtonUp( *this,  MyGeometry, MouseEvent );
}

FReply STimeSlider::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return TimeSliderController->OnMouseMove( *this, MyGeometry, MouseEvent );
}

FVector2D STimeSlider::ComputeDesiredSize( float ) const
{
	return FVector2D(100, 22);
}

FReply STimeSlider::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return TimeSliderController->OnMouseWheel( *this, MyGeometry, MouseEvent );
}

FCursorReply STimeSlider::OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) const
{
	return TimeSliderController->OnCursorQuery( SharedThis(this), MyGeometry, CursorEvent );
}

#undef LOCTEXT_NAMESPACE
