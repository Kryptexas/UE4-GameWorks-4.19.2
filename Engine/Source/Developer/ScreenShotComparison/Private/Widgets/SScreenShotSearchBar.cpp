// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SScreenShotSearchBar.cpp: Implements the SScreenShotSearchBar class.
=============================================================================*/

#include "ScreenShotComparisonPrivatePCH.h"

#define LOCTEXT_NAMESPACE "ScreenShotBrowserToolBar"

void SScreenShotSearchBar::Construct( const FArguments& InArgs, IScreenShotManagerPtr InScreenShotManager )
{
	// Screen Shot manager to set the filter on
	ScreenShotManager = InScreenShotManager.ToSharedRef();

	// Create the search filter and set criteria
	ScreenShotComparisonFilter = MakeShareable( new FScreenShotComparisonFilter() ); 
	ScreenShotFilters = MakeShareable( new ScreenShotFilterCollection() );
	ScreenShotFilters->Add( ScreenShotComparisonFilter );

	ScreenShotManager->SetFilter( ScreenShotFilters );

	PlatformDisplayString = "Any";

	TArray< TSharedPtr<FString> >& PlatformList = ScreenShotManager->GetCachedPlatfomList();

	ChildSlot
	[
		SNew ( SHorizontalBox )
		+ SHorizontalBox::Slot()
		.FillWidth( 1.0f )
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew( SBox )
			.WidthOverride( 100.0f )
			[
				SNew(SComboButton)
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text( this, &SScreenShotSearchBar::GetPlatformString )
				]
				.ContentPadding(FMargin(6.0f, 2.0f))
				.MenuContent()
				[
					SAssignNew(PlatformListView, SListView<TSharedPtr<FString> >)
					.ItemHeight(24.0f)
					.ListItemsSource(&PlatformList)
					.OnGenerateRow(this, &SScreenShotSearchBar::HandlePlatformListViewGenerateRow)
					.OnSelectionChanged(this, &SScreenShotSearchBar::HandlePlatformListSelected )
				]
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth( 0.3f )
		[
			SAssignNew( SearchBox, SSearchBox )
			.OnTextChanged( this, &SScreenShotSearchBar::HandleFilterTextChanged )
		]
		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ButtonColorAndOpacity(FLinearColor(1.0f,1.0f,1.0f,0.0f))
				.OnClicked(this, &SScreenShotSearchBar::RefreshView)
				[
					SNew(SImage)
					.ToolTipText( LOCTEXT( "Refresh Screen shots", "Refresh Screen Shots" ).ToString() )
					.Image(FEditorStyle::GetBrush("AutomationWindow.RefreshTests.Small"))
				]
			]
	];
}

FString SScreenShotSearchBar::GetPlatformString() const
{
	return PlatformDisplayString;
}

/** Filtering */

void SScreenShotSearchBar::HandleFilterTextChanged( const FText& InFilterText )
{
	FString TestName = *InFilterText.ToString();
	ScreenShotComparisonFilter->SetScreenFilter( TestName );
	ScreenShotManager->SetFilter( ScreenShotFilters );
}


void SScreenShotSearchBar::HandlePlatformListSelected( TSharedPtr<FString> PlatformName, ESelectInfo::Type SelectInfo )
{
	FString TestName = *PlatformName;
	if ( TestName == "Any" )
	{
		TestName = "";
	}

	ScreenShotComparisonFilter->SetPlatformFilter( TestName );
	ScreenShotManager->SetFilter( ScreenShotFilters );
	PlatformDisplayString = *PlatformName;
}


TSharedRef<ITableRow> SScreenShotSearchBar::HandlePlatformListViewGenerateRow( TSharedPtr<FString> PlatformName, const TSharedRef<STableViewBase>& OwnerTable )
{
	return SNew(STableRow<TSharedPtr<FString> >, OwnerTable)
	.Content()
	[
		SNew( STextBlock ) .Text( *PlatformName )
	];
}

FReply SScreenShotSearchBar::RefreshView( )
{
	ScreenShotManager->GenerateLists();

	ScreenShotComparisonFilter->SetPlatformFilter( "" );
	ScreenShotManager->SetFilter( ScreenShotFilters );
	PlatformDisplayString = "";

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
