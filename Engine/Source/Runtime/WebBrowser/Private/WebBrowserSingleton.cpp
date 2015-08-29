// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserPrivatePCH.h"
#include "SlateCore.h"
#include "WebBrowserSingleton.h"
#include "WebBrowserApp.h"
#include "WebBrowserHandler.h"
#include "WebBrowserWindow.h"
#include "IPlatformTextField.h"
#include "IVirtualKeyboardEntry.h"
#include "SlateApplication.h"

#if WITH_CEF3
#if PLATFORM_WINDOWS
	#include "AllowWindowsPlatformTypes.h"
#endif
	#pragma push_macro("OVERRIDE")
	#undef OVERRIDE // cef headers provide their own OVERRIDE macro
	#include "include/cef_app.h"
	#pragma pop_macro("OVERRIDE")
#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#endif
#endif

FWebBrowserSingleton::FWebBrowserSingleton()
{
#if WITH_CEF3
	// Provide CEF with command-line arguments.
#if PLATFORM_WINDOWS
	CefMainArgs MainArgs(hInstance);
#elif PLATFORM_MAC || PLATFORM_LINUX
	//TArray<FString> Args;
	//int ArgCount = GSavedCommandLine.ParseIntoArray(Args, TEXT(" "), true);
	CefMainArgs MainArgs(0, nullptr);
#endif

	// WebBrowserApp implements application-level callbacks.
	WebBrowserApp = new FWebBrowserApp;
	WebBrowserApp->OnRenderProcessThreadCreated().BindRaw(this, &FWebBrowserSingleton::HandleRenderProcessCreated);

	// Specify CEF global settings here.
	CefSettings Settings;
	Settings.no_sandbox = true;
	Settings.command_line_args_disabled = true;

	// Specify locale from our settings
	FString LocaleCode = GetCurrentLocaleCode();
	CefString(&Settings.locale) = *LocaleCode;

	// Specify path to resources
#if PLATFORM_WINDOWS
	#if PLATFORM_64BITS
	FString ResourcesPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/CEF3/Win64/Resources")));
	FString LocalesPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/CEF3/Win64/Resources/locales")));
	#else
	FString ResourcesPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/CEF3/Win32/Resources")));
	FString LocalesPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/CEF3/Win32/Resources/locales")));
	#endif
#elif PLATFORM_MAC
	FString ResourcesPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/CEF3/Mac/Chromium Embedded Framework.framework/Resources")));
#elif PLATFORM_LINUX // @todo Linux
	FString ResourcesPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/CEF3/Linux/Resources")));
	FString LocalesPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/ThirdParty/CEF3/Linux/Resources/locales")));
#endif
#if !PLATFORM_MAC // On Mac Chromium ignores custom locales dir. Files need to be stored in Resources folder in the app bundle
	LocalesPath = FPaths::ConvertRelativePathToFull(LocalesPath);
	if (!FPaths::DirectoryExists(LocalesPath))
	{
		UE_LOG(LogWebBrowser, Error, TEXT("Chromium Locales information not found at: %s."), *LocalesPath);
	}
	CefString(&Settings.locales_dir_path) = *LocalesPath;
#endif
	ResourcesPath = FPaths::ConvertRelativePathToFull(ResourcesPath);
	if (!FPaths::DirectoryExists(ResourcesPath))
	{
		UE_LOG(LogWebBrowser, Error, TEXT("Chromium Resources information not found at: %s."), *ResourcesPath);
	}
	CefString(&Settings.resources_dir_path) = *ResourcesPath;

	// Specify path to sub process exe
#if PLATFORM_WINDOWS
	#if PLATFORM_64BITS
	FString SubProcessPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/Win64/UnrealCEFSubProcess.exe")));
	#else
	FString SubProcessPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/Win32/UnrealCEFSubProcess.exe")));
	#endif
#elif PLATFORM_MAC
	FString SubProcessPath(FPaths::Combine(*FPaths::EngineDir(), TEXT("Binaries/Mac/UnrealCEFSubProcess.app/Contents/MacOS/UnrealCEFSubProcess")));
#else // @todo Linux
	FString SubProcessPath(TEXT("UnrealCEFSubProcess"));
#endif
	SubProcessPath = FPaths::ConvertRelativePathToFull(SubProcessPath);
	if (!FPaths::FileExists(SubProcessPath))
	{
		UE_LOG(LogWebBrowser, Error, TEXT("UnrealCEFSubProcess.exe not found, check that this program has been built and is placed in: %s."), *SubProcessPath);
	}
	CefString(&Settings.browser_subprocess_path) = *SubProcessPath;

	// Initialize CEF.
	bool bSuccess = CefInitialize(MainArgs, Settings, WebBrowserApp.get(), nullptr);
	check(bSuccess);
#endif
}

#if WITH_CEF3
void FWebBrowserSingleton::HandleRenderProcessCreated(CefRefPtr<CefListValue> ExtraInfo)
{
	for (int32 Index = WindowInterfaces.Num() - 1; Index >= 0; --Index)
	{
		TSharedPtr<FWebBrowserWindow> BrowserWindow = WindowInterfaces[Index].Pin();
		if (BrowserWindow.IsValid())
		{
			CefRefPtr<CefDictionaryValue> Bindings = BrowserWindow->GetProcessInfo();
			if (Bindings.get())
			{
				ExtraInfo->SetDictionary(ExtraInfo->GetSize(), Bindings);
			}
		}
	}
}
#endif

FWebBrowserSingleton::~FWebBrowserSingleton()
{
#if WITH_CEF3
	// Force all existing browsers to close in case any haven't been deleted
	for (int32 Index = 0; Index < WindowInterfaces.Num(); ++Index)
	{
		if (WindowInterfaces[Index].IsValid())
		{
			WindowInterfaces[Index].Pin()->CloseBrowser(true);
		}
	}
	// Just in case, although we deallocate WebBrowserApp right after this.
	WebBrowserApp->OnRenderProcessThreadCreated().Unbind();
	// CefRefPtr takes care of delete
	WebBrowserApp = nullptr;
	// Shut down CEF.
	CefShutdown();
#endif
}

TSharedPtr<IWebBrowserWindow> FWebBrowserSingleton::CreateBrowserWindow(
	TSharedPtr<FWebBrowserWindow>& BrowserWindowParent,
	TSharedPtr<FWebBrowserWindowInfo>& BrowserWindowInfo
	)
{
#if WITH_CEF3

	//  @todo: Width/Height should be obtained when requesting a UI window from user code which happens later.
	int32 Width = 800;
	int32 Height = 600;
	TOptional<FString> ContentsToLoad;

	bool bShowErrorMessage = BrowserWindowParent->IsShowingErrorMessages();
	bool bThumbMouseButtonNavigation = BrowserWindowParent->IsThumbMouseButtonNavigationEnabled();
	bool bUseTransparency = BrowserWindowParent->UseTransparency();
	FString InitialURL = BrowserWindowInfo->Browser->GetMainFrame()->GetURL().ToWString().c_str();
	TSharedPtr<FWebBrowserWindow> NewBrowserWindow(new FWebBrowserWindow(FIntPoint(Width, Height), InitialURL, ContentsToLoad, bShowErrorMessage, bThumbMouseButtonNavigation, bUseTransparency));
	
	NewBrowserWindow->SetHandler(BrowserWindowInfo->Handler);
	NewBrowserWindow->BindCefBrowser(BrowserWindowInfo->Browser);

	WindowInterfaces.Add(NewBrowserWindow);
	return NewBrowserWindow;
#endif
	return NULL;
}

TSharedPtr<IWebBrowserWindow> FWebBrowserSingleton::CreateBrowserWindow(
	void* OSWindowHandle, 
	FString InitialURL, 
	uint32 Width, 
	uint32 Height, 
	bool bUseTransparency,
	bool bThumbMouseButtonNavigation,
	TOptional<FString> ContentsToLoad, 
	bool ShowErrorMessage,
	FColor BackgroundColor)
{
#if WITH_CEF3
	// Create new window
	TSharedPtr<FWebBrowserWindow> NewWindow(new FWebBrowserWindow(FIntPoint(Width, Height), InitialURL, ContentsToLoad, ShowErrorMessage, bThumbMouseButtonNavigation, bUseTransparency));

	// WebBrowserHandler implements browser-level callbacks.
	CefRefPtr<FWebBrowserHandler> NewHandler(new FWebBrowserHandler);
	NewWindow->SetHandler(NewHandler);

	// Information used when creating the native window.
	CefWindowHandle WindowHandle = (CefWindowHandle)OSWindowHandle; // TODO: check this is correct for all platforms
	CefWindowInfo WindowInfo;

	// Always use off screen rendering so we can integrate with our windows
	WindowInfo.SetAsWindowless(WindowHandle, bUseTransparency);

	// Specify CEF browser settings here.
	CefBrowserSettings BrowserSettings;
	
	// Set max framerate to maximum supported.
	BrowserSettings.windowless_frame_rate = 60;
	BrowserSettings.background_color = CefColorSetARGB(BackgroundColor.A, BackgroundColor.R, BackgroundColor.G, BackgroundColor.B);

	// Disable plugins
	BrowserSettings.plugins = STATE_DISABLED;

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

bool FWebBrowserSingleton::Tick(float DeltaTime)
{
#if WITH_CEF3
	bool bIsSlateAwake = !FSlateApplication::Get().IsSlateAsleep();
	// Remove any windows that have been deleted and check wether it's currently visible
	for (int32 Index = WindowInterfaces.Num() - 1; Index >= 0; --Index)
	{
		if (!WindowInterfaces[Index].IsValid())
		{
			WindowInterfaces.RemoveAtSwap(Index);
		}
		else if (bIsSlateAwake) // only check for Tick activity if Slate is currently ticking
		{
			TSharedPtr<FWebBrowserWindow> BrowserWindow = WindowInterfaces[Index].Pin();
			if(BrowserWindow.IsValid())
			{
				// Test if we've ticked recently. If not assume the browser window has become hidden.
				BrowserWindow->CheckTickActivity();
			}
		}
	}
	CefDoMessageLoopWork();
#endif
	return true;
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
