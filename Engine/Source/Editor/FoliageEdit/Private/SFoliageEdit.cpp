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
#include "DetailLayoutBuilder.h"
#include "SNumericEntryBox.h"

#include "FoliageType_InstancedStaticMesh.h"
#include "Editor/PropertyEditor/Public/PropertyCustomizationHelpers.h"
#include "LevelEditor.h"

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
	FoliageEditMode->OnToolChanged.AddSP(this, &SFoliageEdit::HandleOnToolChanged);

	FFoliageEditCommands::Register();

	UICommandList = MakeShareable(new FUICommandList);

	BindCommands();

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs Args(false, false, false, FDetailsViewArgs::HideNameArea, true);
	Args.bShowActorLabel = false;
	MeshDetailsWidget = PropertyModule.CreateDetailView(Args);

	// We want to use our own customization for UFoliageType
	MeshDetailsWidget->UnregisterInstancedCustomPropertyLayout(UFoliageType::StaticClass());
	MeshDetailsWidget->RegisterInstancedCustomPropertyLayout(UFoliageType::StaticClass(), 
		FOnGetDetailCustomizationInstance::CreateStatic(&FFoliageTypePaintingCustomization::MakeInstance, FoliageEditMode)
		);

	// Everything (or almost) uses this padding, change it to expand the padding.
	FMargin StandardPadding(6.f, 3.f);
	FMargin StandardLeftPadding(6.f, 3.f, 3.f, 3.f);
	FMargin StandardRightPadding(3.f, 3.f, 6.f, 3.f);

	FSlateFontInfo StandardFont = FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont"));

	const FText BlankText = LOCTEXT("Blank", "");

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

			// Filter
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				.Visibility(this, &SFoliageEdit::GetVisibility_Filters)

				+ SHorizontalBox::Slot()
				.Padding(StandardLeftPadding)
				.VAlign(VAlign_Center)
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(this, &SFoliageEdit::GetFilterText)
					.ToolTipText(this, &SFoliageEdit::GetFilterTooltipText)
					.Font(StandardFont)
				]

				+ SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.FillWidth(2.0f)
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
			.Padding(StandardPadding)
			[
				SNew(SSeparator)
				.Orientation(Orient_Horizontal)
			]

			+ SVerticalBox::Slot()
			.Padding(StandardPadding)
			.AutoHeight()
			.HAlign(HAlign_Left)
			[
				SAssignNew(AddFoliageTypeCombo, SComboButton)
				.ForegroundColor(FLinearColor::White)
				.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
				.OnGetMenuContent(this, &SFoliageEdit::OnGetAddFoliageMeshAssetPicker)
				.ContentPadding(FMargin(0.f))
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
				.Style(FEditorStyle::Get(), "DetailsView.Splitter")

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
						// toggle mesh for painting checkbox
						SAssignNew(MeshTreeHeaderRowWidget, SHeaderRow)
						+ SHeaderRow::Column(FoliageMeshColumns::ColumnID_ToggleMesh)
						[
							SNew(SCheckBox)
							.IsChecked(this, &SFoliageEdit::GetState_AllMeshes)
							.OnCheckStateChanged(this, &SFoliageEdit::OnCheckStateChanged_AllMeshes)
						]
						.DefaultLabel(BlankText)
						.HeaderContentPadding(FMargin(0, 1, 0, 1))
						.HAlignHeader(HAlign_Center)
						.HAlignCell(HAlign_Center)
						.FixedWidth(24)

						// mesh text label
						+ SHeaderRow::Column(FoliageMeshColumns::ColumnID_MeshLabel)
						.HeaderContentPadding(FMargin(10, 1, 0, 1))
						.SortMode(this, &SFoliageEdit::GetMeshColumnSortMode)
						.OnSort(this, &SFoliageEdit::OnMeshesColumnSortModeChanged)
						.DefaultLabel(this, &SFoliageEdit::GetMeshesHeaderText)

						// instance count
						+ SHeaderRow::Column(FoliageMeshColumns::ColumnID_InstanceCount)
						.HeaderContentPadding(FMargin(10, 1, 0, 1))
						.DefaultLabel(LOCTEXT("InstanceCount", "Count"))
						.DefaultTooltip(this, &SFoliageEdit::GetTotalInstanceCountTooltipText)
						.FixedWidth(60.f)

						// save Foliage asset
						+SHeaderRow::Column(FoliageMeshColumns::ColumnID_Save).FixedWidth(24.0f)
						.DefaultLabel(BlankText)
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

	UICommandList->MapAction(
		Commands.SelectInvalidInstances,
		FExecuteAction::CreateSP(this, &SFoliageEdit::OnSelectInvalidInstances),
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

void SFoliageEdit::AddFoliageType(const FAssetData& AssetData)
{
	if (AddFoliageTypeCombo.IsValid())
	{
		AddFoliageTypeCombo->SetIsOpen(false);
	}

	GWarn->BeginSlowTask(LOCTEXT("AddFoliageType_LoadPackage", "Fully Loading Package for Add"), true, false);
	UObject* Asset = AssetData.GetAsset();
	GWarn->EndSlowTask();

	UFoliageType* FoliageType = FoliageEditMode->AddFoliageAsset(Asset);
	if (FoliageType)
	{
		MeshTreeWidget->RequestTreeRefresh();
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

EVisibility SFoliageEdit::GetAddFoliageTypeButtonTextVisibility() const
{
	return MeshTreeHeaderRowWidget.IsValid() && MeshTreeHeaderRowWidget->IsHovered() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
}

TSharedRef<SWidget> SFoliageEdit::OnGetAddFoliageMeshAssetPicker()
{
	TArray<const UClass*> ClassFilters;
	ClassFilters.Add(UFoliageType_InstancedStaticMesh::StaticClass());

	return PropertyCustomizationHelpers::MakeAssetPickerWithMenu(FAssetData(),
																false,
																ClassFilters,
																PropertyCustomizationHelpers::GetNewAssetFactoriesForClasses(ClassFilters),
																FOnShouldFilterAsset(),
																FOnAssetSelected::CreateSP(this, &SFoliageEdit::AddFoliageType),
																FSimpleDelegate());
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
		// Treat the entire drop as a transaction (in case multiples types are being added)
		const FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "FoliageMode_DragDropTypesTransaction", "Drag-drop Foliage Type(s)"));

		for (int32 AssetIdx = 0; AssetIdx < DroppedAssetData.Num(); ++AssetIdx)
		{	
			AddFoliageType(DroppedAssetData[AssetIdx]);
		}
	}

	return FReply::Handled();
}
TSharedPtr<SWidget> SFoliageEdit::ConstructFoliageMeshContextMenu() const
{
	const FFoliageEditCommands& Commands = FFoliageEditCommands::Get();
	FMenuBuilder MenuBuilder(true, UICommandList);

	if (MeshTreeWidget->GetSelectedItems().Num() > 0)
	{
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
			MenuBuilder.AddMenuEntry(Commands.SelectInvalidInstances);
		}
	}

	return MenuBuilder.MakeWidget();
}

FText SFoliageEdit::GetFilterText() const
{
	FText TooltipText;
	if (FoliageEditMode->UISettings.GetPaintToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected())
	{
		TooltipText = LOCTEXT("PlacementFilter", "Placement Filter");
	}
	else if (FoliageEditMode->UISettings.GetReapplyToolSelected())
	{
		TooltipText = LOCTEXT("ReapplyFilter", "Reapply Filter");
	}
	else if (FoliageEditMode->UISettings.GetLassoSelectToolSelected())
	{
		TooltipText = LOCTEXT("SelectionFilter", "Selection Filter");
	}

	return TooltipText;
}

FText SFoliageEdit::GetFilterTooltipText() const
{
	FText TooltipText;
	if (FoliageEditMode->UISettings.GetPaintToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected())
	{
		TooltipText = LOCTEXT("PlacementFilter_Tooltip", "Limits the types of geometry upon which foliage instances can be placed.");
	}
	else if (FoliageEditMode->UISettings.GetReapplyToolSelected())
	{
		TooltipText = LOCTEXT("ReapplyFilter_Tooltip", "Limits the types of geometry upon which foliage instances can be reapplied.");
	}
	else if (FoliageEditMode->UISettings.GetLassoSelectToolSelected())
	{
		TooltipText = LOCTEXT("SelectionFilter_Tooltip", "Limits the types of geometry upon which foliage instances can be selected.");
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
	if (FoliageEditMode->UISettings.GetPaintToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected())
	{
		TooltipText = LOCTEXT("FilterLandscapeTooltip_Placement", "Place foliage on landscapes");
	}
	else if (FoliageEditMode->UISettings.GetReapplyToolSelected())
	{
		TooltipText = LOCTEXT("FilterLandscapeTooltip_Reapply", "Reapply to instances on landscapes");
	}
	else if (FoliageEditMode->UISettings.GetLassoSelectToolSelected())
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
	if (FoliageEditMode->UISettings.GetPaintToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected())
	{
		TooltipText = LOCTEXT("FilterStaticMeshTooltip_Placement", "Place foliage on static meshes");
	}
	else if (FoliageEditMode->UISettings.GetReapplyToolSelected())
	{
		TooltipText = LOCTEXT("FilterStaticMeshTooltip_Reapply", "Reapply to instances on static meshes");
	}
	else if (FoliageEditMode->UISettings.GetLassoSelectToolSelected())
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
	if (FoliageEditMode->UISettings.GetPaintToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected())
	{
		TooltipText = LOCTEXT("FilterBSPTooltip_Placement", "Place foliage on BSP");
	}
	else if (FoliageEditMode->UISettings.GetReapplyToolSelected())
	{
		TooltipText = LOCTEXT("FilterBSPTooltip_Reapply", "Reapply to instances on BSP");
	}
	else if (FoliageEditMode->UISettings.GetLassoSelectToolSelected())
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
	if (FoliageEditMode->UISettings.GetPaintToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected())
	{
		TooltipText = LOCTEXT("FilterTranslucentTooltip_Placement", "Place foliage on translucent geometry");
	}
	else if (FoliageEditMode->UISettings.GetReapplyToolSelected())
	{
		TooltipText = LOCTEXT("FilterTranslucentTooltip_Reapply", "Reapply to instances on translucent geometry");
	}
	else if (FoliageEditMode->UISettings.GetLassoSelectToolSelected())
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
	RefreshMeshDetailsWidget();
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

void SFoliageEdit::OnSelectInvalidInstances()
{
	auto MeshUIList = MeshTreeWidget->GetSelectedItems();
	for (FFoliageMeshUIInfoPtr MeshUIPtr : MeshUIList)
	{
		const UFoliageType* FoliageType = MeshUIPtr->Settings;
		FoliageEditMode->SelectInvalidInstances(FoliageType);
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
	return FText::Format(LOCTEXT("FoliageMeshCount", "Meshes ({0})"), FText::AsNumber(NumMeshes));
}

EColumnSortMode::Type SFoliageEdit::GetMeshColumnSortMode() const
{
	return FoliageEditMode->GetFoliageMeshListSortMode();
}

void SFoliageEdit::OnMeshesColumnSortModeChanged(EColumnSortPriority::Type InPriority, const FName& InColumnName, EColumnSortMode::Type InSortMode)
{
	FoliageEditMode->OnFoliageMeshListSortModeChanged(InSortMode);
}

FText SFoliageEdit::GetTotalInstanceCountTooltipText() const
{
	// Probably should cache these values, 
	// but we call this only occasionally when tooltip is active
	int32 InstanceCountTotal = 0;
	int32 InstanceCountCurrentLevel = 0;
	FoliageEditMode->CalcTotalInstanceCount(InstanceCountTotal, InstanceCountCurrentLevel);
			
	return FText::Format(LOCTEXT("FoliageTotalInstanceCount", "Current Level: {0} Total: {1}"), FText::AsNumber(InstanceCountCurrentLevel), FText::AsNumber(InstanceCountTotal));
}

void SFoliageEdit::HandleOnToolChanged()
{
	RefreshMeshDetailsWidget();
}

void SFoliageEdit::RefreshMeshDetailsWidget()
{
	TArray<UObject*> SelectedFoliageTypes;
	auto SelectedFoliageInfoList = MeshTreeWidget->GetSelectedItems();

	for (const auto& Info : SelectedFoliageInfoList)
	{
		SelectedFoliageTypes.Add(Info->Settings);
	}

	const bool bForceRefresh = true;
	MeshDetailsWidget->SetObjects(SelectedFoliageTypes, bForceRefresh);
}

#undef LOCTEXT_NAMESPACE
