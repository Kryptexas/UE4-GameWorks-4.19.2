// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
class SVisualLoggerReport : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVisualLoggerReport) {}

	SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, TArray< TSharedPtr<class STimeline> >&);

	virtual ~SVisualLoggerReport();

	FText GenerateReportText() const;

protected:
	TArray< TSharedPtr<class STimeline> > SelectedItems;
};
