// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Forward Declarations
class FSlateRenderer;
class IWebBrowserWindow;

/**
 * A singleton class that takes care of general web browser tasks
 */
class WEBBROWSER_API IWebBrowserSingleton
{
public:
	/**
	 * Virtual Destructor
	 */
	virtual ~IWebBrowserSingleton() {};

	/**
	 * The Web Browser singleton needs the slate renderer in order to render pages to textures.
	 *
	 * @param  InSlateRenderer The Slate renderer
	 */
	virtual void SetSlateRenderer(TSharedPtr<FSlateRenderer> InSlateRenderer) = 0;

	/**
	 * Update the web browsers internal message loop
	 */
	virtual void PumpMessages() = 0;

	/**
	 * Create a new web browser window
	 *
	 * @param OSWindowHandle Handle of OS Window that the browser will be displayed in (can be null)
	 * @param InitialURL URL that the browser should initially navigate to
	 * @param Width Initial width of the browser
	 * @param Height Initial height of the browser
	 * @param bUseTransparency Whether to allow transparent rendering of pages
	 * @return New Web Browser Window Interface (may be null if not supported)
	 */
	virtual TSharedPtr<IWebBrowserWindow> CreateBrowserWindow(void* OSWindowHandle, FString InitialURL, uint32 Width, uint32 Height, bool bUseTransparency) = 0;
};
