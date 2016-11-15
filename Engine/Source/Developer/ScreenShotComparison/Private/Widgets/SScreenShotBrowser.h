// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SScreenShotBrowser.h: Declares the SScreenShotBrowser class.
=============================================================================*/

#pragma once


/**
 * Implements a Slate widget for browsing active game sessions.
 */

class SScreenShotBrowser
	: public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SScreenShotBrowser) { }
	SLATE_END_ARGS()

public:

	/**
	 * Construct this widget.
	 *
	 * @param InArgs - The declaration data for this widget.
 	 * @param InScreenShotManager - The screen shot manager containing the screen shot data.
	 */
	void Construct( const FArguments& InArgs, IScreenShotManagerRef InScreenShotManager );

public:

	TSharedRef<ITableRow> OnGenerateWidgetForScreenResults(TSharedPtr<FImageComparisonResult> InItem, const TSharedRef<STableViewBase>& OwnerTable);

private:

	void OnDirectoryChanged(const FString& Directory);

	/**
	 * Regenerate the widgets when the filter changes
	 */
	void RebuildTree();

private:

	// The manager containing the screen shots
	IScreenShotManagerPtr ScreenShotManager;

	/** The directory where we're imported comparisons from. */
	FString ComparisonRoot;

	/** The directory where we're imported comparisons from, with changelist */
	FString ComparisonDirectory;

	/** The imported screenshot results */
	TSharedPtr<FComparisonResults> CurrentComparisons;

	/** The imported screenshot results copied into an array usable by the list view */
	TArray<TSharedPtr<FImageComparisonResult>> ComparisonList;

	/**  */
	TSharedPtr< SListView< TSharedPtr<FImageComparisonResult> > > ComparisonView;

	// Delegate to call when screen shot data changes 
	FOnScreenFilterChanged ScreenShotDelegate;
};
