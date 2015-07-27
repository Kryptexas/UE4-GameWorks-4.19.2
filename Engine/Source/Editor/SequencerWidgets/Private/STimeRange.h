// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ITimeSlider.h"

class STimeRangeSlider;

DECLARE_DELEGATE_RetVal( bool, STimeRangeGetter );

class STimeRange : public ITimeSlider
{
public:

	//SLATE_BEGIN_ARGS(STimeRange){}
	//	SLATE_DEFAULT_SLOT(FArguments, Content)
	//SLATE_END_ARGS()
	SLATE_BEGIN_ARGS(STimeRange)
	{}
		/* If we should show frame numbers on the timeline */
		SLATE_ARGUMENT( TAttribute<bool>, ShowFrameNumbers )
	SLATE_END_ARGS()

	/**
	 * Construct the widget
	 * 
	 * @param InArgs   A declaration from which to construct the widget
	 */
	void Construct( const FArguments& InArgs, TSharedRef<ITimeSliderController> InTimeSliderController );

protected:
	FText InTime() const;
	FText OutTime() const;
	FText StartTime() const;
	FText EndTime() const;

	void OnStartTimeCommitted(const FText& NewText, ETextCommit::Type InTextCommit);
	void OnEndTimeCommitted(const FText& NewText, ETextCommit::Type InTextCommit);
	void OnInTimeCommitted(const FText& NewText, ETextCommit::Type InTextCommit);
	void OnOutTimeCommitted(const FText& NewText, ETextCommit::Type InTextCommit);

	FText InTimeTooltip() const;
	FText OutTimeTooltip() const;
	FText StartTimeTooltip() const;
	FText EndTimeTooltip() const;

private:
	TSharedPtr<ITimeSliderController> TimeSliderController;
	TSharedPtr<STimeRangeSlider> TimeRangeSlider;
	TAttribute<bool> ShowFrameNumbers;
};