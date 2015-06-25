// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SlateBasics.h"

enum class EWebBrowserDocumentState;
class IWebBrowserWindow;
class FWebBrowserViewport;
class UObject;

DECLARE_DELEGATE_TwoParams(FJSQueryResultDelegate, int, FString);
DECLARE_DELEGATE_RetVal_FourParams(bool, FOnJSQueryReceivedDelegate, int64, FString, bool, FJSQueryResultDelegate);
DECLARE_DELEGATE_OneParam(FOnJSQueryCanceledDelegate, int64);

DECLARE_DELEGATE_TwoParams(FJSQueryResultDelegate, int, FString);
DECLARE_DELEGATE_RetVal_FourParams(bool, FOnJSQueryReceivedDelegate, int64, FString, bool, FJSQueryResultDelegate);
DECLARE_DELEGATE_OneParam(FOnJSQueryCanceledDelegate, int64);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnBeforePopupDelegate, FString, FString);
DECLARE_DELEGATE_RetVal_OneParam(bool, FOnCreateWindowDelegate, const TWeakPtr<IWebBrowserWindow>&);

class WEBBROWSER_API SWebBrowser
	: public SCompoundWidget
{
public:
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnBeforeBrowse, const FString&, bool)
	DECLARE_DELEGATE_RetVal_ThreeParams(bool, FOnLoadUrl, const FString& /*Method*/, const FString& /*Url*/, FString& /* Response */)

	SLATE_BEGIN_ARGS(SWebBrowser)
		: _InitialURL(TEXT("www.google.com"))
		, _ShowControls(true)
		, _ShowAddressBar(false)
		, _ShowErrorMessage(true)
		, _SupportsTransparency(false)
		, _SupportsThumbMouseButtonNavigation(false)
		, _ViewportSize(FVector2D::ZeroVector)
	{ }

		/** A reference to the parent window. */
		SLATE_ARGUMENT(TSharedPtr<SWindow>, ParentWindow)

		/** URL that the browser will initially navigate to. */
		SLATE_ARGUMENT(FString, InitialURL)

		/** Optional string to load contents as a web page. */
		SLATE_ARGUMENT(TOptional<FString>, ContentsToLoad)

		/** Whether to show standard controls like Back, Forward, Reload etc. */
		SLATE_ARGUMENT(bool, ShowControls)

		/** Whether to show an address bar. */
		SLATE_ARGUMENT(bool, ShowAddressBar)

		/** Whether to show an error message in case of loading errors. */
		SLATE_ARGUMENT(bool, ShowErrorMessage)

		/** Should this browser window support transparency. */
		SLATE_ARGUMENT(bool, SupportsTransparency)

		/** Whether to allow forward and back navigation via the mouse thumb buttons. */
		SLATE_ARGUMENT(bool, SupportsThumbMouseButtonNavigation)

		/** Opaque background color used before a document is loaded and when no document color is specified. */
		SLATE_ARGUMENT(FColor, BackgroundColor)

		/** Desired size of the web browser viewport. */
		SLATE_ATTRIBUTE(FVector2D, ViewportSize);

		/** Called when document loading completed. */
		SLATE_EVENT(FSimpleDelegate, OnLoadCompleted)

		/** Called when document loading failed. */
		SLATE_EVENT(FSimpleDelegate, OnLoadError)

		/** Called when document loading started. */
		SLATE_EVENT(FSimpleDelegate, OnLoadStarted)

		/** Called when document title changed. */
		SLATE_EVENT(FOnTextChanged, OnTitleChanged)

		/** Called when the Url changes. */
		SLATE_EVENT(FOnTextChanged, OnUrlChanged)
	
		/** Called when a custom Javascript message is received from the browser process. */
		SLATE_EVENT(FOnJSQueryReceivedDelegate, OnJSQueryReceived)
	
		/** Called when a pending Javascript message has been canceled, either explicitly or by navigating away from the page containing the script. */
		SLATE_EVENT(FOnJSQueryCanceledDelegate, OnJSQueryCanceled)
		
		/** Called before a popup window happens */
		SLATE_EVENT(FOnBeforePopupDelegate, OnBeforePopup)

		/** Called when the browser requests the creation of a new window */
		SLATE_EVENT(FOnCreateWindowDelegate, OnCreateWindow)

		/** Called before browser navigation. */
		SLATE_EVENT(FOnBeforeBrowse, OnBeforeNavigation)
	
		/** Called to allow bypassing page content on load. */
		SLATE_EVENT(FOnLoadUrl, OnLoadUrl)
	SLATE_END_ARGS()


	/** Default constructor. */
	SWebBrowser();

	/**
	 * Construct the widget.
	 *
	 * @param InArgs  Declaration from which to construct the widget.
	 */
	void Construct(const FArguments& InArgs, const TSharedPtr<IWebBrowserWindow>& InWebBrowserWindow = nullptr);

	/**
	 * Load the specified URL.
	 * 
	 * @param NewURL New URL to load.
	 */
	void LoadURL(FString NewURL);

	/**
	* Load a string as data to create a web page.
	*
	* @param Contents String to load.
	* @param DummyURL Dummy URL for the page.
	*/
	void LoadString(FString Contents, FString DummyURL);

	/** Reload browser contents */
	void Reload();

	/** Get the current title of the web page. */
	FText GetTitleText() const;

	/**
	 * Gets the currently loaded URL.
	 *
	 * @return The URL, or empty string if no document is loaded.
	 */
	FString GetUrl() const;

	/**
	 * Gets the URL that appears in the address bar, this may not be the URL that is currently loaded in the frame.
	 *
	 * @return The address bar URL.
	 */
	FText GetAddressBarUrlText() const;

	/** Whether the document finished loading. */
	bool IsLoaded() const;

	/** Whether the document is currently being loaded. */
	bool IsLoading() const; 

	/** Execute javascript on the current window */
	void ExecuteJavascript(const FString& ScriptText);

	/** 
	 * Expose a UObject instance to the browser runtime.
	 * Properties and Functions will be accessible from JavaScript side.
	 * As all communication with the rendering procesis asynchronous, return values (both for properties and function results) are wrapped into JS Future objects.
	 *
	 * @param Name The name of the object. The object will show up as window.ue4.{Name} on the javascript side. If there is an existing object of the same name, this object will replace it. If bIsPermanent is false and there is an existing permanent binding, the permanent binding will be restored when the temporary one is removed.
	 * @param Object The object instance.
	 * @param bIsPermanent If true, the object will be visible to all pages loaded through this browser widget, otherwise, it will be deleted when navigating away from the current page. Non-permanent bindings should be registered from inside an OnLoadStarted event handler in order to be available before JS code starts loading.
	 */
	void BindUObject(const FString& Name, UObject* Object, bool bIsPermanent = true);

	/**
	 * Remove an existing script binding registered by BindUObject.
	 *
	 * @param Name The name of the object to remove.
	 * @param Object The object will only be removed if it is the same object as the one passed in.
	 * @param bIsPermanent Must match the bIsPermanent argument passed to BindUObject.
	 */
	void UnbindUObject(const FString& Name, UObject* Object, bool bIsPermanent = true);

private:

	/** Returns true if the browser can navigate backwards. */
	bool CanGoBack() const;

	/** Navigate backwards. */
	void GoBack();

	/** Returns true if the browser can navigate forwards. */
	bool CanGoForward() const;

	/** Navigate forwards. */
	void GoForward();

private:

	/** Navigate backwards. */
	FReply OnBackClicked();

	/** Navigate forwards. */
	FReply OnForwardClicked();

	/** Get text for reload button depending on status */
	FText GetReloadButtonText() const;

	/** Reload or stop loading */
	FReply OnReloadClicked();

	/** Invoked whenever text is committed in the address bar. */
	void OnUrlTextCommitted( const FText& NewText, ETextCommit::Type CommitType );

	/** Get whether the page viewport should be visible */
	EVisibility GetViewportVisibility() const;

	/** Get whether loading throbber should be visible */
	EVisibility GetLoadingThrobberVisibility() const;

	/** Callback for document loading state changes. */
	void HandleBrowserWindowDocumentStateChanged(EWebBrowserDocumentState NewState);

	/** Callback to tell slate we want to update the contents of the web view based on changes inside the view. */
	void HandleBrowserWindowNeedsRedraw();

	/** Callback for document title changes. */
	void HandleTitleChanged(FString NewTitle);

	/** Callback for loaded url changes. */
	void HandleUrlChanged(FString NewUrl);
	
	/** Callback for showing browser tool tips. */
	void HandleToolTip(FString ToolTipText);

	/** Callback for received JS queries. */
	bool HandleJSQueryReceived(int64 QueryId, FString QueryString, bool Persistent, FJSQueryResultDelegate ResultDelegate);

	/** Callback for cancelled JS queries. */
	void HandleJSQueryCanceled(int64 QueryId);

	/**
	 * A delegate that is executed prior to browser navigation.
	 *
	 * @return true if the navigation was handled an no further action should be taken by the browser, false if the browser should handle.
	 */
	bool HandleBeforeNavigation(const FString& Url, bool bIsRedirect);
	
	bool HandleLoadUrl(const FString& Method, const FString& Url, FString& OutResponse);

	/**
	 * A delegate that is executed when the browser requests window creation.
	 *
	 * @return true if if the window request was handled, false if the browser requesting the new window should be closed.
	 */
	bool HandleCreateWindow(const TWeakPtr<IWebBrowserWindow>& NewBrowserWindow);

	/** Callback for popup window permission */
	bool HandleBeforePopup(FString URL, FString Target);

private:

	/** Interface for dealing with a web browser window. */
	TSharedPtr<IWebBrowserWindow> BrowserWindow;

	/** Viewport interface for rendering the web page. */
	TSharedPtr<FWebBrowserViewport> BrowserViewport;

	/** The actual viewport widget. Required to update its tool tip property. */
	TSharedPtr<SViewport> ViewportWidget;

	/** The url that appears in the address bar which can differ from the url of the loaded page */
	FText AddressBarUrl;

	/** Editable text widget used for an address bar */
	TSharedPtr< SEditableTextBox > InputText;

	/** A delegate that is invoked when document loading completed. */
	FSimpleDelegate OnLoadCompleted;

	/** A delegate that is invoked when document loading failed. */
	FSimpleDelegate OnLoadError;

	/** A delegate that is invoked when document loading started. */
	FSimpleDelegate OnLoadStarted;

	/** A delegate that is invoked when document title changed. */
	FOnTextChanged OnTitleChanged;

	/** A delegate that is invoked when document address changed. */
	FOnTextChanged OnUrlChanged;
	
	/** A delegate that is invoked when render process Javascript code sends a query message to the client. */
	FOnJSQueryReceivedDelegate OnJSQueryReceived;
	
	/** A delegate that is invoked when render process cancels an ongoing query. Handler must clean up corresponding result delegate. */
	FOnJSQueryCanceledDelegate OnJSQueryCanceled;
	
	/** A delegate that is invoked when the browser attempts to pop up a new window */
	FOnBeforePopupDelegate OnBeforePopup;

	/** A delegate that is invoked when the browser requests a UI window for another browser it spawned */
	FOnCreateWindowDelegate OnCreateWindow;

	/** A delegate that is invoked prior to browser navigation */
	FOnBeforeBrowse OnBeforeNavigation;
	
	/** A delegate that is invoked when loading a resource, allowing the application to provide contents directly */
	FOnLoadUrl OnLoadUrl;
};
