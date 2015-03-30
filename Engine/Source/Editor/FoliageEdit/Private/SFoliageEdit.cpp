// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "SScaleBox.h"

#include "FoliageType.h"
#include "FoliageEdMode.h"
#include "SFoliageEdit.h"
#include "FoliageEditActions.h"
#include "Editor/UnrealEd/Public/AssetThumbnail.h"
#include "Editor/UnrealEd/Public/DragAndDrop/AssetDragDropOp.h"
#include "Editor/UnrealEd/Public/ScopedTransaction.h"
#include "Editor/ContentBrowser/Public/ContentBrowserModule.h"
#include "Editor/UnrealEd/Public/AssetSelection.h"
#include "Editor/IntroTutorials/Public/IIntroTutorials.h"
#include "Editor/PropertyEditor/Public/IDetailsView.h"
#include "Editor/PropertyEditor/Public/PropertyEditorModule.h"
#include "Engine/StaticMesh.h"
#include "SFoliageEditMeshRow.h"
#include "FoliageTypePaintingCustomization.h"


#define LOCTEXT_NAMESPACE "FoliageEd_Mode"

class SFoliageDragDropHandler : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFoliageDragDropHandler) {}
	SLATE_DEFAULT_SLOT(FArguments, Content)
	SLATE_ARGUMENT(TWeakPtr<SFoliageEdit>, FoliageEditPtr)
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
			.BorderBackgroundColor(this, &SFoliageDragDropHandler::GetBackgroundColor)
			.Padding(FMargin(30))
			[
				InArgs._Content.Widget
			]
		];
	}

	~SFoliageDragDropHandler() {}

	FReply OnDrop(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent) override
	{
		bIsDragOn = false;

		TSharedPtr<SFoliageEdit> PinnedPtr = FoliageEditPtr.Pin();
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
	TWeakPtr<SFoliageEdit> FoliageEditPtr;
	bool bIsDragOn;
};


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SFoliageEdit::Construct(const FArguments& InArgs)
{
	FoliageEditMode = (FEdModeFoliage*)GLevelEditorModeTools().GetActiveMode(FBuiltinEditorModes::EM_Foliage);

	FFoliageEditCommands::Register();

	UICommandList = MakeShareable(new FUICommandList);

	BindCommands();

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args(false, false, false, FDetailsViewArgs::HideNameArea, true);
	Args.bShowActorLabel = false;
	MeshDetailsWidget = PropertyModule.CreateDetailView(Args);
	MeshDetailsWidget->RegisterInstancedCustomPropertyLayout(UFoliageType::StaticClass(), 
		FOnGetDetailCustomizationInstance::CreateStatic(&FFoliageTypePaintingCustomization::MakeInstance, FoliageEditMode)
		);

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
					.Visibility(this, &SFoliageEdit::GetVisibility_Radius)
					.Text(LOCTEXT("BrushSize", "Brush Size"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(2.0f)
				.Padding(StandardPadding)
				[
					SNew(SSpinBox<float>)
					.Visibility(this, &SFoliageEdit::GetVisibility_Radius)
					.MinValue(0.0f)
					.MaxValue(65536.0f)
					.MaxSliderValue(8192.0f)
					.SliderExponent(3.0f)
					.Value(this, &SFoliageEdit::GetRadius)
					.OnValueChanged(this, &SFoliageEdit::SetRadius)
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
					.Visibility(this, &SFoliageEdit::GetVisibility_PaintDensity)
					.Text(LOCTEXT("PaintDensity", "Paint Density"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(2.0f)
				.Padding(StandardPadding)
				[
					SNew(SSpinBox<float>)
					.Visibility(this, &SFoliageEdit::GetVisibility_PaintDensity)
					.MinValue(0.0f)
					.MaxValue(2.0f)
					.MaxSliderValue(1.0f)
					.Value(this, &SFoliageEdit::GetPaintDensity)
					.OnValueChanged(this, &SFoliageEdit::SetPaintDensity)
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
					.Visibility(this, &SFoliageEdit::GetVisibility_EraseDensity)
					.Text(LOCTEXT("EraseDensity", "Erase Density"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(2.0f)
				.Padding(StandardPadding)
				[
					SNew(SSpinBox<float>)
					.Visibility(this, &SFoliageEdit::GetVisibility_EraseDensity)
					.MinValue(0.0f)
					.MaxValue(2.0f)
					.MaxSliderValue(1.0f)
					.Value(this, &SFoliageEdit::GetEraseDensity)
					.OnValueChanged(this, &SFoliageEdit::SetEraseDensity)
				]
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				SNew(STextBlock)
				.Visibility(this, &SFoliageEdit::GetVisibility_Filters)
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
					.Visibility(this, &SFoliageEdit::GetVisibility_Filters)
					.OnCheckStateChanged(this, &SFoliageEdit::OnCheckStateChanged_Landscape)
					.IsChecked(this, &SFoliageEdit::GetCheckState_Landscape)
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
					.Visibility(this, &SFoliageEdit::GetVisibility_Filters)
					.OnCheckStateChanged(this, &SFoliageEdit::OnCheckStateChanged_StaticMesh)
					.IsChecked(this, &SFoliageEdit::GetCheckState_StaticMesh)
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
					.Visibility(this, &SFoliageEdit::GetVisibility_Filters)
					.OnCheckStateChanged(this, &SFoliageEdit::OnCheckStateChanged_BSP)
					.IsChecked(this, &SFoliageEdit::GetCheckState_BSP)
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
					.Visibility(this, &SFoliageEdit::GetVisibility_Filters)
					.OnCheckStateChanged(this, &SFoliageEdit::OnCheckStateChanged_Translucent)
					.IsChecked(this, &SFoliageEdit::GetCheckState_Translucent)
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
				+ SHeaderRow::Column(TEXT("ToggleMeshes"))
				[
					SNew(SCheckBox)
					.IsChecked(this, &SFoliageEdit::GetState_AllMeshes)
					.OnCheckStateChanged(this, &SFoliageEdit::OnCheckStateChanged_AllMeshes)
				]
				.HeaderContentPadding(FMargin(0,1,0,1))
				.FixedWidth(24)
								
				+ SHeaderRow::Column(TEXT("Meshes"))
				.HeaderContentPadding(FMargin(10,1,0,1))
				.DefaultLabel(this, &SFoliageEdit::GetMeshesHeaderText)
				.SortMode(this, &SFoliageEdit::GetMeshColumnSortMode)
				.OnSort(this, &SFoliageEdit::OnMeshesColumnSortModeChanged)
			]

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(StandardPadding)
			[
				SNew(SBox)
				.Visibility(this, &SFoliageEdit::GetVisibility_EmptyList)
				.Padding(FMargin(15, 0))
				.MinDesiredHeight(30)
				[
					SNew(SScaleBox)
					.Stretch(EStretch::ScaleToFit)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Foliage_DropStatic", "+ Drop Foliage Here"))
						.ToolTipText(LOCTEXT("Foliage_DropStatic_ToolTip", "Drag static meshes/foliage settings from the content browser into this area."))
					]
				]
			]

			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)

				+SSplitter::Slot()
				.Value(0.3f)
				[
					SAssignNew(MeshTreeWidget, SFoliageMeshTree)
					.TreeItemsSource(&FoliageEditMode->GetFoliageMeshList())
					.SelectionMode(ESelectionMode::Multi)
					.OnGenerateRow(this, &SFoliageEdit::MeshTreeGenerateRow)
					.OnGetChildren( this, &SFoliageEdit::MeshTreeGetChildren)
					.OnContextMenuOpening(this, &SFoliageEdit::ConstructFoliageMeshContextMenu)
					.OnSelectionChanged(this, &SFoliageEdit::MeshTreeOnSelectionChanged)
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
					
				+SSplitter::Slot()
				[
					MeshDetailsWidget.ToSharedRef()
				]
			]
		]

		// Foliage Mesh Drop Zone
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SFoliageDragDropHandler)
			.Visibility(this, &SFoliageEdit::GetVisibility_FoliageDropTarget)
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

SFoliageEdit::~SFoliageEdit()
{
}

void SFoliageEdit::RefreshFullList()
{
	MeshTreeWidget->RequestTreeRefresh();
}

void SFoliageEdit::ClearAllToolSelection()
{
	FoliageEditMode->UISettings.SetLassoSelectToolSelected(false);
	FoliageEditMode->UISettings.SetPaintToolSelected(false);
	FoliageEditMode->UISettings.SetReapplyToolSelected(false);
	FoliageEditMode->UISettings.SetSelectToolSelected(false);
	FoliageEditMode->UISettings.SetPaintBucketToolSelected(false);
}

void SFoliageEdit::BindCommands()
{
	const FFoliageEditCommands& Commands = FFoliageEditCommands::Get();

	UICommandList->MapAction(
		Commands.SetPaint,
		FExecuteAction::CreateSP(this, &SFoliageEdit::OnSetPaint),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SFoliageEdit::IsPaintTool));

	UICommandList->MapAction(
		Commands.SetReapplySettings,
		FExecuteAction::CreateSP(this, &SFoliageEdit::OnSetReapplySettings),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SFoliageEdit::IsReapplySettingsTool));

	UICommandList->MapAction(
		Commands.SetSelect,
		FExecuteAction::CreateSP(this, &SFoliageEdit::OnSetSelectInstance),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SFoliageEdit::IsSelectTool));

	UICommandList->MapAction(
		Commands.SetLassoSelect,
		FExecuteAction::CreateSP(this, &SFoliageEdit::OnSetLasso),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SFoliageEdit::IsLassoSelectTool));

	UICommandList->MapAction(
		Commands.SetPaintBucket,
		FExecuteAction::CreateSP(this, &SFoliageEdit::OnSetPaintFill),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP(this, &SFoliageEdit::IsPaintFillTool));

		//
	UICommandList->MapAction(
		Commands.RemoveFoliageType,
		FExecuteAction::CreateSP(this, &SFoliageEdit::OnRemoveFoliageType),
		FCanExecuteAction(),
		FIsActionChecked());

	UICommandList->MapAction(
		Commands.ShowFoliageTypeInCB,
		FExecuteAction::CreateSP(this, &SFoliageEdit::OnShowFoliageTypeInCB),
		FCanExecuteAction(),
		FIsActionChecked());

	UICommandList->MapAction(
		Commands.SelectAllInstances,
		FExecuteAction::CreateSP(this, &SFoliageEdit::OnSelectAllInstances),
		FCanExecuteAction(),
		FIsActionChecked());

	UICommandList->MapAction(
		Commands.DeselectAllInstances,
		FExecuteAction::CreateSP(this, &SFoliageEdit::OnDeselectAllInstances),
		FCanExecuteAction(),
		FIsActionChecked());
}

TSharedRef<SWidget> SFoliageEdit::BuildToolBar()
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

void SFoliageEdit::OnSetPaint()
{
	ClearAllToolSelection();

	FoliageEditMode->UISettings.SetPaintToolSelected(true);

	FoliageEditMode->NotifyToolChanged();
}

bool SFoliageEdit::IsPaintTool() const
{
	return FoliageEditMode->UISettings.GetPaintToolSelected();
}

void SFoliageEdit::OnSetReapplySettings()
{
	ClearAllToolSelection();

	FoliageEditMode->UISettings.SetReapplyToolSelected(true);

	FoliageEditMode->NotifyToolChanged();
}

bool SFoliageEdit::IsReapplySettingsTool() const
{
	return FoliageEditMode->UISettings.GetReapplyToolSelected();
}

void SFoliageEdit::OnSetSelectInstance()
{
	ClearAllToolSelection();

	FoliageEditMode->UISettings.SetSelectToolSelected(true);

	FoliageEditMode->NotifyToolChanged();
}

bool SFoliageEdit::IsSelectTool() const
{
	return FoliageEditMode->UISettings.GetSelectToolSelected();
}

void SFoliageEdit::OnSetLasso()
{
	ClearAllToolSelection();

	FoliageEditMode->UISettings.SetLassoSelectToolSelected(true);

	FoliageEditMode->NotifyToolChanged();
}

bool SFoliageEdit::IsLassoSelectTool() const
{
	return FoliageEditMode->UISettings.GetLassoSelectToolSelected();
}

void SFoliageEdit::OnSetPaintFill()
{
	ClearAllToolSelection();

	FoliageEditMode->UISettings.SetPaintBucketToolSelected(true);

	FoliageEditMode->NotifyToolChanged();
}

bool SFoliageEdit::IsPaintFillTool() const
{
	return FoliageEditMode->UISettings.GetPaintBucketToolSelected();
}

void SFoliageEdit::SetRadius(float InRadius)
{
	FoliageEditMode->UISettings.SetRadius(InRadius);
}

float SFoliageEdit::GetRadius() const
{
	return FoliageEditMode->UISettings.GetRadius();
}

void SFoliageEdit::SetPaintDensity(float InDensity)
{
	FoliageEditMode->UISettings.SetPaintDensity(InDensity);
}

float SFoliageEdit::GetPaintDensity() const
{
	return FoliageEditMode->UISettings.GetPaintDensity();
}


void SFoliageEdit::SetEraseDensity(float InDensity)
{
	FoliageEditMode->UISettings.SetUnpaintDensity(InDensity);
}

float SFoliageEdit::GetEraseDensity() const
{
	return FoliageEditMode->UISettings.GetUnpaintDensity();
}

EVisibility SFoliageEdit::GetVisibility_EmptyList() const
{
	if (FoliageEditMode->GetFoliageMeshList().Num() > 0)
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::HitTestInvisible;
}

EVisibility SFoliageEdit::GetVisibility_FoliageDropTarget() const
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

EVisibility SFoliageEdit::GetVisibility_NonEmptyList() const
{
	if (FoliageEditMode->GetFoliageMeshList().Num() == 0)
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::HitTestInvisible;
}

FReply SFoliageEdit::OnDrop_ListView(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TArray<FAssetData> DroppedAssetData = AssetUtil::ExtractAssetDataFromDrag(DragDropEvent);
	{
		const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "FoliageMode_AddMeshTransaction", "Foliage Editing: Add Mesh"));

		for (int32 AssetIdx = 0; AssetIdx < DroppedAssetData.Num(); ++AssetIdx)
		{
			GWarn->BeginSlowTask(LOCTEXT("OnDrop_LoadPackage", "Fully Loading Package For Drop"), true, false);
			UObject* Asset = DroppedAssetData[AssetIdx].GetAsset();
			GWarn->EndSlowTask();

			FoliageEditMode->AddFoliageAsset(Asset);
		}
	}

	MeshTreeWidget->RequestTreeRefresh();

	return FReply::Handled();
}
TSharedPtr<SWidget> SFoliageEdit::ConstructFoliageMeshContextMenu() const
{
	const FFoliageEditCommands& Commands = FFoliageEditCommands::Get();
	
	FMenuBuilder MenuBuilder(true, UICommandList);
	MenuBuilder.AddMenuEntry(Commands.RemoveFoliageType);

	MenuBuilder.AddSubMenu(
		LOCTEXT( "ReplaceFoliageType", "Replace Foliage Type" ),
		LOCTEXT ("ReplaceFoliageType_ToolTip", "Replaces selected foliage type with another foliage type asset"),
		FNewMenuDelegate::CreateSP(this, &SFoliageEdit::FillReplaceFoliageTypeSubmenu));

	MenuBuilder.AddMenuEntry(Commands.ShowFoliageTypeInCB);

	if (IsSelectTool() || IsLassoSelectTool())
	{
		MenuBuilder.AddMenuEntry(Commands.SelectAllInstances);
		MenuBuilder.AddMenuEntry(Commands.DeselectAllInstances);
	}

	return MenuBuilder.MakeWidget();
}

void SFoliageEdit::OnCheckStateChanged_Landscape(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterLandscape(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit::GetCheckState_Landscape() const
{
	return FoliageEditMode->UISettings.GetFilterLandscape() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SFoliageEdit::OnCheckStateChanged_StaticMesh(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterStaticMesh(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit::GetCheckState_StaticMesh() const
{
	return FoliageEditMode->UISettings.GetFilterStaticMesh() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SFoliageEdit::OnCheckStateChanged_BSP(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterBSP(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit::GetCheckState_BSP() const
{
	return FoliageEditMode->UISettings.GetFilterBSP() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SFoliageEdit::OnCheckStateChanged_Translucent(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterTranslucent(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit::GetCheckState_Translucent() const
{
	return FoliageEditMode->UISettings.GetFilterTranslucent() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

EVisibility SFoliageEdit::GetVisibility_Radius() const
{
	if (FoliageEditMode->UISettings.GetSelectToolSelected() || FoliageEditMode->UISettings.GetReapplyPaintBucketToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected() )
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility SFoliageEdit::GetVisibility_PaintDensity() const
{
	if (FoliageEditMode->UISettings.GetSelectToolSelected() || FoliageEditMode->UISettings.GetReapplyPaintBucketToolSelected() || FoliageEditMode->UISettings.GetLassoSelectToolSelected())
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility SFoliageEdit::GetVisibility_EraseDensity() const
{
	if (FoliageEditMode->UISettings.GetSelectToolSelected() || FoliageEditMode->UISettings.GetReapplyPaintBucketToolSelected() || FoliageEditMode->UISettings.GetLassoSelectToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected())
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility SFoliageEdit::GetVisibility_Filters() const
{
	if (FoliageEditMode->UISettings.GetSelectToolSelected())
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

TSharedRef<ITableRow> SFoliageEdit::MeshTreeGenerateRow(FFoliageMeshUIInfoPtr Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(SFoliageEditMeshRow, OwnerTable)
		.MeshInfo(Item)
		.FoliageEditMode(FoliageEditMode);
}

void SFoliageEdit::MeshTreeGetChildren(FFoliageMeshUIInfoPtr Item, TArray<FFoliageMeshUIInfoPtr>& OutChildren)
{
	//OutChildren = Item->GetChildren();
}

void SFoliageEdit::MeshTreeOnSelectionChanged(FFoliageMeshUIInfoPtr Item, ESelectInfo::Type SelectInfo)
{
	TArray<UObject*> FoliageMeshes;
	auto InfoList = MeshTreeWidget->GetSelectedItems();

	for (const auto& Info : InfoList)
	{
		FoliageMeshes.Add(Info->Settings);
	}

	MeshDetailsWidget->SetObjects(FoliageMeshes, true);
}

void SFoliageEdit::OnRemoveFoliageType()
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

void SFoliageEdit::FillReplaceFoliageTypeSubmenu(FMenuBuilder& MenuBuilder)
{
	FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

	FAssetPickerConfig AssetPickerConfig;
	AssetPickerConfig.Filter.ClassNames.Add(UFoliageType::StaticClass()->GetFName());
	AssetPickerConfig.Filter.bRecursiveClasses = true;
	AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SFoliageEdit::OnReplaceFoliageTypeSelected);
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

void SFoliageEdit::OnReplaceFoliageTypeSelected(const FAssetData& AssetData)
{
	FSlateApplication::Get().DismissAllMenus();
	
	auto MeshUIList = MeshTreeWidget->GetSelectedItems();
	UFoliageType* NewSettings = Cast<UFoliageType>(AssetData.GetAsset());
	
	if (MeshUIList.Num() && NewSettings)
	{
		for (auto& MeshUI : MeshUIList)
		{
			UFoliageType* OldSettings = MeshUI->Settings;
			if (OldSettings != NewSettings)
			{
				FoliageEditMode->ReplaceSettingsObject(OldSettings, NewSettings);
			}
		}
	}
}

void SFoliageEdit::OnShowFoliageTypeInCB()
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

void SFoliageEdit::OnSelectAllInstances()
{
	TArray<UObject*> SelectedAssets;
	auto MeshUIList = MeshTreeWidget->GetSelectedItems();
	for (FFoliageMeshUIInfoPtr MeshUIPtr : MeshUIList)
	{
		UFoliageType* FoliageType = MeshUIPtr->Settings;
		FoliageEditMode->SelectInstances(FoliageType, true);
	}
}

void SFoliageEdit::OnDeselectAllInstances()
{
	TArray<UObject*> SelectedAssets;
	auto MeshUIList = MeshTreeWidget->GetSelectedItems();
	for (FFoliageMeshUIInfoPtr MeshUIPtr : MeshUIList)
	{
		UFoliageType* FoliageType = MeshUIPtr->Settings;
		FoliageEditMode->SelectInstances(FoliageType, false);
	}
}

ECheckBoxState SFoliageEdit::GetState_AllMeshes() const
{
	bool bHasChecked = false;
	bool bHasUnchecked = false;

	const auto& MeshUIList = FoliageEditMode->GetFoliageMeshList();
	for (const auto& MeshUIPtr : MeshUIList)
	{
		if (MeshUIPtr->Settings->IsSelected)
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

void SFoliageEdit::OnCheckStateChanged_AllMeshes(ECheckBoxState InState)
{
	auto& MeshUIList = FoliageEditMode->GetFoliageMeshList();
	for (auto& MeshUIPtr : MeshUIList)
	{
		MeshUIPtr->Settings->IsSelected = (InState == ECheckBoxState::Checked);
	}
}

FText SFoliageEdit::GetMeshesHeaderText() const
{
	int32 NumMeshes = FoliageEditMode->GetFoliageMeshList().Num();
	return FText::Format(LOCTEXT("FoliageMeshCount", "Meshes - {0}"), FText::AsNumber(NumMeshes));
}

EColumnSortMode::Type SFoliageEdit::GetMeshColumnSortMode() const
{
	return FoliageEditMode->GetFoliageMeshListSortMode();
}

void SFoliageEdit::OnMeshesColumnSortModeChanged(EColumnSortPriority::Type InPriority, const FName& InColumnName, EColumnSortMode::Type InSortMode)
{
	FoliageEditMode->OnFoliageMeshListSortModeChanged(InSortMode);
}

#undef LOCTEXT_NAMESPACE
