// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

// Forward Declarations
class FSlateRenderer;
class IWebBrowserWindow;
class FWebBrowserWindow;
struct FWebBrowserWindowInfo;

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
	 * Create a new web browser window
	 *
	 * @param BrowserWindowParent The parent browser window
	 * @param BrowserWindowInfo Info for setting up the new browser window
	 * @return New Web Browser Window Interface (may be null if not supported)
	 */
	virtual TSharedPtr<IWebBrowserWindow> CreateBrowserWindow(
		TSharedPtr<FWebBrowserWindow>& BrowserWindowParent,
		TSharedPtr<FWebBrowserWindowInfo>& BrowserWindowInfo
		) = 0;

	/**
	 * Create a new web browser window
	 *
	 * @param OSWindowHandle Handle of OS Window that the browser will be displayed in (can be null)
	 * @param InitialURL URL that the browser should initially navigate to
	 * @param Width Initial width of the browser
	 * @param Height Initial height of the browser
	 * @param bUseTransparency Whether to allow transparent rendering of pages
	 * @param ContentsToLoad Optional string to load as a web page
	 * @param ShowErrorMessage Whether to show an error message in case of loading errors.
	 * @param BackgroundColor Opaque background color used before a document is loaded and when no document color is specified.
	 * @return New Web Browser Window Interface (may be null if not supported)
	 */
	virtual TSharedPtr<IWebBrowserWindow> CreateBrowserWindow(
		void* OSWindowHandle, 
		FString InitialURL, 
		uint32 Width, 
		uint32 Height, 
		bool bUseTransparency, 
		bool bThumbMouseButtonNavigation,
		TOptional<FString> ContentsToLoad = TOptional<FString>(), 
		bool ShowErrorMessage = true,
		FColor BackgroundColor = FColor(255, 255, 255, 255)) = 0;
};
