// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IWebBrowserSingleton.h"

#if WITH_CEF3
#if PLATFORM_WINDOWS
	#include "AllowWindowsPlatformTypes.h"
#endif
#pragma push_macro("OVERRIDE")
#undef OVERRIDE // cef headers provide their own OVERRIDE macro
#include "include/internal/cef_ptr.h"
#pragma pop_macro("OVERRIDE")
#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#endif

class CefListValue;
#endif


class FWebBrowserApp;
class FWebBrowserHandler;
class FWebBrowserWindow;


/**
 * Implementation of singleton class that takes care of general web browser tasks
 */
class FWebBrowserSingleton
	: public IWebBrowserSingleton
	, public FTickerObjectBase
{
public:

	/** Default constructor. */
	FWebBrowserSingleton();

	/** Virtual destructor. */
	virtual ~FWebBrowserSingleton();

	/**
	* Gets the Current Locale Code in the format CEF expects
	*
	* @return Locale code as either "xx" or "xx-YY"
	*/
	static FString GetCurrentLocaleCode();

public:

	// IWebBrowserSingleton Interface

	TSharedPtr<IWebBrowserWindow> CreateBrowserWindow(
		TSharedPtr<FWebBrowserWindow>& BrowserWindowParent,
		TSharedPtr<FWebBrowserWindowInfo>& BrowserWindowInfo) override;

	TSharedPtr<IWebBrowserWindow> CreateBrowserWindow(
		void* OSWindowHandle, 
		FString InitialURL, 
		uint32 Width, 
		uint32 Height, 
		bool bUseTransparency, 
		bool bThumbMouseButtonNavigation,
		TOptional<FString> ContentsToLoad = TOptional<FString>(), 
		bool ShowErrorMessage = true,
		FColor BackgroundColor = FColor(255, 255, 255, 255)) override;


public:

	// FTickerObjectBase Interface

	virtual bool Tick(float DeltaTime) override;

private:
#if WITH_CEF3
	/** When new render processes are created, send all permanent variable bindings to them. */
	void HandleRenderProcessCreated(CefRefPtr<CefListValue> ExtraInfo);
	/** Pointer to the CEF App implementation */
	CefRefPtr<FWebBrowserApp>			WebBrowserApp;
	/** List of currently existing browser windows */
	TArray<TWeakPtr<FWebBrowserWindow>>	WindowInterfaces;
#endif
};
