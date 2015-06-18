// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SlateBasics.h"
#include "STextComboBox.h"
#include "SBreadcrumbTrail.h"
#include "SInlineEditableTextBlock.h"
#include "SlateFileDialogsStyles.h"
#include "DirectoryWatcherModule.h"

#define LOCTEXT_NAMESPACE "SlateFileDialogsNamespace"

class SSlateFileOpenDlg;

//-----------------------------------------------------------------------------
struct FFileEntry
{
	FString	Label;
	FString ModDate;
	FString FileSize;
	bool bIsSelected;
	bool bIsDirectory;

	FFileEntry() { }

	FFileEntry(FString InLabel, FString InModDate, FString InFileSize, bool InIsDirectory)
	{
		Label = InLabel;
		ModDate = FString(InModDate);
		FileSize = FString(InFileSize);
		bIsSelected = false;
		bIsDirectory = InIsDirectory;
	}

	inline static bool ConstPredicate ( const TSharedPtr<FFileEntry> Entry1, const TSharedPtr<FFileEntry> Entry2)
	{
		return (Entry1->Label.Compare( Entry2->Label ) < 0);
	}
};

typedef TSharedPtr<FFileEntry> SSlateFileDialogItemPtr;


//-----------------------------------------------------------------------------
class FSlateFileDlgWindow
{
public:
	enum EResult
	{
		Cancel = 0,
		Accept = 1,
		Engine = 2,
		Project = 3
	};

	FSlateFileDlgWindow(FSlateFileDialogsStyle *InStyleSet);

	bool OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath,
		const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames, int32& OutFilterIndex);

	bool OpenFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath,
		const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames);

	bool OpenDirectoryDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath,
		FString& OutFoldername);

	bool SaveFileDialog(const void* ParentWindowHandle, const FString& DialogTitle, const FString& DefaultPath,
		const FString& DefaultFile, const FString& FileTypes, uint32 Flags, TArray<FString>& OutFilenames);

private:	
	TSharedPtr<class SSlateFileOpenDlg> DialogWidget;
	FString CurrentDirectory;

	FSlateFileDialogsStyle* StyleSet;
};



//-----------------------------------------------------------------------------
class SSlateFileOpenDlg : public SCompoundWidget
{
public:
	struct FDirNode
	{
		FString	Label;
		TSharedPtr<STextBlock> TextBlock;
		TSharedPtr<SButton> Button;

		FDirNode() { }

		FDirNode(FString InLabel, TSharedPtr<STextBlock> InTextBlock)
		{
			Label = InLabel;
			TextBlock = InTextBlock;
		}
	};

	SLATE_BEGIN_ARGS(SSlateFileOpenDlg)
		: _CurrentPath(TEXT("")),
		_Filters(TEXT("")),
		_bMultiSelectEnabled(false),
		_WindowTitleText(TEXT("")),
		_AcceptText(LOCTEXT("SlateDialogOpen","Open")),
		_bDirectoriesOnly(false),
		_bSaveFile(false),
		_OutNames(nullptr),
		_OutFilterIndex(nullptr),
		_ParentWindow(nullptr),
		_StyleSet(nullptr)
	{}

	SLATE_ATTRIBUTE(FString, CurrentPath)
	SLATE_ATTRIBUTE(FString, Filters)
	SLATE_ATTRIBUTE(bool, bMultiSelectEnabled)
	SLATE_ATTRIBUTE(FString, WindowTitleText)
	SLATE_ATTRIBUTE(FText, AcceptText)
	SLATE_ATTRIBUTE(bool, bDirectoriesOnly)
	SLATE_ATTRIBUTE(bool, bSaveFile)
	SLATE_ATTRIBUTE(TArray<FString> *, OutNames)
	SLATE_ATTRIBUTE(int32 *, OutFilterIndex)
	SLATE_ATTRIBUTE(TWeakPtr<SWindow>, ParentWindow)
	SLATE_ATTRIBUTE(FSlateFileDialogsStyle *, StyleSet)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	FSlateFileDlgWindow::EResult GetResponse() { return UserResponse; }
	void SetOutNames(TArray<FString> *Ptr) { OutNames = Ptr; }
	void SetOutFilterIndex(int32 *InOutFilterIndex) { OutFilterIndex = InOutFilterIndex; }
	void SetDefaultFile(FString DefaultFile);

private:	
	void OnPathClicked( const FString & CrumbData );
	TSharedPtr<SWidget> OnGetCrumbDelimiterContent(const FString& CrumbData) const;
	void RebuildFileTable();
	void BuildDirectoryPath();
	void ReadDir(bool bIsRefresh = false);
	void RefreshCrumbs ( );
	void OnPathMenuItemClicked( FString ClickedPath );
	void OnItemSelected(TSharedPtr<FFileEntry> Item, ESelectInfo::Type SelectInfo);

	void Tick(const FGeometry &AllottedGeometry, const double InCurrentTime, const float InDeltaTime);

	TSharedRef<ITableRow> OnGenerateWidgetForList(TSharedPtr<FFileEntry> Item, const TSharedRef<STableViewBase> &OwnerTable);
	void OnItemDoubleClicked(TSharedPtr<FFileEntry> Item);

	void OnFilterChanged(TSharedPtr<FString> NewValue, ESelectInfo::Type SelectInfo);
	FReply OnAcceptCancelClick(FSlateFileDlgWindow::EResult ButtonID);
	FReply OnQuickLinkClick(FSlateFileDlgWindow::EResult ButtonID);
	FReply OnDirSublevelClick(int32 Level);
	void OnDirectoryChanged(const TArray <FFileChangeData> &FileChanges);	
	void OnFileNameCommitted(const FText & InText, ETextCommit::Type InCommitType);

	void OnNewDirectoryCommitted(const FText & InText, ETextCommit::Type InCommitType);
	FReply OnNewDirectoryClick();
	bool OnNewDirectoryTextChanged(const FText &InText, FText &ErrorMsg);
	FReply OnNewDirectoryAcceptCancelClick(FSlateFileDlgWindow::EResult ButtonID);

	bool GetFilterExtension(FString &OutString);
	void ParseFilters();

	TArray< FDirNode > DirectoryNodesArray;
	TArray<TSharedPtr<FFileEntry>> FoldersArray;
	TArray<TSharedPtr<FFileEntry>> FilesArray;
	TArray<TSharedPtr<FFileEntry>> LineItemArray;	

	TSharedPtr<STextComboBox> FilterCombo;
	TSharedPtr<SHorizontalBox> FilterHBox;
	TSharedPtr<SInlineEditableTextBlock> SaveFilenameEditBox;
	TSharedPtr<SInlineEditableTextBlock> NewDirectoryEditBox;
	TSharedPtr<SBox> SaveFilenameSizeBox;
	TSharedPtr<STextBlock> WindowTitle;
	TSharedPtr<SListView<TSharedPtr<FFileEntry>>> ListView;
	TSharedPtr<SBreadcrumbTrail<FString>> PathBreadcrumbTrail;

	TSharedPtr<SButton> NewDirCancelButton;
	TSharedPtr<SBox> NewDirectorySizeBox;
	TSharedPtr<STextBlock> DirErrorMsg;

	TArray<TSharedPtr<FString>> FilterNameArray;
	TArray<TSharedPtr<FString>> FilterListArray;

	int32 FilterIndex;

	FSlateFileDlgWindow::EResult	 UserResponse;

	bool bNeedsBuilding;
	bool bRebuildDirPath;
	bool bDirectoryHasChanged;

	int32 DirNodeIndex;
	FString SaveFilename;

	TAttribute<TWeakPtr<SWindow>> ParentWindow;
	TAttribute<FString> CurrentPath;
	TAttribute<FString> Filters;
	TAttribute<FString> WindowTitleText;
	TAttribute<bool> bMultiSelectEnabled;
	TAttribute<TArray<FString> *> OutNames;
	TAttribute<int32 *> OutFilterIndex;
	TAttribute<bool> bDirectoriesOnly;
	TAttribute<bool> bSaveFile;
	TAttribute<FText> AcceptText;
	TAttribute<FSlateFileDialogsStyle *> StyleSet;

	IDirectoryWatcher *DirectoryWatcher;
	FDelegateHandle OnDialogDirectoryChangedDelegateHandle;
	FString RegisteredPath;
	FString NewDirectoryName;
};



//-----------------------------------------------------------------------------
class SSlateFileDialogRow : public SMultiColumnTableRow<SSlateFileDialogItemPtr>
{
public:
	
	SLATE_BEGIN_ARGS(SSlateFileDialogRow) { }
	SLATE_ARGUMENT(SSlateFileDialogItemPtr, DialogItem)
	SLATE_ATTRIBUTE(FSlateFileDialogsStyle *, StyleSet)
	SLATE_END_ARGS()


	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase> &InOwnerTableView)
	{
		check(InArgs._DialogItem.IsValid());
		
		DialogItem = InArgs._DialogItem;
		StyleSet = InArgs._StyleSet;
		
		SMultiColumnTableRow<SSlateFileDialogItemPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	virtual TSharedRef<SWidget> GenerateWidgetForColumn( const FName& ColumnName ) override
	{
		FSlateFontInfo ItemFont = StyleSet.Get()->GetFontStyle("SlateFileDialogs.Dialog");
		struct EVisibility FolderIconVisibility = DialogItem->bIsDirectory ? EVisibility::Visible : EVisibility::Hidden;

		if (ColumnName == TEXT("Pathname"))
		{
			return SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					.AutoWidth()
					.Padding(FMargin(5.0f, 2.0f))
					[
						SNew(SImage)
						.Image(StyleSet.Get()->GetBrush("SlateFileDialogs.Folder16"))
						.Visibility(FolderIconVisibility)
					]

					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					.AutoWidth()
					.Padding(FMargin(5.0f, 0.0f, 0.0f, 0.0f))
					[
						SNew(STextBlock)
						.Text(DialogItem->Label)
						.Font(ItemFont)
					];
		}
		else if (ColumnName == TEXT("ModDate"))
		{
			return SNew(STextBlock)
					.Text(DialogItem->ModDate)
					.Font(ItemFont);
		}
		else if (ColumnName == TEXT("FileSize"))
		{
			return SNew(STextBlock)
					.Text(DialogItem->FileSize)
					.Font(ItemFont);
		}
		else
		{
			return SNullWidget::NullWidget;
		}
	}	
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION


private:
	SSlateFileDialogItemPtr DialogItem;
	TAttribute<FSlateFileDialogsStyle *> StyleSet;
};

#undef LOCTEXT_NAMESPACE
