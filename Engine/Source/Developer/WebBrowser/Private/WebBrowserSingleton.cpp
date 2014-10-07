// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserPrivatePCH.h"
#include "WebBrowserSingleton.h"
#include "WebBrowserApp.h"
#include "WebBrowserHandler.h"
#include "WebBrowserWindow.h"

#if WITH_CEF3
#if PLATFORM_WINDOWS
	#include "AllowWindowsPlatformTypes.h"
#endif
	#include "include/cef_app.h"
#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#endif
#endif

FWebBrowserSingleton::FWebBrowserSingleton()
{
#if WITH_CEF3
	// Provide CEF with command-line arguments.
	CefMainArgs MainArgs(hInstance);

	// WebBrowserApp implements application-level callbacks.
	WebBrowserApp = new FWebBrowserApp;

	// Specify CEF global settings here.
	CefSettings Settings;
	Settings.no_sandbox = true;
	Settings.command_line_args_disabled = true;

	// Specify locale from our settings
	FString LocaleCode = GetCurrentLocaleCode();
	CefString(&Settings.locale) = *LocaleCode;

	// Specify path to resources
	FString ResourcesPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/CEF3/Win64/Resources")));
	FString LocalesPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/CEF3/Win64/Resources/locales")));
	CefString(&Settings.locales_dir_path) = *LocalesPath;
	CefString(&Settings.resources_dir_path) = *ResourcesPath;

	// Specify path to sub process exe
	FString SubProcessPath(TEXT("UnrealCEFSubProcess.exe"));
	CefString(&Settings.browser_subprocess_path) = *SubProcessPath;

	// Initialize CEF.
	CefInitialize(MainArgs, Settings, WebBrowserApp.get(), nullptr);
#endif
}

FWebBrowserSingleton::~FWebBrowserSingleton()
{
#if WITH_CEF3
	// Force all existing browsers to close in case any haven't been deleted
	for (int32 Index = 0; Index < WindowInterfaces.Num(); ++Index)
	{
		if (WindowInterfaces[Index].IsValid())
		{
			WindowInterfaces[Index].Pin()->CloseBrowser();
		}
	}

	// CefRefPtr takes care of delete
	WebBrowserApp = nullptr;
	// Shut down CEF.
	CefShutdown();
#endif
}

void FWebBrowserSingleton::SetSlateRenderer(TSharedPtr<FSlateRenderer> InSlateRenderer)
{
	SlateRenderer = InSlateRenderer;
}

void FWebBrowserSingleton::PumpMessages()
{
#if WITH_CEF3
	// Remove any windows that have been deleted
	for (int32 Index = WindowInterfaces.Num() - 1; Index >= 0; --Index)
	{
		if (!WindowInterfaces[Index].IsValid())
		{
			WindowInterfaces.RemoveAtSwap(Index);
		}
	}
	CefDoMessageLoopWork();
#endif
}

TSharedPtr<IWebBrowserWindow> FWebBrowserSingleton::CreateBrowserWindow(void* OSWindowHandle, FString InitialURL, uint32 Width, uint32 Height, bool bUseTransparency)
{
#if WITH_CEF3
	// Create new window
	TSharedPtr<FWebBrowserWindow> NewWindow(new FWebBrowserWindow(SlateRenderer, FIntPoint(Width, Height)));

	// WebBrowserHandler implements browser-level callbacks.
	CefRefPtr<FWebBrowserHandler> NewHandler(new FWebBrowserHandler);
	NewWindow->SetHandler(NewHandler);

	// Information used when creating the native window.
	CefWindowHandle WindowHandle = (CefWindowHandle)OSWindowHandle; // TODO: check this is correct for all platforms
	CefWindowInfo WindowInfo;

	// Always use off screen rendering so we can integrate with our windows
	WindowInfo.SetAsOffScreen(WindowHandle);
	WindowInfo.SetTransparentPainting(bUseTransparency);

	// Specify CEF browser settings here.
	CefBrowserSettings BrowserSettings;

	CefString URL = *InitialURL;

	// Create the CEF browser window.
	if (CefBrowserHost::CreateBrowser(WindowInfo, NewHandler.get(), URL, BrowserSettings, NULL))
	{
		WindowInterfaces.Add(NewWindow);
		return NewWindow;
	}
#endif
	return NULL;
}

FString FWebBrowserSingleton::GetCurrentLocaleCode()
{
	FCultureRef Culture = FInternationalization::Get().GetCurrentCulture();
	FString LocaleCode = Culture->GetTwoLetterISOLanguageName();
	FString Country = Culture->GetRegion();
	if (!Country.IsEmpty())
	{
		LocaleCode = LocaleCode + TEXT("-") + Country;
	}
	return LocaleCode;
}
