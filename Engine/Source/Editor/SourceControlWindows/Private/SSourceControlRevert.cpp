// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SourceControlWindowsPCH.h"
#include "AssetToolsModule.h"
#include "PackageTools.h"

#define LOCTEXT_NAMESPACE "SSourceControlRevert"

//-------------------------------------
//Source Control Window Constants
//-------------------------------------
namespace ERevertResults
{
	enum Type
	{
		REVERT_ACCEPTED,
		REVERT_CANCELED
	};
}


struct FRevertCheckBoxListViewItem
{
	/**
	 * Constructor
	 *
	 * @param	InText	String that should appear for the item in the list view
	 */
	FRevertCheckBoxListViewItem( FString InText )
	{
		Text = InText;
		IsEnabled = true;
		IsSelected = false;
	}

	void OnCheckStateChanged( const ESlateCheckBoxState::Type NewCheckedState )
	{
		IsSelected = (NewCheckedState == ESlateCheckBoxState::Checked);
	}

	ESlateCheckBoxState::Type OnIsChecked() const
	{
		return (IsEnabled ? IsSelected : false) ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
	}


	bool IsEnabled;
	bool IsSelected;
	FString Text;
};

/**
 * Source control panel for reverting files. Allows the user to select which files should be reverted, as well as
 * provides the option to only allow unmodified files to be reverted.
 */
class SSourceControlRevertWidget : public SCompoundWidget
{
public:

	//* @param	InXamlName		Name of the XAML file defining this panel
	//* @param	InPackageNames	Names of the packages to be potentially reverted
	SLATE_BEGIN_ARGS( SSourceControlRevertWidget )
		: _ParentWindow()
		, _CheckedOutPackages()
	{}

		SLATE_ATTRIBUTE( TSharedPtr<SWindow>, ParentWindow )	
		SLATE_ATTRIBUTE( TArray<FString>, CheckedOutPackages )

	SLATE_END_ARGS()

	/**
	 * Constructor.
	 */
	SSourceControlRevertWidget()
	{
	}

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	void Construct( const FArguments& InArgs )
	{
		ParentFrame = InArgs._ParentWindow.Get();

		for ( TArray<FString>::TConstIterator PackageIter( InArgs._CheckedOutPackages.Get() ); PackageIter; ++PackageIter )
		{
			ListViewItemSource.Add( MakeShareable(new FRevertCheckBoxListViewItem(*PackageIter) ));
		}


		ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[

				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("SourceControl.Revert", "SelectFiles", "Select the files that should be reverted below").ToString())
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10,0)
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					.Padding(5)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged(this, &SSourceControlRevertWidget::ColumnHeaderClicked)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("SourceControl.Revert", "ListHeader", "File Name").ToString())
						]
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(10,0)
				.MaxHeight(300)
				[
					SNew(SBorder)
					.Padding(5)
					[
						SAssignNew(RevertListView, SListViewType)
						.ItemHeight(24)
						.ListItemsSource(&ListViewItemSource)
						.OnGenerateRow(this, &SSourceControlRevertWidget::OnGenerateRowForList)
					]
				]
				+SVerticalBox::Slot()
				.Padding(0, 10, 0, 0)
				.FillHeight(1)
				.VAlign(VAlign_Bottom)
				.HAlign(HAlign_Fill)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(15,5)
					.HAlign(HAlign_Left)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged(this, &SSourceControlRevertWidget::RevertUnchangedToggled)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("SourceControl.Revert", "RevertUnchanged", "Revert Unchanged Only").ToString())
						]
					]
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.FillWidth(1)
					.Padding(5)
					[
						SNew(SButton)
						.OnClicked(this, &SSourceControlRevertWidget::OKClicked)
						.IsEnabled(this, &SSourceControlRevertWidget::IsOKEnabled)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("SourceControl.Revert", "RevertButton", "Revert Files").ToString())
						]
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					.Padding(0,5,5,5)
					[
						SNew(SButton)
						.OnClicked(this, &SSourceControlRevertWidget::CancelClicked)
						.Text(NSLOCTEXT("SourceControl.Revert", "CancelButton", "Cancel").ToString())
					]
				]
			]
		];

		DialogResult = ERevertResults::REVERT_CANCELED;
		bAlreadyQueriedModifiedFiles = false;
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

	/**
	 * Populates the provided array with the names of the packages the user elected to revert, if any.
	 *
	 * @param	OutPackagesToRevert	Array of package names to revert, as specified by the user in the dialog
	 */
	void GetPackagesToRevert( TArray<FString>& OutPackagesToRevert )
	{
		for (int32 i=0; i<ListViewItemSource.Num(); i++)
		{
			if (ListViewItemSource[i]->IsSelected)
			{
				OutPackagesToRevert.Add(ListViewItemSource[i]->Text);
			}
		}
	}


	ERevertResults::Type GetResult()
	{
		return DialogResult;
	}

private:

	TSharedRef<ITableRow> OnGenerateRowForList( TSharedPtr<FRevertCheckBoxListViewItem> ListItemPtr, const TSharedRef<STableViewBase>& OwnerTable )
	{
		TSharedPtr<SCheckBox> CheckBox;
		TSharedRef<ITableRow> Row =
			SNew(STableRow< TSharedPtr<FString> >, OwnerTable)
			[
				SAssignNew(CheckBox, SCheckBox)
				.OnCheckStateChanged(ListItemPtr.ToSharedRef(), &FRevertCheckBoxListViewItem::OnCheckStateChanged)
				.IsChecked(ListItemPtr.ToSharedRef(), &FRevertCheckBoxListViewItem::OnIsChecked)
				[
					SNew(STextBlock)				
					.Text(ListItemPtr->Text)
				]		
			];

		return Row;
	}


	/** Called when the settings of the dialog are to be accepted*/
	FReply OKClicked()
	{
		DialogResult = ERevertResults::REVERT_ACCEPTED;
		ParentFrame.Pin()->RequestDestroyWindow();

		return FReply::Handled();
	}

	bool IsOKEnabled() const
	{
		for (int32 i=0; i<ListViewItemSource.Num(); i++)
		{
			if (ListViewItemSource[i]->IsSelected)
			{
				return true;
			}
		}
		return false;
	}

	/** Called when the settings of the dialog are to be ignored*/
	FReply CancelClicked()
	{
		DialogResult = ERevertResults::REVERT_CANCELED;
		ParentFrame.Pin()->RequestDestroyWindow();

		return FReply::Handled();
	}

	/** Called when the user checks or unchecks the revert unchanged checkbox; updates the list view accordingly */
	void RevertUnchangedToggled( const ESlateCheckBoxState::Type NewCheckedState )
	{
		const bool bChecked = (NewCheckedState == ESlateCheckBoxState::Checked);
		
		// If this is the first time the user has checked the "Revert Unchanged" checkbox, query source control to find
		// the packages that are modified from the version on the server. Due to the fact that this is a synchronous and potentially
		// slow operation, we only do it once upfront, and then cache the results so that future toggling is nearly instant.
		if ( bChecked && !bAlreadyQueriedModifiedFiles )
		{
			TArray<FString> PackagesToCheck;
			for (int32 i=0; i<ListViewItemSource.Num(); i++)
			{
				PackagesToCheck.Add(SourceControlHelpers::PackageFilename(ListViewItemSource[i]->Text));		
			}

			// Make sure we update the modified state of the files
			TSharedRef<FUpdateStatus, ESPMode::ThreadSafe> UpdateStatusOperation = ISourceControlOperation::Create<FUpdateStatus>();
			UpdateStatusOperation->SetUpdateModifiedState(true);
			ISourceControlModule::Get().GetProvider().Execute(UpdateStatusOperation, PackagesToCheck);

			// Find the files modified from the server version
			TArray< FSourceControlStateRef > SourceControlStates;
			ISourceControlModule::Get().GetProvider().GetState( PackagesToCheck, SourceControlStates, EStateCacheUsage::Use );

			ModifiedPackages.Empty();
			for( TArray< FSourceControlStateRef >::TConstIterator Iter(SourceControlStates); Iter; Iter++ )
			{
				FSourceControlStateRef SourceControlState = *Iter;
				if(SourceControlState->IsModified())
				{
					FString PackageName;
					if ( FPackageName::TryConvertFilenameToLongPackageName(SourceControlState->GetFilename(), PackageName) )
					{
						ModifiedPackages.Add(PackageName);
					}
				}
			}
			
			bAlreadyQueriedModifiedFiles = true;
		}

		// Iterate over each list view item, setting its enabled/selected state appropriately based on whether "Revert Unchanged" is
		// checked or not

		for (int32 i=0; i<ListViewItemSource.Num(); i++)
		{
			TSharedPtr<FRevertCheckBoxListViewItem> CurItem = ListViewItemSource[i];

			bool bItemIsModified = false;
			for ( int32 ModifiedIndex = 0; ModifiedIndex < ModifiedPackages.Num(); ++ModifiedIndex )
			{
				if ( (CurItem->Text) == ModifiedPackages[ModifiedIndex] )
				{
					bItemIsModified = true;
					break;
				}
			}

			// Disable the item if the checkbox is checked and the item is modified
			CurItem->IsEnabled = !( bItemIsModified && bChecked );
		}
	}

	/**
	 * Called whenever a column header is clicked, or in the case of the dialog, also when the "Check/Uncheck All" column header
	 * checkbox is called, because its event bubbles to the column header. 
	 */
	void ColumnHeaderClicked( const ESlateCheckBoxState::Type NewCheckedState )
	{
		for (int32 i=0; i<ListViewItemSource.Num(); i++)
		{
			TSharedPtr<FRevertCheckBoxListViewItem> CurListViewItem = ListViewItemSource[i];

			if ( CurListViewItem->IsEnabled )
			{
				CurListViewItem->IsSelected = (NewCheckedState == ESlateCheckBoxState::Checked);
			}
			else
			{
				CurListViewItem->IsSelected = false;
			}
		}
	}



	TWeakPtr<SWindow> ParentFrame;
	ERevertResults::Type DialogResult;

	/** Cache whether the dialog has already queried source control for modified files or not as an optimization */
	bool bAlreadyQueriedModifiedFiles;


	/** ListView for the packages the user can revert */
	typedef SListView<TSharedPtr<FRevertCheckBoxListViewItem>> SListViewType;
	TSharedPtr<SListViewType> RevertListView;

	/** Collection of items serving as the data source for the list view */
	TArray<TSharedPtr<FRevertCheckBoxListViewItem>> ListViewItemSource;

	/** List of package names that are modified from the versions stored in source control; Used as an optimization */
	TArray<FString> ModifiedPackages;
};

bool FSourceControlWindows::PromptForRevert( const TArray<FString>& InPackageNames )
{
	bool bReverted = false;

	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

	// Only add packages that are actually already checked out to the prompt
	TArray<FString> CheckedOutPackages;
	for ( TArray<FString>::TConstIterator PackageIter( InPackageNames ); PackageIter; ++PackageIter )
	{
		FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(SourceControlHelpers::PackageFilename(*PackageIter), EStateCacheUsage::Use);
		if( SourceControlState.IsValid() && (SourceControlState->IsCheckedOut() || SourceControlState->IsAdded()) )
		{
			CheckedOutPackages.Add( *PackageIter );
		}
	}

	// If any of the packages are checked out, provide the revert prompt
	if ( CheckedOutPackages.Num() > 0 )
	{
		TSharedRef<SWindow> NewWindow = SNew(SWindow)
			.Title( NSLOCTEXT("SourceControl.RevertWindow", "Title", "Revert Files") )
			.SizingRule( ESizingRule::Autosized )
			.SupportsMinimize(false) 
			.SupportsMaximize(false);

		TSharedRef<SSourceControlRevertWidget> SourceControlWidget = 
			SNew(SSourceControlRevertWidget)
			.ParentWindow(NewWindow)
			.CheckedOutPackages(CheckedOutPackages);

		NewWindow->SetContent(SourceControlWidget);

		FSlateApplication::Get().AddModalWindow(NewWindow, NULL);

		// If the user decided to revert some packages, go ahead and do revert the ones they selected
		if ( SourceControlWidget->GetResult() == ERevertResults::REVERT_ACCEPTED )
		{
			TArray<FString> PackagesToRevert;
			SourceControlWidget->GetPackagesToRevert( PackagesToRevert );
			check( PackagesToRevert.Num() > 0 );

			// attempt to unload the packages we are about to revert
			TArray<UPackage*> PackagesToUnload;
			for ( TArray<FString>::TConstIterator PackageIter( InPackageNames ); PackageIter; ++PackageIter )
			{
				UPackage* Package = FindPackage(NULL, **PackageIter);
				if(Package != NULL)
				{
					PackagesToUnload.Add(Package);
				}
			}
			
			if(PackagesToUnload.Num() == 0 || PackageTools::UnloadPackages(PackagesToUnload))
			{
				for( auto PackageIter( PackagesToUnload.CreateIterator() ); PackageIter; ++PackageIter )
				{
					(*PackageIter)->ClearFlags(RF_WasLoaded);
				}

				SourceControlProvider.Execute( ISourceControlOperation::Create<FRevert>(), SourceControlHelpers::PackageFilenames(PackagesToRevert) );
			}
			else
			{
				FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("SCC_Revert_PackageUnloadFailed", "Could not unload assets when reverting files") );
			}
			
			bReverted = true;
		}
	}


	return bReverted;
}

#undef LOCTEXT_NAMESPACE
