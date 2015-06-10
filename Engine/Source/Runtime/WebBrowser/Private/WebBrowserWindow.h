// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_CEF3

#include "IWebBrowserWindow.h"
#include "WebBrowserHandler.h"

#if PLATFORM_WINDOWS
	#include "AllowWindowsPlatformTypes.h"
#endif

#include "include/internal/cef_ptr.h"
#include "include/cef_render_handler.h"

#if PLATFORM_WINDOWS
	#include "HideWindowsPlatformTypes.h"
#endif


struct FInputEvent;
class FSlateRenderer;
class FSlateUpdatableTexture;


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
	FWebBrowserWindow(FIntPoint ViewportSize, FString URL, TOptional<FString> ContentsToLoad, bool ShowErrorMessage);

	/** Virtual Destructor. */
	virtual ~FWebBrowserWindow();

public:

	/**
	* Set the CEF Handler receiving browser callbacks for this window.
	*
	* @param InHandler Pointer to the handler for this window.
	*/
	void SetHandler(CefRefPtr<FWebBrowserHandler> InHandler);

	/** Close this window so that it can no longer be used. */
	void CloseBrowser();

public:

	// IWebBrowserWindow Interface

	virtual void LoadURL(FString NewURL) override;
	virtual void LoadString(FString Contents, FString DummyURL) override;
	virtual void SetViewportSize(FIntPoint WindowSize) override;
	virtual FSlateShaderResource* GetTexture() override;
	virtual bool IsValid() const override;
	virtual bool HasBeenPainted() const override;
	virtual bool IsClosing() const override;
	virtual EWebBrowserDocumentState GetDocumentLoadingState() const override;
	virtual FString GetTitle() const override;
	virtual FString GetUrl() const override;
	virtual void OnKeyDown(const FKeyEvent& InKeyEvent) override;
	virtual void OnKeyUp(const FKeyEvent& InKeyEvent) override;
	virtual void OnKeyChar(const FCharacterEvent& InCharacterEvent) override;
	virtual void OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual void OnFocus(bool SetFocus) override;
	virtual void OnCaptureLost() override;
	virtual bool CanGoBack() const override;
	virtual void GoBack() override;
	virtual bool CanGoForward() const override;
	virtual void GoForward() override;
	virtual bool IsLoading() const override;
	virtual void Reload() override;
	virtual void StopLoad() override;

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

	DECLARE_DERIVED_EVENT(FWebBrowserWindow, IWebBrowserWindow::FOnNeedsRedraw, FOnNeedsRedraw);
	virtual FOnNeedsRedraw& OnNeedsRedraw() override
	{
		return NeedsRedrawEvent;
	}

private:

	/**
	 * Called to pass reference to the underlying CefBrowser as this is not created at the same time
	 * as the FWebBrowserWindow.
	 *
	 * @param Browser The CefBrowser for this window.
	 */
	void BindCefBrowser(CefRefPtr<CefBrowser> Browser);

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
	void SetUrl(const CefString& Url);

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
	void OnCursorChange(CefCursorHandle Cursor);

public:

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

private:

	/** Current state of the document being loaded. */
	EWebBrowserDocumentState DocumentState;

	/** Interface to the texture we are rendering to. */
	FSlateUpdatableTexture* UpdatableTexture;

	/** Temporary storage for the raw texture data. */
	TArray<uint8> TextureData;

    /** Wether the texture data contain updates that have not been copied to the UpdatableTexture yet. */
    bool bTextureDataDirty;

	/** Pointer to the CEF Handler for this window. */
	CefRefPtr<FWebBrowserHandler> Handler;

	/** Pointer to the CEF Browser for this window. */
	CefRefPtr<CefBrowser> InternalCefBrowser;

	/** Current title of this window. */
	FString Title;

	/** Current Url of this window. */
	FString CurrentUrl;

	/** Current size of this window. */
	FIntPoint ViewportSize;

	/** Whether this window is closing. */
	bool bIsClosing;

	/** Whether this window has been painted at least once. */
	bool bHasBeenPainted;

	/** Optional text to load as a web page. */
	TOptional<FString> ContentsToLoad;

	/** Delegate for broadcasting load state changes. */
	FOnDocumentStateChanged DocumentStateChangedEvent;

	/** Whether to show an error message in case of loading errors. */
	bool ShowErrorMessage;

	/** Delegate for broadcasting title changes. */
	FOnTitleChanged TitleChangedEvent;

	/** Delegate for broadcasting address changes. */
	FOnUrlChanged UrlChangedEvent;

	/** Delegate for notifying that the window needs refreshing. */
	FOnNeedsRedraw NeedsRedrawEvent;
};


#endif
