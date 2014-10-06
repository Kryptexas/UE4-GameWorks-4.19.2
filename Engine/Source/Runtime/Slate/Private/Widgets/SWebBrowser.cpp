// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"
#include "SWebBrowser.h"

#include "WebBrowserModule.h"
#include "IWebBrowserSingleton.h"
#include "IWebBrowserWindow.h"
#include "WebBrowserViewport.h"

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
	BrowserWindow = IWebBrowserModule::Get().GetSingleton()->CreateBrowserWindow(OSWindowHandle, InArgs._InitialURL, InArgs._ViewportSize.Get().X, InArgs._ViewportSize.Get().Y);

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
			SAssignNew(ViewportWidget, SViewport)
			.ViewportSize(InArgs._ViewportSize)
			.EnableGammaCorrection(false)
		]
	];

	if (BrowserWindow.IsValid())
	{
		BrowserViewport = MakeShareable(new FWebBrowserViewport(BrowserWindow, ViewportWidget));
		ViewportWidget->SetViewportInterface(BrowserViewport.ToSharedRef());
	}
}

FText SWebBrowser::GetTitleText() const
{
	if (BrowserWindow.IsValid())
	{
		return FText::FromString(BrowserWindow->GetTitle());
	}
	return LOCTEXT("InvalidWindow", "Browser Window is not valid/supported");
}

bool SWebBrowser::CanGoBack() const
{
	if (BrowserWindow.IsValid())
	{
		return BrowserWindow->CanGoBack();
	}
	return false;
}

FReply SWebBrowser::OnBackClicked()
{
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->GoBack();
	}
	return FReply::Handled();
}

bool SWebBrowser::CanGoForward() const
{
	if (BrowserWindow.IsValid())
	{
		return BrowserWindow->CanGoForward();
	}
	return false;
}

FReply SWebBrowser::OnForwardClicked()
{
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->GoForward();
	}
	return FReply::Handled();
}

FText SWebBrowser::GetReloadButtonText() const
{
	static FText Reload = LOCTEXT("Reload", "Reload");
	static FText Stop = LOCTEXT("Stop", "Stop");

	if (BrowserWindow.IsValid())
	{
		if (BrowserWindow->IsLoading())
		{
			return Stop;
		}
	}
	return Reload;
}

FReply SWebBrowser::OnReloadClicked()
{
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
	return FReply::Handled();
}
