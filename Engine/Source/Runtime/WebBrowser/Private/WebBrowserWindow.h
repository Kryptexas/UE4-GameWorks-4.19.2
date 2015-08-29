// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_CEF3

#include "IWebBrowserWindow.h"
#include "WebBrowserHandler.h"
#include "WebJSScripting.h"
#include "SlateCore.h"
#include "SlateBasics.h"
#include "CoreUObject.h"

#if PLATFORM_WINDOWS
	#include "AllowWindowsPlatformTypes.h"
#endif

#pragma push_macro("OVERRIDE")
#undef OVERRIDE // cef headers provide their own OVERRIDE macro
#include "include/internal/cef_ptr.h"
#include "include/cef_render_handler.h"
#pragma pop_macro("OVERRIDE")

#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#endif

/**
 * Helper for containing items required for CEF browser window creation.
 */
struct  FWebBrowserWindowInfo
{
	FWebBrowserWindowInfo(CefRefPtr<CefBrowser> InBrowser, CefRefPtr<FWebBrowserHandler> InHandler) 
		: Browser(InBrowser)
		, Handler(InHandler)
	{}
	CefRefPtr<CefBrowser> Browser;
	CefRefPtr<FWebBrowserHandler> Handler;
};

/**
 * Implementation of interface for dealing with a Web Browser window.
 */
class FWebBrowserWindow
	: public IWebBrowserWindow
	, public TSharedFromThis<FWebBrowserWindow>
{
	// Allow the Handler to access functions only it needs
	friend class FWebBrowserHandler;

public:
	/**
	 * Creates and initializes a new instance.
	 * 
	 * @param InViewportSize Initial size of the browser window.
	 * @param InInitialURL The Initial URL that will be loaded.
	 * @param InContentsToLoad Optional string to load as a web page.
	 * @param InShowErrorMessage Whether to show an error message in case of loading errors.
	 */
	FWebBrowserWindow(FIntPoint ViewportSize, FString URL, TOptional<FString> ContentsToLoad, bool ShowErrorMessage, bool bThumbMouseButtonNavigation, bool bUseTransparency);

	/** Virtual Destructor. */
	virtual ~FWebBrowserWindow();

public:

	/**
	* Set the CEF Handler receiving browser callbacks for this window.
	*
	* @param InHandler Pointer to the handler for this window.
	*/
	void SetHandler(CefRefPtr<FWebBrowserHandler> InHandler);

	/**
	 * Called to pass reference to the underlying CefBrowser as this is not created at the same time
	 * as the FWebBrowserWindow.
	 *
	 * @param Browser The CefBrowser for this window.
	 */
	void BindCefBrowser(CefRefPtr<CefBrowser> Browser);

	bool IsShowingErrorMessages() { return ShowErrorMessage; }
	bool IsThumbMouseButtonNavigationEnabled() { return bThumbMouseButtonNavigation; }
	bool UseTransparency() { return bUseTransparency; }

public:

	// IWebBrowserWindow Interface

	virtual void LoadURL(FString NewURL) override;
	virtual void LoadString(FString Contents, FString DummyURL) override;
	virtual void SetViewportSize(FIntPoint WindowSize) override;
	virtual FSlateShaderResource* GetTexture(bool bIsPopup = false) override;
	virtual bool IsValid() const override;
	virtual bool IsInitialized() const override;
	virtual bool IsClosing() const override;
	virtual EWebBrowserDocumentState GetDocumentLoadingState() const override;
	virtual FString GetTitle() const override;
	virtual FString GetUrl() const override;
	virtual bool OnKeyDown(const FKeyEvent& InKeyEvent) override;
	virtual bool OnKeyUp(const FKeyEvent& InKeyEvent) override;
	virtual bool OnKeyChar(const FCharacterEvent& InCharacterEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup) override;
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup) override;
	virtual void OnFocus(bool SetFocus, bool bIsPopup) override;
	virtual void OnCaptureLost() override;
	virtual bool CanGoBack() const override;
	virtual void GoBack() override;
	virtual bool CanGoForward() const override;
	virtual void GoForward() override;
	virtual bool IsLoading() const override;
	virtual void Reload() override;
	virtual void StopLoad() override;
	virtual void ExecuteJavascript(const FString& Script) override;
	virtual void CloseBrowser(bool bForce) override;
	virtual void BindUObject(const FString& Name, UObject* Object, bool bIsPermanent = true) override;
	virtual void UnbindUObject(const FString& Name, UObject* Object = nullptr, bool bIsPermanent = true) override;

	DECLARE_DERIVED_EVENT(FWebBrowserWindow, IWebBrowserWindow::FOnDocumentStateChanged, FOnDocumentStateChanged);
	virtual FOnDocumentStateChanged& OnDocumentStateChanged() override
	{
		return DocumentStateChangedEvent;
	}

	DECLARE_DERIVED_EVENT(FWebBrowserWindow, IWebBrowserWindow::FOnTitleChanged, FOnTitleChanged);
	virtual FOnTitleChanged& OnTitleChanged() override
	{
		return TitleChangedEvent;
	}

	DECLARE_DERIVED_EVENT(FWebBrowserWindow, IWebBrowserWindow::FOnUrlChanged, FOnUrlChanged);
	virtual FOnUrlChanged& OnUrlChanged() override
	{
		return UrlChangedEvent;
	}

	DECLARE_DERIVED_EVENT(FWebBrowserWindow, IWebBrowserWindow::FOnToolTip, FOnToolTip);
	virtual FOnToolTip& OnToolTip() override
	{
		return ToolTipEvent;
	}

	DECLARE_DERIVED_EVENT(FWebBrowserWindow, IWebBrowserWindow::FOnNeedsRedraw, FOnNeedsRedraw);
	virtual FOnNeedsRedraw& OnNeedsRedraw() override
	{
		return NeedsRedrawEvent;
	}
	
	virtual FOnBeforeBrowse& OnBeforeBrowse() override
	{
		return BeforeBrowseDelegate;
	}
	
	virtual FOnLoadUrl& OnLoadUrl() override
	{
		return LoadUrlDelegate;
	}

	virtual FOnCreateWindow& OnCreateWindow() override
	{
		return CreateWindowDelegate;
	}

	virtual FOnCloseWindow& OnCloseWindow() override
	{
		return CloseWindowDelegate;
	}

	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) override
	{
		return Cursor == EMouseCursor::Default ? FCursorReply::Unhandled() : FCursorReply::Cursor(Cursor);
	}

	virtual FOnBeforePopupDelegate& OnBeforePopup() override
	{
		return BeforePopupDelegate;
	}

	DECLARE_DERIVED_EVENT(FWebBrowserWindow, IWebBrowserWindow::FOnShowPopup, FOnShowPopup);
	virtual FOnShowPopup& OnShowPopup() override
	{
		return ShowPopupEvent;
	}

	DECLARE_DERIVED_EVENT(FWebBrowserWindow, IWebBrowserWindow::FOnDismissPopup, FOnDismissPopup);
	virtual FOnDismissPopup& OnDismissPopup() override
	{
		return DismissPopupEvent;
	}

private:

	/**
	 * Used to obtain the internal CEF browser.
	 *
	 * @return The bound CEF browser.
	 */
	CefRefPtr<CefBrowser> GetCefBrowser();

	/**
	 * Sets the Title of this window.
	 * 
	 * @param InTitle The new title of this window.
	 */
	void SetTitle(const CefString& InTitle);

	/**
	 * Sets the url of this window.
	 * 
	 * @param InUrl The new url of this window.
	 */
	void SetUrl(const CefString& InUrl);

	/**
	 * Sets the tool tip for this window.
	 * 
	 * @param InToolTip The text to show in the ToolTip. Empty string for no tool tip.
	 */
	void SetToolTip(const CefString& InToolTip);

	/**
	 * Get the current proportions of this window.
	 *
	 * @param Rect Reference to CefRect to store sizes.
	 * @return Whether Rect was set up correctly.
	 */
	bool GetViewRect(CefRect& Rect);

	/** Notifies clients that document loading has failed. */
	void NotifyDocumentError();

	/**
	 * Notifies clients that the loading state of the document has changed.
	 *
	 * @param IsLoading Whether the document is loading (false = completed).
	 */
	void NotifyDocumentLoadingStateChange(bool IsLoading);

	/**
	 * Called when there is an update to the rendered web page.
	 *
	 * @param Type Paint type.
	 * @param DirtyRects List of image areas that have been changed.
	 * @param Buffer Pointer to the raw texture data.
	 * @param Width Width of the texture.
	 * @param Height Height of the texture.
	 */
	void OnPaint(CefRenderHandler::PaintElementType Type, const CefRenderHandler::RectList& DirtyRects, const void* Buffer, int Width, int Height);
	
	/**
	 * Called when cursor would change due to web browser interaction.
	 * 
	 * @param Cursor Handle to CEF mouse cursor.
	 */
	void OnCursorChange(CefCursorHandle Cursor, CefRenderHandler::CursorType Type, const CefCursorInfo& CustomCursorInfo);

	/**
	 * Called when a message was received from the renderer process.
	 *
	 * @param Browser The CefBrowser for this window.
	 * @param SourceProcess The process id of the sender of the message. Currently always PID_RENDERER.
	 * @param Message The actual message.
	 * @return true if the message was handled, else false.
	 */
	bool OnProcessMessageReceived(CefRefPtr<CefBrowser> Browser, CefProcessId SourceProcess, CefRefPtr<CefProcessMessage> Message);

	/**
	 * Called before browser navigation.
	 *
	 * @param Browser The CefBrowser for this window.
	 * @param Frame The CefFrame the request came from.
	 * @param Request The CefRequest containing web request info.
	 * @param bIsRedirect true if the navigation was a result of redirection, false otherwise.
	 * @return true if the navigation was handled and no further processing of the navigation request should be disabled, false if the navigation should be handled by the default CEF implementation.
	 */
	bool OnBeforeBrowse(CefRefPtr<CefBrowser> Browser, CefRefPtr<CefFrame> Frame, CefRefPtr<CefRequest> Request, bool bIsRedirect);
	
	/**
	 * Called before loading a resource.
	 */
	CefRefPtr<CefResourceHandler> GetResourceHandler( CefRefPtr< CefFrame > Frame, CefRefPtr< CefRequest > Request );

	/** 
	 * Called when browser reports a key event that was not handled by it
	 */
	bool OnUnhandledKeyEvent(const CefKeyEvent& CefEvent);


	/** Specifies if window creation functionality is available. 
	 *
	 * @return true if window creation functionality was provided, false otherwise.  If false, RequestCreateWindow() will always return false.
	 */
	bool SupportsNewWindows();

	/** Called when the browser requests a new UI window
	 *
	 * @param NewBrowserWindow The web browser window to display in the new UI window.
	 * @param BrowserPopupFeatures The popup features and settings for the browser window.
	 * @return true if the UI window was created, false otherwise.
	 */
	bool RequestCreateWindow(const TSharedRef<IWebBrowserWindow>& NewBrowserWindow, const TSharedPtr<IWebBrowserPopupFeatures>& BrowserPopupFeatures);
	
	//bool SupportsCloseWindows();
	//bool RequestCloseWindow(const TSharedRef<IWebBrowserWindow>& BrowserWindow);


	/**
	 * Called once the browser begins closing procedures.
	 */
	void OnBrowserClosing();
	
	/**
	 * Called once the browser is closed.
	 */
	void OnBrowserClosed();

	/**
	 * Called to show the popup widget
	 *
 	 * @param PopupSize The size and location of the popup widget.
	 */
	void ShowPopup(CefRect PopupSize);

	/**
	 * Called to hide the popup widget again
	 */
	void HidePopup();

public:

	// Trigger an OnBeforePopup event chain
	bool OnCefBeforePopup(const CefString& Target_Url, const CefString& Target_Frame_Name);

	/**
	 * Gets the Cef Keyboard Modifiers based on a Key Event.
	 * 
	 * @param KeyEvent The Key event.
	 * @return Bits representing keyboard modifiers.
	 */
	static int32 GetCefKeyboardModifiers(const FKeyEvent& KeyEvent);

	/**
	 * Gets the Cef Mouse Modifiers based on a Mouse Event.
	 * 
	 * @param InMouseEvent The Mouse event.
	 * @return Bits representing mouse modifiers.
	 */
	static int32 GetCefMouseModifiers(const FPointerEvent& InMouseEvent);

	/**
	 * Gets the Cef Input Modifiers based on an Input Event.
	 * 
	 * @param InputEvent The Input event.
	 * @return Bits representing input modifiers.
	 */
	static int32 GetCefInputModifiers(const FInputEvent& InputEvent);

public:

	/**
	 * Called from the WebBrowserSingleton tick event. Should test wether the widget got a tick from Slate last frame and set the state to hidden if not.
	 */
	void CheckTickActivity();

	/**
	 * Called on every browser window when CEF launches a new render process.
	 * Used to ensure global JS objects are registered as soon as possible.
	 */
	CefRefPtr<CefDictionaryValue> GetProcessInfo();

private:
	/** Helper that calls WasHidden on the CEF host object when the value changes */
	void SetIsHidden(bool bValue);

	/** Used by the key down and up handlers to convert Slate key events to the CEF equivalent. */
	void PopulateCefKeyEvent(const FKeyEvent& InKeyEvent, CefKeyEvent& OutKeyEvent);

	/** Used to convert a FPointerEvent to a CefMouseEvent */
	CefMouseEvent GetCefMouseEvent(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent, bool bIsPopup);

private:

	/** Current state of the document being loaded. */
	EWebBrowserDocumentState DocumentState;

	/** Interface to the texture we are rendering to. */
	FSlateUpdatableTexture* UpdatableTextures[2];

	/** Pointer to the CEF Handler for this window. */
	CefRefPtr<FWebBrowserHandler> Handler;

	/** Pointer to the CEF Browser for this window. */
	CefRefPtr<CefBrowser> InternalCefBrowser;

	/** Current title of this window. */
	FString Title;

	/** Current Url of this window. */
	FString CurrentUrl;

	/** Current tool tip. */
	FString ToolTipText;

	/** Current size of this window. */
	FIntPoint ViewportSize;

	/** Whether this window is closing. */
	bool bIsClosing;

	/** Whether this window has been painted at least once. */
	bool bIsInitialized;

	/** Optional text to load as a web page. */
	TOptional<FString> ContentsToLoad;

	/** Delegate for broadcasting load state changes. */
	FOnDocumentStateChanged DocumentStateChangedEvent;

	/** Whether to show an error message in case of loading errors. */
	bool ShowErrorMessage;

	/** Whether to allow forward and back navigation via the mouse thumb buttons. */
	bool bThumbMouseButtonNavigation;

	/** Whether transparency is enabled. */
	bool bUseTransparency;

	/** Delegate for broadcasting title changes. */
	FOnTitleChanged TitleChangedEvent;

	/** Delegate for broadcasting address changes. */
	FOnUrlChanged UrlChangedEvent;

	/** Delegate for showing or hiding tool tips. */
	FOnToolTip ToolTipEvent;

	/** Delegate for notifying that the window needs refreshing. */
	FOnNeedsRedraw NeedsRedrawEvent;

	/** Delegate that is executed prior to browser navigation. */
	FOnBeforeBrowse BeforeBrowseDelegate;

	/** Delegate for overriding Url contents. */
	FOnLoadUrl LoadUrlDelegate;

	/** Delegate for notifying that a popup window is attempting to open. */
	FOnBeforePopupDelegate BeforePopupDelegate;
	
	/** Delegate for handaling requests to create new windows. */
	FOnCreateWindow CreateWindowDelegate;

	/** Delegate for handaling requests to close new windows that were created. */
	FOnCloseWindow CloseWindowDelegate;

	/** Delegate for handaling requests to show the popup menu. */
	FOnShowPopup ShowPopupEvent;

	/** Delegate for handaling requests to close new windows that were created. */
	FOnDismissPopup DismissPopupEvent;

	/** Tracks the current mouse cursor */
	EMouseCursor::Type Cursor;

	/** Tracks wether the widget is currently hidden or not*/
	bool bIsHidden;

	/** Used to detect when the widget is hidden*/
	bool bTickedLastFrame;

	/** Used for unhandled key events forwarding*/
	TOptional<FKeyEvent> PreviousKeyDownEvent;
	TOptional<FKeyEvent> PreviousKeyUpEvent;
	TOptional<FCharacterEvent> PreviousCharacterEvent;
	bool bIgnoreKeyDownEvent;
	bool bIgnoreKeyUpEvent;
	bool bIgnoreCharacterEvent;

	/** Used to ignore any popup menus when forwarding focus gained/lost events*/
	bool bMainHasFocus;
	bool bPopupHasFocus;

	FIntRect PopupRect;

	/** Handling of passing and marshalling messages for JS integration is delegated to a helper class*/
	TSharedPtr<FWebJSScripting> Scripting;
};

#endif
