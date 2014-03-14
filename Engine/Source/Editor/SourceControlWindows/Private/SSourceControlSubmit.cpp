// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SourceControlWindowsPCH.h"
#include "AssetToolsModule.h"
#include "MessageLog.h"


IMPLEMENT_MODULE( FDefaultModuleImpl, SourceControlWindows );

#if SOURCE_CONTROL_WITH_SLATE

#define LOCTEXT_NAMESPACE "SSourceControlSubmit"

//-------------------------------------
//Source Control Window Constants
//-------------------------------------
namespace ESubmitResults
{
	enum Type
	{
		SUBMIT_ACCEPTED,
		SUBMIT_CANCELED
	};
}

//-----------------------------------------------
//Source Control Check in Helper Struct
//-----------------------------------------------
class FChangeListDescription
{
public:
	TArray<FString> FilesForAdd;
	TArray<FString> FilesForSubmit;
	FString Description;
};

/** Holds the display string and check state for items being submitted. */
struct TSubmitItemData
{
	/** The display string for the item. */
	TSharedPtr< FString > DisplayString;

	/** Whether or not the item is checked. */
	bool bIsChecked;

	TSubmitItemData(TSharedPtr< FString > InListItemPtr)
	{
		DisplayString = InListItemPtr;
		bIsChecked = true;
	}
};

/** The item that appears in the list of items for submitting. */
class SSubmitItem : public STableRow< TSharedPtr<FString> >
{
public:
	SLATE_BEGIN_ARGS( SSubmitItem )
		: _SubmitItemData()
	{}
		/** The item data to be displayed. */
		SLATE_ARGUMENT( TSharedPtr< TSubmitItemData >, SubmitItemData )
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		SubmitItemData = InArgs._SubmitItemData;

		STableRow< TSharedPtr<FString> >::Construct(
			STableRow< TSharedPtr<FString> >::FArguments()
			[
				SNew(SCheckBox)
				.IsChecked(this, &SSubmitItem::IsChecked)
				.OnCheckStateChanged(this, &SSubmitItem::OnCheckStateChanged)
				[
					SNew(STextBlock)				
					.Text(*SubmitItemData->DisplayString)
				]
			]
			,InOwnerTableView
		);
	}

private:
	/** The check status of the item. */
	ESlateCheckBoxState::Type IsChecked() const
	{
		return SubmitItemData->bIsChecked;
	}

	/** Changes the check status of the item .*/
	void OnCheckStateChanged(ESlateCheckBoxState::Type InState)
	{
		SubmitItemData->bIsChecked = InState == ESlateCheckBoxState::Checked;
	}

private:
	/** The item data associated with this list item. */
	TSharedPtr< TSubmitItemData > SubmitItemData;
};

class SSourceControlSubmitWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SSourceControlSubmitWidget )
		: _ParentWindow()
		, _InAddFiles() 
		, _InOpenFiles()
	{}

		SLATE_ATTRIBUTE( TSharedPtr<SWindow>, ParentWindow )
		SLATE_ATTRIBUTE( TArray<FString>, InAddFiles)
		SLATE_ATTRIBUTE( TArray<FString>, InOpenFiles)

	SLATE_END_ARGS()

	SSourceControlSubmitWidget()
	{
	}

	void Construct( const FArguments& InArgs )
	{
		ParentFrame = InArgs._ParentWindow.Get();
		AddFiles(InArgs._InAddFiles.Get(), InArgs._InOpenFiles.Get());

		ChildSlot
		[
			
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew( STextBlock )
					.Text( NSLOCTEXT("SourceControl.SubmitPanel", "ChangeListDesc", "Changelist Description").ToString() )
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(5, 0, 5, 5))
				[
					SNew( SHorizontalBox )
					+SHorizontalBox::Slot()
					.MaxWidth(500.f)
					[
						SAssignNew( ChangeListDescriptionTextCtrl, SEditableTextBox )
						.SelectAllTextWhenFocused( true )
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(5, 0))
				.MaxHeight(300)
				[
					SNew(SBorder)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush(TEXT("PackageDialog.ListHeader")))
							[
								SNew(SHorizontalBox)
								+SHorizontalBox::Slot()
								.AutoWidth()
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Center)
								[
									SNew(SCheckBox)
									.IsChecked(this, &SSourceControlSubmitWidget::GetToggleSelectedState)
									.OnCheckStateChanged(this, &SSourceControlSubmitWidget::OnToggleSelectedCheckBox)
								]
								+SHorizontalBox::Slot()
								.VAlign(VAlign_Center)
								.HAlign(HAlign_Left)
								.FillWidth(1)
								[
									SNew(STextBlock) 
									.Text(LOCTEXT("File", "File").ToString())
								]
							]
						]
						+SVerticalBox::Slot()
						.Padding(2)
						[
							SNew(SBox)
							.WidthOverride(500.0f)
							[
								SAssignNew( ListView, SListType)
								.ItemHeight(24)
								.ListItemsSource( &ListViewItems )
								.OnGenerateRow( this, &SSourceControlSubmitWidget::OnGenerateRowForList )
							]
						]
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(5, 5, 5, 0))
				[
					SNew( SBorder)
					.Visibility(this, &SSourceControlSubmitWidget::IsWarningPanelVisible)
					.Padding(5)
					[
						SNew( STextBlock )
						.Text( NSLOCTEXT("SourceControl.SubmitPanel", "ChangeListDescWarning", "Changelist description is required to submit").ToString() )
					]
				]
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(5)
				[
					SNew(SWrapBox)
					.UseAllottedWidth(true)
					+SWrapBox::Slot()
					.Padding(0.0f, 0.0f, 16.0f, 0.0f)
					[
						SNew(SCheckBox)
						.OnCheckStateChanged( this, &SSourceControlSubmitWidget::OnCheckStateChanged_KeepCheckedOut)
						.IsChecked( this, &SSourceControlSubmitWidget::GetKeepCheckedOut )
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("SourceControl.SubmitPanel", "KeepCheckedOut", "Keep Files Checked Out").ToString() )
						]
					]
				]
				+SVerticalBox::Slot()
				.FillHeight(1)
				.VAlign(VAlign_Bottom)
				.Padding(5,5,5,0)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					.FillWidth(1)
					[
						SNew(SButton)
						.IsEnabled(this, &SSourceControlSubmitWidget::IsOKEnabled)
						.Text( NSLOCTEXT("SourceControl.SubmitPanel", "OKButton", "OK").ToString() )
						.OnClicked(this, &SSourceControlSubmitWidget::OKClicked)
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign(HAlign_Right)
					[
						SNew(SButton)
						.Text( NSLOCTEXT("SourceControl.SubmitPanel", "CancelButton", "Cancel").ToString() )
						.OnClicked(this, &SSourceControlSubmitWidget::CancelClicked)
					]
				]
			]					

		]
		;

		DialogResult = ESubmitResults::SUBMIT_CANCELED;
		KeepCheckedOut = ESlateCheckBoxState::Unchecked;
	}


public:
	ESubmitResults::Type GetResult() {return DialogResult;}


	void AddFiles(const TArray<FString>& InAddFiles, const TArray<FString>& InOpenFiles)
	{
		for (int32 i = 0; i < InAddFiles.Num(); ++i)
		{
			AddFileToListView (InAddFiles[i]);
		}
		for (int32 i = 0; i < InOpenFiles.Num(); ++i)
		{
			AddFileToListView (InOpenFiles[i]);
		}
	}

private:
	ESubmitResults::Type DialogResult;

	typedef SListView< TSharedPtr< TSubmitItemData > > SListType;
	/** ListBox for selecting which object to consolidate */
	TSharedPtr< SListType > ListView;
	/** Collection of objects (Widgets) to display in the List View. */
	TArray< TSharedPtr< TSubmitItemData > > ListViewItems;

	/** 
	 * Get all the selected items in the dialog
	 * 
	 * @param bAllIfNone - if true, returns all the items when none are selected
	 * @return the array of items to be altered by OnToggleSelectedCheckBox
	 */
	TArray< TSharedPtr<TSubmitItemData> > GetSelectedItems( const bool bAllIfNone ) const
	{
		// Get the list of highlighted items
		TArray< TSharedPtr<TSubmitItemData> > SelectedItems = ListView->GetSelectedItems();
		if(SelectedItems.Num() == 0)
		{
			// If no items are explicitly highlighted, return all items in the list.
			SelectedItems = ListViewItems;
		}

		return SelectedItems;
	}

	/** 
	 * @return the desired toggle state for the ToggleSelectedCheckBox.
	 * Returns Unchecked, unless all of the selected items are Checked.
	 */
	ESlateCheckBoxState::Type GetToggleSelectedState() const
	{
		// Default to a Checked state
		ESlateCheckBoxState::Type PendingState = ESlateCheckBoxState::Checked;

		TArray< TSharedPtr<TSubmitItemData> > SelectedItems = GetSelectedItems(true);

		// Iterate through the list of selected items
		for(auto SelectedItem = SelectedItems.CreateConstIterator(); SelectedItem; ++SelectedItem)
		{
			TSubmitItemData* const Item = SelectedItem->Get();
			if(!Item->bIsChecked)
			{
				// If any item in the selection is Unchecked, then represent the entire set of highlighted items as Unchecked,
				// so that the first (user) toggle of ToggleSelectedCheckBox consistently Checks all highlighted items
				PendingState = ESlateCheckBoxState::Unchecked;
			}
		}

		return PendingState;
	}

	/** 
	 * Toggles the highlighted items.
	 * If no items are explicitly highlighted, toggles all items in the list.
	 */
	void OnToggleSelectedCheckBox(ESlateCheckBoxState::Type InNewState)
	{
		TArray< TSharedPtr<TSubmitItemData> > SelectedItems = GetSelectedItems(true);

		const bool bIsChecked = (InNewState == ESlateCheckBoxState::Checked);
		for(auto SelectedItem = SelectedItems.CreateConstIterator(); SelectedItem; ++SelectedItem)
		{
			TSubmitItemData* const Item = SelectedItem->Get();
			Item->bIsChecked = bIsChecked;
		}

		ListView->RequestListRefresh();
	}

public:

	/**Gets the requested files and the change list description*/
	void FillChangeListDescription (FChangeListDescription& OutDesc, TArray<FString>& InAddFiles, TArray<FString>& InOpenFiles)
	{
		OutDesc.Description = ChangeListDescriptionTextCtrl->GetText().ToString();

		check(ListViewItems.Num() == InAddFiles.Num() + InOpenFiles.Num());

		for (int32 i = 0; i < InAddFiles.Num(); ++i)
		{
			if (ListViewItems[i]->bIsChecked)
			{
				OutDesc.FilesForAdd.Add(InAddFiles[i]);
			}
		}
		for (int32 i = 0; i < InOpenFiles.Num(); ++i)
		{
			if (ListViewItems[i + InAddFiles.Num()]->bIsChecked)
			{
				OutDesc.FilesForSubmit.Add(InOpenFiles[i]);
			}
		}
	}

	/** Does the user want to keep the files checked out */
	bool WantToKeepCheckedOut()
	{
		return KeepCheckedOut == ESlateCheckBoxState::Checked ? true : false;
	}

private:

	/** Called when the settings of the dialog are to be accepted*/
	FReply OKClicked()
	{
		DialogResult = ESubmitResults::SUBMIT_ACCEPTED;
		ParentFrame.Pin()->RequestDestroyWindow();

		return FReply::Handled();
	}

	/** Called when the settings of the dialog are to be ignored*/
	FReply CancelClicked()
	{
		DialogResult = ESubmitResults::SUBMIT_CANCELED;
		ParentFrame.Pin()->RequestDestroyWindow();

		return FReply::Handled();
	}

	/** Called to check if the OK button is enabled or not. */
	bool IsOKEnabled() const
	{
		return !ChangeListDescriptionTextCtrl->GetText().IsEmpty();
	}

	/** Check if the warning panel should be visible. */
	EVisibility IsWarningPanelVisible() const
	{
		return IsOKEnabled()? EVisibility::Hidden : EVisibility::Visible;
	}

	/** Called when the Keep checked out Checkbox is changed */
	void OnCheckStateChanged_KeepCheckedOut(ESlateCheckBoxState::Type InState)
	{
		KeepCheckedOut = InState;
	}

	/** Get the current state of the Keep Checked Out checkbox  */
	ESlateCheckBoxState::Type GetKeepCheckedOut() const
	{
		return KeepCheckedOut;
	}

	/**Helper function to append a check box and label to the list view*/
	void AddFileToListView (const FString& InFileName)
	{
		ListViewItems.Add( MakeShareable( new TSubmitItemData( MakeShareable(new FString(InFileName) ) ) ) );
	}

	TSharedRef<ITableRow> OnGenerateRowForList( TSharedPtr< TSubmitItemData > SubmitItemData, const TSharedRef<STableViewBase>& OwnerTable )
	{
		TSharedRef<ITableRow> Row =
			SNew( SSubmitItem, OwnerTable )
				.SubmitItemData(SubmitItemData);

		return Row;
	}

private:
	TWeakPtr<SWindow> ParentFrame;

	/** Internal widgets to save having to get in multiple places*/
	TSharedPtr<SEditableTextBox> ChangeListDescriptionTextCtrl;

	ESlateCheckBoxState::Type	KeepCheckedOut;
};

static void FindFilesForCheckIn(const TArray<FString>& InPackagesNames, TArray<FString>& OutAddFiles, TArray<FString>& OutOpenFiles)
{
	ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

	for( int32 PackageIndex = 0 ; PackageIndex < InPackagesNames.Num() ; ++PackageIndex )
	{
		FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(SourceControlHelpers::PackageFilename(InPackagesNames[PackageIndex]), EStateCacheUsage::Use);
		if(SourceControlState.IsValid())
		{
			if( SourceControlState->IsCheckedOut() || SourceControlState->IsAdded() )
			{
				OutOpenFiles.Add(InPackagesNames[PackageIndex]);
			}
			else
			{
				if( !SourceControlState->IsSourceControlled() )
				{
					OutAddFiles.Add(InPackagesNames[PackageIndex]);
				}
			}
		}
	}
}


bool FSourceControlWindows::PromptForCheckin(const TArray<FString>& InPackageNames)
{
	bool bCheckInSuccess = true;

	TArray<FString> AddFiles;
	TArray<FString> OpenFiles;
	FindFilesForCheckIn(InPackageNames, AddFiles, OpenFiles);

	if (AddFiles.Num() || OpenFiles.Num())
	{
		TSharedRef<SWindow> NewWindow = SNew(SWindow)
			.Title(NSLOCTEXT("SourceControl.SubmitWindow", "Title", "Submit Files"))
			.SizingRule( ESizingRule::Autosized )
			.SupportsMaximize(false)
			.SupportsMinimize(false);

		TSharedRef<SSourceControlSubmitWidget> SourceControlWidget = 
			SNew(SSourceControlSubmitWidget)
			.ParentWindow(NewWindow)
			.InAddFiles(AddFiles)
			.InOpenFiles(OpenFiles);

		NewWindow->SetContent(
			SourceControlWidget
			);

		FSlateApplication::Get().AddModalWindow(NewWindow, NULL);

		if (SourceControlWidget->GetResult() == ESubmitResults::SUBMIT_ACCEPTED)
		{
			ISourceControlProvider& SourceControlProvider = ISourceControlModule::Get().GetProvider();

			//try the actual submission
			FChangeListDescription Description;

			//Get description from the dialog
			SourceControlWidget->FillChangeListDescription(Description, AddFiles, OpenFiles);

			// Convert to source control paths
			Description.FilesForAdd = SourceControlHelpers::PackageFilenames(Description.FilesForAdd);
			Description.FilesForSubmit = SourceControlHelpers::PackageFilenames(Description.FilesForSubmit);

			//revert all unchanged files that were submitted
			if ( Description.FilesForSubmit.Num() > 0 )
			{
				SourceControlHelpers::RevertUnchangedFiles(SourceControlProvider, Description.FilesForSubmit);

				//make sure all files are still checked out
				for (int32 VerifyIndex = Description.FilesForSubmit.Num()-1; VerifyIndex >= 0; --VerifyIndex)
				{
					FString TempFilename = Description.FilesForSubmit[VerifyIndex];
					const FString PackageName = FPackageName::FilenameToLongPackageName(TempFilename);
					FSourceControlStatePtr SourceControlState = SourceControlProvider.GetState(SourceControlHelpers::PackageFilename(PackageName), EStateCacheUsage::Use);
					if( SourceControlState.IsValid() && !SourceControlState->IsCheckedOut() && !SourceControlState->IsAdded() )
					{
						Description.FilesForSubmit.RemoveAt(VerifyIndex);
					}
				}
			}

			TArray<FString> CombinedFileList = Description.FilesForAdd;
			CombinedFileList.Append(Description.FilesForSubmit);

			if(Description.FilesForAdd.Num() > 0)
			{
				bCheckInSuccess &= (SourceControlProvider.Execute(ISourceControlOperation::Create<FMarkForAdd>(), Description.FilesForAdd) == ECommandResult::Succeeded);
			}

			if(CombinedFileList.Num() > 0)
			{
				TSharedRef<FCheckIn, ESPMode::ThreadSafe> CheckInOperation = ISourceControlOperation::Create<FCheckIn>();
				CheckInOperation->SetDescription(Description.Description);
				bCheckInSuccess &= (SourceControlProvider.Execute(CheckInOperation, CombinedFileList) == ECommandResult::Succeeded);
			}

			if(!bCheckInSuccess)
			{
				FMessageLog("SourceControl").Notify(LOCTEXT("SCC_Checkin_Failed", "Failed to check in files!"));
			}
			// If we checked in ok, do we want to to re-check out the files we just checked in?
			else
			{
				if( SourceControlWidget->WantToKeepCheckedOut() == true )
				{
					// Re-check out files
					if(SourceControlProvider.Execute(ISourceControlOperation::Create<FCheckOut>(), CombinedFileList) != ECommandResult::Succeeded)
					{
						FMessageDialog::Open( EAppMsgType::Ok, LOCTEXT("SCC_Checkin_ReCheckOutFailed", "Failed to re-check out files.") );
					}
				}
			}
		}
	}

	return bCheckInSuccess;
}

#undef LOCTEXT_NAMESPACE

#endif // SOURCE_CONTROL_WITH_SLATE
