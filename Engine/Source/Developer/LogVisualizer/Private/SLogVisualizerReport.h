// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SLogVisualizerReport : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLogVisualizerReport) {}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, SLogVisualizer* InAnalyzer, TArray< TSharedPtr<struct FActorsVisLog> >&);

	virtual ~SLogVisualizerReport();

	FText GenerateReportText() const;

protected:
	TArray< TSharedPtr<struct FActorsVisLog> > SelectedItems;
};
