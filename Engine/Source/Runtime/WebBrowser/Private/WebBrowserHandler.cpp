// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "WebBrowserPrivatePCH.h"
#include "WebBrowserHandler.h"
#include "WebBrowserWindow.h"
#include "WebBrowserSingleton.h"
#include "WebBrowserPopupFeatures.h"


#define LOCTEXT_NAMESPACE "WebBrowserHandler"

#if WITH_CEF3
FWebBrowserHandler::FWebBrowserHandler()
	: ShowErrorMessage(true)
{
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
				bool bUIWindowCreated = BrowserWindowParent->RequestCreateWindow(NewBrowserWindow.ToSharedRef(), BrowserPopupFeatures);
				if(!bUIWindowCreated)
				{
					NewBrowserWindow->CloseBrowser(true);
				}
			}
			else
			{
				Browser->GetHost()->CloseBrowser(true);
			}
		}
	}
}

 bool FWebBrowserHandler::DoClose(CefRefPtr<CefBrowser> Browser)
 {
	 TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if(BrowserWindow.IsValid())
	{
		BrowserWindow->OnBrowserClosing();
	}

	return false;
 }

void FWebBrowserHandler::OnBeforeClose(CefRefPtr<CefBrowser> Browser)
{
	TSharedPtr<FWebBrowserWindow> BrowserWindow = BrowserWindowPtr.Pin();
	if (BrowserWindow.IsValid())
	{
		BrowserWindow->OnBrowserClosed();
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
				NewHandler->SetPopupFeatures(MakeShareable(new FWebBrowserPopupFeatures(PopupFeatures)));
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
	return Retval;
}

bool FWebBrowserHandler::OnKeyEvent(CefRefPtr<CefBrowser> Browser,
	const CefKeyEvent& Event,
	CefEventHandle OsEvent)
{
#if UE_BUILD_DEBUG
	// Show dev tools on CMD/CTRL+ALT+I
	if( (Event.type == KEYEVENT_RAWKEYDOWN || Event.type == KEYEVENT_KEYDOWN) &&
#if PLATFORM_MAC
		(Event.modifiers == (EVENTFLAG_COMMAND_DOWN | EVENTFLAG_SHIFT_DOWN)) &&
#else
		(Event.modifiers == (EVENTFLAG_CONTROL_DOWN | EVENTFLAG_SHIFT_DOWN)) &&
#endif
		(Event.unmodified_character == 'i' || Event.unmodified_character == 'I')
	  )
	{

		CefPoint Point;
		CefString TargetUrl = "chrome-devtools://devtools/devtools.html";
		CefString TargetFrameName = "devtools";
		CefPopupFeatures PopupFeatures;
		CefWindowInfo WindowInfo;
		CefRefPtr<CefClient> NewClient;
		CefBrowserSettings BrowserSettings;
		bool NoJavascriptAccess = false;

		PopupFeatures.xSet = false;
		PopupFeatures.ySet = false;
		PopupFeatures.heightSet = false;
		PopupFeatures.widthSet = false;
		PopupFeatures.locationBarVisible = false;
		PopupFeatures.menuBarVisible = false;
		PopupFeatures.toolBarVisible  = false;
		PopupFeatures.statusBarVisible  = false;
		PopupFeatures.resizable = true;

		// Set max framerate to maximum supported.
		BrowserSettings.windowless_frame_rate = 60;
		// Disable plugins
		BrowserSettings.plugins = STATE_DISABLED;

		// OnBeforePopup already takes care of all the details required to ask the host application to create a new browser window.
		bool bSuppressWindowCreation = OnBeforePopup(Browser, Browser->GetFocusedFrame(), TargetUrl, TargetFrameName, PopupFeatures, WindowInfo, NewClient, BrowserSettings, &NoJavascriptAccess);

		if(! bSuppressWindowCreation)
		{
			Browser->GetHost()->ShowDevTools(WindowInfo, NewClient, BrowserSettings, Point);
		}
	}
#endif

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
