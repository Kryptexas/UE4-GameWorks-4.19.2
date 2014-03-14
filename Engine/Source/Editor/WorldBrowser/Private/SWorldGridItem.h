// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateTextures.h"
#include "GraphEditor.h"
#include "SNodePanel.h"
#include "AssetThumbnail.h"

class FWorldViewModel;

class FGridItemThumbnail : public ISlateViewport, public TSharedFromThis<FGridItemThumbnail>
{
public:
	FGridItemThumbnail(FSlateTextureRenderTarget2DResource* InThumbnailRenderTarget, TSharedPtr<FLevelModel> InItemModel);
	~FGridItemThumbnail();

	/* ISlateViewport interface */
	virtual FIntPoint GetSize() const OVERRIDE;
	virtual class FSlateShaderResource* GetViewportRenderTargetTexture() const OVERRIDE;
	virtual bool RequiresVsync() const OVERRIDE;
	
	/** Request thumbnail redraw*/
	void UpdateThumbnail();

private:
	void UpdateTextureData(FObjectThumbnail* ObjectThumbnail);

private:
	TSharedPtr<FLevelModel>					LevelModel;
	/** Rendering resource for slate */
	FSlateTexture2DRHIRef*					ThumbnailTexture;
	/** Shared render target for slate */
	FSlateTextureRenderTarget2DResource*	ThumbnailRenderTarget;
};

//----------------------------------------------------------------
//
//
//----------------------------------------------------------------
class SWorldGridItem 
	: public SNodePanel::SNode
{
public:
	SLATE_BEGIN_ARGS(SWorldGridItem)
	{}
		/** The world data */
		SLATE_ARGUMENT(TSharedPtr<FLevelCollectionModel>, InWorldModel)
		/** Data for the asset this item represents */
		SLATE_ARGUMENT(TSharedPtr<FLevelModel>, InItemModel)
		//
		SLATE_ARGUMENT(FSlateTextureRenderTarget2DResource*, ThumbnailRenderTarget)
	SLATE_END_ARGS()

	~SWorldGridItem();

	void Construct(const FArguments& InArgs);
	
	// SNodePanel::SNode interface start
	virtual FVector2D GetDesiredSizeForMarquee() const OVERRIDE;
	virtual UObject* GetObjectBeingDisplayed() const OVERRIDE;
	virtual FVector2D GetPosition() const OVERRIDE;
	virtual const FSlateBrush* GetShadowBrush(bool bSelected) const OVERRIDE;
	// SNodePanel::SNode interface end
	
	/** @return Deferred item refresh */
	void RequestRefresh();

	/** @return LevelModel associated with this item */
	const TSharedPtr<FLevelModel>&	GetLevelModel() const;
	
	/** @return Item width in world units */
	FOptionalSize GetItemWidth() const;
	
	/** @return Item height in world units */
	FOptionalSize GetItemHeight() const;
	
	/** @return Rectangle in world units for this item as FSlateRect*/
	FSlateRect GetItemRect() const;
	
	/** @return Whether this item can be edited (loaded and not locked) */
	bool IsItemEditable() const;

	/** @return Whether this item is selected */
	bool IsItemSelected() const;
	
	/** @return Whether this item is enabled */
	bool IsItemEnabled() const;

private:
	// SWidget interface start
	virtual TSharedPtr<SToolTip> GetToolTip() OVERRIDE;
	virtual FVector2D ComputeDesiredSize() const OVERRIDE;
	virtual bool OnHitTest(const FGeometry& MyGeometry, FVector2D InAbsoluteCursorPosition) OVERRIDE;
	virtual int32 OnPaint(const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, 
					FSlateWindowElementList& OutDrawElements, int32 LayerId, 
					const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const OVERRIDE;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) OVERRIDE;
	// SWidget interface end

	TSharedRef<SToolTip> CreateToolTipWidget();
	
	//
	const FSlateBrush* GetLevelStatusBrush() const;
	FString GetLevelNameText() const;

public:
	bool							bAffectedByMarquee;

private:
	/** The world data */
	TSharedPtr<FLevelCollectionModel> WorldModel;

	/** The data for this item */
	TSharedPtr<FLevelModel>			LevelModel;

	mutable bool					bNeedRefresh;
	bool							bIsDragging;

	TSharedPtr<FGridItemThumbnail>	Thumbnail;
	/** The actual widget for the thumbnail. */
	TSharedPtr<SViewport>			ThumbnailViewport;

	const FSlateBrush*				MapPackageUnloaded;
	const FSlateBrush*				MapPackagePending;
	const FSlateBrush*				MapPackageLoaded;
	const FSlateBrush*				ProgressBarImage;

	/** Curve sequence to drive loading animation */
	FCurveSequence					CurveSequence;
};

