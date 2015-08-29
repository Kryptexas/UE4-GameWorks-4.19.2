// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "SSequencerTrackArea.h"
#include "TimeSliderController.h"

void SSequencerTrackArea::Construct( const FArguments& InArgs, TSharedRef<FSequencerTimeSliderController> InTimeSliderController )
{
	TimeSliderController = InTimeSliderController;
	SVerticalBox::Construct( InArgs );
}

FReply SSequencerTrackArea::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return TimeSliderController->OnMouseButtonDown( SharedThis(this), MyGeometry, MouseEvent );
}

FReply SSequencerTrackArea::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return TimeSliderController->OnMouseButtonUp( SharedThis(this),  MyGeometry, MouseEvent );
}

FReply SSequencerTrackArea::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return TimeSliderController->OnMouseMove( SharedThis(this), MyGeometry, MouseEvent );
}

FReply SSequencerTrackArea::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return TimeSliderController->OnMouseWheel( SharedThis(this), MyGeometry, MouseEvent );
}

void SSequencerTrackArea::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	FVector2D Size = AllottedGeometry.GetLocalSize();

	if (SizeLastFrame.IsSet() && Size.X != SizeLastFrame->X)
	{
		// Zoom by the difference in horizontal size
		const float Difference = Size.X - SizeLastFrame->X;
		TRange<float> OldRange = TimeSliderController->GetViewRange().GetAnimationTarget();

		TimeSliderController->SetViewRange(
			OldRange.GetLowerBoundValue(),
			OldRange.GetUpperBoundValue() + (Difference * OldRange.Size<float>() / SizeLastFrame->X),
			EViewRangeInterpolation::Immediate
			);
	}

	SizeLastFrame = Size;
}