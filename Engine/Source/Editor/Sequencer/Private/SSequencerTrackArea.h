// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SSequencerOutlinerArea;
class FSequencerTimeSliderController;

/**
 * The area where tracks( rows of sections ) are displayed
 */
class SSequencerTrackArea : public SVerticalBox
{
public:
	DECLARE_DELEGATE_OneParam(FOnValueChanged, float)


	void Construct( const FArguments& InArgs, TSharedRef<FSequencerTimeSliderController> InTimeSliderController );
	
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;

	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;

private:

	/** Time slider controller for controlling zoom/pan etc */
	TSharedPtr<class FSequencerTimeSliderController> TimeSliderController;

	/** Keep a hold of the size of the area so we can maintain zoom levels */
	TOptional<FVector2D> SizeLastFrame;
};
