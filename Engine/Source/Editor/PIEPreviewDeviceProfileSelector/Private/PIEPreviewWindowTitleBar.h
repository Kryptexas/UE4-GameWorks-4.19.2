// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "SWindowTitleBar.h"
#include "PIEPreviewWindowStyle.h"
#include "PIEPreviewWindowCoreStyle.h"

/**
* Implements a window PIE title bar widget.
*/
class PIEPREVIEWDEVICEPROFILESELECTOR_API SPIEPreviewWindowTitleBar
	: public SWindowTitleBar
{

private:
	/**
	* Creates widgets for this window's title bar area.
	*/
	virtual void MakeTitleBarContentWidgets(TSharedPtr< SWidget >& OutLeftContent, TSharedPtr< SWidget >& OutRightContent) override;
	
	void ApplyWindowRotation(TSharedPtr<SWindow> OwnerWindow);

	void ApplyWindowScaleFactor(TSharedPtr<SWindow> OwnerWindow);

	const FSlateBrush* GetScreenRotationButtonImage() const;

	const FSlateBrush* GetQuarterMobileContentScaleFactorImage() const;
	
	const FSlateBrush* GetHalfMobileContentScaleFactorImage() const;

	const FSlateBrush* GetFullMobileContentScaleFactorImage() const;

	FReply ScreenRotationButton_OnClicked();


	FReply QuarterMobileContentScaleFactorButton_OnClicked();


	FReply HalfMobileContentScaleFactorButton_OnClicked();


	FReply FullMobileContentScaleFactorButton_OnClicked();

private:

	// Holds the screen rotation button.
	TSharedPtr<SButton> ScreenRotationButton;

	// Holds the 0.25X button.
	TSharedPtr<SButton> QuarterMobileContentScaleFactorButton;

	// Holds the 0.5X button.
	TSharedPtr<SButton> HalfMobileContentScaleFactorButton;

	// Holds the 1X button.
	TSharedPtr<SButton> FullMobileContentScaleFactorButton;

	bool IsPortrait;

	float ScaleFactor = 1.0f;
};
#endif