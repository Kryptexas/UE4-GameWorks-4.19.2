// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "FoliagePaletteItem.h"
#include "FoliageEdMode.h"
#include "SFoliagePalette.h"
#include "FoliageType.h"

#include "LevelEditor.h"
#include "Editor/UnrealEd/Public/AssetThumbnail.h"
#include "Engine/StaticMesh.h"

#define LOCTEXT_NAMESPACE "FoliageEd_Mode"

namespace EInstanceCountMagnitude
{
	enum Type
	{
		Thousand,
		Million,
		Billion,
		Max
	};
}

FFoliagePaletteItemModel::FFoliagePaletteItemModel(FFoliageMeshUIInfoPtr InTypeInfo, TSharedRef<SFoliagePalette> InFoliagePalette, FEdModeFoliage* InFoliageEditMode)
	: TypeInfo(InTypeInfo)
	, FoliagePalette(InFoliagePalette)
	, FoliageEditMode(InFoliageEditMode)
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool = LevelEditorModule.GetFirstLevelEditor()->GetThumbnailPool();
	FAssetData AssetData = FAssetData(TypeInfo->Settings->GetStaticMesh());

	int32 MaxThumbnailSize = FoliagePaletteConstants::ThumbnailSizeRange.Max;
	TSharedPtr< FAssetThumbnail > Thumbnail = MakeShareable(new FAssetThumbnail(AssetData, MaxThumbnailSize, MaxThumbnailSize, ThumbnailPool));
	FAssetThumbnailConfig ThumbnailConfig;
	if (TypeInfo->Settings->IsAsset())
	{
		//@todo: access the foliage type asset type action and get the color from that (and maybe change it from pale blue - hard to see against the white thumbnail)
		ThumbnailConfig.AssetTypeColorOverride = FColor(128, 192, 255);
	}

	ThumbnailWidget = Thumbnail->MakeThumbnailWidget(ThumbnailConfig);
}

TSharedPtr<SFoliagePalette> FFoliagePaletteItemModel::GetFoliagePalette() const
{
	return FoliagePalette.Pin();
}

FFoliageMeshUIInfoPtr FFoliagePaletteItemModel::GetTypeUIInfo() const
{
	return TypeInfo;
}

UFoliageType* FFoliagePaletteItemModel::GetFoliageType() const
{
	return TypeInfo->Settings;
}

const FFoliageUISettings& FFoliagePaletteItemModel::GetFoliageUISettings() const
{
	return FoliageEditMode->UISettings;
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
						.Text(this, &FFoliagePaletteItemModel::GetFoliageTypeDisplayNameText)
						.Font(FEditorStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont"))
						.HighlightText(this, &FFoliagePaletteItemModel::GetPaletteSearchText)
					]
				]

				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(FMargin(0.f, 3.f, 0.f, 0.f))
				[
					
					SNew(SHorizontalBox)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(0.f, 0.f, 3.f, 0.f))
					[
						SNew(SBorder)
						.Padding(6.f)
						.HAlign(HAlign_Center)
						.Visibility(this, &FFoliagePaletteItemModel::GetTooltipThumbnailVisibility)
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

					+ SHorizontalBox::Slot()
					[
						SNew(SBorder)
						.Padding(6.f)
						.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
						[
							SNew(SVerticalBox)

							+ SVerticalBox::Slot()
							[
								SNew(SHorizontalBox)
								
								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(STextBlock)
									.Text(LOCTEXT("InstanceCountHeading", "Instance Count: "))
									.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								]

								+ SHorizontalBox::Slot()
								.AutoWidth()
								[
									SNew(STextBlock)
									.Text(this, &FFoliagePaletteItemModel::GetInstanceCountText, false)
								]
							]
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

FText FFoliagePaletteItemModel::GetFoliageTypeDisplayNameText() const
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

FText FFoliagePaletteItemModel::GetPaletteSearchText() const
{
	if (FoliagePalette.IsValid())
	{
		return FoliagePalette.Pin()->GetSearchText();
	}
	else
	{
		return FText();
	}
}

FText FFoliagePaletteItemModel::GetInstanceCountText(bool bRounded) const
{
	const int32	InstanceCountTotal = TypeInfo->InstanceCountTotal;
	const int32	InstanceCountCurrentLevel = TypeInfo->InstanceCountCurrentLevel;

	if (InstanceCountCurrentLevel != InstanceCountTotal)
	{
		return FText::Format(LOCTEXT("InstanceCount_Total", "{0} ({1})"), FText::AsNumber(InstanceCountCurrentLevel), FText::AsNumber(InstanceCountTotal));
	}
	else if (bRounded)
	{
		// Note: Instance counts greater than 999 billion (unlikely) will not be formatted properly
		static const FText Suffixes[EInstanceCountMagnitude::Max] =
		{
			LOCTEXT("Suffix_Thousand", "K"),
			LOCTEXT("Suffix_Million", "M"),
			LOCTEXT("Suffix_Billion", "B")
		};

		int32 NumThousands = 0;
		float DisplayValue = InstanceCountCurrentLevel;
		while (DisplayValue >= 1000.f && NumThousands < EInstanceCountMagnitude::Max)
		{
			DisplayValue /= 1000.f;
			NumThousands++;
		}

		if (NumThousands > 0)
		{
			// Allow 3 significant figures
			const int32 NumFractionalDigits = DisplayValue >= 100.f ? 0 : DisplayValue >= 10.f ? 1 : 2;

			FNumberFormattingOptions Options;
			Options.SetMaximumFractionalDigits(NumFractionalDigits);
			return FText::Format(LOCTEXT("InstanceCount_CurrentLevel", "{0}{1}"), FText::AsNumber(DisplayValue, &Options), Suffixes[NumThousands - 1]);
		}
	}

	return FText::AsNumber(InstanceCountCurrentLevel);
}

void FFoliagePaletteItemModel::SetTypeActiveInPalette(bool bSetActiveInPalette)
{
	UFoliageType* FoliageType = GetFoliageType();
	if (FoliageType->IsSelected != bSetActiveInPalette)
	{
		FoliageType->Modify();
		FoliageType->IsSelected = bSetActiveInPalette;
	}
}

bool FFoliagePaletteItemModel::IsActive() const
{
	return TypeInfo->Settings->IsSelected;
}

void FFoliagePaletteItemModel::HandleCheckStateChanged(const ECheckBoxState NewCheckedState, TAttribute<bool> IsItemWidgetSelected)
{
	if (!IsItemWidgetSelected.IsSet()) { return; }

	const bool bShouldActivate = NewCheckedState == ECheckBoxState::Checked;
	if (!IsItemWidgetSelected.Get())
	{
		SetTypeActiveInPalette(bShouldActivate);
	}
	else if (FoliagePalette.IsValid())
	{
		FoliagePalette.Pin()->ActivateAllSelectedTypes(bShouldActivate);
	}
}

ECheckBoxState FFoliagePaletteItemModel::GetCheckBoxState() const
{
	return TypeInfo->Settings->IsSelected ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

bool FFoliagePaletteItemModel::IsSaveEnabled() const
{
	UFoliageType* FoliageType = GetFoliageType();

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
	return (FoliagePalette.IsValid() && FoliagePalette.Pin()->ShouldShowTooltips()) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
}

EVisibility FFoliagePaletteItemModel::GetTooltipThumbnailVisibility() const
{
	return (FoliagePalette.IsValid() && FoliagePalette.Pin()->IsActiveViewMode(EFoliagePaletteViewMode::Tree)) ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
}

////////////////////////////////////////////////
// SFoliagePaletteItemTile
////////////////////////////////////////////////

const float SFoliagePaletteItemTile::MinScaleForOverlayItems = 0.2f;

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
			
			// Thumbnail
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
			
			// Checkbox
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

			// Save Button
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			.Padding(FMargin(3.f))
			[
				Model->CreateSaveAssetButton(SaveButtonVisibility)
			]

			// Instance Count
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(6.f, 8.f))
			[
				SNew(STextBlock)
				.Visibility(this, &SFoliagePaletteItemTile::GetInstanceCountVisibility)
				.Text(Model.ToSharedRef(), &FFoliagePaletteItemModel::GetInstanceCountText, true)
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
	return CanShowOverlayItems() && (IsHovered() || (IsSelected() && Model->GetFoliagePalette()->AnySelectedTileHovered())) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SFoliagePaletteItemTile::GetSaveButtonVisibility() const
{
	return IsHovered() && CanShowOverlayItems() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SFoliagePaletteItemTile::GetInstanceCountVisibility() const
{
	return CanShowOverlayItems() ? EVisibility::Visible : EVisibility::Collapsed;
}

bool SFoliagePaletteItemTile::CanShowOverlayItems() const
{
	return Model->GetFoliageUISettings().GetPaletteThumbnailScale() >= MinScaleForOverlayItems;
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
				.Text(Model.ToSharedRef(), &FFoliagePaletteItemModel::GetFoliageTypeDisplayNameText)
				.HighlightText(Model.ToSharedRef(), &FFoliagePaletteItemModel::GetPaletteSearchText)
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
				.Text(Model.ToSharedRef(), &FFoliagePaletteItemModel::GetInstanceCountText, true)
			];
	}
	else if (ColumnName == FoliagePaletteTreeColumns::ColumnID_Save)
	{
		TableRowContent = Model->CreateSaveAssetButton();
	}

	return TableRowContent.ToSharedRef();
}

#undef LOCTEXT_NAMESPACE