// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#include "UnrealEd.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "FoliageEdMode.h"
#include "FoliageEditActions.h"
#include "SFoliagePalette.h"
#include "FoliageTypePaintingCustomization.h"
#include "FoliagePaletteItem.h"

#include "LevelEditor.h"
#include "Editor/UnrealEd/Public/AssetThumbnail.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/PropertyEditor/Public/PropertyCustomizationHelpers.h"
#include "Editor/UnrealEd/Public/DragAndDrop/AssetDragDropOp.h"
#include "Editor/UnrealEd/Public/AssetSelection.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Engine/StaticMesh.h"

#include "SScaleBox.h"
#include "SWidgetSwitcher.h"
#include "SExpandableArea.h"

#define LOCTEXT_NAMESPACE "FoliageEd_Mode"

void SFoliagePalette::Construct(const FArguments& InArgs)
{
	FoliageEditMode = InArgs._FoliageEdMode;

	//@todo: Save these as config options
	bShowTooltips = true;
	ActiveViewMode = EPaletteViewMode::Thumbnail;

	FoliageEditMode->OnToolChanged.AddSP(this, &SFoliagePalette::HandleOnToolChanged);

	BindCommands();

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args(false, false, false, FDetailsViewArgs::HideNameArea, true);
	Args.bShowActorLabel = false;
	DetailsWidget = PropertyModule.CreateDetailView(Args);

	// We want to use our own customization for UFoliageType
	DetailsWidget->RegisterInstancedCustomPropertyLayout(UFoliageType::StaticClass(), 
		FOnGetDetailCustomizationInstance::CreateStatic(&FFoliageTypePaintingCustomization::MakeInstance, FoliageEditMode)
		);

	FMargin StandardPadding(6.f, 3.f);
	const FText BlankText = LOCTEXT("Blank", "");

	// Tile View Widget
	static const float TileSize = 64.f;
	SAssignNew(TileViewWidget, SFoliageTypeTileView)
		.ListItemsSource(&FoliageEditMode->GetFoliageMeshList())
		.SelectionMode(ESelectionMode::Multi)
		.OnGenerateTile(this, &SFoliagePalette::GenerateTile)
		.OnContextMenuOpening(this, &SFoliagePalette::ConstructFoliageTypeContextMenu)
		.OnSelectionChanged(this, &SFoliagePalette::OnSelectionChanged)
		.ItemHeight(TileSize)
		.ItemWidth(TileSize)
		.ItemAlignment(EListItemAlignment::LeftAligned)
		.ClearSelectionOnClick(true)
		.OnMouseButtonDoubleClick(this, &SFoliagePalette::OnItemDoubleClicked);

	// Tree View Widget
	SAssignNew(TreeViewWidget, SFoliageTypeTreeView)
	.TreeItemsSource(&FoliageEditMode->GetFoliageMeshList())
	.SelectionMode(ESelectionMode::Multi)
	.OnGenerateRow(this, &SFoliagePalette::TreeViewGenerateRow)
	.OnGetChildren(this, &SFoliagePalette::TreeViewGetChildren)
	.OnContextMenuOpening(this, &SFoliagePalette::ConstructFoliageTypeContextMenu)
	.OnSelectionChanged(this, &SFoliagePalette::OnSelectionChanged)
	.OnMouseButtonDoubleClick(this, &SFoliagePalette::OnItemDoubleClicked)
	.HeaderRow
	(
		// Toggle Active
		SAssignNew(TreeViewHeaderRow, SHeaderRow)
		+ SHeaderRow::Column(FoliagePaletteTreeColumns::ColumnID_ToggleActive)
		[
			SNew(SCheckBox)
			.IsChecked(this, &SFoliagePalette::GetState_AllMeshes)
			.OnCheckStateChanged(this, &SFoliagePalette::OnCheckStateChanged_AllMeshes)
		]
		.DefaultLabel(BlankText)
		.HeaderContentPadding(FMargin(0, 1, 0, 1))
		.HAlignHeader(HAlign_Center)
		.HAlignCell(HAlign_Center)
		.FixedWidth(24)

		// Type
		+ SHeaderRow::Column(FoliagePaletteTreeColumns::ColumnID_Type)
		.HeaderContentPadding(FMargin(10, 1, 0, 1))
		.SortMode(this, &SFoliagePalette::GetMeshColumnSortMode)
		.OnSort(this, &SFoliagePalette::OnMeshesColumnSortModeChanged)
		.DefaultLabel(this, &SFoliagePalette::GetMeshesHeaderText)

		// Instance Count
		+ SHeaderRow::Column(FoliagePaletteTreeColumns::ColumnID_InstanceCount)
		.HeaderContentPadding(FMargin(10, 1, 0, 1))
		.DefaultLabel(LOCTEXT("InstanceCount", "Count"))
		.DefaultTooltip(this, &SFoliagePalette::GetTotalInstanceCountTooltipText)
		.FixedWidth(60.f)

		// Save Asset
		+ SHeaderRow::Column(FoliagePaletteTreeColumns::ColumnID_Save)
		.FixedWidth(24.0f)
		.DefaultLabel(BlankText)
	);

	ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Fill)
		[
			// Top bar
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryTop"))
			.Padding(FMargin(6.f, 2.f))
			.BorderBackgroundColor(FLinearColor(.6f, .6f, .6f, 1.0f))
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					// +Add Foliage Type button
					SAssignNew(AddFoliageTypeCombo, SComboButton)
					.ForegroundColor(FLinearColor::White)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
					.OnGetMenuContent(this, &SFoliagePalette::GetAddFoliageTypePicker)
					.ContentPadding(FMargin(1.f))
					.ButtonContent()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.AutoWidth()
						.Padding(1.f)
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "FoliageEditMode.AddFoliageType.Text")
							.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.9"))
							.Text(FText::FromString(FString(TEXT("\xf067"))) /*fa-plus*/)
						]
						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(1.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("AddFoliageTypeButtonLabel", "Add Foliage Type"))
							.TextStyle(FEditorStyle::Get(), "FoliageEditMode.AddFoliageType.Text")
						]
					]
				]

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Right)
				[
					// View Options
					SNew( SComboButton )
					.ContentPadding(0)
					.ForegroundColor( FSlateColor::UseForeground() )
					.ButtonStyle( FEditorStyle::Get(), "ToggleButton" )
					.OnGetMenuContent(this, &SFoliagePalette::GetViewOptionsMenuContent)
					.ButtonContent()
					[
						SNew(SImage)
						.Image( FEditorStyle::GetBrush("GenericViewButton") )
					]
				]
				
			]
		]

		+ SVerticalBox::Slot()
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			.Style(FEditorStyle::Get(), "FoliageEditMode.Splitter")

			+ SSplitter::Slot()
			.Value(0.6f)
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(StandardPadding)
				[
					SNew(SBox)
					.Visibility(this, &SFoliagePalette::GetDropFoliageHintVisibility)
					.Padding(FMargin(15, 0))
					.MinDesiredHeight(30)
					[
						SNew(SScaleBox)
						.Stretch(EStretch::ScaleToFit)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Foliage_DropStatic", "+ Drop Foliage Here"))
							.ToolTipText(LOCTEXT("Foliage_DropStatic_ToolTip", "Drag and drop foliage types or static meshes from the Content Browser to add them to the palette"))
						]
					]
				]

				+ SVerticalBox::Slot()
				[
					SAssignNew(WidgetSwitcher, SWidgetSwitcher)

					+ SWidgetSwitcher::Slot()
					[
						// Thumbnail View
						SNew(SScrollBorder, TileViewWidget.ToSharedRef())
						.Content()
						[
							TileViewWidget.ToSharedRef()
						]
					]

					+ SWidgetSwitcher::Slot()
					[
						// Tree View
						SNew(SScrollBorder, TreeViewWidget.ToSharedRef())
						.Style(&FEditorStyle::Get().GetWidgetStyle<FScrollBorderStyle>("FoliageEditMode.TreeView.ScrollBorder"))
						.Content()
						[
							TreeViewWidget.ToSharedRef()
						]
					]
				]
				
				+ SVerticalBox::Slot()
				.Padding(FMargin(0.f))
				.VAlign(VAlign_Bottom)
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					
					// Selected type name area
					+ SHorizontalBox::Slot()
					.Padding(FMargin(3.f))
					.VAlign(VAlign_Bottom)
					.AutoWidth()
					[
						SNew(STextBlock)
						.Text(this, &SFoliagePalette::GetDetailsNameAreaText)
					]

					// Show/Hide Details
					+ SHorizontalBox::Slot()
					.HAlign(HAlign_Right)
					[
						SNew(SButton)
						.ToolTipText(this, &SFoliagePalette::GetShowHideDetailsTooltipText)
						.ForegroundColor(FSlateColor::UseForeground())
						.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
						.OnClicked(this, &SFoliagePalette::OnShowHideDetailsClicked)
						.ContentPadding(FMargin(2.f))
						.Content()
						[
							SNew(SHorizontalBox)

							+SHorizontalBox::Slot()
							.AutoWidth()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
								.Image(FEditorStyle::GetBrush("LevelEditor.Tabs.Details"))
							]

							+ SHorizontalBox::Slot()
							.Padding(FMargin(3.f, 0.f))
							.AutoWidth()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Center)
							[
								SNew(SImage)
								.Image(this, &SFoliagePalette::GetShowHideDetailsImage)
							]
						]
					]
				]
			]

			// Details
			+SSplitter::Slot()
			[
				DetailsWidget.ToSharedRef()
			]
		]
	];
}

void SFoliagePalette::Refresh()
{
	TreeViewWidget->RequestTreeRefresh();
	TileViewWidget->RequestListRefresh();
}

bool SFoliagePalette::AnySelectedTileHovered() const
{
	for (auto& TypeInfo : GetActiveViewWidget()->GetSelectedItems())
	{
		if (TileViewWidget->WidgetFromItem(TypeInfo)->AsWidget()->IsHovered())
		{
			return true;
		}
	}

	return false;
}

void SFoliagePalette::ActivateAllSelectedTypes(bool bActivate) const
{
	// Apply the new check state to all of the selected types
	for (FFoliageMeshUIInfoPtr& TypeInfo : GetActiveViewWidget()->GetSelectedItems())
	{
		if (TypeInfo->Settings->IsSelected != bActivate)
		{
			TypeInfo->Settings->Modify();
			TypeInfo->Settings->IsSelected = bActivate;
		}
	}
}

void SFoliagePalette::BindCommands()
{
	UICommandList = MakeShareable(new FUICommandList);
	const FFoliageEditCommands& Commands = FFoliageEditCommands::Get();

	// Context menu commands
	UICommandList->MapAction(
		Commands.RemoveFoliageType,
		FExecuteAction::CreateSP(this, &SFoliagePalette::OnRemoveFoliageType),
		FCanExecuteAction());

	UICommandList->MapAction(
		Commands.ShowFoliageTypeInCB,
		FExecuteAction::CreateSP(this, &SFoliagePalette::OnShowFoliageTypeInCB),
		FCanExecuteAction());

	UICommandList->MapAction(
		Commands.SelectAllInstances,
		FExecuteAction::CreateSP(this, &SFoliagePalette::OnSelectAllInstances),
		FCanExecuteAction::CreateSP(this, &SFoliagePalette::CanSelectInstances));

	UICommandList->MapAction(
		Commands.DeselectAllInstances,
		FExecuteAction::CreateSP(this, &SFoliagePalette::OnDeselectAllInstances),
		FCanExecuteAction::CreateSP(this, &SFoliagePalette::CanSelectInstances));

	UICommandList->MapAction(
		Commands.SelectInvalidInstances,
		FExecuteAction::CreateSP(this, &SFoliagePalette::OnSelectInvalidInstances),
		FCanExecuteAction::CreateSP(this, &SFoliagePalette::CanSelectInstances));
}

void SFoliagePalette::AddFoliageType(const FAssetData& AssetData)
{
	if (AddFoliageTypeCombo.IsValid())
	{
		AddFoliageTypeCombo->SetIsOpen(false);
	}

	GWarn->BeginSlowTask(LOCTEXT("AddFoliageType_LoadPackage", "Loading Foliage Type"), true, false);
	UObject* Asset = AssetData.GetAsset();
	GWarn->EndSlowTask();

	UFoliageType* FoliageType = FoliageEditMode->AddFoliageAsset(Asset);
	if (FoliageType)
	{
		Refresh();
	}
}

TSharedRef<SWidget> SFoliagePalette::GetAddFoliageTypePicker()
{
	TArray<const UClass*> ClassFilters;
	ClassFilters.Add(UFoliageType_InstancedStaticMesh::StaticClass());

	return PropertyCustomizationHelpers::MakeAssetPickerWithMenu(FAssetData(),
		false,
		ClassFilters,
		PropertyCustomizationHelpers::GetNewAssetFactoriesForClasses(ClassFilters),
		FOnShouldFilterAsset(),
		FOnAssetSelected::CreateSP(this, &SFoliagePalette::AddFoliageType),
		FSimpleDelegate());
}

void SFoliagePalette::HandleOnToolChanged()
{
	RefreshMeshDetailsWidget();
}

void SFoliagePalette::SetViewMode(EPaletteViewMode::Type NewViewMode)
{
	if (ActiveViewMode != NewViewMode)
	{
		switch (NewViewMode)
		{
		case EPaletteViewMode::Thumbnail:
			// Set the tile selection to be the current tree selections
			TileViewWidget->ClearSelection();
			for (auto& TypeInfo : TreeViewWidget->GetSelectedItems())
			{
				TileViewWidget->SetItemSelection(TypeInfo, true);
			}
			break;
			
		case EPaletteViewMode::Tree:
			// Set the tree selection to be the current tile selection
			TreeViewWidget->ClearSelection();
			for (auto& TypeInfo : TileViewWidget->GetSelectedItems())
			{
				TreeViewWidget->SetItemSelection(TypeInfo, true);
			}
			break;
		}

		ActiveViewMode = NewViewMode;
		WidgetSwitcher->SetActiveWidgetIndex(ActiveViewMode);
	}
}

bool SFoliagePalette::IsActiveViewMode(EPaletteViewMode::Type ViewMode) const
{
	return ActiveViewMode == ViewMode;
}

void SFoliagePalette::ToggleShowTooltips()
{
	bShowTooltips = !bShowTooltips;
}

bool SFoliagePalette::ShouldShowTooltips() const
{
	return bShowTooltips;
}

void SFoliagePalette::OnSelectionChanged(FFoliageMeshUIInfoPtr Item, ESelectInfo::Type SelectInfo)
{
	RefreshMeshDetailsWidget();
}

void SFoliagePalette::OnItemDoubleClicked(FFoliageMeshUIInfoPtr Item) const
{
	Item->Settings->IsSelected = !Item->Settings->IsSelected;
}

TSharedRef<SWidget> SFoliagePalette::GetViewOptionsMenuContent()
{	
	const FFoliageEditCommands& Commands = FFoliageEditCommands::Get();
	FMenuBuilder MenuBuilder(true, UICommandList);

	MenuBuilder.BeginSection("FoliagePaletteViewMode", LOCTEXT("ViewModeHeading", "Palette View Mode"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ThumbnailView", "Thumbnails"),
			LOCTEXT("ThumbnailView_ToolTip", "Display thumbnails for each foliage type in the palette."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SFoliagePalette::SetViewMode, EPaletteViewMode::Thumbnail),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SFoliagePalette::IsActiveViewMode, EPaletteViewMode::Thumbnail)
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);

		MenuBuilder.AddMenuEntry(
			LOCTEXT("ListView", "List"),
			LOCTEXT("ListView_ToolTip", "Display foliage types in the palette as a list."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SFoliagePalette::SetViewMode, EPaletteViewMode::Tree),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SFoliagePalette::IsActiveViewMode, EPaletteViewMode::Tree)
			),
			NAME_None,
			EUserInterfaceActionType::RadioButton
		);
	}
	MenuBuilder.EndSection();

	MenuBuilder.BeginSection("FoliagePaletteViewOptions", LOCTEXT("ViewOptionsHeading", "View Options"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("ShowTooltips", "Show Tooltips"),
			LOCTEXT("ShowTooltips_ToolTip", "Whether to show tooltips when hovering over foliage types in the palette."),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateSP(this, &SFoliagePalette::ToggleShowTooltips),
				FCanExecuteAction(),
				FIsActionChecked::CreateSP(this, &SFoliagePalette::ShouldShowTooltips)
			),
			NAME_None,
			EUserInterfaceActionType::ToggleButton
		);

		// Tile size slider
		// Hide inactive types
	}
	MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

TSharedPtr<SListView<FFoliageMeshUIInfoPtr>> SFoliagePalette::GetActiveViewWidget() const
{
	if (ActiveViewMode == EPaletteViewMode::Thumbnail)
	{
		return TileViewWidget;
	}
	else if (ActiveViewMode == EPaletteViewMode::Tree)
	{
		return TreeViewWidget;
	}
	
	return nullptr;
}

EVisibility SFoliagePalette::GetDropFoliageHintVisibility() const
{
	return FoliageEditMode->GetFoliageMeshList().Num() == 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

//	CONTEXT MENU

TSharedPtr<SWidget> SFoliagePalette::ConstructFoliageTypeContextMenu()
{
	const FFoliageEditCommands& Commands = FFoliageEditCommands::Get();
	FMenuBuilder MenuBuilder(true, UICommandList);

	if (GetActiveViewWidget()->GetSelectedItems().Num() > 0)
	{
		if (AreAnyNonAssetTypesSelected())
		{
			MenuBuilder.BeginSection("StaticMeshFoliageTypeOptions", LOCTEXT("StaticMeshFoliageTypeOptionsHeader", "Static Mesh"));
			{
				MenuBuilder.AddMenuEntry(
					LOCTEXT("SaveAsFoliageType", "Save As Foliage Type..."),
					LOCTEXT("SaveAsFoliageType_ToolTip", "Creates a Foliage Type asset with these settings that can be reused in other levels."),
					FSlateIcon(FEditorStyle::GetStyleSetName(), "Level.SaveIcon16x"),
					FUIAction(
						FExecuteAction::CreateSP(this, &SFoliagePalette::OnSaveSelectedAsFoliageType),
						FCanExecuteAction::CreateSP(this, &SFoliagePalette::OnCanSaveSelectedAsFoliageType)
					),
					NAME_None
				);
			}
			MenuBuilder.EndSection();
		}

		MenuBuilder.BeginSection("FoliageTypeOptions", LOCTEXT("FoliageTypeOptionsHeader", "Foliage Type"));
		{
			MenuBuilder.AddMenuEntry(Commands.RemoveFoliageType);

			MenuBuilder.AddSubMenu(
				LOCTEXT("ReplaceFoliageType", "Replace With..."),
				LOCTEXT("ReplaceFoliageType_ToolTip", "Replaces selected foliage type with another foliage type asset"),
				FNewMenuDelegate::CreateSP(this, &SFoliagePalette::FillReplaceFoliageTypeSubmenu));

			MenuBuilder.AddMenuEntry(Commands.ShowFoliageTypeInCB);
		}
		MenuBuilder.EndSection();
		
		MenuBuilder.BeginSection("InstanceSelectionOptions", LOCTEXT("InstanceSelectionOptionsHeader", "Selection"));
		{
			MenuBuilder.AddMenuEntry(Commands.SelectAllInstances);
			MenuBuilder.AddMenuEntry(Commands.DeselectAllInstances);
			MenuBuilder.AddMenuEntry(Commands.SelectInvalidInstances);
		}
		MenuBuilder.EndSection();
	}

	return MenuBuilder.MakeWidget();
}

void SFoliagePalette::OnSaveSelectedAsFoliageType()
{
	for (FFoliageMeshUIInfoPtr& TypeInfo : GetActiveViewWidget()->GetSelectedItems())
	{
		UFoliageType* SavedSettings = FoliageEditMode->SaveFoliageTypeObject(TypeInfo->Settings);
		if (SavedSettings)
		{
			TypeInfo->Settings = SavedSettings;
		}
	}
}

bool SFoliagePalette::OnCanSaveSelectedAsFoliageType() const
{
	for (FFoliageMeshUIInfoPtr& TypeInfo : GetActiveViewWidget()->GetSelectedItems())
	{
		if (TypeInfo->Settings->IsAsset())
		{
			// At least one selected type is an asset
			return false;
		}
	}

	return true;
}

bool SFoliagePalette::AreAnyNonAssetTypesSelected() const
{
	for (FFoliageMeshUIInfoPtr& TypeInfo : GetActiveViewWidget()->GetSelectedItems())
	{
		if (!TypeInfo->Settings->IsAsset())
		{
			// At least one selected type isn't an asset
			return true;
		}
	}

	return false;
}

void SFoliagePalette::FillReplaceFoliageTypeSubmenu(FMenuBuilder& MenuBuilder)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(UFoliageType::StaticClass()->GetFName());
	AssetPickerConfig.Filter.bRecursiveClasses = true;
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SFoliagePalette::OnReplaceFoliageTypeSelected);
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
	AssetPickerConfig.bAllowNullSelection = false;

	TSharedRef<SWidget> MenuContent = SNew(SBox)
		.WidthOverride(384)
		.HeightOverride(500)
		[
			ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
		];

	MenuBuilder.AddWidget(MenuContent, FText::GetEmpty(), true);
}

void SFoliagePalette::OnReplaceFoliageTypeSelected(const FAssetData& AssetData)
{
	FSlateApplication::Get().DismissAllMenus();

	UFoliageType* NewSettings = Cast<UFoliageType>(AssetData.GetAsset());
	if (GetActiveViewWidget()->GetSelectedItems().Num() && NewSettings)
	{
		for (FFoliageMeshUIInfoPtr& TypeInfo : GetActiveViewWidget()->GetSelectedItems())
		{
			UFoliageType* OldSettings = TypeInfo->Settings;
			if (OldSettings != NewSettings)
			{
				FoliageEditMode->ReplaceSettingsObject(OldSettings, NewSettings);
			}
		}
	}
}

void SFoliagePalette::OnRemoveFoliageType()
{
	int32 NumInstances = 0;
	TArray<UFoliageType*> FoliageTypeList;
	for (FFoliageMeshUIInfoPtr& TypeInfo : GetActiveViewWidget()->GetSelectedItems())
	{
		NumInstances += TypeInfo->InstanceCountTotal;
		FoliageTypeList.Add(TypeInfo->Settings);
	}

	bool bProceed = true;
	if (NumInstances > 0)
	{
		FText Message = FText::Format(NSLOCTEXT("UnrealEd", "FoliageMode_DeleteMesh", "Are you sure you want to remove {0} instances?"), FText::AsNumber(NumInstances));
		bProceed = (FMessageDialog::Open(EAppMsgType::YesNo, Message) == EAppReturnType::Yes);
	}

	if (bProceed)
	{
		FoliageEditMode->RemoveFoliageType(FoliageTypeList.GetData(), FoliageTypeList.Num());
	}
}

void SFoliagePalette::OnShowFoliageTypeInCB()
{
	TArray<UObject*> SelectedAssets;
	for (FFoliageMeshUIInfoPtr& TypeInfo : GetActiveViewWidget()->GetSelectedItems())
	{
		UFoliageType* FoliageType = TypeInfo->Settings;
		if (FoliageType->IsAsset())
		{
			SelectedAssets.Add(FoliageType);
		}
		else
		{
			SelectedAssets.Add(FoliageType->GetStaticMesh());
		}
	}

	if (SelectedAssets.Num())
	{
		GEditor->SyncBrowserToObjects(SelectedAssets);
	}
}

void SFoliagePalette::OnSelectAllInstances()
{
	for (FFoliageMeshUIInfoPtr& TypeInfo : GetActiveViewWidget()->GetSelectedItems())
	{
		UFoliageType* FoliageType = TypeInfo->Settings;
		FoliageEditMode->SelectInstances(FoliageType, true);
	}
}

void SFoliagePalette::OnDeselectAllInstances()
{
	for (FFoliageMeshUIInfoPtr& TypeInfo : GetActiveViewWidget()->GetSelectedItems())
	{
		UFoliageType* FoliageType = TypeInfo->Settings;
		FoliageEditMode->SelectInstances(FoliageType, false);
	}
}

void SFoliagePalette::OnSelectInvalidInstances()
{
	for (FFoliageMeshUIInfoPtr& TypeInfo : GetActiveViewWidget()->GetSelectedItems())
	{
		const UFoliageType* FoliageType = TypeInfo->Settings;
		FoliageEditMode->SelectInvalidInstances(FoliageType);
	}
}

bool SFoliagePalette::CanSelectInstances() const
{
	return FoliageEditMode->UISettings.GetSelectToolSelected() || FoliageEditMode->UISettings.GetLassoSelectToolSelected();
}

// THUMBNAIL VIEW

TSharedRef<ITableRow> SFoliagePalette::GenerateTile(FFoliageMeshUIInfoPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedPtr<FFoliagePaletteItemModel> ItemModel = MakeShareable(new FFoliagePaletteItemModel(Item, SharedThis(this), FoliageEditMode));
	return SNew(SFoliagePaletteItemTile, OwnerTable, ItemModel);
}

// TREE VIEW

TSharedRef<ITableRow> SFoliagePalette::TreeViewGenerateRow(FFoliageMeshUIInfoPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedPtr<FFoliagePaletteItemModel> ItemModel = MakeShareable(new FFoliagePaletteItemModel(Item, SharedThis(this), FoliageEditMode));
	return SNew(SFoliagePaletteItemRow, OwnerTable, ItemModel);
}

void SFoliagePalette::TreeViewGetChildren(FFoliageMeshUIInfoPtr Item, TArray<FFoliageMeshUIInfoPtr>& OutChildren)
{
	//OutChildren = Item->GetChildren();
}

ECheckBoxState SFoliagePalette::GetState_AllMeshes() const
{
	bool bHasChecked = false;
	bool bHasUnchecked = false;

	for (const FFoliageMeshUIInfoPtr& TypeInfo : FoliageEditMode->GetFoliageMeshList())
	{
		if (TypeInfo->Settings->IsSelected)
		{
			bHasChecked = true;
		}
		else
		{
			bHasUnchecked = true;
		}

		if (bHasChecked && bHasUnchecked)
		{
			return ECheckBoxState::Undetermined;
		}
	}

	return bHasChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SFoliagePalette::OnCheckStateChanged_AllMeshes(ECheckBoxState InState)
{
	for (const FFoliageMeshUIInfoPtr& TypeInfo : FoliageEditMode->GetFoliageMeshList())
	{
		TypeInfo->Settings->IsSelected = (InState == ECheckBoxState::Checked);
	}
}

FText SFoliagePalette::GetMeshesHeaderText() const
{
	int32 NumMeshes = FoliageEditMode->GetFoliageMeshList().Num();
	return FText::Format(LOCTEXT("FoliageMeshCount", "Meshes ({0})"), FText::AsNumber(NumMeshes));
}

EColumnSortMode::Type SFoliagePalette::GetMeshColumnSortMode() const
{
	return FoliageEditMode->GetFoliageMeshListSortMode();
}

void SFoliagePalette::OnMeshesColumnSortModeChanged(EColumnSortPriority::Type InPriority, const FName& InColumnName, EColumnSortMode::Type InSortMode)
{
	FoliageEditMode->OnFoliageMeshListSortModeChanged(InSortMode);
}

FText SFoliagePalette::GetTotalInstanceCountTooltipText() const
{
	// Probably should cache these values, 
	// but we call this only occasionally when tooltip is active
	int32 InstanceCountTotal = 0;
	int32 InstanceCountCurrentLevel = 0;
	FoliageEditMode->CalcTotalInstanceCount(InstanceCountTotal, InstanceCountCurrentLevel);

	return FText::Format(LOCTEXT("FoliageTotalInstanceCount", "Current Level: {0} Total: {1}"), FText::AsNumber(InstanceCountCurrentLevel), FText::AsNumber(InstanceCountTotal));
}

//	DETAILS VIEW

void SFoliagePalette::RefreshMeshDetailsWidget()
{
	TArray<UObject*> SelectedFoliageTypes;

	for (const auto& Info : GetActiveViewWidget()->GetSelectedItems())
	{
		SelectedFoliageTypes.Add(Info->Settings);
	}

	const bool bForceRefresh = true;
	DetailsWidget->SetObjects(SelectedFoliageTypes, bForceRefresh);
}

FText SFoliagePalette::GetDetailsNameAreaText() const
{
	FText OutText;

	auto SelectedTypes = GetActiveViewWidget()->GetSelectedItems();
	if (SelectedTypes.Num() == 1)
	{
		FName DisplayName;
		UFoliageType* SelectedType = SelectedTypes[0]->Settings;
		if (SelectedType->IsAsset())
		{
			DisplayName = SelectedType->GetFName();
		}
		else
		{
			DisplayName = SelectedType->GetStaticMesh()->GetFName();
		}
		OutText = FText::FromName(DisplayName);
	}
	else if (SelectedTypes.Num() > 1)
	{
		OutText = FText::Format(LOCTEXT("DetailsNameAreaText_Multiple", "{0} Types Selected"), FText::AsNumber(SelectedTypes.Num()));
	}

	return OutText;
}

FText SFoliagePalette::GetShowHideDetailsTooltipText() const
{
	const bool bDetailsCurrentlyVisible = DetailsWidget->GetVisibility() != EVisibility::Collapsed;
	return bDetailsCurrentlyVisible ? LOCTEXT("HideDetails_Tooltip", "Hide details for the selected foliage types.") : LOCTEXT("ShowDetails_Tooltip", "Show details for the selected foliage types.");
}

const FSlateBrush* SFoliagePalette::GetShowHideDetailsImage() const
{
	const bool bDetailsCurrentlyVisible = DetailsWidget->GetVisibility() != EVisibility::Collapsed;
	return FEditorStyle::Get().GetBrush(bDetailsCurrentlyVisible ? "Symbols.DoubleDownArrow" : "Symbols.DoubleUpArrow");
}

FReply SFoliagePalette::OnShowHideDetailsClicked() const
{
	const bool bDetailsCurrentlyVisible = DetailsWidget->GetVisibility() != EVisibility::Collapsed;
	DetailsWidget->SetVisibility(bDetailsCurrentlyVisible ? EVisibility::Collapsed : EVisibility::SelfHitTestInvisible);

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE