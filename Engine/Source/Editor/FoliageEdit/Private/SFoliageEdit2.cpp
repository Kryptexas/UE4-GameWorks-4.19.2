// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "SScaleBox.h"

#include "FoliageType.h"
#include "FoliageEdMode.h"
#include "SFoliageEdit2.h"
#include "FoliageEditActions.h"
#include "Editor/UnrealEd/Public/AssetThumbnail.h"
#include "Editor/UnrealEd/Public/DragAndDrop/AssetDragDropOp.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/UnrealEd/Public/AssetSelection.h"
#include "Editor/IntroTutorials/Public/IIntroTutorials.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Engine/StaticMesh.h"
#include "SFoliageEditMeshRow.h"

#define LOCTEXT_NAMESPACE "FoliageEd_Mode"

class SFoliageDragDropHandler2 : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFoliageDragDropHandler2) {}
	SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_ARGUMENT(TWeakPtr<SFoliageEdit2>, FoliageEditPtr)
	SLATE_END_ARGS()

	/** SCompoundWidget functions */
	void Construct(const FArguments& InArgs)
	{
		FoliageEditPtr = InArgs._FoliageEditPtr;
		bIsDragOn = false;

		this->ChildSlot
		[
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("WhiteBrush"))
			.BorderBackgroundColor(this, &SFoliageDragDropHandler2::GetBackgroundColor)
			.Padding(FMargin(30))
			[
				InArgs._Content.Widget
			]
		];
	}

	~SFoliageDragDropHandler2() {}

	FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		bIsDragOn = false;

		TSharedPtr<SFoliageEdit2> PinnedPtr = FoliageEditPtr.Pin();
		if (PinnedPtr.IsValid())
		{
			return PinnedPtr->OnDrop_ListView(MyGeometry, DragDropEvent);
		}
		return FReply::Handled();
	}

	virtual void OnDragEnter(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		bIsDragOn = true;
	}

	virtual void OnDragLeave(const FDragDropEvent& DragDropEvent) override
	{
		bIsDragOn = false;
	}

private:
	FSlateColor GetBackgroundColor() const
	{
		return bIsDragOn ? FLinearColor(1.0f, 0.6f, 0.1f, 0.9f) : FLinearColor(0.1f, 0.1f, 0.1f, 0.9f);
	}

private:
	TWeakPtr<SFoliageEdit2> FoliageEditPtr;
	bool bIsDragOn;
};


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SFoliageEdit2::Construct(const FArguments& InArgs)
{
	FoliageEditMode = (FEdModeFoliage*)GLevelEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Foliage);

	FFoliageEditCommands::Register();

	UICommandList = MakeShareable(new FUICommandList);

	BindCommands();

	AssetThumbnailPool = MakeShareable(new FAssetThumbnailPool(512, TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &SFoliageEdit2::IsHovered))));

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args(false, false, false, FDetailsViewArgs::HideNameArea, true);
	Args.bShowActorLabel = false;
	MeshDetailsWidget = PropertyModule.CreateDetailView(Args);

	
	// Everything (or almost) uses this padding, change it to expand the padding.
	FMargin StandardPadding(0.0f, 4.0f, 0.0f, 4.0f);

	this->ChildSlot
	[
		SNew(SOverlay)

		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				BuildToolBar()
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				.ToolTipText(LOCTEXT("BrushSize_Tooltip", "The size of the foliage brush"))

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(StandardPadding)
				[
					SNew(STextBlock)
					.Visibility(this, &SFoliageEdit2::GetVisibility_Radius)
					.Text(LOCTEXT("BrushSize", "Brush Size"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(2.0f)
				.Padding(StandardPadding)
				[
					SNew(SSpinBox<float>)
					.Visibility(this, &SFoliageEdit2::GetVisibility_Radius)
					.MinValue(0.0f)
					.MaxValue(65536.0f)
					.MaxSliderValue(8192.0f)
					.SliderExponent(3.0f)
					.Value(this, &SFoliageEdit2::GetRadius)
					.OnValueChanged(this, &SFoliageEdit2::SetRadius)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				.ToolTipText(LOCTEXT("PaintDensity_Tooltip", "The density of foliage to paint. This is a multiplier for the individual foliage type's density specifier."))

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(StandardPadding)
				[
					SNew(STextBlock)
					.Visibility(this, &SFoliageEdit2::GetVisibility_PaintDensity)
					.Text(LOCTEXT("PaintDensity", "Paint Density"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(2.0f)
				.Padding(StandardPadding)
				[
					SNew(SSpinBox<float>)
					.Visibility(this, &SFoliageEdit2::GetVisibility_PaintDensity)
					.MinValue(0.0f)
					.MaxValue(2.0f)
					.MaxSliderValue(1.0f)
					.Value(this, &SFoliageEdit2::GetPaintDensity)
					.OnValueChanged(this, &SFoliageEdit2::SetPaintDensity)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				.ToolTipText(LOCTEXT("EraseDensity_Tooltip", "The density of foliage to leave behind when erasing with the Shift key held. 0 will remove all foliage."))

				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.Padding(StandardPadding)
				[
					SNew(STextBlock)
					.Visibility(this, &SFoliageEdit2::GetVisibility_EraseDensity)
					.Text(LOCTEXT("EraseDensity", "Erase Density"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(2.0f)
				.Padding(StandardPadding)
				[
					SNew(SSpinBox<float>)
					.Visibility(this, &SFoliageEdit2::GetVisibility_EraseDensity)
					.MinValue(0.0f)
					.MaxValue(2.0f)
					.MaxSliderValue(1.0f)
					.Value(this, &SFoliageEdit2::GetEraseDensity)
					.OnValueChanged(this, &SFoliageEdit2::SetEraseDensity)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				SNew(STextBlock)
				.Visibility(this, &SFoliageEdit2::GetVisibility_Filters)
				.Text(LOCTEXT("Filter", "Filter"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				SNew(SWrapBox)
				.UseAllottedWidth(true)

				+ SWrapBox::Slot()
				.Padding(0, 0, 16, 0)
				[
					SNew(SCheckBox)
					.Visibility(this, &SFoliageEdit2::GetVisibility_Filters)
					.OnCheckStateChanged(this, &SFoliageEdit2::OnCheckStateChanged_Landscape)
					.IsChecked(this, &SFoliageEdit2::GetCheckState_Landscape)
					.ToolTipText(LOCTEXT("FilterLandscape_Tooltip", "Place foliage on Landscape"))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Landscape", "Landscape"))
					]
				]

				+ SWrapBox::Slot()
				.Padding(0, 0, 16, 0)
				[
					SNew(SCheckBox)
					.Visibility(this, &SFoliageEdit2::GetVisibility_Filters)
					.OnCheckStateChanged(this, &SFoliageEdit2::OnCheckStateChanged_StaticMesh)
					.IsChecked(this, &SFoliageEdit2::GetCheckState_StaticMesh)
					.ToolTipText(LOCTEXT("FilterStaticMesh_Tooltip", "Place foliage on StaticMeshes"))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("StaticMeshes", "Static Meshes"))
					]
				]

				+ SWrapBox::Slot()
				.Padding(0, 0, 16, 0)
				[
					SNew(SCheckBox)
					.Visibility(this, &SFoliageEdit2::GetVisibility_Filters)
					.OnCheckStateChanged(this, &SFoliageEdit2::OnCheckStateChanged_BSP)
					.IsChecked(this, &SFoliageEdit2::GetCheckState_BSP)
					.ToolTipText(LOCTEXT("FilterBSP_Tooltip", "Place foliage on BSP"))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("BSP", "BSP"))
					]
				]

				+ SWrapBox::Slot()
				.Padding(0, 0, 16, 0)
				[
					SNew(SCheckBox)
					.Visibility(this, &SFoliageEdit2::GetVisibility_Filters)
					.OnCheckStateChanged(this, &SFoliageEdit2::OnCheckStateChanged_Translucent)
					.IsChecked(this, &SFoliageEdit2::GetCheckState_Translucent)
					.ToolTipText(LOCTEXT("FilterTranslucent_Tooltip", "Place foliage on translucent surfaces"))
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Translucent", "Translucent"))
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHeaderRow)

				+ SHeaderRow::Column(TEXT("Meshes"))
				.DefaultLabel(LOCTEXT("Meshes", "Meshes"))
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				SNew(SBox)
				.Visibility(this, &SFoliageEdit2::GetVisibility_EmptyList)
				.Padding(FMargin(15, 0))
				.MinDesiredHeight(30)
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFit)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Foliage_DropStatic", "+ Drop Static Meshes Here"))
						.ToolTipText(LOCTEXT("Foliage_DropStatic_ToolTip", "Drag static meshes from the content browser into this area."))
					]
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBox)
				.MinDesiredHeight(100)
				[
					SAssignNew(MeshTreeWidget, SFoliageMeshTree)
					.TreeItemsSource(&FoliageEditMode->GetFoliageMeshList())
					.SelectionMode(ESelectionMode::Multi)
					.OnGenerateRow(this, &SFoliageEdit2::MeshTreeGenerateRow)
					.OnGetChildren( this, &SFoliageEdit2::MeshTreeGetChildren)
					.OnContextMenuOpening(this, &SFoliageEdit2::ConstructFoliageMeshContextMenu)
					.OnSelectionChanged(this, &SFoliageEdit2::MeshTreeOnSelectionChanged)
					.HeaderRow
						(
							SNew( SHeaderRow ).Visibility(EVisibility::Collapsed)
							// toggle mesh for painting checkbox
							+SHeaderRow::Column(FoliageMeshColumns::ColumnID_ToggleMesh).FixedWidth(24.0f)
							// mesh text label
							+SHeaderRow::Column(FoliageMeshColumns::ColumnID_MeshLabel).FillWidth(1.0f)
							// instance count
							+SHeaderRow::Column(FoliageMeshColumns::ColumnID_InstanceCount).FillWidth(0.3f)
							// save Foliage asset
							+SHeaderRow::Column(FoliageMeshColumns::ColumnID_Save).FixedWidth(24.0f)
						)
				]
			]
			
			+ SVerticalBox::Slot()
			.Padding(StandardPadding)
			[
				MeshDetailsWidget.ToSharedRef()
			]
		]

		// Static Mesh Drop Zone
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SFoliageDragDropHandler2)
			.Visibility(this, &SFoliageEdit2::GetVisibility_FoliageDropTarget)
			.FoliageEditPtr(SharedThis(this))
			[
				SNew(SScaleBox)
				.Stretch(EStretch::ScaleToFit)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Foliage_AddFoliageMesh", "+ Foliage Mesh"))
					.ShadowOffset(FVector2D(1.f, 1.f))
				]
			]
		]
	];

	RefreshFullList();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SFoliageEdit2::~SFoliageEdit2()
{
	// Release all rendering resources being held onto
	AssetThumbnailPool->ReleaseResources();
}

TSharedPtr<FAssetThumbnail> SFoliageEdit2::CreateThumbnail(UStaticMesh* InStaticMesh)
{
	return MakeShareable(new FAssetThumbnail(InStaticMesh, 80, 80, AssetThumbnailPool));
}

void SFoliageEdit2::RefreshFullList()
{
	MeshTreeWidget->RequestTreeRefresh();
}

void SFoliageEdit2::ReplaceItem(const TSharedPtr<SFoliageEditMeshDisplayItem> InDisplayItemToReplaceIn, UStaticMesh* InNewStaticMesh)
{
	//TArray<FFoliageMeshUIInfo>& FoliageMeshList = FoliageEditMode->GetFoliageMeshList();

	//for (FFoliageMeshUIInfo& UIInfo : FoliageMeshList)
	//{
	//	UFoliageType* Settings = UIInfo.Settings;
	//	if (Settings->GetStaticMesh() == InNewStaticMesh)
	//	{
	//		// This is the info for the new static mesh. Update the display item.
	//		InDisplayItemToReplaceIn->Replace(Settings,
	//			CreateThumbnail(Settings->GetStaticMesh()),
	//			MakeShareable(new FFoliageMeshUIInfo(UIInfo)));
	//		break;
	//	}
	//}
}

void SFoliageEdit2::ClearAllToolSelection()
{
	FoliageEditMode->UISettings.SetLassoSelectToolSelected(false);
	FoliageEditMode->UISettings.SetPaintToolSelected(false);
	FoliageEditMode->UISettings.SetReapplyToolSelected(false);
	FoliageEditMode->UISettings.SetSelectToolSelected(false);
	FoliageEditMode->UISettings.SetPaintBucketToolSelected(false);
}

void SFoliageEdit2::BindCommands()
{
	const FFoliageEditCommands& Commands = FFoliageEditCommands::Get();

	UICommandList->MapAction(
		Commands.SetPaint,
		FExecuteAction::CreateSP(this, &SFoliageEdit2::OnSetPaint),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SFoliageEdit2::IsPaintTool));

	UICommandList->MapAction(
		Commands.SetReapplySettings,
		FExecuteAction::CreateSP(this, &SFoliageEdit2::OnSetReapplySettings),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SFoliageEdit2::IsReapplySettingsTool));

	UICommandList->MapAction(
		Commands.SetSelect,
		FExecuteAction::CreateSP(this, &SFoliageEdit2::OnSetSelectInstance),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SFoliageEdit2::IsSelectTool));

	UICommandList->MapAction(
		Commands.SetLassoSelect,
		FExecuteAction::CreateSP(this, &SFoliageEdit2::OnSetLasso),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SFoliageEdit2::IsLassoSelectTool));

	UICommandList->MapAction(
		Commands.SetPaintBucket,
		FExecuteAction::CreateSP(this, &SFoliageEdit2::OnSetPaintFill),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SFoliageEdit2::IsPaintFillTool));

		//
	UICommandList->MapAction(
		Commands.RemoveFoliageType,
		FExecuteAction::CreateSP(this, &SFoliageEdit2::OnRemoveFoliageType),
		FCanExecuteAction(),
		FIsActionChecked());

	UICommandList->MapAction(
		Commands.ShowFoliageTypeInCB,
		FExecuteAction::CreateSP(this, &SFoliageEdit2::OnShowFoliageTypeInCB),
		FCanExecuteAction(),
		FIsActionChecked());

}

TSharedRef<SWidget> SFoliageEdit2::BuildToolBar()
{
	FToolBarBuilder Toolbar(UICommandList, FMultiBoxCustomization::None);
	{
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetPaint);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetReapplySettings);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetSelect);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetLassoSelect);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetPaintBucket);
	}

	IIntroTutorials& IntroTutorials = FModuleManager::LoadModuleChecked<IIntroTutorials>(TEXT("IntroTutorials"));

	return
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.Padding(4, 0)
		[
			SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SNew(SBorder)
				.HAlign(HAlign_Center)
				.Padding(0)
				.BorderImage(FEditorStyle::GetBrush("NoBorder"))
				.IsEnabled(FSlateApplication::Get().GetNormalExecutionAttribute())
				[
					Toolbar.MakeWidget()
				]
			]

			// Tutorial link
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Bottom)
			.Padding(4)
			[
				IntroTutorials.CreateTutorialsWidget(TEXT("FoliageMode"))
			]
		];
}

void SFoliageEdit2::OnSetPaint()
{
	ClearAllToolSelection();

	FoliageEditMode->UISettings.SetPaintToolSelected(true);

	FoliageEditMode->NotifyToolChanged();
}

bool SFoliageEdit2::IsPaintTool() const
{
	return FoliageEditMode->UISettings.GetPaintToolSelected();
}

void SFoliageEdit2::OnSetReapplySettings()
{
	ClearAllToolSelection();

	FoliageEditMode->UISettings.SetReapplyToolSelected(true);

	FoliageEditMode->NotifyToolChanged();
}

bool SFoliageEdit2::IsReapplySettingsTool() const
{
	return FoliageEditMode->UISettings.GetReapplyToolSelected();
}

void SFoliageEdit2::OnSetSelectInstance()
{
	ClearAllToolSelection();

	FoliageEditMode->UISettings.SetSelectToolSelected(true);

	FoliageEditMode->NotifyToolChanged();
}

bool SFoliageEdit2::IsSelectTool() const
{
	return FoliageEditMode->UISettings.GetSelectToolSelected();
}

void SFoliageEdit2::OnSetLasso()
{
	ClearAllToolSelection();

	FoliageEditMode->UISettings.SetLassoSelectToolSelected(true);

	FoliageEditMode->NotifyToolChanged();
}

bool SFoliageEdit2::IsLassoSelectTool() const
{
	return FoliageEditMode->UISettings.GetLassoSelectToolSelected();
}

void SFoliageEdit2::OnSetPaintFill()
{
	ClearAllToolSelection();

	FoliageEditMode->UISettings.SetPaintBucketToolSelected(true);

	FoliageEditMode->NotifyToolChanged();
}

bool SFoliageEdit2::IsPaintFillTool() const
{
	return FoliageEditMode->UISettings.GetPaintBucketToolSelected();
}

void SFoliageEdit2::SetRadius(float InRadius)
{
	FoliageEditMode->UISettings.SetRadius(InRadius);
}

float SFoliageEdit2::GetRadius() const
{
	return FoliageEditMode->UISettings.GetRadius();
}

void SFoliageEdit2::SetPaintDensity(float InDensity)
{
	FoliageEditMode->UISettings.SetPaintDensity(InDensity);
}

float SFoliageEdit2::GetPaintDensity() const
{
	return FoliageEditMode->UISettings.GetPaintDensity();
}


void SFoliageEdit2::SetEraseDensity(float InDensity)
{
	FoliageEditMode->UISettings.SetUnpaintDensity(InDensity);
}

float SFoliageEdit2::GetEraseDensity() const
{
	return FoliageEditMode->UISettings.GetUnpaintDensity();
}

EVisibility SFoliageEdit2::GetVisibility_EmptyList() const
{
	if (FoliageEditMode->GetFoliageMeshList().Num() > 0)
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::HitTestInvisible;
}

EVisibility SFoliageEdit2::GetVisibility_FoliageDropTarget() const
{
	if ( FSlateApplication::Get().IsDragDropping() )
	{
		TArray<FAssetData> DraggedAssets = AssetUtil::ExtractAssetDataFromDrag(FSlateApplication::Get().GetDragDroppingContent());
		for ( const FAssetData& AssetData : DraggedAssets )
		{
			if ( AssetData.IsValid() && (AssetData.GetClass()->IsChildOf(UStaticMesh::StaticClass()) ||	AssetData.GetClass()->IsChildOf(UFoliageType::StaticClass())) )
			{
				return EVisibility::Visible;
			}
		}
	}
	
	return EVisibility::Hidden;
}

EVisibility SFoliageEdit2::GetVisibility_NonEmptyList() const
{
	if (FoliageEditMode->GetFoliageMeshList().Num() == 0)
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::HitTestInvisible;
}

FReply SFoliageEdit2::OnDrop_ListView(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TArray<FAssetData> DroppedAssetData = AssetUtil::ExtractAssetDataFromDrag(DragDropEvent);

	for (int32 AssetIdx = 0; AssetIdx < DroppedAssetData.Num(); ++AssetIdx)
	{
		GWarn->BeginSlowTask(LOCTEXT("OnDrop_LoadPackage", "Fully Loading Package For Drop"), true, false);
		UObject* Asset = DroppedAssetData[AssetIdx].GetAsset();
		GWarn->EndSlowTask();

		UFoliageType* FoliageType = FoliageEditMode->AddFoliageAsset(Asset);
		if (FoliageType)
		{
			MeshTreeWidget->RequestTreeRefresh();
		}
	}

	return FReply::Handled();
}
TSharedPtr<SWidget> SFoliageEdit2::ConstructFoliageMeshContextMenu() const
{
	const FFoliageEditCommands& Commands = FFoliageEditCommands::Get();
	
	FMenuBuilder MenuBuilder(true, UICommandList);
	MenuBuilder.AddMenuEntry(Commands.RemoveFoliageType);

	MenuBuilder.AddSubMenu(
		LOCTEXT( "ReplaceFoliageType", "Replace Foliage Type" ),
		LOCTEXT ("ReplaceFoliageType_ToolTip", "Replaces selected foliage type with another foliage type asset"),
		FNewMenuDelegate::CreateSP(this, &SFoliageEdit2::FillReplaceFoliageTypeSubmenu));
	
	MenuBuilder.AddMenuEntry(Commands.ShowFoliageTypeInCB);

	//// Add common commands
	//MenuBuilder.BeginSection("Levels", LOCTEXT("LevelsHeader", "Levels"));
	//{
	//	// Visibility commands
	//	MenuBuilder.AddSubMenu(
	//		LOCTEXT("VisibilityHeader", "Visibility"),
	//		LOCTEXT("VisibilitySubMenu_ToolTip", "Selected Level(s) visibility commands"),
	//		FNewMenuDelegate::CreateSP(this, &FStreamingLevelCollectionModel::FillVisibilitySubMenu));

	//	// Lock commands
	//	MenuBuilder.AddSubMenu(
	//		LOCTEXT("LockHeader", "Lock"),
	//		LOCTEXT("LockSubMenu_ToolTip", "Selected Level(s) lock commands"),
	//		FNewMenuDelegate::CreateSP(this, &FStreamingLevelCollectionModel::FillLockSubMenu));
	//}
	//MenuBuilder.EndSection();

	return MenuBuilder.MakeWidget();
}

void SFoliageEdit2::OnCheckStateChanged_Landscape(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterLandscape(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit2::GetCheckState_Landscape() const
{
	return FoliageEditMode->UISettings.GetFilterLandscape() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SFoliageEdit2::OnCheckStateChanged_StaticMesh(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterStaticMesh(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit2::GetCheckState_StaticMesh() const
{
	return FoliageEditMode->UISettings.GetFilterStaticMesh() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SFoliageEdit2::OnCheckStateChanged_BSP(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterBSP(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit2::GetCheckState_BSP() const
{
	return FoliageEditMode->UISettings.GetFilterBSP() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SFoliageEdit2::OnCheckStateChanged_Translucent(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterTranslucent(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit2::GetCheckState_Translucent() const
{
	return FoliageEditMode->UISettings.GetFilterTranslucent() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

EVisibility SFoliageEdit2::GetVisibility_Radius() const
{
	if (FoliageEditMode->UISettings.GetSelectToolSelected() || FoliageEditMode->UISettings.GetReapplyPaintBucketToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected() )
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility SFoliageEdit2::GetVisibility_PaintDensity() const
{
	if (FoliageEditMode->UISettings.GetSelectToolSelected() || FoliageEditMode->UISettings.GetReapplyPaintBucketToolSelected() || FoliageEditMode->UISettings.GetLassoSelectToolSelected())
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility SFoliageEdit2::GetVisibility_EraseDensity() const
{
	if (FoliageEditMode->UISettings.GetSelectToolSelected() || FoliageEditMode->UISettings.GetReapplyPaintBucketToolSelected() || FoliageEditMode->UISettings.GetLassoSelectToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected())
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility SFoliageEdit2::GetVisibility_Filters() const
{
	if (FoliageEditMode->UISettings.GetSelectToolSelected())
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

TSharedRef<ITableRow> SFoliageEdit2::MeshTreeGenerateRow(FFoliageMeshUIInfoPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SFoliageEditMeshRow, OwnerTable)
		.MeshInfo(Item)
		.FoliageEditMode(FoliageEditMode);
}

void SFoliageEdit2::MeshTreeGetChildren(FFoliageMeshUIInfoPtr Item, TArray<FFoliageMeshUIInfoPtr>& OutChildren)
{
	//OutChildren = Item->GetChildren();
}

void SFoliageEdit2::MeshTreeOnSelectionChanged(FFoliageMeshUIInfoPtr Item, ESelectInfo::Type SelectInfo)
{
	TArray<UObject*> FoliageMeshes;
	auto InfoList = MeshTreeWidget->GetSelectedItems();

	for (const auto& Info : InfoList)
	{
		FoliageMeshes.Add(Info->Settings);
	}

	MeshDetailsWidget->SetObjects(FoliageMeshes, true);
}

void SFoliageEdit2::OnRemoveFoliageType()
{
	int32 NumInstances = 0;
	TArray<UFoliageType*> FoliageTypeList;
	
	auto MeshUIList = MeshTreeWidget->GetSelectedItems();
	for (FFoliageMeshUIInfoPtr& MeshUIInfo : MeshUIList)
	{
		NumInstances+= MeshUIInfo->InstanceCountTotal;
		FoliageTypeList.Add(MeshUIInfo->Settings);
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

void SFoliageEdit2::FillReplaceFoliageTypeSubmenu(FMenuBuilder& MenuBuilder)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(UFoliageType::StaticClass()->GetFName());
	AssetPickerConfig.Filter.bRecursiveClasses = true;
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SFoliageEdit2::OnReplaceFoliageTypeSelected);
	AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
	AssetPickerConfig.bAllowNullSelection = false;
	
	TSharedRef<SWidget> MenuContent = SNew(SBox)
	.WidthOverride(384)
	.HeightOverride(500)
	[
		ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig)
	];
	
	MenuBuilder.AddWidget( MenuContent, FText::GetEmpty(), true);
}

void SFoliageEdit2::OnReplaceFoliageTypeSelected(const FAssetData& AssetData)
{
	FSlateApplication::Get().DismissAllMenus();
	
	auto MeshUIList = MeshTreeWidget->GetSelectedItems();
	if (MeshUIList.Num())
	{
		UFoliageType* OldSettings = MeshUIList[0]->Settings;

		UFoliageType* NewSettings = Cast<UFoliageType>(AssetData.GetAsset());
		if (NewSettings && OldSettings != NewSettings)
		{
			FoliageEditMode->ReplaceSettingsObject(OldSettings, NewSettings);
		}
	}
}

void SFoliageEdit2::OnShowFoliageTypeInCB()
{
	TArray<UObject*> SelectedAssets;
	auto MeshUIList = MeshTreeWidget->GetSelectedItems();
	for (FFoliageMeshUIInfoPtr  MeshUIPtr : MeshUIList)
	{
		UFoliageType* FoliageType = MeshUIPtr->Settings;
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

#undef LOCTEXT_NAMESPACE
