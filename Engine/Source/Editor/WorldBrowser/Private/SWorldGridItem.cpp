// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#include "WorldBrowserPrivatePCH.h"

#include "ObjectTools.h"
#include "EdGraphUtilities.h"
#include "SWorldGridItem.h"

#define LOCTEXT_NAMESPACE "WorldBrowser"

FGridItemThumbnail::FGridItemThumbnail(FSlateTextureRenderTarget2DResource* InThumbnailRenderTarget, 
									   TSharedPtr<FLevelModel> InItemModel)
	: LevelModel(InItemModel)
	, ThumbnailTexture(NULL)
	, ThumbnailRenderTarget(InThumbnailRenderTarget)
{
	FIntPoint RTSize = ThumbnailRenderTarget->GetSizeXY();
	ThumbnailTexture = new FSlateTexture2DRHIRef(RTSize.X, RTSize.Y, PF_B8G8R8A8, NULL, TexCreate_Dynamic);
	BeginInitResource(ThumbnailTexture);
}

FGridItemThumbnail::~FGridItemThumbnail()
{
	BeginReleaseResource(ThumbnailTexture);
	
	// Wait for all resources to be released
	FlushRenderingCommands();
	delete ThumbnailTexture;
	ThumbnailTexture = NULL;
}

FIntPoint FGridItemThumbnail::GetSize() const
{
	return ThumbnailRenderTarget->GetSizeXY();
}

FSlateShaderResource* FGridItemThumbnail::GetViewportRenderTargetTexture() const
{
	return ThumbnailTexture;
}

bool FGridItemThumbnail::RequiresVsync() const
{
	return false;
}

void FGridItemThumbnail::UpdateTextureData(FObjectThumbnail* ObjectThumbnail)
{
	check(ThumbnailTexture)
	
	if (ObjectThumbnail &&
		ObjectThumbnail->GetImageWidth() > 0 && 
		ObjectThumbnail->GetImageHeight() > 0 && 
		ObjectThumbnail->GetUncompressedImageData().Num() > 0)
	{
		// Make bulk data for updating the texture memory later
		FSlateTextureDataPtr ThumbnailBulkData = MakeShareable(new FSlateTextureData(
			ObjectThumbnail->GetImageWidth(), 
			ObjectThumbnail->GetImageHeight(), 
			GPixelFormats[PF_B8G8R8A8].BlockBytes, 
			ObjectThumbnail->AccessImageData())
			);
					
		// Update the texture RHI
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			UpdateGridItemThumbnailResourse,
			FSlateTexture2DRHIRef*, Texture, ThumbnailTexture,
			FSlateTextureDataPtr, BulkData, ThumbnailBulkData,
		{
			Texture->SetTextureData(BulkData);
			Texture->UpdateRHI();
		});
	}
}

void FGridItemThumbnail::UpdateThumbnail()
{
	// No need images for persistent and always loaded levels
	if (LevelModel->IsAlwaysLoaded() || 
		LevelModel->IsPersistent())
	{
		return;
	}
	
	// Load image from a package header
	if (!LevelModel->IsVisible())
	{
		TSet<FName> ObjectFullNames;
		ObjectFullNames.Add(LevelModel->GetAssetName());
		FThumbnailMap ThumbnailMap;
			
		if (ThumbnailTools::ConditionallyLoadThumbnailsFromPackage(LevelModel->GetPackageFileName(), ObjectFullNames, ThumbnailMap))
		{
			UpdateTextureData(ThumbnailMap.Find(LevelModel->GetAssetName()));
		}
	}
	// Render image from a loaded visble level
	else
	{
		ULevel* TargetLevel = LevelModel->GetLevelObject();
		if (TargetLevel)
		{
			FIntPoint RTSize = ThumbnailRenderTarget->GetSizeXY();
			
			// Set persistent world package as transient to avoid package dirtying during thumbnail rendering
			FUnmodifiableObject ImmuneWorld(TargetLevel->OwningWorld);
			
			FObjectThumbnail NewThumbnail;
			// Generate the thumbnail
			ThumbnailTools::RenderThumbnail(
				TargetLevel,
				RTSize.X,
				RTSize.Y,
				ThumbnailTools::EThumbnailTextureFlushMode::NeverFlush,
				ThumbnailRenderTarget,
				&NewThumbnail
				);

			UPackage* MyOutermostPackage = CastChecked<UPackage>(TargetLevel->GetOutermost());
			ThumbnailTools::CacheThumbnail(LevelModel->GetAssetName().ToString(), &NewThumbnail, MyOutermostPackage);
			UpdateTextureData(&NewThumbnail);
		}
	}
}

//----------------------------------------------------------------
//
//
//----------------------------------------------------------------
SWorldGridItem::~SWorldGridItem()
{
	LevelModel->ChangedEvent.RemoveAll(this);
}

void SWorldGridItem::Construct(const FArguments& InArgs)
{
	WorldModel = InArgs._InWorldModel;
	LevelModel = InArgs._InItemModel;

	MapPackageLoaded = FEditorStyle::GetBrush("WorldBrowser.LevelLoaded");
	MapPackageUnloaded = FEditorStyle::GetBrush("WorldBrowser.LevelUnloaded");
	MapPackagePending = FEditorStyle::GetBrush("WorldBrowser.LevelPending");

	ProgressBarImage = FEditorStyle::GetBrush(TEXT("ProgressBar.Marquee"));
	
	bNeedRefresh = true;
	bIsDragging = false;
	bAffectedByMarquee = false;

	Thumbnail = MakeShareable(new FGridItemThumbnail(InArgs._ThumbnailRenderTarget, LevelModel));
	ThumbnailViewport = SNew(SViewport)
							.EnableGammaCorrection(false);
							
	ThumbnailViewport->SetViewportInterface(Thumbnail.ToSharedRef());
	// This will grey out tile in case level is not loaded or locked
	ThumbnailViewport->SetEnabled(TAttribute<bool>(this, &SWorldGridItem::IsItemEnabled));

	LevelModel->ChangedEvent.AddSP(this, &SWorldGridItem::RequestRefresh);
			
	ChildSlot
	[
		ThumbnailViewport.ToSharedRef()
	];

	SetToolTip(CreateToolTipWidget());

	CurveSequence = FCurveSequence(0.0f, 0.5f);
	CurveSequence.Play();
}

void SWorldGridItem::RequestRefresh()
{
	bNeedRefresh = true;
}

UObject* SWorldGridItem::GetObjectBeingDisplayed() const
{
	return LevelModel->GetNodeObject();
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
TSharedRef<SToolTip> SWorldGridItem::CreateToolTipWidget()
{
	TSharedPtr<SToolTip>		TooltipWidget;
	TSharedPtr<SVerticalBox>	TooltipSectionsWidget;
	
	SAssignNew(TooltipWidget, SToolTip)
	.TextMargin(1)
	.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ToolTipBorder"))
	[
		SNew(SBorder)
		.Padding(6)
		.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.NonContentBorder"))
		[
			SAssignNew(TooltipSectionsWidget, SVerticalBox)

			// Level name section
			+SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0,0,0,4)
			[
				SNew(SBorder)
				.Padding(6)
				.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
				[
					SNew(SVerticalBox)
					
					+SVerticalBox::Slot()
					.VAlign(VAlign_Center)
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						// Level status
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.AutoWidth()
						[
							SNew(SImage)
							.Image(this, &SWorldGridItem::GetLevelStatusBrush)
						]

						// Level name
						+SHorizontalBox::Slot()
						.Padding(6,0,0,0)
						.HAlign(HAlign_Left)
						.AutoWidth()
						[
							SNew(STextBlock)
							.Text(this, &SWorldGridItem::GetLevelNameText) 
							.Font(FEditorStyle::GetFontStyle("ContentBrowser.TileViewTooltip.NameFont"))
						]
					]
				]
			]
		]
	];

	TArray< TPair<TAttribute<FText>, TAttribute<FText>> > CustomFields;
	LevelModel->GetGridItemTooltipFields(CustomFields);

	if (CustomFields.Num() > 0)
	{
		TSharedPtr<SUniformGridPanel> TooltipFieldsWidget;
		// Add section for custom fields
		TooltipSectionsWidget->AddSlot()
			.AutoHeight()
			.Padding(0,0,0,4)
		[
				SNew(SBorder)
				.Padding(6)
				.BorderImage(FEditorStyle::GetBrush("ContentBrowser.TileViewTooltip.ContentBorder"))
				[
					SAssignNew(TooltipFieldsWidget, SUniformGridPanel)
				]
		];

		// Add custom fields
		for (int32 FieldIdx = 0; FieldIdx < CustomFields.Num(); ++FieldIdx)
		{
			// Name:
			TooltipFieldsWidget->AddSlot(0, FieldIdx)
				.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(CustomFields[FieldIdx].Key) 
			];
		
			// Value:
			TooltipFieldsWidget->AddSlot(1, FieldIdx)
				.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(CustomFields[FieldIdx].Value) 
			];
		}
	}

	return TooltipWidget.ToSharedRef();
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FVector2D SWorldGridItem::GetPosition() const
{
	return LevelModel->GetLevelPosition2D();
}

const TSharedPtr<FLevelModel>& SWorldGridItem::GetLevelModel() const
{
	return LevelModel;
}

const FSlateBrush* SWorldGridItem::GetShadowBrush(bool bSelected) const
{
	return bSelected ? FEditorStyle::GetBrush(TEXT("Graph.CompactNode.ShadowSelected")) : FEditorStyle::GetBrush(TEXT("Graph.Node.Shadow"));
}

FOptionalSize SWorldGridItem::GetItemWidth() const
{
	return FOptionalSize(LevelModel->GetLevelSize2D().X);
}

FOptionalSize SWorldGridItem::GetItemHeight() const
{
	return FOptionalSize(LevelModel->GetLevelSize2D().Y);
}

FSlateRect SWorldGridItem::GetItemRect() const
{
	FVector2D LevelSize = LevelModel->GetLevelSize2D();
	FVector2D LevelPos = GetPosition();
	return FSlateRect(LevelPos, LevelPos + LevelSize);
}

TSharedPtr<SToolTip> SWorldGridItem::GetToolTip()
{
	// Hide tooltip in case item is being dragged now
	if (LevelModel->GetLevelTranslationDelta().Size() > KINDA_SMALL_NUMBER)
	{
		return NULL;
	}
	
	return SNodePanel::SNode::GetToolTip();
}

FVector2D SWorldGridItem::GetDesiredSizeForMarquee() const
{
	// we don't want to select items in non visible layers
	if (!WorldModel->PassesAllFilters(LevelModel))
	{
		return FVector2D::ZeroVector;
	}

	return SNodePanel::SNode::GetDesiredSizeForMarquee();
}

FVector2D SWorldGridItem::ComputeDesiredSize() const
{
	return LevelModel->GetLevelSize2D();
}

int32 SWorldGridItem::OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& ClippingRect, 
								FSlateWindowElementList& OutDrawElements, int32 LayerId, 
								const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const bool bIsVisible = FSlateRect::DoRectanglesIntersect(AllottedGeometry.GetClippingRect(), ClippingRect);
	
	if (bIsVisible)
	{
		// Redraw thumbnail image if requested
		if (bNeedRefresh)
		{
			Thumbnail->UpdateThumbnail();
			bNeedRefresh = false;
		}
		
		LayerId = SNodePanel::SNode::OnPaint(AllottedGeometry, ClippingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
		
		const bool bSelected = (IsItemSelected() || bAffectedByMarquee);

		// Draw the node's selection.
		if (bSelected)
		{
			FPaintGeometry SelectionGeometry = AllottedGeometry.ToInflatedPaintGeometry(FVector2D(4,4)/AllottedGeometry.Scale);
			SelectionGeometry.DrawScale = 0.5;
				
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 1,
				SelectionGeometry,
				GetShadowBrush(bSelected),
				ClippingRect
				);
		}

		// Highlight potentially visible streaming levels from current preview point
		//const auto& PreviewStreamingLevels = WorldModel->GetPreviewStreamingLevels();
		//if (PreviewStreamingLevels.Find(LevelModel->TileDetails->PackageName) != INDEX_NONE)
		//{
		//	FPaintGeometry SelectionGeometry = AllottedGeometry.ToInflatedPaintGeometry(FVector2D(4,4)/AllottedGeometry.Scale);
		//	SelectionGeometry.DrawScale = 0.5;

		//	FSlateDrawElement::MakeBox(
		//		OutDrawElements,
		//		LayerId + 1,
		//		SelectionGeometry,
		//		GetShadowBrush(true),
		//		ClippingRect,
		//		ESlateDrawEffect::None,
		//		FLinearColor::Green
		//		);
		//}

		// Draw progress bar if level is currently loading
		if (LevelModel->IsLoading())
		{
			const float ProgressBarAnimOffset = ProgressBarImage->ImageSize.X * CurveSequence.GetLerpLooping() / AllottedGeometry.Scale;
			const float ProgressBarImageSize = ProgressBarImage->ImageSize.X / AllottedGeometry.Scale;
			const float ProgressBarImageHeight = ProgressBarImage->ImageSize.Y / AllottedGeometry.Scale;
			const FSlateRect ProgressBarClippingRect = AllottedGeometry.ToPaintGeometry().ToSlateRect().IntersectionWith(ClippingRect);
		
			FPaintGeometry LoadingBarPaintGeometry = AllottedGeometry.ToPaintGeometry(
					FVector2D(ProgressBarAnimOffset - ProgressBarImageSize, 0 ),
					FVector2D(AllottedGeometry.Size.X + ProgressBarImageSize, FMath::Min(AllottedGeometry.Size.Y, ProgressBarImageHeight))
			);
			LoadingBarPaintGeometry.DrawScale = 1.f;
								
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				LayerId + 1,
				LoadingBarPaintGeometry,
				ProgressBarImage,
				ProgressBarClippingRect,
				ESlateDrawEffect::None,
				FLinearColor(1.0f, 1.0f, 1.0f, 0.5f)
			);
		}
	}
	
	return LayerId;
}

FReply SWorldGridItem::OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent)
{
	if (OnHitTest(InMyGeometry, InMouseEvent.GetScreenSpacePosition()))
	{
		LevelModel->MakeLevelCurrent();
		return FReply::Handled();
	}
	
	return FReply::Unhandled();
}

bool SWorldGridItem::OnHitTest(const FGeometry& MyGeometry, FVector2D InAbsoluteCursorPosition)
{
	FVector2D CursorOffset = (InAbsoluteCursorPosition - MyGeometry.AbsolutePosition)/MyGeometry.Scale;
	FVector2D LevelPos = LevelModel->GetLevelPosition2D();
	FVector2D CursorWorldPos = LevelPos + CursorOffset;
	return LevelModel->HitTest2D(CursorWorldPos);
}

FString SWorldGridItem::GetLevelNameText() const
{
	return LevelModel->GetDisplayName();
}

const FSlateBrush* SWorldGridItem::GetLevelStatusBrush() const
{
	if (LevelModel->IsLoaded())
	{
		return MapPackageLoaded;
	}

	return LevelModel->IsLoading() ? 
		MapPackagePending : 
		MapPackageUnloaded;
}

bool SWorldGridItem::IsItemEditable() const
{
	return LevelModel->IsEditable();
}

bool SWorldGridItem::IsItemSelected() const
{
	return LevelModel->GetLevelSelectionFlag();
}

bool SWorldGridItem::IsItemEnabled() const
{
	if (WorldModel->IsSimulating())
	{
		return LevelModel->IsVisible();
	}
	else
	{
		return LevelModel->IsEditable();
	}
}

#undef LOCTEXT_NAMESPACE
