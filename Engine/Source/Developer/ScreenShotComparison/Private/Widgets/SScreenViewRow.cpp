// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SScreenViewRow.cpp: implements the SScreenViewRow class.
=============================================================================*/

#include "ScreenShotComparisonPrivatePCH.h"

#define LOCTEXT_NAMESPACE "SScreenShotBrowser"


void SScreenViewRow::Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
{
	STableRow<TSharedPtr<IScreenShotData> >::ConstructInternal(
		STableRow::FArguments()
			.ShowSelection(true),
		InOwnerTableView
	);

	// The screen shot item to view
	TSharedPtr<IScreenShotData> ScreenShotData = InArgs._ScreenShotData;

	// The display string for the view
	FString InfoText = FString::Printf( *LOCTEXT("ScreenShotName", "Screen shot: %s").ToString(), *ScreenShotData->GetName() );

	ChildSlot
	[
		SNew(SBorder)
			[
			SNew( SVerticalBox )
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew( SBorder )
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
				.Content()
				[
					SNew( STextBlock ) .Text( InfoText )
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				// The screen view being tested
				SAssignNew( ScreenShotDataView, SListView< TSharedPtr<IScreenShotData> > )
				// Tell the screen view where to get its source data
				.ListItemsSource( &ScreenShotData->GetFilteredChildren() )
				// When the list view needs to generate a widget for each screen view, use this method
				.OnGenerateRow( this, &SScreenViewRow::OnGenerateWidgetForScreenView )
				.SelectionMode(ESelectionMode::None)
				.HeaderRow
				(
					SNew(SHeaderRow)
					+ SHeaderRow::Column( "Platform" )
						.HAlignHeader(HAlign_Center)
						.VAlignHeader(VAlign_Center)
						.HAlignCell(HAlign_Center)
						.VAlignCell(VAlign_Center)
						.FixedWidth( 200.0f )
						.DefaultLabel(LOCTEXT("PlatformName", "Platform").ToString())
					+ SHeaderRow::Column( "Current View" )
						.HAlignHeader(HAlign_Center)
						.VAlignHeader(VAlign_Center)
						.HAlignCell(HAlign_Center)
						.VAlignCell(VAlign_Center)
 						.FixedWidth( 300.0f )
						.DefaultLabel(LOCTEXT("CurrentViewName", "Current View").ToString())
					+ SHeaderRow::Column( "History View" )
					.DefaultLabel(LOCTEXT("HistoryViewName", "History").ToString())
				)
			]
		]
	];
}


TSharedRef<ITableRow> SScreenViewRow::OnGenerateWidgetForScreenView( TSharedPtr<IScreenShotData> InScreenShotDataItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	// Create a new row per platform
	return
		SNew( SScreenPlatformRow, OwnerTable )
		. ScreenShotDataItem( InScreenShotDataItem );
}

#undef LOCTEXT_NAMESPACE