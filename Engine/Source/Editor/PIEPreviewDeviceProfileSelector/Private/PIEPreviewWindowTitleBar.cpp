// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PIEPreviewWindowTitleBar.h"

#if WITH_EDITOR

void SPIEPreviewWindowTitleBar::MakeTitleBarContentWidgets(TSharedPtr< SWidget >& OutLeftContent, TSharedPtr< SWidget >& OutRightContent)
{
	TSharedPtr< SWidget > OutRightContentBaseWindow;
	SWindowTitleBar::MakeTitleBarContentWidgets(OutLeftContent, OutRightContentBaseWindow);
	
	ScreenRotationButton = SNew(SButton)
		.IsFocusable(false)
		.IsEnabled(true)
		.ContentPadding(0)
		.OnClicked(this, &SPIEPreviewWindowTitleBar::ScreenRotationButton_OnClicked)
		.Cursor(EMouseCursor::Default)
		.ButtonStyle(FCoreStyle::Get(), "NoBorder")
		[
			SNew(SImage)
			.Image(this, &SPIEPreviewWindowTitleBar::GetScreenRotationButtonImage)
			.ColorAndOpacity(this, &SPIEPreviewWindowTitleBar::GetWindowTitleContentColor)
		]
	;

	QuarterMobileContentScaleFactorButton = SNew(SButton)
		.IsFocusable(false)
		.IsEnabled(true)
		.ContentPadding(0)
		.OnClicked(this, &SPIEPreviewWindowTitleBar::QuarterMobileContentScaleFactorButton_OnClicked)
		.Cursor(EMouseCursor::Default)
		.ButtonStyle(FCoreStyle::Get(), "NoBorder")
		[
			SNew(SImage)
			.Image(this, &SPIEPreviewWindowTitleBar::GetQuarterMobileContentScaleFactorImage)
			.ColorAndOpacity(this, &SPIEPreviewWindowTitleBar::GetWindowTitleContentColor)
		]
	;

	HalfMobileContentScaleFactorButton = SNew(SButton)
		.IsFocusable(false)
		.IsEnabled(true)
		.ContentPadding(0.0f)
		.OnClicked(this, &SPIEPreviewWindowTitleBar::HalfMobileContentScaleFactorButton_OnClicked)
		.Cursor(EMouseCursor::Default)
		.ButtonStyle(FCoreStyle::Get(), "NoBorder")
		[
			SNew(SImage)
			.Image(this, &SPIEPreviewWindowTitleBar::GetHalfMobileContentScaleFactorImage)
			.ColorAndOpacity(this, &SPIEPreviewWindowTitleBar::GetWindowTitleContentColor)
		]
	;

	FullMobileContentScaleFactorButton = SNew(SButton)
		.IsFocusable(false)
		.IsEnabled(true)
		.ContentPadding(0.0f)
		.OnClicked(this, &SPIEPreviewWindowTitleBar::FullMobileContentScaleFactorButton_OnClicked)
		.Cursor(EMouseCursor::Default)
		.ButtonStyle(FCoreStyle::Get(), "NoBorder")
		[
			SNew(SImage)
			.Image(this, &SPIEPreviewWindowTitleBar::GetFullMobileContentScaleFactorImage)
			.ColorAndOpacity(this, &SPIEPreviewWindowTitleBar::GetWindowTitleContentColor)
		];



	TSharedRef< SHorizontalBox > WindowTitleBarButtons =
	SNew(SHorizontalBox)
	.Visibility(EVisibility::SelfHitTestInvisible);

	WindowTitleBarButtons->AddSlot()
	.AutoWidth()
	[
	ScreenRotationButton.ToSharedRef()
	];

	WindowTitleBarButtons->AddSlot()
	.AutoWidth()
	[
	QuarterMobileContentScaleFactorButton.ToSharedRef()
	];
	WindowTitleBarButtons->AddSlot()
	.AutoWidth()
	[
	HalfMobileContentScaleFactorButton.ToSharedRef()
	];
	WindowTitleBarButtons->AddSlot()
	.AutoWidth()
	[
	FullMobileContentScaleFactorButton.ToSharedRef()
	];
	if (OutRightContentBaseWindow.IsValid())
	{
		WindowTitleBarButtons->AddSlot()
			.AutoWidth()
			[
				OutRightContentBaseWindow.ToSharedRef()
			];
	}
	

	OutRightContent = SNew(SBox)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.Padding(FMargin(2.0f, 0.0f, 0.0f, 0.0f))
		[
			WindowTitleBarButtons
		];
}
	
void SPIEPreviewWindowTitleBar::ApplyWindowRotation(TSharedPtr<SWindow> OwnerWindow)
{
	FVector2D Size = OwnerWindow->GetInitialDesiredSizeInScreen() * ScaleFactor;
	IsPortrait = !IsPortrait;

	if (IsPortrait)
		Swap(Size.X, Size.Y);

	int x, y, w, h;
	OwnerWindow->GetNativeWindow()->GetFullScreenInfo(x, y, w, h);
	OwnerWindow->GetNativeWindow()->ReshapeWindow((w - Size.X) * 0.5f, (h - Size.Y) * 0.5f, Size.X, Size.Y);
}

void SPIEPreviewWindowTitleBar::ApplyWindowScaleFactor(TSharedPtr<SWindow> OwnerWindow)
{
	FVector2D Size = OwnerWindow->GetInitialDesiredSizeInScreen() * ScaleFactor;

	if (IsPortrait)
		Swap(Size.X, Size.Y);

	int x, y, w, h;
	OwnerWindow->GetNativeWindow()->GetFullScreenInfo(x, y, w, h);
	OwnerWindow->GetNativeWindow()->ReshapeWindow((w - Size.X) * 0.5f, (h - Size.Y) * 0.5f, Size.X, Size.Y);
}

const FSlateBrush* SPIEPreviewWindowTitleBar::GetScreenRotationButtonImage() const
{
	TSharedPtr<SWindow> OwnerWindow = OwnerWindowPtr.Pin();
	
	if (!OwnerWindow.IsValid())
	{
		return nullptr;
	}

	if (ScreenRotationButton->IsPressed())
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").ScreenRotationButtonStyle.Pressed;
	}
	else if (ScreenRotationButton->IsHovered())
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").ScreenRotationButtonStyle.Hovered;
	}
	else
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").ScreenRotationButtonStyle.Normal;
	}

}

const FSlateBrush* SPIEPreviewWindowTitleBar::GetQuarterMobileContentScaleFactorImage() const
{
	TSharedPtr<SWindow> OwnerWindow = OwnerWindowPtr.Pin();
	
	if (!OwnerWindow.IsValid())
	{
		return nullptr;
	}

	if (QuarterMobileContentScaleFactorButton->IsPressed())
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").QuarterMobileContentScaleFactorButtonStyle.Pressed;
	}
	else if (QuarterMobileContentScaleFactorButton->IsHovered())
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").QuarterMobileContentScaleFactorButtonStyle.Hovered;
	}
	else
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").QuarterMobileContentScaleFactorButtonStyle.Normal;
	}

}

const FSlateBrush* SPIEPreviewWindowTitleBar::GetHalfMobileContentScaleFactorImage() const
{
	TSharedPtr<SWindow> OwnerWindow = OwnerWindowPtr.Pin();
	
	if (!OwnerWindow.IsValid())
	{
		return nullptr;
	}

	if (HalfMobileContentScaleFactorButton->IsPressed())
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").HalfMobileContentScaleFactorButtonStyle.Pressed;
	}
	else if (HalfMobileContentScaleFactorButton->IsHovered())
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").HalfMobileContentScaleFactorButtonStyle.Hovered;
	}
	else
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").HalfMobileContentScaleFactorButtonStyle.Normal;
	}

}

const FSlateBrush* SPIEPreviewWindowTitleBar::GetFullMobileContentScaleFactorImage() const
{
	TSharedPtr<SWindow> OwnerWindow = OwnerWindowPtr.Pin();
	
	if (!OwnerWindow.IsValid())
	{
		return nullptr;
	}

	if (FullMobileContentScaleFactorButton->IsPressed())
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").FullMobileContentScaleFactorButtonStyle.Pressed;
	}
	else if (FullMobileContentScaleFactorButton->IsHovered())
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").FullMobileContentScaleFactorButtonStyle.Hovered;
	}
	else
	{
		return &FPIEPreviewWindowCoreStyle::Get().GetWidgetStyle<FPIEPreviewWindowStyle>("PIEWindow").FullMobileContentScaleFactorButtonStyle.Normal;
	}

}

FReply SPIEPreviewWindowTitleBar::ScreenRotationButton_OnClicked()
{
	TSharedPtr<SWindow> OwnerWindow = OwnerWindowPtr.Pin();

	if (OwnerWindow.IsValid())
	{
		ApplyWindowRotation(OwnerWindow);
	}

	return FReply::Handled();
}

FReply SPIEPreviewWindowTitleBar::QuarterMobileContentScaleFactorButton_OnClicked()
{
	TSharedPtr<SWindow> OwnerWindow = OwnerWindowPtr.Pin();

	if (OwnerWindow.IsValid())
	{
		ScaleFactor = 0.25f;
		ApplyWindowScaleFactor(OwnerWindow);
	}

	return FReply::Handled();
}

FReply SPIEPreviewWindowTitleBar::HalfMobileContentScaleFactorButton_OnClicked()
{
	TSharedPtr<SWindow> OwnerWindow = OwnerWindowPtr.Pin();

	if (OwnerWindow.IsValid())
	{
		ScaleFactor = 0.5f;
		ApplyWindowScaleFactor(OwnerWindow);
	}

	return FReply::Handled();
}

FReply SPIEPreviewWindowTitleBar::FullMobileContentScaleFactorButton_OnClicked()
{
	TSharedPtr<SWindow> OwnerWindow = OwnerWindowPtr.Pin();

	if (OwnerWindow.IsValid())
	{
		ScaleFactor = 1.0f;
		ApplyWindowScaleFactor(OwnerWindow);
	}

	return FReply::Handled();
}

#endif