// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SScreenShotBrowser.cpp: Implements the SScreenShotBrowser class.
=============================================================================*/

#include "ScreenShotComparisonPrivatePCH.h"
#include "SDirectoryPicker.h"

#include "SScreenComparisonRow.h"

/* SScreenShotBrowser interface
 *****************************************************************************/

void SScreenShotBrowser::Construct( const FArguments& InArgs,  IScreenShotManagerRef InScreenShotManager  )
{
	ScreenShotManager = InScreenShotManager;
	ComparisonRoot = FPaths::ConvertRelativePathToFull(FPaths::GameSavedDir() / TEXT("Exported"));


	ChildSlot
	[
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)

			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SDirectoryPicker)
				.Directory(ComparisonRoot)
				.OnDirectoryChanged(this, &SScreenShotBrowser::OnDirectoryChanged)
			]
		]

		+SVerticalBox::Slot()
		.FillHeight( 1.0f )
		[
			SAssignNew(ComparisonView, SListView< TSharedPtr<FImageComparisonResult> >)
			.ListItemsSource(&ComparisonList)
			.OnGenerateRow(this, &SScreenShotBrowser::OnGenerateWidgetForScreenResults)
			.SelectionMode(ESelectionMode::None)
		]
	];

	// Register for callbacks
	ScreenShotManager->RegisterScreenShotUpdate(ScreenShotDelegate);

	RebuildTree();
}

void SScreenShotBrowser::OnDirectoryChanged(const FString& Directory)
{
	ComparisonRoot = Directory;

	RebuildTree();
}

TSharedRef<ITableRow> SScreenShotBrowser::OnGenerateWidgetForScreenResults(TSharedPtr<FImageComparisonResult> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	// Create the row widget.
	return
		SNew(SScreenComparisonRow, OwnerTable)
		.ComparisonDirectory(ComparisonDirectory)
		.Comparisons(CurrentComparisons)
		.ComparisonResult(InItem);
}

void SScreenShotBrowser::RebuildTree()
{
	TArray<FString> Changelists;
	FString SearchDirectory = ComparisonRoot / TEXT("*");
	IFileManager::Get().FindFiles(Changelists, *SearchDirectory, false, true);

	ComparisonDirectory = ComparisonRoot / TEXT("3120027");
	CurrentComparisons = ScreenShotManager->ImportScreensots(ComparisonDirectory);
	
	ComparisonList.Reset();

	if ( CurrentComparisons.IsValid() )
	{
		// Copy the comparisons to an array as shared pointers the list view can use.
		for ( FImageComparisonResult& Result : CurrentComparisons->Comparisons )
		{
			ComparisonList.Add(MakeShareable(new FImageComparisonResult(Result)));
		}
	}

	ComparisonView->RequestListRefresh();
}
