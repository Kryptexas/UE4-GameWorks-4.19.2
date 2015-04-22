// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "FoliageType_InstancedStaticMesh.h"
#include "FoliageEdMode.h"
#include "SFoliageEdit.h"
#include "FoliageEditActions.h"
#include "Editor/IntroTutorials/Public/IIntroTutorials.h"
#include "SNumericEntryBox.h"
#include "SFoliagePalette.h"
#include "SScaleBox.h"


// Possibly not needed (move over to SFoliagePalette)
#include "Editor/UnrealEd/Public/AssetSelection.h"
#include "Editor/UnrealEd/Public/ScopedTransaction.h"

// Seemingly totally unneeded
//#include "DetailLayoutBuilder.h"


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

	IIntroTutorials& IntroTutorials = FModuleManager::LoadModuleChecked<IIntroTutorials>(TEXT("IntroTutorials"));

	// Everything (or almost) uses this padding, change it to expand the padding.
	FMargin StandardPadding(6.f, 3.f);
	FMargin StandardLeftPadding(6.f, 3.f, 3.f, 3.f);
	FMargin StandardRightPadding(3.f, 3.f, 6.f, 3.f);

	FSlateFontInfo StandardFont = FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"));

	const FText BlankText = LOCTEXT("Blank", "");

	this->ChildSlot
	[
		SNew(SVerticalBox)

		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(1.f, 5.f, 0.f, 5.f)
			[
				BuildToolBar()
			]

			+ SHorizontalBox::Slot()
			.Padding(0.f, 2.f, 2.f, 0.f)
			[
				SNew(SBorder)
				.BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
				.Padding(StandardPadding)
				[
					SNew(SVerticalBox)

					// Active Tool Title
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.Padding(StandardLeftPadding)
						.HAlign(HAlign_Left)
						[
							SNew(STextBlock)
							.Text(this, &SFoliageEdit::GetActiveToolName)
							.TextStyle(FEditorStyle::Get(), "FoliageEditMode.ActiveToolName.Text")
						]

						+ SHorizontalBox::Slot()
						.Padding(StandardRightPadding)
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							// Tutorial link
							IntroTutorials.CreateTutorialsWidget(TEXT("FoliageMode"))
						]
					]

					// Brush Size
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						.ToolTipText(LOCTEXT("BrushSize_Tooltip", "The size of the foliage brush"))
						.Visibility(this, &SFoliageEdit::GetVisibility_Radius)

						+ SHorizontalBox::Slot()
						.Padding(StandardLeftPadding)
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("BrushSize", "Brush Size"))
							.Font(StandardFont)
						]
						+ SHorizontalBox::Slot()
						.Padding(StandardRightPadding)
						.FillWidth(2.0f)
						.MaxWidth(100.f)
						.VAlign(VAlign_Center)
						[
							SNew(SNumericEntryBox<float>)
							.Font(StandardFont)
							.AllowSpin(true)
							.MinValue(0.0f)
							.MaxValue(65536.0f)
							.MaxSliderValue(8192.0f)
							.SliderExponent(3.0f)
							.Value(this, &SFoliageEdit::GetRadius)
							.OnValueChanged(this, &SFoliageEdit::SetRadius)
						]
					]

					// Paint Density
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						.ToolTipText(LOCTEXT("PaintDensity_Tooltip", "The density of foliage to paint. This is a multiplier for the individual foliage type's density specifier."))
						.Visibility(this, &SFoliageEdit::GetVisibility_PaintDensity)

						+ SHorizontalBox::Slot()
						.Padding(StandardLeftPadding)
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PaintDensity", "Paint Density"))
							.Font(StandardFont)
						]
						+ SHorizontalBox::Slot()
						.Padding(StandardRightPadding)
						.FillWidth(2.0f)
						.MaxWidth(100.f)
						.VAlign(VAlign_Center)
						[
							SNew(SNumericEntryBox<float>)
							.Font(StandardFont)
							.AllowSpin(true)
							.MinValue(0.0f)
							.MaxValue(2.0f)
							.MaxSliderValue(1.0f)
							.Value(this, &SFoliageEdit::GetPaintDensity)
							.OnValueChanged(this, &SFoliageEdit::SetPaintDensity)
						]
					]

					// Erase Density
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						.ToolTipText(LOCTEXT("EraseDensity_Tooltip", "The density of foliage to leave behind when erasing with the Shift key held. 0 will remove all foliage."))
						.Visibility(this, &SFoliageEdit::GetVisibility_EraseDensity)

						+ SHorizontalBox::Slot()
						.Padding(StandardLeftPadding)
						.FillWidth(1.0f)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("EraseDensity", "Erase Density"))
							.Font(StandardFont)
						]
						+ SHorizontalBox::Slot()
						.Padding(StandardRightPadding)
						.FillWidth(2.0f)
						.MaxWidth(100.f)
						.VAlign(VAlign_Center)
						[
							SNew(SNumericEntryBox<float>)
							.Font(StandardFont)
							.AllowSpin(true)
							.MinValue(0.0f)
							.MaxValue(2.0f)
							.MaxSliderValue(1.0f)
							.Value(this, &SFoliageEdit::GetEraseDensity)
							.OnValueChanged(this, &SFoliageEdit::SetEraseDensity)
						]
					]

					// Filters
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						.Visibility(this, &SFoliageEdit::GetVisibility_Filters)

						+ SHorizontalBox::Slot()
						.VAlign(VAlign_Center)
						.Padding(StandardPadding)
						[
							SNew(SWrapBox)
							.UseAllottedWidth(true)

							+ SWrapBox::Slot()
							.Padding(FMargin(0.f, 0.f, 6.f, 0.f))
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
								.Padding(FMargin(0.f, 5.f, 0.f, 0.f))
								[
									SNew(SCheckBox)
									.Visibility(this, &SFoliageEdit::GetVisibility_Filters)
									.OnCheckStateChanged(this, &SFoliageEdit::OnCheckStateChanged_Landscape)
									.IsChecked(this, &SFoliageEdit::GetCheckState_Landscape)
									.ToolTipText(this, &SFoliageEdit::GetTooltipText_Landscape)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("Landscape", "Landscape"))
										.Font(StandardFont)
									]
								]

								+ SVerticalBox::Slot()
								.Padding(FMargin(0.f, 5.f, 0.f, 0.f))
								[
									SNew(SCheckBox)
									.Visibility(this, &SFoliageEdit::GetVisibility_Filters)
									.OnCheckStateChanged(this, &SFoliageEdit::OnCheckStateChanged_BSP)
									.IsChecked(this, &SFoliageEdit::GetCheckState_BSP)
									.ToolTipText(this, &SFoliageEdit::GetTooltipText_BSP)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("BSP", "BSP"))
										.Font(StandardFont)
									]
								]
							]

							+ SWrapBox::Slot()
							.Padding(FMargin(0.f, 0.f, 6.f, 0.f))
							[
								SNew(SVerticalBox)

								+ SVerticalBox::Slot()
								.Padding(FMargin(0.f, 5.f, 0.f, 0.f))
								[
									SNew(SCheckBox)
									.Visibility(this, &SFoliageEdit::GetVisibility_Filters)
									.OnCheckStateChanged(this, &SFoliageEdit::OnCheckStateChanged_StaticMesh)
									.IsChecked(this, &SFoliageEdit::GetCheckState_StaticMesh)
									.ToolTipText(this, &SFoliageEdit::GetTooltipText_StaticMesh)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("StaticMeshes", "Static Meshes"))
										.Font(StandardFont)
									]
								]

								+ SVerticalBox::Slot()
								.Padding(FMargin(0.f, 5.f, 0.f, 0.f))
								[
									SNew(SCheckBox)
									.Visibility(this, &SFoliageEdit::GetVisibility_Filters)
									.OnCheckStateChanged(this, &SFoliageEdit::OnCheckStateChanged_Translucent)
									.IsChecked(this, &SFoliageEdit::GetCheckState_Translucent)
									.ToolTipText(this, &SFoliageEdit::GetTooltipText_Translucent)
									[
										SNew(STextBlock)
										.Text(LOCTEXT("Translucent", "Translucent"))
										.Font(StandardFont)
									]
								]
							]
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Fill)
					.Padding(StandardPadding)
					[
						SNew(SWrapBox)
						.UseAllottedWidth(true)
						.Visibility(this, &SFoliageEdit::GetVisibility_SelectionOptions)

						// Select all instances
						+ SWrapBox::Slot()
						.Padding(FMargin(0.f, 0.f, 5.f, 0.f))
						[
							SNew(SButton)
							.OnClicked(this, &SFoliageEdit::OnSelectAllInstances)
							.Text(LOCTEXT("SelectAllInstances", "Select All"))
							.ToolTipText(LOCTEXT("SelectAllInstances_Tooltip", "Selects all foliage instances"))
						]

						// Select all invalid instances
						+ SWrapBox::Slot()
						.Padding(FMargin(0.f, 0.f, 5.f, 0.f))
						[
							SNew(SButton)
							.OnClicked(this, &SFoliageEdit::OnSelectInvalidInstances)
							.Text(LOCTEXT("SelectInvalidInstances", "Select Invalid"))
							.ToolTipText(LOCTEXT("SelectInvalidInstances_Tooltip", "Selects all foliage instances that are not placed in a valid location"))
						]

						// Deselect all
						+ SWrapBox::Slot()
						.Padding(FMargin(0.f, 0.f, 5.f, 0.f))
						[
							SNew(SButton)
							.OnClicked(this, &SFoliageEdit::OnDeselectAllInstances)
							.Text(LOCTEXT("DeselectAllInstances", "Deselect All"))
							.ToolTipText(LOCTEXT("DeselectAllInstances_Tooltip", "Deselects all foliage instances"))
						]
					]
				]
			]
		]

		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.VAlign(VAlign_Fill)
		.Padding(0.f, 5.f, 0.f, 0.f)
		[
			SNew(SOverlay)

			+ SOverlay::Slot()
			[
				SAssignNew(FoliagePalette, SFoliagePalette)
				.FoliageEdMode(FoliageEditMode)
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
		]
	];

	RefreshFullList();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SFoliageEdit::RefreshFullList()
{
	FoliagePalette->Refresh();
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
}

TSharedRef<SWidget> SFoliageEdit::BuildToolBar()
{
	FToolBarBuilder Toolbar(UICommandList, FMultiBoxCustomization::None, nullptr, Orient_Vertical);
	Toolbar.SetLabelVisibility(EVisibility::Collapsed);
	Toolbar.SetStyle(&FEditorStyle::Get(), "FoliageEditToolbar");
	{
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetPaint);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetReapplySettings);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetSelect);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetLassoSelect);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetPaintBucket);
	}

	return
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
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
		];
}

void SFoliageEdit::AddFoliageType(const FAssetData& AssetData)
{
	GWarn->BeginSlowTask(LOCTEXT("AddFoliageType_LoadPackage", "Fully Loading Package for Add"), true, false);
	UObject* Asset = AssetData.GetAsset();
	GWarn->EndSlowTask();

	UFoliageType* FoliageType = FoliageEditMode->AddFoliageAsset(Asset);
	if (FoliageType)
	{
		FoliagePalette->Refresh();
	}
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

FText SFoliageEdit::GetActiveToolName() const
{
	FText OutText;
	if (IsPaintTool())
	{
		OutText = LOCTEXT("FoliageToolName_Paint", "Paint");
	}
	else if (IsReapplySettingsTool())
	{
		OutText = LOCTEXT("FoliageToolName_Reapply", "Reapply");
	}
	else if (IsSelectTool())
	{
		OutText = LOCTEXT("FoliageToolName_Select", "Select");
	}
	else if (IsLassoSelectTool())
	{
		OutText = LOCTEXT("FoliageToolName_LassoSelect", "Lasso Select");
	}
	else if (IsPaintFillTool())
	{
		OutText = LOCTEXT("FoliageToolName_Fill", "Fill");
	}

	return OutText;
}

void SFoliageEdit::SetRadius(float InRadius)
{
	FoliageEditMode->UISettings.SetRadius(InRadius);
}

TOptional<float> SFoliageEdit::GetRadius() const
{
	return FoliageEditMode->UISettings.GetRadius();
}

void SFoliageEdit::SetPaintDensity(float InDensity)
{
	FoliageEditMode->UISettings.SetPaintDensity(InDensity);
}

TOptional<float> SFoliageEdit::GetPaintDensity() const
{
	return FoliageEditMode->UISettings.GetPaintDensity();
}


void SFoliageEdit::SetEraseDensity(float InDensity)
{
	FoliageEditMode->UISettings.SetUnpaintDensity(InDensity);
}

TOptional<float> SFoliageEdit::GetEraseDensity() const
{
	return FoliageEditMode->UISettings.GetUnpaintDensity();
}

EVisibility SFoliageEdit::GetVisibility_SelectionOptions() const
{
	return IsSelectTool() || IsLassoSelectTool() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
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

FReply SFoliageEdit::OnSelectAllInstances()
{
	for (FFoliageMeshUIInfoPtr& TypeInfo : FoliageEditMode->GetFoliageMeshList())
	{
		if (TypeInfo->InstanceCountCurrentLevel > 0)
		{
			UFoliageType* FoliageType = TypeInfo->Settings;
			FoliageEditMode->SelectInstances(FoliageType, true);
		}
	}

	return FReply::Handled();
}

FReply SFoliageEdit::OnSelectInvalidInstances()
{
	for (FFoliageMeshUIInfoPtr& TypeInfo : FoliageEditMode->GetFoliageMeshList())
	{
		if (TypeInfo->InstanceCountCurrentLevel > 0)
		{
			const UFoliageType* FoliageType = TypeInfo->Settings;
			FoliageEditMode->SelectInstances(FoliageType, false);
			FoliageEditMode->SelectInvalidInstances(FoliageType);
		}
	}

	return FReply::Handled();
}

FReply SFoliageEdit::OnDeselectAllInstances()
{
	for (FFoliageMeshUIInfoPtr& TypeInfo : FoliageEditMode->GetFoliageMeshList())
	{
		if (TypeInfo->InstanceCountCurrentLevel > 0)
		{
			UFoliageType* FoliageType = TypeInfo->Settings;
			FoliageEditMode->SelectInstances(FoliageType, false);
		}
	}

	return FReply::Handled();
}

FReply SFoliageEdit::OnDrop_ListView(const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent)
{
	TArray<FAssetData> DroppedAssetData = AssetUtil::ExtractAssetDataFromDrag(DragDropEvent);
	{
		// Treat the entire drop as a transaction (in case multiples types are being added)
		const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "FoliageMode_DragDropTypesTransaction", "Drag-drop Foliage Type(s)"));

		for (int32 AssetIdx = 0; AssetIdx < DroppedAssetData.Num(); ++AssetIdx)
		{	
			AddFoliageType(DroppedAssetData[AssetIdx]);
		}
	}

	return FReply::Handled();
}

FText SFoliageEdit::GetFilterText() const
{
	FText TooltipText;
	if (IsPaintTool() || IsPaintFillTool())
	{
		TooltipText = LOCTEXT("PlacementFilter", "Placement Filter");
	}
	else if (IsReapplySettingsTool())
	{
		TooltipText = LOCTEXT("ReapplyFilter", "Reapply Filter");
	}
	else if (IsLassoSelectTool())
	{
		TooltipText = LOCTEXT("SelectionFilter", "Selection Filter");
	}

	return TooltipText;
}

void SFoliageEdit::OnCheckStateChanged_Landscape(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterLandscape(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit::GetCheckState_Landscape() const
{
	return FoliageEditMode->UISettings.GetFilterLandscape() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FText SFoliageEdit::GetTooltipText_Landscape() const
{
	FText TooltipText;
	if (IsPaintTool() || IsPaintFillTool())
	{
		TooltipText = LOCTEXT("FilterLandscapeTooltip_Placement", "Place foliage on landscapes");
	}
	else if (IsReapplySettingsTool())
	{
		TooltipText = LOCTEXT("FilterLandscapeTooltip_Reapply", "Reapply to instances on landscapes");
	}
	else if (IsLassoSelectTool())
	{
		TooltipText = LOCTEXT("FilterLandscapeTooltip_Select", "Select instances on landscapes");
	}

	return TooltipText;
}

void SFoliageEdit::OnCheckStateChanged_StaticMesh(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterStaticMesh(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit::GetCheckState_StaticMesh() const
{
	return FoliageEditMode->UISettings.GetFilterStaticMesh() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FText SFoliageEdit::GetTooltipText_StaticMesh() const
{
	FText TooltipText;
	if (IsPaintTool() || IsPaintFillTool())
	{
		TooltipText = LOCTEXT("FilterStaticMeshTooltip_Placement", "Place foliage on static meshes");
	}
	else if (IsReapplySettingsTool())
	{
		TooltipText = LOCTEXT("FilterStaticMeshTooltip_Reapply", "Reapply to instances on static meshes");
	}
	else if (IsLassoSelectTool())
	{
		TooltipText = LOCTEXT("FilterStaticMeshTooltip_Select", "Select instances on static meshes");
	}

	return TooltipText;
}

void SFoliageEdit::OnCheckStateChanged_BSP(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterBSP(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit::GetCheckState_BSP() const
{
	return FoliageEditMode->UISettings.GetFilterBSP() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FText SFoliageEdit::GetTooltipText_BSP() const
{
	FText TooltipText;
	if (IsPaintTool() || IsPaintFillTool())
	{
		TooltipText = LOCTEXT("FilterBSPTooltip_Placement", "Place foliage on BSP");
	}
	else if (IsReapplySettingsTool())
	{
		TooltipText = LOCTEXT("FilterBSPTooltip_Reapply", "Reapply to instances on BSP");
	}
	else if (IsLassoSelectTool())
	{
		TooltipText = LOCTEXT("FilterBSPTooltip_Select", "Select instances on BSP");
	}

	return TooltipText;
}

void SFoliageEdit::OnCheckStateChanged_Translucent(ECheckBoxState InState)
{
	FoliageEditMode->UISettings.SetFilterTranslucent(InState == ECheckBoxState::Checked ? true : false);
}

ECheckBoxState SFoliageEdit::GetCheckState_Translucent() const
{
	return FoliageEditMode->UISettings.GetFilterTranslucent() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FText SFoliageEdit::GetTooltipText_Translucent() const
{
	FText TooltipText;
	if (IsPaintTool() || IsPaintFillTool())
	{
		TooltipText = LOCTEXT("FilterTranslucentTooltip_Placement", "Place foliage on translucent geometry");
	}
	else if (IsReapplySettingsTool())
	{
		TooltipText = LOCTEXT("FilterTranslucentTooltip_Reapply", "Reapply to instances on translucent geometry");
	}
	else if (IsLassoSelectTool())
	{
		TooltipText = LOCTEXT("FilterTranslucentTooltip_Select", "Select instances on translucent geometry");
	}

	return TooltipText;
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
	if (!FoliageEditMode->UISettings.GetPaintToolSelected())
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility SFoliageEdit::GetVisibility_EraseDensity() const
{
	if (!FoliageEditMode->UISettings.GetPaintToolSelected())
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

#undef LOCTEXT_NAMESPACE
