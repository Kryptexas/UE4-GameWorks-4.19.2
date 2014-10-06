// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class IWebBrowserWindow;
class FWebBrowserViewport;

class SLATE_API SWebBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SWebBrowser)
		: _InitialURL(TEXT("www.google.com"))
		, _ShowControls(true)
		, _ViewportSize(FVector2D(320, 240))
		{}

		/** A reference to the parent window */
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)

		/** URL that the browser will initially navigate to */
		SLATE_ARGUMENT(FString, InitialURL)

		/** Whether to show standard controls like Back, Forward, Reload etc. */
		SLATE_ARGUMENT(bool, ShowControls)

		/** Desired size of the web browser viewport */
		SLATE_ATTRIBUTE(FVector2D, ViewportSize);
	SLATE_END_ARGS()

	/**
	 * Constructor
	 */
	SWebBrowser();

	/**
	 * Construct the widget.
	 *
	 * @param InArgs  Declaration from which to construct the widget.
	 */
	void Construct(const FArguments& InArgs);

	/**
	 * Get the current title of the web page
	 */
	FText GetTitleText() const;

private:
	/**
	 * Returns true if the browser can navigate backwards.
	 */
	bool CanGoBack() const;

	/**
	 * Navigate backwards.
	 */
	FReply OnBackClicked();

	/**
	 * Returns true if the browser can navigate forwards.
	 */
	bool CanGoForward() const;

	/**
	 * Navigate forwards.
	 */
	FReply OnForwardClicked();

	/**
	 * Get text for reload button depending on status
	 */
	FText GetReloadButtonText() const;

	/**
	 * Reload or stop loading
	 */
	FReply OnReloadClicked();

private:
	/** Interface for dealing with a web browser window */
	TSharedPtr<IWebBrowserWindow>	BrowserWindow;

	/** Viewport interface for rendering the web page */
	TSharedPtr<FWebBrowserViewport>	BrowserViewport;
};
