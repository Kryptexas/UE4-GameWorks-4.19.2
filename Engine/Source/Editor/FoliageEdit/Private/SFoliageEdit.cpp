// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

#include "SFoliageEdit.h"
#include "FoliageEditActions.h"
#include "FoliageEdMode.h"
#include "Runtime/Engine/Classes/Foliage/InstancedFoliageSettings.h"
#include "Editor/UnrealEd/Public/AssetThumbnail.h"

#include "Editor/UnrealEd/Public/DragAndDrop/AssetDragDropOp.h"
#include "Editor/UnrealEd/Public/AssetSelection.h"

#include "SFoliageEditMeshDisplayItem.h"

#define LOCTEXT_NAMESPACE "FoliageEd_Mode"

class SFoliageDragDropHandler : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SFoliageDragDropHandler) {}

	SLATE_DEFAULT_SLOT( FArguments, Content )

	SLATE_ARGUMENT( TSharedPtr<SFoliageEdit>, FoliageEditPtr )

	SLATE_END_ARGS()

	/** SCompoundWidget functions */
	void Construct(const FArguments& InArgs)
	{
		FoliageEditPtr = InArgs._FoliageEditPtr;

		this->ChildSlot
		[
			InArgs._Content.Widget
		];
	}

	~SFoliageDragDropHandler() {}

	FReply OnDrop( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
	{
		FoliageEditPtr->DisableDragDropOverlay();
		return FoliageEditPtr->OnDrop_ListView(MyGeometry, DragDropEvent);
	}

	virtual void OnDragEnter( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent ) OVERRIDE
	{
		FoliageEditPtr->EnableDragDropOverlay();
		SCompoundWidget::OnDragEnter(MyGeometry, DragDropEvent);
	}

	virtual void OnDragLeave( const FDragDropEvent& DragDropEvent ) OVERRIDE
	{
		FoliageEditPtr->DisableDragDropOverlay();
		SCompoundWidget::OnDragLeave(DragDropEvent);
	}

private:
	TSharedPtr<SFoliageEdit> FoliageEditPtr;
};

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void SFoliageEdit::Construct(const FArguments& InArgs)
{
	FoliageEditMode = (FEdModeFoliage*)GEditorModeTools().GetActiveMode( FBuiltinEditorModes::EM_Foliage );

	FFoliageEditCommands::Register();

	UICommandList = MakeShareable( new FUICommandList );

	BindCommands();

	AssetThumbnailPool = MakeShareable( new FAssetThumbnailPool(512, TAttribute<bool>::Create( TAttribute<bool>::FGetter::CreateSP(this, &SFoliageEdit::IsHovered) ) ) );

	bOverlayOverride = false;

	// Everything (or almost) uses this padding, change it to expand the padding.
	FMargin StandardPadding(0.0f, 4.0f, 0.0f, 4.0f);

	this->ChildSlot
		[
			SNew( SScrollBox )
			+ SScrollBox::Slot()
			.Padding( 0.0f )
			[
				SNew( SVerticalBox )
				 
				+SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(StandardPadding)
				[
					BuildToolBar()
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( SHorizontalBox )

					+SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(StandardPadding)
					[
						SNew(STextBlock)
							.Visibility( this, &SFoliageEdit::GetVisibility_Radius )
							.Text(LOCTEXT("BrushSize", "Brush Size").ToString())
					]
					+SHorizontalBox::Slot()
						.FillWidth(2.0f)
						.Padding(StandardPadding)
					[
						SNew(SSpinBox<float>)
							.Visibility( this, &SFoliageEdit::GetVisibility_Radius )
							.MinValue(0.0f)
							.MaxValue(65536.0f)
							.MaxSliderValue(8192.0f)
							.SliderExponent(3.0f)
							.Value(this, &SFoliageEdit::GetRadius)
							.OnValueChanged(this, &SFoliageEdit::SetRadius)
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( SHorizontalBox )

					+SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(StandardPadding)
					[
						SNew(STextBlock)
							.Visibility( this, &SFoliageEdit::GetVisibility_PaintDensity )
							.Text(LOCTEXT("PaintDensity", "Paint Density").ToString())
					]
					+SHorizontalBox::Slot()
						.FillWidth(2.0f)
						.Padding(StandardPadding)
					[
						SNew(SSpinBox<float>)
							.Visibility( this, &SFoliageEdit::GetVisibility_PaintDensity )
							.MinValue(0.0f)
							.MaxValue(2.0f)
							.MaxSliderValue(1.0f)
							.Value(this, &SFoliageEdit::GetPaintDensity)
							.OnValueChanged(this, &SFoliageEdit::SetPaintDensity)
					]
				]
			
				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( SHorizontalBox )

					+SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(StandardPadding)
					[
						SNew(STextBlock)
							.Visibility( this, &SFoliageEdit::GetVisibility_EraseDensity )
							.Text(LOCTEXT("EraseDensity", "Erase Density").ToString())
					]
					+SHorizontalBox::Slot()
						.FillWidth(2.0f)
						.Padding(StandardPadding)
					[
						SNew(SSpinBox<float>)
							.Visibility( this, &SFoliageEdit::GetVisibility_EraseDensity )
							.MinValue(0.0f)
							.MaxValue(2.0f)
							.MaxSliderValue(1.0f)
							.Value(this, &SFoliageEdit::GetEraseDensity)
							.OnValueChanged(this, &SFoliageEdit::SetEraseDensity)
					]
				]
		
				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(StandardPadding)
				[
					SNew(STextBlock)
						.Visibility( this, &SFoliageEdit::GetVisibility_Filters )
						.Text(LOCTEXT("Filter", "Filter").ToString())
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(StandardPadding)
				[
					SNew(SWrapBox)
						.UseAllottedWidth(true)

					+SWrapBox::Slot()
						.Padding(0.0f, 0.0f, 16.0f, 0.0f)
					[
						SNew(SCheckBox)
							.Visibility( this, &SFoliageEdit::GetVisibility_Filters )
							.OnCheckStateChanged( this, &SFoliageEdit::OnCheckStateChanged_Landscape )
							.IsChecked( this, &SFoliageEdit::GetCheckState_Landscape )
							[
								SNew(STextBlock)
									.Text(LOCTEXT("Landscape", "Landscape").ToString())
							]
					]
				
					+SWrapBox::Slot()
						.Padding(0.0f, 0.0f, 16.0f, 0.0f)
					[
						SNew(SCheckBox)
							.Visibility( this, &SFoliageEdit::GetVisibility_Filters )
							.OnCheckStateChanged( this, &SFoliageEdit::OnCheckStateChanged_StaticMesh )
							.IsChecked( this, &SFoliageEdit::GetCheckState_StaticMesh )
							[
								SNew(STextBlock)
									.Text(LOCTEXT("StaticMeshes", "Static Meshes").ToString())
							]
					]

					+SWrapBox::Slot()
					[
						SNew(SCheckBox)
							.Visibility( this, &SFoliageEdit::GetVisibility_Filters )
							.OnCheckStateChanged( this, &SFoliageEdit::OnCheckStateChanged_BSP )
							.IsChecked( this, &SFoliageEdit::GetCheckState_BSP )
							[
								SNew(STextBlock)
									.Text(LOCTEXT("BSP", "BSP").ToString())
							]
					]
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SHeaderRow)

					+SHeaderRow::Column(TEXT("Meshes"))
						.DefaultLabel(LOCTEXT("Meshes", "Meshes").ToString())
				]

				+SVerticalBox::Slot()
				.AutoHeight()
				.Padding(StandardPadding)
				[

					SNew(SOverlay)

					+SOverlay::Slot()
					[
						SNew(SFoliageDragDropHandler)
							.FoliageEditPtr(SharedThis(this))
							[
								SAssignNew(ItemScrollBox, SScrollBox)
							]
					]

					+SOverlay::Slot()
					[
						SNew(SHorizontalBox)
							.Visibility( this, &SFoliageEdit::GetVisibility_EmptyList )

						+SHorizontalBox::Slot()
							.FillWidth(1.0f)
						[
							SNew(SBorder)
							.BorderImage( FEditorStyle::GetBrush("Menu.Background") )
							.BorderBackgroundColor(FLinearColor(1, 1, 1, 0.8f))
							.Content()
							[
								SNew(SHorizontalBox)

								+SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SSpacer)
								]

								+SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
								[
									SNew(STextBlock)
										.Visibility( this, &SFoliageEdit::GetVisibility_EmptyListText )
										.Text( LOCTEXT("EmptyFoliageListMessage", "Drag static meshes from the content browser into this area.").ToString() )	
								]

								+SHorizontalBox::Slot()
									.FillWidth(1.0f)
									[
										SNew(SSpacer)
									]
							]
						]
					]
				]
			]
		];

		RefreshFullList();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

SFoliageEdit::~SFoliageEdit()
{
	// Release all rendering resources being held onto
	AssetThumbnailPool->ReleaseResources();
}

void SFoliageEdit::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	AssetThumbnailPool->Tick(InDeltaTime);
}

TSharedPtr<FAssetThumbnail> SFoliageEdit::CreateThumbnail(UStaticMesh* InStaticMesh)
{
	return MakeShareable( new FAssetThumbnail( InStaticMesh, 80, 80, AssetThumbnailPool ) );
}

void SFoliageEdit::RefreshFullList()
{
	for( int DisplayItemIdx = 0; DisplayItemIdx < DisplayItemList.Num(); )
	{
		RemoveItemFromScrollbox(DisplayItemList[DisplayItemIdx]);
	}

	TArray<struct FFoliageMeshUIInfo>& FoliageMeshList = FoliageEditMode->GetFoliageMeshList();

	for( int MeshIdx = 0; MeshIdx < FoliageMeshList.Num(); MeshIdx++ )
	{
		AddItemToScrollbox( FoliageMeshList[MeshIdx] );
	}
}

void SFoliageEdit::AddItemToScrollbox(struct FFoliageMeshUIInfo& InFoliageInfoToAdd)
{
	TSharedRef< SFoliageEditMeshDisplayItem > DisplayItem = 
		SNew( SFoliageEditMeshDisplayItem )
			.FoliageEditPtr( SharedThis(this) )
			.FoliageSettingsPtr( InFoliageInfoToAdd.MeshInfo->Settings )
			.AssociatedStaticMesh(InFoliageInfoToAdd.StaticMesh)
			.AssetThumbnail( CreateThumbnail(InFoliageInfoToAdd.StaticMesh) )
			.FoliageMeshUIInfo(MakeShareable(new FFoliageMeshUIInfo( InFoliageInfoToAdd ) ));

	DisplayItemList.Add(DisplayItem);

	ItemScrollBox->AddSlot()
	[
		DisplayItem			
	];
}

void SFoliageEdit::RemoveItemFromScrollbox(const TSharedPtr<SFoliageEditMeshDisplayItem> InWidgetToRemove)
{
	DisplayItemList.Remove(InWidgetToRemove.ToSharedRef());

	ItemScrollBox->RemoveSlot(InWidgetToRemove.ToSharedRef());
}

void SFoliageEdit::ReplaceItem(const TSharedPtr<class SFoliageEditMeshDisplayItem> InDisplayItemToReplaceIn, UStaticMesh* InNewStaticMesh)
{
	TArray<struct FFoliageMeshUIInfo>& FoliageMeshList = FoliageEditMode->GetFoliageMeshList();

	for(int32 UIInfoIndex = 0; UIInfoIndex < FoliageMeshList.Num(); ++UIInfoIndex)
	{
		if(FoliageMeshList[UIInfoIndex].StaticMesh == InNewStaticMesh)
		{
			// This is the info for the new static mesh. Update the display item.
			InDisplayItemToReplaceIn->Replace(FoliageMeshList[UIInfoIndex].MeshInfo->Settings, 
												FoliageMeshList[UIInfoIndex].StaticMesh, 
												CreateThumbnail(FoliageMeshList[UIInfoIndex].StaticMesh), 
												MakeShareable(new FFoliageMeshUIInfo( FoliageMeshList[UIInfoIndex] ) ) );
			break;
		}
	}
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
		FExecuteAction::CreateSP( this, &SFoliageEdit::OnSetPaint ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SFoliageEdit::IsPaintTool ) );

	UICommandList->MapAction(
		Commands.SetReapplySettings,
		FExecuteAction::CreateSP( this, &SFoliageEdit::OnSetReapplySettings ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SFoliageEdit::IsReapplySettingsTool ) );

	UICommandList->MapAction(
		Commands.SetSelect,
		FExecuteAction::CreateSP( this, &SFoliageEdit::OnSetSelectInstance ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SFoliageEdit::IsSelectTool ) );

	UICommandList->MapAction(
		Commands.SetLassoSelect,
		FExecuteAction::CreateSP( this, &SFoliageEdit::OnSetLasso ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SFoliageEdit::IsLassoSelectTool ) );

	UICommandList->MapAction(
		Commands.SetPaintBucket,
		FExecuteAction::CreateSP( this, &SFoliageEdit::OnSetPaintFill ),
		FCanExecuteAction(),
		FIsActionChecked::CreateSP( this, &SFoliageEdit::IsPaintFillTool ) );
}

TSharedRef<SWidget> SFoliageEdit::BuildToolBar()
{
	FToolBarBuilder Toolbar(UICommandList, FMultiBoxCustomization::None );
	{
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetPaint);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetReapplySettings);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetSelect);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetLassoSelect);
		Toolbar.AddToolBarButton(FFoliageEditCommands::Get().SetPaintBucket);
	}

	return
		SNew(SHorizontalBox)

		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(4,0)
		[
			SNew(SBorder)
			.Padding(0)
			.BorderImage(FEditorStyle::GetBrush("NoBorder"))
			.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
			[
				Toolbar.MakeWidget()
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

void SFoliageEdit::OnDragEnter_ListView( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	bOverlayOverride = true;
	SCompoundWidget::OnDragEnter(MyGeometry, DragDropEvent);
}

void SFoliageEdit::OnDragLeave_ListView( const FDragDropEvent& DragDropEvent )
{
	bOverlayOverride = false;
	SCompoundWidget::OnDragLeave(DragDropEvent);
}

FReply SFoliageEdit::OnDragOver_ListView( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{

	return FReply::Handled();
}

EVisibility SFoliageEdit::GetVisibility_EmptyList() const
{
	if( DisplayItemList.Num() > 0 && !bOverlayOverride )
	{
		return EVisibility::Collapsed;
	}
	
	return EVisibility::HitTestInvisible;
}

EVisibility SFoliageEdit::GetVisibility_EmptyListText() const
{
	if( DisplayItemList.Num() > 0 )
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::HitTestInvisible;
}

EVisibility SFoliageEdit::GetVisibility_NonEmptyList() const
{
	if( DisplayItemList.Num() == 0 )
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::HitTestInvisible;
}

bool SFoliageEdit::CanAddStaticMesh(const UStaticMesh* const InStaticMesh) const
{
	for(int32 FoliageIdx = 0; FoliageIdx < FoliageEditMode->GetFoliageMeshList().Num(); ++FoliageIdx)
	{
		if( FoliageEditMode->GetFoliageMeshList()[FoliageIdx].StaticMesh == InStaticMesh )
		{
			return false;
		}
	}
	return true;
}

FReply SFoliageEdit::OnDrop_ListView( const FGeometry& MyGeometry, const FDragDropEvent& DragDropEvent )
{
	TArray< FAssetData > DroppedAssetData = AssetUtil::ExtractAssetDataFromDrag( DragDropEvent );

	for (int32 AssetIdx = 0; AssetIdx < DroppedAssetData.Num(); ++AssetIdx)
	{
		GWarn->BeginSlowTask( LOCTEXT("OnDrop_LoadPackage", "Fully Loading Package For Drop"), true, false );
		UStaticMesh* StaticMesh = Cast<UStaticMesh>( DroppedAssetData[ AssetIdx ].GetAsset() );
		GWarn->EndSlowTask();

		if( StaticMesh && CanAddStaticMesh(StaticMesh) )
		{
			FoliageEditMode->AddFoliageMesh(StaticMesh);
			AddItemToScrollbox(FoliageEditMode->GetFoliageMeshList()[FoliageEditMode->GetFoliageMeshList().Num() - 1]);
		}

	}

	return FReply::Handled();
}

void SFoliageEdit::OnCheckStateChanged_Landscape(ESlateCheckBoxState::Type InState)
{
	FoliageEditMode->UISettings.SetFilterLandscape(InState == ESlateCheckBoxState::Checked? true : false);
}

ESlateCheckBoxState::Type SFoliageEdit::GetCheckState_Landscape() const
{
	return FoliageEditMode->UISettings.GetFilterLandscape()? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEdit::OnCheckStateChanged_StaticMesh(ESlateCheckBoxState::Type InState)
{
	FoliageEditMode->UISettings.SetFilterStaticMesh(InState == ESlateCheckBoxState::Checked? true : false);
}

ESlateCheckBoxState::Type SFoliageEdit::GetCheckState_StaticMesh() const
{
	return FoliageEditMode->UISettings.GetFilterStaticMesh()? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SFoliageEdit::OnCheckStateChanged_BSP(ESlateCheckBoxState::Type InState)
{
	FoliageEditMode->UISettings.SetFilterBSP(InState == ESlateCheckBoxState::Checked? true : false);
}

ESlateCheckBoxState::Type SFoliageEdit::GetCheckState_BSP() const
{
	return FoliageEditMode->UISettings.GetFilterBSP()? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

EVisibility SFoliageEdit::GetVisibility_Radius() const
{
	if( FoliageEditMode->UISettings.GetSelectToolSelected() || FoliageEditMode->UISettings.GetReapplyPaintBucketToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected() )
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility SFoliageEdit::GetVisibility_PaintDensity() const
{
	if( FoliageEditMode->UISettings.GetSelectToolSelected() || FoliageEditMode->UISettings.GetReapplyPaintBucketToolSelected() || FoliageEditMode->UISettings.GetLassoSelectToolSelected())
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility SFoliageEdit::GetVisibility_EraseDensity() const
{
	if( FoliageEditMode->UISettings.GetSelectToolSelected() || FoliageEditMode->UISettings.GetReapplyPaintBucketToolSelected() || FoliageEditMode->UISettings.GetLassoSelectToolSelected() || FoliageEditMode->UISettings.GetPaintBucketToolSelected() )
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

EVisibility SFoliageEdit::GetVisibility_Filters() const
{
	if( FoliageEditMode->UISettings.GetSelectToolSelected() )
	{
		return EVisibility::Collapsed;
	}

	return EVisibility::Visible;
}

void SFoliageEdit::DisableDragDropOverlay(void) 
{ 
	bOverlayOverride = false; 
}

void SFoliageEdit::EnableDragDropOverlay(void) 
{ 
	bOverlayOverride = true; 
}

#undef LOCTEXT_NAMESPACE
