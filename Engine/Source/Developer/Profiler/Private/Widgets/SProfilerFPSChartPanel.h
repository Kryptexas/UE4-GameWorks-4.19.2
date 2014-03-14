// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** A custom widget that acts as a container for widgets like SDataGraph or SEventTree. */
class SProfilerFPSChartPanel : public SCompoundWidget
{
public:
	/** Default constructor. */
	SProfilerFPSChartPanel();
		
	/** Virtual destructor. */
	virtual ~SProfilerFPSChartPanel();

	SLATE_BEGIN_ARGS( SProfilerFPSChartPanel )
		{}

		SLATE_ARGUMENT( TSharedPtr<FFPSAnalyzer>, FPSAnalyzer )
	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param InArgs - The declaration data for this widget
	 */
	void Construct( const FArguments& InArgs );

	// SWidget interface
	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) OVERRIDE;
	// End of SWidget interface

	/** Restores default state for the graph panel by removing all connected data sources and reseting view to default values. */
	void RestoreDefaultState();
};