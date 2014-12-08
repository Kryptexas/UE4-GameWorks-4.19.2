// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserPrivatePCH.h"
#include "SWebBrowser.h"
#include "SThrobber.h"
#if WITH_EDITOR || IS_PROGRAM
#include "WebBrowserModule.h"
#include "IWebBrowserSingleton.h"
#include "IWebBrowserWindow.h"
#include "WebBrowserViewport.h"
#endif

#define LOCTEXT_NAMESPACE "WebBrowser"

SWebBrowser::SWebBrowser()
{
}

void SWebBrowser::Construct(const FArguments& InArgs)
{
	void* OSWindowHandle = nullptr;
	if (InArgs._ParentWindow.IsValid())
	{
		TSharedPtr<FGenericWindow> NativeWindow = InArgs._ParentWindow->GetNativeWindow();
		OSWindowHandle = NativeWindow->GetOSWindowHandle();
	}

#if WITH_EDITOR || IS_PROGRAM
	BrowserWindow = IWebBrowserModule::Get().GetSingleton()->CreateBrowserWindow(OSWindowHandle,
	                                                                             InArgs._InitialURL,
	                                                                             InArgs._ViewportSize.Get().X,
	                                                                             InArgs._ViewportSize.Get().Y,
	                                                                             InArgs._SupportsTransparency);
#endif

	TSharedPtr<SViewport> ViewportWidget;

	ChildSlot
	[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0, 5)
		[
			SNew(SHorizontalBox)
			.Visibility(InArgs._ShowControls ? EVisibility::Visible : EVisibility::Collapsed)
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("Back","Back"))
				.IsEnabled(this, &SWebBrowser::CanGoBack)
				.OnClicked(this, &SWebBrowser::OnBackClicked)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(LOCTEXT("Forward", "Forward"))
				.IsEnabled(this, &SWebBrowser::CanGoForward)
				.OnClicked(this, &SWebBrowser::OnForwardClicked)
			]
			+SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.Text(this, &SWebBrowser::GetReloadButtonText)
				.OnClicked(this, &SWebBrowser::OnReloadClicked)
			]
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Right)
			.Padding(5)
			[
				SNew(STextBlock)
				.Text(this, &SWebBrowser::GetTitleText)
				.Justification(ETextJustify::Right)
			]
		]
		+SVerticalBox::Slot()
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SAssignNew(ViewportWidget, SViewport)
				.ViewportSize(InArgs._ViewportSize)
				.EnableGammaCorrection(false)
				.EnableBlending(InArgs._SupportsTransparency)
				.IgnoreTextureAlpha(!InArgs._SupportsTransparency)
				.Visibility(this, &SWebBrowser::GetViewportVisibility)
			]
			+SOverlay::Slot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SCircularThrobber)
				.Radius(10.0f)
				.ToolTipText(LOCTEXT("LoadingThrobberToolTip", "Loading page..."))
				.Visibility(this, &SWebBrowser::GetLoadingThrobberVisibility)
			]
		]
	];

#if WITH_EDITOR || IS_PROGRAM
	if (BrowserWindow.IsValid())
	{
		BrowserViewport = MakeShareable(new FWebBrowserViewport(BrowserWindow, ViewportWidget));
		ViewportWidget->SetViewportInterface(BrowserViewport.ToSharedRef());
	}
#endif
}

FText SWebBrowser::GetTitleText() const
{
#if WITH_EDITOR || IS_PROGRAM
	if (BrowserWindow.IsValid())
	{
		return FText::FromString(BrowserWindow->GetTitle());
	}
#endif
	return LOCTEXT("InvalidWindow", "Browser Window is not valid/supported");
}

bool SWebBrowser::CanGoBack() const
{
#if WITH_EDITOR || IS_PROGRAM
	if (BrowserWindow.IsValid())
	{
		return BrowserWindow->CanGoBack();
	}
#endif
	return false;
}

FReply SWebBrowser::OnBackClicked()
{
#if WITH_EDITOR || IS_PROGRAM
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->GoBack();
	}
#endif
	return FReply::Handled();
}

bool SWebBrowser::CanGoForward() const
{
#if WITH_EDITOR || IS_PROGRAM
	if (BrowserWindow.IsValid())
	{
		return BrowserWindow->CanGoForward();
	}
#endif
	return false;
}

FReply SWebBrowser::OnForwardClicked()
{
#if WITH_EDITOR || IS_PROGRAM
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->GoForward();
	}
#endif
	return FReply::Handled();
}

FText SWebBrowser::GetReloadButtonText() const
{
	static FText Reload = LOCTEXT("Reload", "Reload");
	static FText Stop = LOCTEXT("Stop", "Stop");

#if WITH_EDITOR || IS_PROGRAM
	if (BrowserWindow.IsValid())
	{
		if (BrowserWindow->IsLoading())
		{
			return Stop;
		}
	}
#endif
	return Reload;
}

FReply SWebBrowser::OnReloadClicked()
{
#if WITH_EDITOR || IS_PROGRAM
	if (BrowserWindow.IsValid())
	{
		if (BrowserWindow->IsLoading())
		{
			BrowserWindow->StopLoad();
		}
		else
		{
			BrowserWindow->Reload();
		}
	}
#endif
	return FReply::Handled();
}

EVisibility SWebBrowser::GetViewportVisibility() const
{
#if WITH_EDITOR || IS_PROGRAM
	if (BrowserWindow.IsValid() && BrowserWindow->HasBeenPainted())
	{
		return EVisibility::Visible;
	}
#endif
	return EVisibility::Hidden;
}

EVisibility SWebBrowser::GetLoadingThrobberVisibility() const
{
#if WITH_EDITOR || IS_PROGRAM
	if (BrowserWindow.IsValid() && !BrowserWindow->HasBeenPainted())
	{
		return EVisibility::Visible;
	}
#endif
	return EVisibility::Hidden;
}

#undef LOCTEXT_NAMESPACE
