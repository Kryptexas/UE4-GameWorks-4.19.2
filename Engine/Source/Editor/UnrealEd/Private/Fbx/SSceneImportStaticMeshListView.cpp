// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "SSceneImportStaticMeshListView.h"
#include "ClassIconFinder.h"
#include "Dialogs/DlgPickAssetPath.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "SFbxSceneOptionWindow.h"
#include "FbxImporter.h"
#include "STextComboBox.h"

#define LOCTEXT_NAMESPACE "SFbxSceneStaticMeshListView"

const FName SceneImportCheckBoxSelectionHeaderIdName(TEXT("CheckBoxSelectionHeaderId"));
const FName SceneImportClassIconHeaderIdName(TEXT("ClassIconHeaderId"));
const FName SceneImportAssetNameHeaderIdName(TEXT("AssetNameHeaderId"));
const FName SceneImportContentPathHeaderIdName(TEXT("ContentPathHeaderId"));
const FName SceneImportOptionsNameHeaderIdName(TEXT("OptionsNameHeaderId"));

/** The item used for visualizing the class in the tree. */
class SFbxMeshItemTableListViewRow : public SMultiColumnTableRow<FbxMeshInfoPtr>
{
public:

	SLATE_BEGIN_ARGS(SFbxMeshItemTableListViewRow)
		: _FbxMeshInfo(nullptr)
	{}

	/** The item content. */
	SLATE_ARGUMENT(FbxMeshInfoPtr, FbxMeshInfo)
	SLATE_END_ARGS()

	/**
	* Construct the widget
	*
	* @param InArgs   A declaration from which to construct the widget
	*/
	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		FbxMeshInfo = InArgs._FbxMeshInfo;

		//This is suppose to always be valid
		check(FbxMeshInfo.IsValid());

		SMultiColumnTableRow<FbxMeshInfoPtr>::Construct(
			FSuperRowType::FArguments()
			.Style(FEditorStyle::Get(), "DataTableEditor.CellListViewRow"),
			InOwnerTableView
			);
	}

	/** Overridden from SMultiColumnTableRow.  Generates a widget for this column of the list view. */
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (ColumnName == SceneImportCheckBoxSelectionHeaderIdName)
		{
			return SNew(SCheckBox)
				.OnCheckStateChanged(this, &SFbxMeshItemTableListViewRow::OnItemCheckChanged)
				.IsChecked(this, &SFbxMeshItemTableListViewRow::IsItemChecked);
		}
		else if (ColumnName == SceneImportClassIconHeaderIdName)
		{
			UClass *IconClass = FbxMeshInfo->GetType();
			const FSlateBrush* ClassIcon = FClassIconFinder::FindIconForClass(IconClass);

			TSharedRef<SOverlay> IconContent = SNew(SOverlay)
				+ SOverlay::Slot()
				.HAlign(HAlign_Right)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(ClassIcon)
				];


			// Add the component mobility icon
			IconContent->AddSlot()
				.HAlign(HAlign_Left)
				[
					SNew(SImage)
					.Image(this, &SFbxMeshItemTableListViewRow::GetBrushForOverrideIcon)
				];
			return IconContent;
		}
		else if (ColumnName == SceneImportAssetNameHeaderIdName)
		{
			return SNew(STextBlock)
				.Text(FText::FromString(FbxMeshInfo->Name))
				.ToolTipText(FText::FromString(FbxMeshInfo->Name));
		}
		else if (ColumnName == SceneImportContentPathHeaderIdName)
		{
			return SNew(STextBlock)
				.Text(this, &SFbxMeshItemTableListViewRow::GetAssetFullName)
				.ColorAndOpacity(this, &SFbxMeshItemTableListViewRow::GetContentPathTextColor)
				.ToolTipText(this, &SFbxMeshItemTableListViewRow::GetAssetFullName);
		}
		else if (ColumnName == SceneImportOptionsNameHeaderIdName)
		{
			return SNew(STextBlock)
				.Text(this, &SFbxMeshItemTableListViewRow::GetAssetOptionName)
				.ToolTipText(this, &SFbxMeshItemTableListViewRow::GetAssetOptionName);
		}

		return SNullWidget::NullWidget;
	}

	const FSlateBrush* GetBrushForOverrideIcon() const
	{
		if (UFbxSceneImportFactory::DefaultOptionName.Compare(FbxMeshInfo->OptionName) != 0)
		{
			return FEditorStyle::GetBrush("FBXIcon.ImportOptionsOverride");
		}
		return FEditorStyle::GetBrush("FBXIcon.ImportOptionsDefault");
	}

private:

	void OnItemCheckChanged(ECheckBoxState CheckType)
	{
		if (!FbxMeshInfo.IsValid())
			return;
		FbxMeshInfo->bImportAttribute = CheckType == ECheckBoxState::Checked;
	}

	ECheckBoxState IsItemChecked() const
	{
		return FbxMeshInfo->bImportAttribute ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
	}

	FSlateColor GetContentPathTextColor() const
	{
		return FbxMeshInfo->bOverridePath ? FLinearColor(0.75f, 0.75f, 0.0f, 1.0f) : FSlateColor::UseForeground();
	}

	FText GetAssetFullName() const
	{
		return FText::FromString(FbxMeshInfo->GetFullImportName());
	}

	FText GetAssetOptionName() const
	{
			return FText::FromString(FbxMeshInfo->OptionName);
	}

	/** The node info to build the tree view row from. */
	FbxMeshInfoPtr FbxMeshInfo;
};

//////////////////////////////////////////////////////////////////////////
// Static Mesh List

SFbxSceneStaticMeshListView::~SFbxSceneStaticMeshListView()
{
	SceneInfo = nullptr;
	GlobalImportSettings = nullptr;
	SceneImportOptionsStaticMeshDisplay = nullptr;
	CurrentStaticMeshImportOptions = nullptr;
	FbxMeshesArray.Empty();
	SceneImportOptionsStaticMeshDisplay = nullptr;

	OverrideNameOptions.Empty();
	OverrideNameOptionsMap = nullptr;
	OptionComboBox = nullptr;
	DefaultOptionNamePtr = nullptr;
}

void SFbxSceneStaticMeshListView::Construct(const SFbxSceneStaticMeshListView::FArguments& InArgs)
{
	SceneInfo = InArgs._SceneInfo;
	GlobalImportSettings = InArgs._GlobalImportSettings;
	OverrideNameOptionsMap = InArgs._OverrideNameOptionsMap;
	SceneImportOptionsStaticMeshDisplay = InArgs._SceneImportOptionsStaticMeshDisplay;

	check(SceneInfo.IsValid());
	check(GlobalImportSettings != nullptr);
	check(OverrideNameOptionsMap != nullptr);
	check(SceneImportOptionsStaticMeshDisplay != nullptr);

	SFbxSceneOptionWindow::CopyStaticMeshOptionsToFbxOptions(GlobalImportSettings, SceneImportOptionsStaticMeshDisplay);
	//Set the default options to the current global import settings
	GlobalImportSettings->bTransformVertexToAbsolute = false;
	GlobalImportSettings->StaticMeshLODGroup = NAME_None;
	CurrentStaticMeshImportOptions = GlobalImportSettings;
	
	DefaultOptionNamePtr = MakeShareable(new FString(UFbxSceneImportFactory::DefaultOptionName));
	OverrideNameOptions.Add(DefaultOptionNamePtr);
	OverrideNameOptionsMap->Add(UFbxSceneImportFactory::DefaultOptionName, GlobalImportSettings);
	
	for (auto MeshInfoIt = SceneInfo->MeshInfo.CreateIterator(); MeshInfoIt; ++MeshInfoIt)
	{
		FbxMeshInfoPtr MeshInfo = (*MeshInfoIt);

		if (!MeshInfo->bIsSkelMesh)
		{
			MeshInfo->OptionName = UFbxSceneImportFactory::DefaultOptionName;
			FbxMeshesArray.Add(MeshInfo);
		}
	}

	SListView::Construct
	(
		SListView::FArguments()
		.ListItemsSource(&FbxMeshesArray)
		.SelectionMode(ESelectionMode::Multi)
		.OnGenerateRow(this, &SFbxSceneStaticMeshListView::OnGenerateRowFbxSceneListView)
		.OnContextMenuOpening(this, &SFbxSceneStaticMeshListView::OnOpenContextMenu)
		.OnSelectionChanged(this, &SFbxSceneStaticMeshListView::OnSelectionChanged)
		.HeaderRow
		(
			SNew(SHeaderRow)
				
			+ SHeaderRow::Column(SceneImportCheckBoxSelectionHeaderIdName)
			.FixedWidth(20)
			.DefaultLabel(FText::GetEmpty())
			[
				SNew(SCheckBox)
				.HAlign(HAlign_Center)
				.OnCheckStateChanged(this, &SFbxSceneStaticMeshListView::OnToggleSelectAll)
			]
				
			+ SHeaderRow::Column(SceneImportClassIconHeaderIdName)
			.FixedWidth(20)
			.DefaultLabel(FText::GetEmpty())
				
			+ SHeaderRow::Column(SceneImportAssetNameHeaderIdName)
			.FillWidth(300)
			.HAlignCell(EHorizontalAlignment::HAlign_Left)
			.DefaultLabel(LOCTEXT("AssetNameHeaderName", "Asset Name"))

			+ SHeaderRow::Column(SceneImportOptionsNameHeaderIdName)
			.FillWidth(300)
			.HAlignCell(EHorizontalAlignment::HAlign_Left)
			.DefaultLabel(LOCTEXT("OptionsNameHeaderName", "Options Name"))

/*			+ SHeaderRow::Column(SceneImportContentPathHeaderIdName)
			.FillWidth(300)
			.HAlignCell(EHorizontalAlignment::HAlign_Left)
			.DefaultLabel(LOCTEXT("ContentPathHeaderName", "Content Path"))*/
		)
	);
}

TSharedRef< ITableRow > SFbxSceneStaticMeshListView::OnGenerateRowFbxSceneListView(FbxMeshInfoPtr Item, const TSharedRef< STableViewBase >& OwnerTable)
{
	TSharedRef< SFbxMeshItemTableListViewRow > ReturnRow = SNew(SFbxMeshItemTableListViewRow, OwnerTable)
		.FbxMeshInfo(Item);
	return ReturnRow;
}

TSharedPtr<SWidget> SFbxSceneStaticMeshListView::OnOpenContextMenu()
{
	TArray<FbxMeshInfoPtr> SelectedItems;
	int32 SelectCount = GetSelectedItems(SelectedItems);
	// Build up the menu for a selection
	const bool bCloseAfterSelection = true;
	FMenuBuilder MenuBuilder(bCloseAfterSelection, TSharedPtr<FUICommandList>());

	// We always create a section here, even if there is no parent so that clients can still extend the menu
	MenuBuilder.BeginSection("FbxScene_SM_ImportSection");
	{
		const FSlateIcon PlusIcon(FEditorStyle::GetStyleSetName(), "Plus");
		MenuBuilder.AddMenuEntry(LOCTEXT("CheckForImport", "Add Selection To Import"), FText(), PlusIcon, FUIAction(FExecuteAction::CreateSP(this, &SFbxSceneStaticMeshListView::AddSelectionToImport)));
		const FSlateIcon MinusIcon(FEditorStyle::GetStyleSetName(), "PropertyWindow.Button_RemoveFromArray");
		MenuBuilder.AddMenuEntry(LOCTEXT("UncheckForImport", "Remove Selection From Import"), FText(), MinusIcon, FUIAction(FExecuteAction::CreateSP(this, &SFbxSceneStaticMeshListView::RemoveSelectionFromImport)));
		MenuBuilder.AddMenuSeparator(TEXT("FbxImportScene_SM_Separator"));
	}
	MenuBuilder.EndSection();
	MenuBuilder.BeginSection("FbxScene_SM_OptionsSection", LOCTEXT("FbxScene_SM_Options", "Options:"));
	{
		for (auto OptionName : OverrideNameOptions)
		{
			MenuBuilder.AddMenuEntry(FText::FromString(*OptionName.Get()), FText(), FSlateIcon(), FUIAction(FExecuteAction::CreateSP(this, &SFbxSceneStaticMeshListView::AssignToOptions, *OptionName.Get())));
		}
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedPtr<FString> SFbxSceneStaticMeshListView::FindOptionNameFromName(FString OptionName)
{
	for (TSharedPtr<FString> OptionNamePtr : OverrideNameOptions)
	{
		if (OptionNamePtr->Compare(OptionName) == 0)
		{
			return OptionNamePtr;
		}
	}
	return TSharedPtr<FString>();
}

void SFbxSceneStaticMeshListView::AssignToOptions(FString OptionName)
{
	bool IsDefaultOptions = OptionName.Compare(UFbxSceneImportFactory::DefaultOptionName) == 0;
	if (!OverrideNameOptionsMap->Contains(OptionName) && !IsDefaultOptions)
	{
		return;
	}
	TArray<FbxMeshInfoPtr> SelectedItems;
	GetSelectedItems(SelectedItems);
	for (FbxMeshInfoPtr ItemPtr : SelectedItems)
	{
		ItemPtr->OptionName = OptionName;
	}
	OptionComboBox->SetSelectedItem(FindOptionNameFromName(OptionName));
}

void SFbxSceneStaticMeshListView::AddSelectionToImport()
{
	SetSelectionImportState(true);
}

void SFbxSceneStaticMeshListView::RemoveSelectionFromImport()
{
	SetSelectionImportState(false);
}

void SFbxSceneStaticMeshListView::SetSelectionImportState(bool MarkForImport)
{
	TArray<FbxMeshInfoPtr> SelectedItems;
	GetSelectedItems(SelectedItems);
	for (auto Item : SelectedItems)
	{
		FbxMeshInfoPtr ItemPtr = Item;
		ItemPtr->bImportAttribute = MarkForImport;
	}
}

void SFbxSceneStaticMeshListView::OnFinishedChangingProperties(const FPropertyChangedEvent& PropertyChangedEvent)
{
	SFbxSceneOptionWindow::CopyStaticMeshOptionsToFbxOptions(CurrentStaticMeshImportOptions, SceneImportOptionsStaticMeshDisplay);
}


void SFbxSceneStaticMeshListView::OnSelectionChanged(FbxMeshInfoPtr Item, ESelectInfo::Type SelectionType)
{
	//Change the option selection
	TArray<FbxMeshInfoPtr> SelectedItems;
	GetSelectedItems(SelectedItems);
	for (FbxMeshInfoPtr SelectItem : SelectedItems)
	{
		for (auto kvp : *OverrideNameOptionsMap)
		{
			if (kvp.Key.Compare(SelectItem->OptionName) == 0)
			{
				OptionComboBox->SetSelectedItem(FindOptionNameFromName(kvp.Key));
				return;
			}
		}
	}
	//Select Default in case we don't have a valid OptionName
	OptionComboBox->SetSelectedItem(FindOptionNameFromName(UFbxSceneImportFactory::DefaultOptionName));
}


bool SFbxSceneStaticMeshListView::CanDeleteOverride()  const
{
	return CurrentStaticMeshImportOptions != GlobalImportSettings;
}

FReply SFbxSceneStaticMeshListView::OnDeleteOverride()
{
	if (!CanDeleteOverride())
	{
		return FReply::Unhandled();
	}

	FString CurrentOptionName;

	for (auto kvp : *(OverrideNameOptionsMap))
	{
		if (kvp.Value == CurrentStaticMeshImportOptions)
		{
			CurrentOptionName = kvp.Key;
			if (CurrentOptionName.Compare(UFbxSceneImportFactory::DefaultOptionName) == 0)
			{
				//Cannot delete the default options
				return FReply::Handled();
			}
			break;
		}
	}
	if (CurrentOptionName.IsEmpty())
	{
		return FReply::Handled();
	}

	TArray<TSharedPtr<FFbxMeshInfo>> RemoveKeys;
	TArray<TSharedPtr<FFbxMeshInfo>> SceneMeshOverrideKeys;
	for (TSharedPtr<FFbxMeshInfo> MeshInfo : FbxMeshesArray)
	{
		if (CurrentOptionName.Compare(MeshInfo->OptionName) == 0)
		{
			MeshInfo->OptionName = UFbxSceneImportFactory::DefaultOptionName;
		}
	}
	OverrideNameOptions.Remove(FindOptionNameFromName(CurrentOptionName));
	OverrideNameOptionsMap->Remove(CurrentOptionName);
	UnFbx::FBXImportOptions *OldOverrideOption = CurrentStaticMeshImportOptions;
	delete OldOverrideOption;

	CurrentStaticMeshImportOptions = GlobalImportSettings;
	OptionComboBox->SetSelectedItem(OverrideNameOptions[0]);

	return FReply::Handled();
}

FReply SFbxSceneStaticMeshListView::OnSelectAssetUsing()
{
	ClearSelection();
	FString CurrentOptionName;

	for (auto kvp : *(OverrideNameOptionsMap))
	{
		if (kvp.Value == CurrentStaticMeshImportOptions)
		{
			CurrentOptionName = kvp.Key;
			break;
		}
	}
	if (CurrentOptionName.IsEmpty())
	{
		return FReply::Handled();
	}

	for (FbxMeshInfoPtr MeshInfo : FbxMeshesArray)
	{
		if (CurrentOptionName.Compare(MeshInfo->OptionName) == 0)
		{
			SetItemSelection(MeshInfo, true);
		}
	}
	return FReply::Handled();
}

void SFbxSceneStaticMeshListView::OnChangedOverrideOptions(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo)
{
	check(ItemSelected.IsValid());
	if (ItemSelected.Get()->Compare(UFbxSceneImportFactory::DefaultOptionName) == 0)
	{
		CurrentStaticMeshImportOptions = GlobalImportSettings;
	}
	else if(OverrideNameOptionsMap->Contains(*(ItemSelected.Get())))
	{
		CurrentStaticMeshImportOptions = *OverrideNameOptionsMap->Find(*(ItemSelected.Get()));
	}
	SFbxSceneOptionWindow::CopyFbxOptionsToStaticMeshOptions(CurrentStaticMeshImportOptions, SceneImportOptionsStaticMeshDisplay);
}

FString SFbxSceneStaticMeshListView::FindUniqueOptionName()
{
	int32 SuffixeIndex = 1;
	bool bFoundSimilarName = true;
	FString UniqueOptionName;
	while (bFoundSimilarName)
	{
		UniqueOptionName = TEXT("Options ") + FString::FromInt(SuffixeIndex++);
		bFoundSimilarName = false;
		for (auto kvp : *(OverrideNameOptionsMap))
		{
			if (kvp.Key.Compare(UniqueOptionName) == 0)
			{
				bFoundSimilarName = true;
				break;
			}
		}
	}
	return UniqueOptionName;
}

FReply SFbxSceneStaticMeshListView::OnCreateOverrideOptions()
{
	UnFbx::FBXImportOptions *OverrideOption = new UnFbx::FBXImportOptions();
	SFbxSceneOptionWindow::CopyFbxOptionsToFbxOptions(GlobalImportSettings, OverrideOption);

	FString OverrideName = FindUniqueOptionName();
	TSharedPtr<FString> OverrideNamePtr = MakeShareable(new FString(OverrideName));
	OverrideNameOptions.Add(OverrideNamePtr);
	OverrideNameOptionsMap->Add(OverrideName, OverrideOption);
	//Update the selection to the new override
	OptionComboBox->SetSelectedItem(OverrideNamePtr);
	return FReply::Handled();
}

TSharedPtr<STextComboBox> SFbxSceneStaticMeshListView::CreateOverrideOptionComboBox()
{
	OptionComboBox = SNew(STextComboBox)
		.OptionsSource(&OverrideNameOptions)
		.InitiallySelectedItem(DefaultOptionNamePtr)
		.OnSelectionChanged(this, &SFbxSceneStaticMeshListView::OnChangedOverrideOptions);
	return OptionComboBox;
}

void SFbxSceneStaticMeshListView::OnToggleSelectAll(ECheckBoxState CheckType)
{
	for (auto MeshInfo : FbxMeshesArray)
	{
		MeshInfo->bImportAttribute = CheckType == ECheckBoxState::Checked;
	}
}

#undef LOCTEXT_NAMESPACE
