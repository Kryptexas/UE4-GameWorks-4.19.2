// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PIEPreviewWindow.h"

#if WITH_EDITOR
SPIEPreviewWindow::SPIEPreviewWindow() 
{
}

TSharedRef<SWidget> SPIEPreviewWindow::MakeWindowTitleBar(const TSharedRef<SWindow>& Window, const TSharedPtr<SWidget>& CenterContent, EHorizontalAlignment CenterContentAlignment)
{
	TSharedRef<SPIEPreviewWindowTitleBar> WindowTitleBar = SNew(SPIEPreviewWindowTitleBar, Window, CenterContent, CenterContentAlignment)
		.Visibility(EVisibility::SelfHitTestInvisible);
	return WindowTitleBar;
}

EHorizontalAlignment SPIEPreviewWindow::GetTitleAlignment()
{
	return EHorizontalAlignment::HAlign_Left;
}

#endif