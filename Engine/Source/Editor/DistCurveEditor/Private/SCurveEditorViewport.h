// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SDistributionCurveEditor;
class FCurveEditorViewportClient;


/*-----------------------------------------------------------------------------
   SCurveEditorViewport
-----------------------------------------------------------------------------*/

class SCurveEditorViewport : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCurveEditorViewport)
		: _CurveEdOptions(IDistributionCurveEditor::FCurveEdOptions())
		{}

		SLATE_ARGUMENT(TWeakPtr<SDistributionCurveEditor>, CurveEditor)
		SLATE_ARGUMENT(IDistributionCurveEditor::FCurveEdOptions, CurveEdOptions)
	SLATE_END_ARGS()

	/** Destructor */
	~SCurveEditorViewport();

	/** SCompoundWidget interface */
	void Construct(const FArguments& InArgs);
	
	/** Refreshes the viewport */
	void RefreshViewport();

	/** Returns true if the viewport is visible */
	bool IsVisible() const;

	/** Scrolls the scrollbar... expected range is from 0.0 to 1.0 */
	void SetVerticalScrollBarPosition(float Position);

	/** Accessors */
	TSharedPtr<FSceneViewport> GetViewport() const;
	TSharedPtr<FCurveEditorViewportClient> GetViewportClient() const;
	TSharedPtr<SViewport> GetViewportWidget() const;
	TSharedPtr<SScrollBar> GetVerticalScrollBar() const;

protected:
	/** Returns the visibility of the viewport scrollbars */
	EVisibility GetViewportVerticalScrollBarVisibility() const;

	/** Called when the viewport scrollbars are scrolled */
	void OnViewportVerticalScrollBarScrolled(float InScrollOffsetFraction);

private:
	/** Pointer back to the Texture editor tool that owns us */
	TWeakPtr<SDistributionCurveEditor> CurveEditorPtr;
	
	/** Level viewport client */
	TSharedPtr<FCurveEditorViewportClient> ViewportClient;

	/** Slate viewport for rendering and I/O */
	TSharedPtr<FSceneViewport> Viewport;

	/** Viewport widget*/
	TSharedPtr<SViewport> ViewportWidget;

	/** Vertical scrollbar */
	TSharedPtr<SScrollBar> ViewportVerticalScrollBar;
};
