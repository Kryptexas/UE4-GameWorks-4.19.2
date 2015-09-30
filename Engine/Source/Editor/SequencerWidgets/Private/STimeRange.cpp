// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerWidgetsPrivatePCH.h"
#include "STimeRange.h"
#include "STimeRangeSlider.h"
#include "SlateStyle.h"

#define LOCTEXT_NAMESPACE "STimeRange"

/*
* Custom time range text box for the inputs
*/
class STimeRangeTextBox : public SEditableTextBox
{
public:
	void Construct(const FArguments& InArgs) { SEditableTextBox::Construct(InArgs); }
protected:
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(46.f, 16.f); }
};

void STimeRange::Construct( const STimeRange::FArguments& InArgs, TSharedRef<ITimeSliderController> InTimeSliderController )
{
	TimeSliderController = InTimeSliderController;
	ShowFrameNumbers = InArgs._ShowFrameNumbers;
	TimeSnapInterval = InArgs._TimeSnapInterval;
	
	this->ChildSlot
	.HAlign(HAlign_Fill)
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 2.0f)
		[
			SNew(STimeRangeTextBox)
			.Text(this, &STimeRange::StartTime)
			.ToolTipText(this, &STimeRange::StartTimeTooltip )
			.OnTextCommitted( this, &STimeRange::OnStartTimeCommitted )
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 2.0f)
		[
			SNew(STimeRangeTextBox)			
			.Text(this, &STimeRange::InTime)
			.ToolTipText(this, &STimeRange::InTimeTooltip)
			.OnTextCommitted( this, &STimeRange::OnInTimeCommitted )
		]
		+SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Padding(2.0f, 4.0f)
		[
			SAssignNew(TimeRangeSlider, STimeRangeSlider, InTimeSliderController, SharedThis(this))
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 2.0f)
		[
			SNew(STimeRangeTextBox)
			.Text(this, &STimeRange::OutTime)
			.ToolTipText(this, &STimeRange::OutTimeTooltip)
			.OnTextCommitted( this, &STimeRange::OnOutTimeCommitted )
		]
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(2.0f, 2.0f)
		[
			SNew(STimeRangeTextBox)
			.Text(this, &STimeRange::EndTime)
			.ToolTipText(this, &STimeRange::EndTimeTooltip)
			.OnTextCommitted( this, &STimeRange::OnEndTimeCommitted )
		]
	];
}

FText STimeRange::StartTime() const
{
	if (TimeSliderController.IsValid())
	{
		float StartTime = TimeSliderController.Get()->GetClampRange().GetLowerBoundValue();
		bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;

		FString StartTimeStr;
		if (bShowFrameNumbers)
		{
			int32 Frame = TimeSliderController.Get()->TimeToFrame(StartTime);
			StartTimeStr = FString::Printf( TEXT("%d"), Frame);
		}
		else
		{
			StartTimeStr = FString::Printf( TEXT("%.2f"), StartTime);
		}

		return FText::FromString(StartTimeStr);
	}

	return FText::FromString("");
}

FText STimeRange::EndTime() const
{
	if (TimeSliderController.IsValid())
	{
		float EndTime = TimeSliderController.Get()->GetClampRange().GetUpperBoundValue();
		bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;

		FString EndTimeStr;
		if (bShowFrameNumbers)
		{
			int32 Frame = TimeSliderController.Get()->TimeToFrame(EndTime);
			EndTimeStr = FString::Printf( TEXT("%d"), Frame);
		}
		else
		{
			EndTimeStr = FString::Printf( TEXT("%.2f"), EndTime);
		}

		return FText::FromString(EndTimeStr);
	}

	return FText::FromString("");
}

FText STimeRange::InTime() const
{
	if (TimeSliderController.IsValid())
	{
		float InTime = TimeSliderController.Get()->GetViewRange().GetLowerBoundValue();
		bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;

		FString InTimeStr;
		if (bShowFrameNumbers)
		{
			int32 Frame = TimeSliderController.Get()->TimeToFrame(InTime);
			InTimeStr = FString::Printf( TEXT("%d"), Frame);
		}
		else
		{
			InTimeStr = FString::Printf( TEXT("%.2f"), InTime);
		}

		return FText::FromString(InTimeStr);
	}

	return FText::FromString("");
}

FText STimeRange::OutTime() const
{
	if (TimeSliderController.IsValid())
	{
		float OutTime = TimeSliderController.Get()->GetViewRange().GetUpperBoundValue();
		bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;

		FString OutTimeStr;
		if (bShowFrameNumbers)
		{
			int32 Frame = TimeSliderController.Get()->TimeToFrame(OutTime);
			OutTimeStr = FString::Printf( TEXT("%d"), Frame);
		}
		else
		{
			OutTimeStr = FString::Printf( TEXT("%.2f"), OutTime); 
		}

		return FText::FromString(OutTimeStr);
	}

	return FText::FromString("");
}

void STimeRange::OnStartTimeCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	FString NewTextStr = NewText.ToString();

	if (TimeSliderController.IsValid())
	{
		float NewStart;
		bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;

		if (bShowFrameNumbers)
		{
			int32 NewStartFrame = FCString::Atoi(*NewTextStr);
			NewStart = TimeSliderController.Get()->FrameToTime(NewStartFrame);
		}
		else
		{
			NewStart = FCString::Atof(*NewTextStr);
		}

		TimeSliderController.Get()->SetClampRange(NewStart, TimeSliderController.Get()->GetClampRange().GetUpperBoundValue());
	}
}

void STimeRange::OnEndTimeCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	FString NewTextStr = NewText.ToString();

	if (TimeSliderController.IsValid())
	{
		float NewEnd;
		bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;

		if (bShowFrameNumbers)
		{
			int32 NewEndFrame = FCString::Atoi(*NewTextStr);
			NewEnd = TimeSliderController.Get()->FrameToTime(NewEndFrame);
		}
		else
		{
			NewEnd = FCString::Atof(*NewTextStr);
		}

		TimeSliderController.Get()->SetClampRange(TimeSliderController.Get()->GetClampRange().GetLowerBoundValue(), NewEnd);
	}
}

void STimeRange::OnInTimeCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	FString NewTextStr = NewText.ToString();

	if (TimeSliderController.IsValid())
	{
		float NewIn;
		bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;

		if (bShowFrameNumbers)
		{
			int32 NewInFrame = FCString::Atoi(*NewTextStr);

			NewIn = TimeSliderController.Get()->FrameToTime(NewInFrame);
		}
		else
		{
			NewIn = FCString::Atof(*NewTextStr);
		}

		if (NewIn < TimeSliderController.Get()->GetClampRange().GetLowerBoundValue())
		{
			TimeSliderController.Get()->SetClampRange(NewIn, TimeSliderController.Get()->GetClampRange().GetUpperBoundValue());
		}

		TimeSliderController.Get()->SetViewRange(NewIn, TimeSliderController.Get()->GetViewRange().GetUpperBoundValue(), EViewRangeInterpolation::Immediate);
	}
}

void STimeRange::OnOutTimeCommitted(const FText& NewText, ETextCommit::Type InTextCommit)
{
	FString NewTextStr = NewText.ToString();

	if (TimeSliderController.IsValid())
	{
		float NewOut;
		bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;

		if (bShowFrameNumbers)
		{
			int32 NewOutFrame = FCString::Atoi(*NewTextStr);
			NewOut = TimeSliderController.Get()->FrameToTime(NewOutFrame);
		}
		else
		{
			NewOut = FCString::Atof(*NewTextStr);
		}

		if (NewOut > TimeSliderController.Get()->GetClampRange().GetUpperBoundValue())
		{
			TimeSliderController.Get()->SetClampRange(TimeSliderController.Get()->GetClampRange().GetLowerBoundValue(), NewOut);
		}

		TimeSliderController.Get()->SetViewRange(TimeSliderController.Get()->GetViewRange().GetLowerBoundValue(), NewOut, EViewRangeInterpolation::Immediate);
	}
}

FText STimeRange::StartTimeTooltip() const
{
	bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;
	if (bShowFrameNumbers)
	{
		return LOCTEXT("StartFrameTooltip", "Start frame");
	}
	else
	{
		return LOCTEXT("StartTimeTooltip", "Start time");
	}
}

FText STimeRange::InTimeTooltip() const
{
	bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;
	if (bShowFrameNumbers)
	{
		return LOCTEXT("InFrameTooltip", "In frame");
	}
	else
	{
		return LOCTEXT("InTimeTooltip", "In time");
	}
}

FText STimeRange::OutTimeTooltip() const
{
	bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;
	if (bShowFrameNumbers)
	{
		return LOCTEXT("OutFrameTooltip", "Out frame");
	}
	else
	{
		return LOCTEXT("OutTimeTooltip", "Out time");
	}
}

FText STimeRange::EndTimeTooltip() const
{
	bool bShowFrameNumbers = ShowFrameNumbers.IsBound() ? ShowFrameNumbers.Get() : false;
	if (bShowFrameNumbers)
	{
		return LOCTEXT("EndFrameTooltip", "End frame");
	}
	else
	{
		return LOCTEXT("EndTimeTooltip", "End time");
	}
}

float STimeRange::GetTimeSnapInterval() const
{
	if (TimeSnapInterval.IsBound())
	{
		return TimeSnapInterval.Get();
	}
	return 1.f;
}

#undef LOCTEXT_NAMESPACE
