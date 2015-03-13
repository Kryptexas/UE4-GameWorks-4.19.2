// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserPrivatePCH.h"
#include "WebBrowserHandler.h"
#include "WebBrowserWindow.h"
#include "WebBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "WebBrowserHandler"

#if WITH_CEF3
FWebBrowserHandler::FWebBrowserHandler()
	: ShowErrorMessage(true)
{ }

void FWebBrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> Browser, const CefString& Title)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->SetTitle(Title);
	}
}

void FWebBrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> Browser)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->BindCefBrowser(Browser);
	}
}

void FWebBrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> Browser)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->BindCefBrowser(nullptr);
	}
}

void FWebBrowserHandler::OnLoadError(CefRefPtr<CefBrowser> Browser,
	CefRefPtr<CefFrame> Frame,
	CefLoadHandler::ErrorCode InErrorCode,
	const CefString& ErrorText,
	const CefString& FailedUrl)
{
	// Don't display an error for downloaded files.
	if (InErrorCode == ERR_ABORTED)
	{
		return;
	}

	// notify browser window
	if (Frame->IsMain())
	{
		TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

		if (BrowserWindow.IsValid())
		{
			BrowserWindow->NotifyDocumentError();
		}
	}

	// Display a load error message.
	if (ShowErrorMessage)
	{
		FFormatNamedArguments Args;
		{
			Args.Add(TEXT("FailedUrl"), FText::FromString(FailedUrl.ToWString().c_str()));
			Args.Add(TEXT("ErrorText"), FText::FromString(ErrorText.ToWString().c_str()));
			Args.Add(TEXT("ErrorCode"), FText::AsNumber(InErrorCode));
		}
		FText ErrorMsg = FText::Format(LOCTEXT("WebBrowserLoadError", "Failed to load URL {FailedUrl} with error {ErrorText} ({ErrorCode})."), Args);
		FString ErrorHTML = TEXT("<html><body bgcolor=\"white\"><h2>") + ErrorMsg.ToString() + TEXT("</h2></body></html>");

		Frame->LoadString(*ErrorHTML, FailedUrl);
	}
}

void FWebBrowserHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->NotifyDocumentLoadingStateChange(isLoading);
	}
}

bool FWebBrowserHandler::GetViewRect(CefRefPtr<CefBrowser> Browser, CefRect& Rect)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		return BrowserWindow->GetViewRect(Rect);
	}

	return false;
}

void FWebBrowserHandler::OnPaint(CefRefPtr<CefBrowser> Browser,
	PaintElementType Type,
	const RectList& DirtyRects,
	const void* Buffer,
	int Width, int Height)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnPaint(Type, DirtyRects, Buffer, Width, Height);
	}
}

void FWebBrowserHandler::OnCursorChange(CefRefPtr<CefBrowser> Browser, CefCursorHandle Cursor)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnCursorChange(Cursor);
	}
}

bool FWebBrowserHandler::OnBeforeResourceLoad(CefRefPtr<CefBrowser> Browser, CefRefPtr<CefFrame> Frame, CefRefPtr<CefRequest> Request)
{
	const FString LanguageHeaderText(TEXT("Accept-Language"));
	const FString LocaleCode = FWebBrowserSingleton::GetCurrentLocaleCode();
	CefRequest::HeaderMap HeaderMap;
	Request->GetHeaderMap(HeaderMap);
	auto LanguageHeader = HeaderMap.find(*LanguageHeaderText);
	if (LanguageHeader != HeaderMap.end())
	{
		(*LanguageHeader).second = *LocaleCode;
	}
	else
	{
		HeaderMap.insert(std::pair<CefString, CefString>(*LanguageHeaderText, *LocaleCode));
	}
	Request->SetHeaderMap(HeaderMap);
	return false;
}

void FWebBrowserHandler::SetBrowserWindow(TSharedPtr<FWebBrowserWindow> InBrowserWindow)
{
	BrowserWindowPtr = InBrowserWindow;
}
#endif

#undef LOCTEXT_NAMESPACE
