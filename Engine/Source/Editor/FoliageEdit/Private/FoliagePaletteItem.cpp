// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "FoliagePaletteItem.h"
#include "SFoliagePalette.h"
#include "FoliageEdMode.h"
#include "FoliageType.h"

#include "LevelEditor.h"
#include "Editor/UnrealEd/Public/AssetThumbnail.h"
#include "Engine/StaticMesh.h"

#define LOCTEXT_NAMESPACE "FoliageEd_Mode"

FFoliagePaletteItemModel::FFoliagePaletteItemModel(FFoliageMeshUIInfoPtr InTypeInfo, TSharedRef<SFoliagePalette> InFoliagePalette, FEdModeFoliage* InFoliageEditMode)
	: TypeInfo(InTypeInfo)
	, FoliagePalette(InFoliagePalette)
	, FoliageEditMode(InFoliageEditMode)
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool = LevelEditorModule.GetFirstLevelEditor()->GetThumbnailPool();
	FAssetData AssetData = FAssetData(TypeInfo->Settings->GetStaticMesh());

	TSharedPtr< FAssetThumbnail > Thumbnail = MakeShareable(new FAssetThumbnail(AssetData, 128, 128, ThumbnailPool));
	FAssetThumbnailConfig ThumbnailConfig;
	if (TypeInfo->Settings->IsAsset())
	{
		//@todo: access the foliage type asset type action and get the color from that
		ThumbnailConfig.AssetTypeColorOverride = FColor(128, 192, 255);
	}

	ThumbnailWidget = Thumbnail->MakeThumbnailWidget(ThumbnailConfig);
}

const TSharedPtr<SFoliagePalette>& FFoliagePaletteItemModel::GetFoliagePalette() const
{
	return FoliagePalette;
}

const UFoliageType* FFoliagePaletteItemModel::GetFoliageType() const
{
	return TypeInfo->Settings;
}

TSharedRef<SWidget> FFoliagePaletteItemModel::GetThumbnailWidget() const
{
	return ThumbnailWidget.ToSharedRef();
}

TSharedRef<SToolTip> FFoliagePaletteItemModel::CreateTooltipWidget() const
{
	return 
		SNew(SToolTip)
		.TextMargin(1)
		.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder"))
		.Visibility(this, &FFoliagePaletteItemModel::GetTooltipVisibility)
		[
			SNew(SBorder)
			.Padding(3.f)
			.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder"))
			[
				SNew(SVerticalBox)

				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.Padding(FMargin(6.f))
					.HAlign(HAlign_Left)
					.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
					[
						SNew(STextBlock)
						.Text(this, &FFoliagePaletteItemModel::GetFoliageTypeNameText)
						.Font(FEditorStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont"))
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 3.f, 0.f, 0.f))
				[
					SNew(SBorder)
					.Visibility(this, &FFoliagePaletteItemModel::GetTooltipThumbnailVisibility)
					.Padding(6.f)
					.HAlign(HAlign_Left)
					.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
					[
						SNew(SBox)
						.HeightOverride(64.f)
						.WidthOverride(64.f)
						[
							GetThumbnailWidget()
						]
					]
				]
			]
		];
}

TSharedRef<SCheckBox> FFoliagePaletteItemModel::CreateActivationCheckBox(TAttribute<bool> IsItemWidgetSelected, TAttribute<EVisibility> InVisibility)
{
	return
		SNew(SCheckBox)
		.Padding(0.f)
		.OnCheckStateChanged(this, &FFoliagePaletteItemModel::HandleCheckStateChanged, IsItemWidgetSelected)
		.Visibility(InVisibility)
		.IsChecked(this, &FFoliagePaletteItemModel::GetCheckBoxState)
		.ToolTipText(LOCTEXT("TileCheckboxTooltip", "Check to activate the currently selected types in the palette"));
}

TSharedRef<SButton> FFoliagePaletteItemModel::CreateSaveAssetButton(TAttribute<EVisibility> InVisibility)
{
	return 
		SNew(SButton)
		.ContentPadding(0)
		.ButtonStyle(FEditorStyle::Get(), "ToggleButton")
		.Visibility(InVisibility)
		.IsEnabled(this, &FFoliagePaletteItemModel::IsSaveEnabled)
		.OnClicked(this, &FFoliagePaletteItemModel::HandleSaveAsset)
		.ToolTipText(LOCTEXT("SaveButtonToolTip", "Save foliage asset"))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Content()
		[
			SNew(SImage)
			.Image(FEditorStyle::GetBrush("Level.SaveIcon16x"))
		];
}

FText FFoliagePaletteItemModel::GetFoliageTypeNameText() const
{
	if (TypeInfo->Settings->IsAsset())
	{
		return FText::FromName(TypeInfo->Settings->GetFName());
	}
	else
	{
		return FText::FromName(TypeInfo->Settings->GetStaticMesh()->GetFName());
	}
}

FText FFoliagePaletteItemModel::GetInstanceCountText() const
{
	const int32	InstanceCountTotal = TypeInfo->InstanceCountTotal;
	const int32	InstanceCountCurrentLevel = TypeInfo->InstanceCountCurrentLevel;

	if (InstanceCountCurrentLevel != InstanceCountTotal)
	{
		return FText::Format(LOCTEXT("InstanceCount", "{0} ({1})"), FText::AsNumber(InstanceCountCurrentLevel), FText::AsNumber(InstanceCountTotal));
	}
	else
	{
		return FText::AsNumber(InstanceCountCurrentLevel);
	}
}

void FFoliagePaletteItemModel::HandleCheckStateChanged(const ECheckBoxState NewCheckedState, TAttribute<bool> IsItemWidgetSelected)
{
	if (!IsItemWidgetSelected.IsSet()) { return; }

	const bool bShouldActivate = NewCheckedState == ECheckBoxState::Checked;
	if (!IsItemWidgetSelected.Get())
	{
		if (TypeInfo->Settings->IsSelected != bShouldActivate)
		{
			TypeInfo->Settings->Modify();
			TypeInfo->Settings->IsSelected = bShouldActivate;
		}
	}
	else
	{
		FoliagePalette->ActivateAllSelectedTypes(bShouldActivate);
	}
}

ECheckBoxState FFoliagePaletteItemModel::GetCheckBoxState() const
{
	return TypeInfo->Settings->IsSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

bool FFoliagePaletteItemModel::IsSaveEnabled() const
{
	UFoliageType* FoliageType = TypeInfo->Settings;

	// Saving is enabled for non-assets and dirty assets
	return (!FoliageType->IsAsset() || FoliageType->GetOutermost()->IsDirty());
}

FReply FFoliagePaletteItemModel::HandleSaveAsset()
{
	UFoliageType* SavedSettings = FoliageEditMode->SaveFoliageTypeObject(TypeInfo->Settings);
	if (SavedSettings)
	{
		TypeInfo->Settings = SavedSettings;
	}

	return FReply::Handled();
}

EVisibility FFoliagePaletteItemModel::GetTooltipVisibility() const
{
	return FoliagePalette->ShouldShowTooltips() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
}

EVisibility FFoliagePaletteItemModel::GetTooltipThumbnailVisibility() const
{
	return FoliagePalette->IsActiveViewMode(EPaletteViewMode::Tree) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
}

////////////////////////////////////////////////
// SFoliagePaletteItemTile
////////////////////////////////////////////////

void SFoliagePaletteItemTile::Construct(const FArguments& InArgs, TSharedRef<STableViewBase> InOwnerTableView, TSharedPtr<FFoliagePaletteItemModel>& InModel)
{
	Model = InModel;

	auto IsSelectedGetter = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SFoliagePaletteItemTile::IsSelected));
	auto CheckBoxVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &SFoliagePaletteItemTile::GetCheckBoxVisibility));
	auto SaveButtonVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &SFoliagePaletteItemTile::GetSaveButtonVisibility));

	STableRow<FFoliageMeshUIInfoPtr>::Construct(
		STableRow<FFoliageMeshUIInfoPtr>::FArguments()
		.Style(FEditorStyle::Get(), "ContentBrowser.AssetListView.TableRow")
		.Padding(1.f)
		.Content()
		[
			SNew(SOverlay)
			.ToolTip(Model->CreateTooltipWidget())
			
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.Padding(4.f)
				.BorderImage(FEditorStyle::GetBrush("ContentBrowser.ThumbnailShadow"))
				.ForegroundColor(FLinearColor::White)
				.ColorAndOpacity(this, &SFoliagePaletteItemTile::GetTileColorAndOpacity)
				[
					Model->GetThumbnailWidget()
				]
			]
			
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			.Padding(FMargin(3.f))
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ContentBrowser.ThumbnailShadow"))
				.BorderBackgroundColor(FLinearColor::Black)
				.ForegroundColor(FLinearColor::White)
				.Padding(3.f)
				[
					Model->CreateActivationCheckBox(IsSelectedGetter, CheckBoxVisibility)
				]
			]

			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			.Padding(FMargin(3.f))
			[
				Model->CreateSaveAssetButton(SaveButtonVisibility)
			]

			+ SOverlay::Slot()

			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(6.f, 8.f))
			[
				SNew(STextBlock)
				.Text(Model.Get(), &FFoliagePaletteItemModel::GetInstanceCountText)
				.ShadowOffset(FVector2D(1.f, 1.f))
				.ColorAndOpacity(FLinearColor(.85f, .85f, .85f, 1.f))
			]
		], InOwnerTableView);
}

FLinearColor SFoliagePaletteItemTile::GetTileColorAndOpacity() const
{
	float Alpha = Model->GetFoliageType()->IsSelected ? 1.f : 0.5f;
	return FLinearColor(1.f, 1.f, 1.f, Alpha);
}

EVisibility SFoliagePaletteItemTile::GetCheckBoxVisibility() const
{
	return IsHovered() || (IsSelected() && Model->GetFoliagePalette()->AnySelectedTileHovered()) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SFoliagePaletteItemTile::GetSaveButtonVisibility() const
{
	return IsHovered() ? EVisibility::Visible : EVisibility::Collapsed;
}

////////////////////////////////////////////////
// SFoliagePaletteItemRow
////////////////////////////////////////////////

void SFoliagePaletteItemRow::Construct(const FArguments& InArgs, TSharedRef<STableViewBase> InOwnerTableView, TSharedPtr<FFoliagePaletteItemModel>& InModel)
{
	Model = InModel;
	SMultiColumnTableRow<FFoliageMeshUIInfoPtr>::Construct(FSuperRowType::FArguments(), InOwnerTableView);

	SetToolTip(Model->CreateTooltipWidget());
}

TSharedRef<SWidget> SFoliagePaletteItemRow::GenerateWidgetForColumn(const FName& ColumnName)
{
	TSharedPtr<SWidget> TableRowContent = SNullWidget::NullWidget;

	if (ColumnName == FoliagePaletteTreeColumns::ColumnID_ToggleActive)
	{
		auto IsSelectedGetter = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SFoliagePaletteItemTile::IsSelected));
		TableRowContent = Model->CreateActivationCheckBox(IsSelectedGetter);
	} 
	else if (ColumnName == FoliagePaletteTreeColumns::ColumnID_Type)
	{
		TableRowContent =
			SNew(SHorizontalBox)
			
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SExpanderArrow, SharedThis(this))
			]
			
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(Model.Get(), &FFoliagePaletteItemModel::GetFoliageTypeNameText)
			];
	}
	else if (ColumnName == FoliagePaletteTreeColumns::ColumnID_InstanceCount)
	{
		TableRowContent =
			SNew(SHorizontalBox)
			
			+SHorizontalBox::Slot()
			.Padding(FMargin(10,1,0,1))
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(Model.Get(), &FFoliagePaletteItemModel::GetInstanceCountText)
			];
	}
	else if (ColumnName == FoliagePaletteTreeColumns::ColumnID_Save)
	{
		TableRowContent = Model->CreateSaveAssetButton();
	}

	return TableRowContent.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE