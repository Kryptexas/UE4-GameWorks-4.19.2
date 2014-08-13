// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EditorTutorial.h"

class STutorialContent;

/** Delegate used when drawing/arranging widgets */
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPaintNamedWidget, TSharedRef<SWidget> /*InWidget*/, const FGeometry& /*InGeometry*/);

/** Delegate used to inform widgets of the current window size, so they can auto-adjust layout */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnCacheWindowSize, const FVector2D& /*InWindowSize*/);

/**
 * The widget which displays multiple 'floating' pieces of content overlaid onto the editor
 */
class STutorialOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( STutorialOverlay ) 
	{
		_Visibility = EVisibility::SelfHitTestInvisible;
	}

	/** The window this content is displayed over */
	SLATE_ARGUMENT(TWeakPtr<SWindow>, ParentWindow)

	/** Whether this a standalone widget (with its own close button) or part of a group of other widgets, paired with tutorial navigation */
	SLATE_ARGUMENT(bool, IsStandalone)

	/** Delegate fired when the close button is clicked */
	SLATE_ARGUMENT(FSimpleDelegate, OnClosed)

	/** Whether we can show full window content in this overlay (i.e. in the same window as the navigation controls) */
	SLATE_ARGUMENT(bool, AllowNonWidgetContent)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, FTutorialStage* const InStage);

	/** SWidget implementation */
	virtual int32 OnPaint( const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled ) const override;

private:
	/** Recursive function used to re-generate widget geometry and forward the geometry of named widgets onto their respective content */
	int32 TraverseWidgets(TSharedRef<SWidget> InWidget, const FGeometry& InGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId) const;

	/* Opens the browser that the given widget requires if it is not already. */
	void OpenBrowserForWidgetAnchor(const FTutorialWidgetContent &WidgetContent);

	/* Creates a map of broswers required by the various widget types. */
	void AddTabInfo();

private:
	/** Reference to the canvas we use to position our content widgets */
	TSharedPtr<SCanvas> OverlayCanvas;

	/** The window this content is displayed over */
	TWeakPtr<SWindow> ParentWindow;

	/** Whether this a standalone widget (with its own close button) or part of a group of other widgets, paired with tutorial navigation */
	bool bIsStandalone;

	/** Delegate fired when a cos ebutton is clicked in tutorial content */
	FSimpleDelegate OnClosed;

	/** Delegate used when drawing/arranging widgets */
	FOnPaintNamedWidget OnPaintNamedWidget;

	/** Delegate used to reset drawing of named widgets */
	FSimpleMulticastDelegate OnResetNamedWidget;

	/** Delegate used to inform widgets of the current window size, so they can auto-adjust layout */
	FOnCacheWindowSize OnCacheWindowSize;

	/* A map of which tab requires invoking based on the tag name of the asset */
	TMap< FString, FString >	BrowserTabMap;
};