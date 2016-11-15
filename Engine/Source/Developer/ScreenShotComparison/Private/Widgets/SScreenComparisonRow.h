// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Widget to display a particular view.
 */
class SScreenComparisonRow : public STableRow< TSharedPtr<FImageComparisonResult> >
{
public:

	SLATE_BEGIN_ARGS( SScreenComparisonRow )
		{}

		SLATE_ARGUMENT( FString, ComparisonDirectory )
		SLATE_ARGUMENT( TSharedPtr<FComparisonResults>, Comparisons )
		SLATE_ARGUMENT( TSharedPtr<FImageComparisonResult>, ComparisonResult )

	SLATE_END_ARGS()
	
	/**
	 * Construct the widget.
	 *
	 * @param InArgs   A declaration from which to construct the widget.
 	 * @param InOwnerTableView   The owning table data.
	 */
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView );

private:

	//Holds the screen shot info.
	TSharedPtr<FImageComparisonResult> ComparisonResult;

	//The cached actual size of the screenshot
	FIntPoint CachedActualImageSize;

	//Holds the dynamic brush.
	TSharedPtr<FSlateDynamicImageBrush> ApprovedBrush;

	//Holds the dynamic brush.
	TSharedPtr<FSlateDynamicImageBrush> UnapprovedBrush;

	//Holds the dynamic brush.
	TSharedPtr<FSlateDynamicImageBrush> ComparisonBrush;
};
