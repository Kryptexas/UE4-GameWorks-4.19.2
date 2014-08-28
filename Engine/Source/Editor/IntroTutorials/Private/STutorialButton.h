// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class STutorialButton : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(STutorialButton) {}
	
	SLATE_ARGUMENT(FName, Context)

	SLATE_ARGUMENT(TWeakPtr<SWindow>, ContextWindow)

	SLATE_END_ARGS()

	/** Widget constructor */
	void Construct(const FArguments& Args);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void HandleMainFrameLoad(TSharedPtr<SWindow> InRootWindow, bool bIsNewProjectWindow);

	FReply HandleButtonClicked();

	EVisibility GetVisibility() const;

	void DismissAlert();

private:
	/** Flag to defer tutorial open until the first Tick() */
	bool bDeferTutorialOpen;

	/** Whether we have a tutorial for this context */
	bool bTutorialAvailable;

	/** Whether we have completed the tutorial for this content */
	bool bTutorialCompleted;

	/** Whether we have dismissed the tutorial for this content */
	bool bTutorialDismissed;

	/** Context that this widget was created for (i.e. what part of the editor) */
	FName Context;

	/** Window that the tutorial should be launched in */
	TWeakPtr<SWindow> ContextWindow;

	/** Animation curve for displaying pulse */
	FCurveSequence PulseAnimation;
};