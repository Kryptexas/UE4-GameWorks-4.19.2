// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/AssetRegistry/Public/AssetData.h"
#include "AssetThumbnail.h"
#include "ClassIconFinder.h"
#include "DecoratedDragDropOp.h"

class FAssetDragDropOp : public FDecoratedDragDropOp
{
public:
	static FString GetTypeId() {static FString Type = TEXT("FAssetDragDropOp"); return Type;}

	/** Data for the asset this item represents */
	TArray<FAssetData> AssetData;

	/** Pool for maintaining and rendering thumbnails */
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;

	/** Handle to the thumbnail resource */
	TSharedPtr<FAssetThumbnail> AssetThumbnail;

	/** The actor factory to use if converting this asset to an actor */
	TWeakObjectPtr< UActorFactory > ActorFactory;

	static TSharedRef<FAssetDragDropOp> New(const FAssetData& InAssetData, UActorFactory* ActorFactory = NULL)
	{
		TArray<FAssetData> AssetDataArray;
		AssetDataArray.Add(InAssetData);
		return New(AssetDataArray, ActorFactory);
	}

	static TSharedRef<FAssetDragDropOp> New(const TArray<FAssetData>& InAssetData, UActorFactory* ActorFactory = NULL)
	{
		TSharedRef<FAssetDragDropOp> Operation = MakeShareable(new FAssetDragDropOp);
		FSlateApplication::GetDragDropReflector().RegisterOperation<FAssetDragDropOp>(Operation);

		Operation->MouseCursor = EMouseCursor::GrabHandClosed;

		Operation->ThumbnailSize = 64;

		Operation->AssetData = InAssetData;
		Operation->ActorFactory = ActorFactory;

		Operation->Init();

		Operation->Construct();
		return Operation;
	}

private:
	int32 ThumbnailSize;
	
public:
	virtual ~FAssetDragDropOp()
	{
		if ( ThumbnailPool.IsValid() )
		{
			// Release all rendering resources being held onto
			ThumbnailPool->ReleaseResources();
		}
	}
	
	virtual TSharedPtr<SWidget> GetDefaultDecorator() const OVERRIDE
	{
		TSharedPtr<SWidget> ThumbnailWidget;

		if ( AssetThumbnail.IsValid() )
		{
			ThumbnailWidget = AssetThumbnail->MakeThumbnailWidget();
		}
		else
		{
			ThumbnailWidget = SNew(SImage) .Image( FEditorStyle::GetDefaultBrush() );
		}
		
		const FSlateBrush* ActorTypeBrush = FEditorStyle::GetDefaultBrush();
		if ( ActorFactory.IsValid() && AssetData.Num() > 0 )
		{
			AActor* DefaultActor = ActorFactory->GetDefaultActor( AssetData[0] );
			ActorTypeBrush = FClassIconFinder::FindIconForActor( DefaultActor );
		}

		return 
			SNew(SBorder)
			.BorderImage(FEditorStyle::GetBrush("ContentBrowser.AssetDragDropTooltipBackground"))
			.Content()
			[
				SNew(SHorizontalBox)

				// Left slot is asset thumbnail
				+SHorizontalBox::Slot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				[
					SNew(SBox) 
					.WidthOverride(ThumbnailSize) 
					.HeightOverride(ThumbnailSize)
					.Content()
					[
						SNew(SOverlay)

						+SOverlay::Slot()
						[
							ThumbnailWidget.ToSharedRef()
						]

						+SOverlay::Slot()
						.HAlign(HAlign_Center)
						.VAlign(VAlign_Top)
						.Padding(FMargin(0, 4, 0, 0))
						[
							SNew(SBorder)
							.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
							.Visibility(AssetData.Num() > 1 ? EVisibility::Visible : EVisibility::Collapsed)
							.Content()
							[
								SNew(STextBlock) .Text(FText::AsNumber(AssetData.Num()))
							]
						]

						+SOverlay::Slot()
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Bottom)
						.Padding(FMargin(4, 4))
						[
							SNew(SImage)
							.Image( ActorTypeBrush )
							.Visibility( (ActorTypeBrush != FEditorStyle::GetDefaultBrush()) ? EVisibility::Visible : EVisibility::Collapsed)
						]
					]
				]

				// Right slot is for optional tooltip
				+SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Top)
				[
					SNew(SBox)
					.Visibility(this, &FAssetDragDropOp::GetTooltipVisibility)
					.Content()
					[
						SNew(SHorizontalBox)

						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(3.0f)
						[
							SNew(SImage) 
							.Image(this, &FAssetDragDropOp::GetIcon)
						]

						+SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0,0,3,0)
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock) 
							.Text(this, &FAssetDragDropOp::GetHoverText)
						]
					]
				]

			];
	}

	void Init()
	{
		if ( AssetData.Num() > 0 && ThumbnailSize > 0 )
		{
			// Create a thumbnail pool to hold the single thumbnail rendered
			ThumbnailPool = MakeShareable( new FAssetThumbnailPool(1, /*InAreRealTileThumbnailsAllowed=*/false) );

			// Create the thumbnail handle
			AssetThumbnail = MakeShareable( new FAssetThumbnail( AssetData[0], ThumbnailSize, ThumbnailSize, ThumbnailPool ) );

			// Request the texture then tick the pool once to render the thumbnail
			AssetThumbnail->GetViewportRenderTargetTexture();
			ThumbnailPool->Tick(0);
		}
	}

	EVisibility GetTooltipVisibility() const
	{
		if(CurrentIconBrush != NULL && CurrentHoverText.Len() > 0)
		{
			return EVisibility::Visible;
		}
		else
		{
			return EVisibility::Collapsed;
		}
	}
};
