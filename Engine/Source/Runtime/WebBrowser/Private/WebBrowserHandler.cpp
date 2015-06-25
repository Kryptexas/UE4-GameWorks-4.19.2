// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserPrivatePCH.h"
#include "WebBrowserHandler.h"
#include "WebBrowserWindow.h"
#include "WebBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "WebBrowserHandler"

#if WITH_CEF3
FWebBrowserHandler::FWebBrowserHandler()
	: ShowErrorMessage(true)
{
	// This has to match the config in UnrealCEFSubpProcess
	CefMessageRouterConfig MessageRouterConfig;
	MessageRouterConfig.js_query_function = "ueQuery";
	MessageRouterConfig.js_cancel_function = "ueQueryCancel";
	MessageRouter = CefMessageRouterBrowserSide::Create(MessageRouterConfig);
	MessageRouter->AddHandler(this, true);
}

void FWebBrowserHandler::OnTitleChange(CefRefPtr<CefBrowser> Browser, const CefString& Title)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->SetTitle(Title);
	}
}

void FWebBrowserHandler::OnAddressChange(CefRefPtr<CefBrowser> Browser, CefRefPtr<CefFrame> Frame, const CefString& Url)
{
	if (Frame->IsMain())
	{
		TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

		if (BrowserWindow.IsValid())
		{
			BrowserWindow->SetUrl(Url);
		}
	}
}

bool FWebBrowserHandler::OnTooltip(CefRefPtr<CefBrowser> Browser, CefString& Text)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->SetToolTip(Text);
	}

	return false;
}


void FWebBrowserHandler::OnAfterCreated(CefRefPtr<CefBrowser> Browser)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{

		BrowserWindow->BindCefBrowser(Browser);
	}
	else
	{
		if(Browser->IsPopup())
		{
			TSharedPtr<FWebBrowserWindow> BrowserWindowParent = BrowserWindowParentPtr.Pin();
			if(BrowserWindowParent.IsValid() && BrowserWindowParent->SupportsNewWindows())
			{
				TSharedPtr<FWebBrowserWindowInfo> NewBrowserWindowInfo = MakeShareable(new FWebBrowserWindowInfo(Browser, this));
				TSharedPtr<IWebBrowserWindow> NewBrowserWindow = IWebBrowserModule::Get().GetSingleton()->CreateBrowserWindow( 
					BrowserWindowParent,
					NewBrowserWindowInfo
					);	

				{
					// @todo: At the moment we need to downcast since the handler does not support using the interface.
					TSharedPtr<FWebBrowserWindow> HandlerSpecificBrowserWindow = StaticCastSharedPtr<FWebBrowserWindow>(NewBrowserWindow);
					BrowserWindowPtr = HandlerSpecificBrowserWindow;
				}

				// Request a UI window for the browser.  If it is not created we do some cleanup.
				bool bUIWindowCreated = BrowserWindowParent->RequestCreateWindow(NewBrowserWindow.ToSharedRef());
				if(!bUIWindowCreated)
				{
					NewBrowserWindow->CloseBrowser();
				}
			}
			else
			{
				Browser->GetHost()->CloseBrowser(true);
			}
		}
	}
}

void FWebBrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> Browser)
{
	MessageRouter->OnBeforeClose(Browser);
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->BindCefBrowser(nullptr);
	}
}

bool FWebBrowserHandler::OnBeforePopup( CefRefPtr<CefBrowser> Browser,
									   CefRefPtr<CefFrame> Frame,
									   const CefString& TargetUrl,
									   const CefString& TArgetFrameName,
									   const CefPopupFeatures& PopupFeatures,
									   CefWindowInfo& WindowInfo,
									   CefRefPtr<CefClient>& Client,
									   CefBrowserSettings& Settings,
									   bool* NoJavascriptAccess )
{

	// By default, we ignore any popup requests unless they are handled by us in some way.
	bool bSupressCEFWindowCreation = true;
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		bSupressCEFWindowCreation = BrowserWindow->OnCefBeforePopup(TargetUrl, TArgetFrameName);

		if(!bSupressCEFWindowCreation)
		{
			if(BrowserWindow->SupportsNewWindows())
			{
				CefRefPtr<FWebBrowserHandler> NewHandler(new FWebBrowserHandler);
				NewHandler->SetBrowserWindowParent(BrowserWindow);
				Client = NewHandler;
				
				CefWindowHandle ParentWindowHandle = BrowserWindow->GetCefBrowser()->GetHost()->GetWindowHandle();

				// Always use off screen rendering so we can integrate with our windows
				WindowInfo.SetAsWindowless(ParentWindowHandle, BrowserWindow->UseTransparency());

				// We need to rely on CEF to create our window so we set the WindowInfo, BrowserSettings, Client, and then return false
				bSupressCEFWindowCreation = false;
			}
			else
			{
				bSupressCEFWindowCreation = true;
			}
		}
	}

	return bSupressCEFWindowCreation; 
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

void FWebBrowserHandler::OnLoadStart(CefRefPtr<CefBrowser> Browser, CefRefPtr<CefFrame> Frame)
{
}

void FWebBrowserHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> Browser, bool bIsLoading, bool bCanGoBack, bool bCanGoForward)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();

	if (BrowserWindow.IsValid())
	{
		BrowserWindow->NotifyDocumentLoadingStateChange(bIsLoading);
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

void FWebBrowserHandler::OnCursorChange(CefRefPtr<CefBrowser> Browser, CefCursorHandle Cursor, CefRenderHandler::CursorType Type, const CefCursorInfo& CustomCursorInfo)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnCursorChange(Cursor, Type, CustomCursorInfo);
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

void FWebBrowserHandler::OnRenderProcessTerminated(CefRefPtr<CefBrowser> Browser, TerminationStatus Status)
{
	MessageRouter->OnRenderProcessTerminated(Browser);
}

bool FWebBrowserHandler::OnBeforeBrowse(CefRefPtr<CefBrowser> Browser,
	CefRefPtr<CefFrame> Frame,
	CefRefPtr<CefRequest> Request,
	bool IsRedirect)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		if(BrowserWindow->OnBeforeBrowse(Browser, Frame, Request, IsRedirect))
		{
			return true;
		}
	}

	MessageRouter->OnBeforeBrowse(Browser, Frame);
	return false;
}

CefRefPtr<CefResourceHandler> FWebBrowserHandler::GetResourceHandler( CefRefPtr<CefBrowser> Browser, CefRefPtr< CefFrame > Frame, CefRefPtr< CefRequest > Request )
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		return BrowserWindow->GetResourceHandler(Frame, Request);
	}
	return NULL;
}

void FWebBrowserHandler::SetBrowserWindow(TSharedPtr<FWebBrowserWindow> InBrowserWindow)
{
	BrowserWindowPtr = InBrowserWindow;
}

void FWebBrowserHandler::SetBrowserWindowParent(TSharedPtr<FWebBrowserWindow> InBrowserWindow)
{
	BrowserWindowParentPtr = InBrowserWindow;
}

bool FWebBrowserHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> Browser,
	CefProcessId SourceProcess,
	CefRefPtr<CefProcessMessage> Message)
{
	bool Retval = false;
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		Retval = BrowserWindow->OnProcessMessageReceived(Browser, SourceProcess, Message);
	}
	if (! Retval)
	{
		Retval = MessageRouter->OnProcessMessageReceived(Browser, SourceProcess, Message);
	}
	return Retval;
}

bool FWebBrowserHandler::OnQuery(CefRefPtr<CefBrowser> Browser,
		CefRefPtr<CefFrame> Frame,
		int64 QueryId,
		const CefString& Request,
		bool Persistent,
		CefRefPtr<CefMessageRouterBrowserSide::Callback> Callback)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		return BrowserWindow->OnQuery(QueryId, Request, Persistent, Callback);
	}
	return false;
}

void FWebBrowserHandler::OnQueryCanceled(CefRefPtr<CefBrowser> Browser, CefRefPtr<CefFrame> Frame, int64 QueryId)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnQueryCanceled(QueryId);
	}
}

bool FWebBrowserHandler::OnKeyEvent(CefRefPtr<CefBrowser> Browser,
	const CefKeyEvent& Event,
	CefEventHandle OsEvent)
{
#if PLATFORM_MAC
	// We need to handle standard Copy/Paste/etc... shortcuts on OS X
	if( (Event.type == KEYEVENT_RAWKEYDOWN || Event.type == KEYEVENT_KEYDOWN) &&
		(Event.modifiers & EVENTFLAG_COMMAND_DOWN) != 0 &&
		(Event.modifiers & EVENTFLAG_CONTROL_DOWN) == 0 &&
		(Event.modifiers & EVENTFLAG_ALT_DOWN) == 0 &&
		( (Event.modifiers & EVENTFLAG_SHIFT_DOWN) == 0 || Event.unmodified_character == 'z' )
	  )
	{
		CefRefPtr<CefFrame> Frame = Browser->GetFocusedFrame();
		if (Frame)
		{
			switch (Event.unmodified_character)
			{
				case 'a':
					Frame->SelectAll();
					return true;
				case 'c':
					Frame->Copy();
					return true;
				case 'v':
					Frame->Paste();
					return true;
				case 'x':
					Frame->Cut();
					return true;
				case 'z':
					if( (Event.modifiers & EVENTFLAG_SHIFT_DOWN) == 0 )
					{
						Frame->Undo();
					}
					else
					{
						Frame->Redo();
					}
					return true;
			}
		}
	}
#endif

	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		return BrowserWindow->OnUnhandledKeyEvent(Event);
	}

	return false;
}
#endif

#undef LOCTEXT_NAMESPACE
