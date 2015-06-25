// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once


struct FGeometry;
struct FKeyEvent;
struct FCharacterEvent;
struct FPointerEvent;
class FReply;
class FCursorReply;
class FSlateShaderResource;


enum class EWebBrowserDocumentState
{
	Completed,
	Error,
	Loading,
	NoDocument
};

DECLARE_DELEGATE_TwoParams(FJSQueryResultDelegate, int, FString);

/**
 * Interface for dealing with a Web Browser window
 */
class IWebBrowserWindow
{
public:

	/**
	 * Load the specified URL
	 *
	 * @param NewURL New URL to load
	 */
	virtual void LoadURL(FString NewURL) = 0;

	/**
	 * Load a string as data to create a web page
	 *
	 * @param Contents String to load
	 * @param DummyURL Dummy URL for the page
	 */
	virtual void LoadString(FString Contents, FString DummyURL) = 0;

	/**
	 * Set the desired size of the web browser viewport
	 * 
	 * @param WindowSize Desired viewport size
	 */
	virtual void SetViewportSize(FIntPoint WindowSize) = 0;

	/**
	 * Gets interface to the texture representation of the browser
	 *
	 * @return A slate shader resource that can be rendered
	 */
	virtual FSlateShaderResource* GetTexture() = 0;

	/**
	 * Checks whether the web browser is valid and ready for use
	 */
	virtual bool IsValid() const = 0;

	/**
	 * Checks whether the web browser has finished loaded the initial page.
	 */
	virtual bool IsInitialized() const = 0;

	/**
	 * Checks whether the web browser is currently being shut down
	 */
	virtual bool IsClosing() const = 0;

	/** Gets the loading state of the current document. */
	virtual EWebBrowserDocumentState GetDocumentLoadingState() const = 0;

	/**
	 * Gets the current title of the Browser page
	 */
	virtual FString GetTitle() const = 0;

	/**
	 * Gets the currently loaded URL.
	 *
	 * @return The URL, or empty string if no document is loaded.
	 */
	virtual FString GetUrl() const = 0;

	/**
	 * Notify the browser that a key has been pressed
	 *
	 * @param  InKeyEvent  Key event
	 */
	virtual bool OnKeyDown(const FKeyEvent& InKeyEvent) = 0;

	/**
	 * Notify the browser that a key has been released
	 *
	 * @param  InKeyEvent  Key event
	 */
	virtual bool OnKeyUp(const FKeyEvent& InKeyEvent) = 0;

	/**
	 * Notify the browser of a character event
	 *
	 * @param InCharacterEvent Character event
	 */
	virtual bool OnKeyChar(const FCharacterEvent& InCharacterEvent) = 0;

	/**
	 * Notify the browser that a mouse button was pressed within it
	 *
	 * @param MyGeometry The Geometry of the browser
	 * @param MouseEvent Information about the input event
	 * 
	 * @return FReply::Handled() if the mouse event was handled, FReply::Unhandled() oterwise
	 */
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) = 0;

	/**
	 * Notify the browser that a mouse button was released within it
	 *
	 * @param MyGeometry The Geometry of the browser
	 * @param MouseEvent Information about the input event
	 * 
	 * @return FReply::Handled() if the mouse event was handled, FReply::Unhandled() oterwise
	 */
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) = 0;

	/**
	 * Notify the browser of a double click event
	 *
	 * @param MyGeometry The Geometry of the browser
	 * @param MouseEvent Information about the input event
	 * 
	 * @return FReply::Handled() if the mouse event was handled, FReply::Unhandled() oterwise
	 */
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) = 0;

	/**
	 * Notify the browser that a mouse moved within it
	 *
	 * @param MyGeometry The Geometry of the browser
	 * @param MouseEvent Information about the input event
	 * 
	 * @return FReply::Handled() if the mouse event was handled, FReply::Unhandled() oterwise
	 */
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) = 0;

	/**
	 * Called when the mouse wheel is spun
	 *
	 * @param MyGeometry The Geometry of the browser
	 * @param MouseEvent Information about the input event
	 * 
	 * @return FReply::Handled() if the mouse event was handled, FReply::Unhandled() oterwise
	 */
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) = 0;

	/**
	 * The system asks each widget under the mouse to provide a cursor. This event is bubbled.
	 * 
	 * @return FCursorReply::Unhandled() if the event is not handled; return FCursorReply::Cursor() otherwise.
	 */
	virtual FCursorReply OnCursorQuery(const FGeometry& MyGeometry, const FPointerEvent& CursorEvent) = 0;

	/**
	 * Called when browser receives/loses focus
	 */
	virtual void OnFocus(bool SetFocus) = 0;

	/** Called when Capture lost */
	virtual void OnCaptureLost() = 0;

	/**
	 * Returns true if the browser can navigate backwards.
	 */
	virtual bool CanGoBack() const = 0;

	/** Navigate backwards. */
	virtual void GoBack() = 0;

	/**
	 * Returns true if the browser can navigate forwards.
	 */
	virtual bool CanGoForward() const = 0;

	/** Navigate forwards. */
	virtual void GoForward() = 0;

	/**
	 * Returns true if the browser is currently loading.
	 */
	virtual bool IsLoading() const = 0;

	/** Reload the current page. */
	virtual void Reload() = 0;

	/** Stop loading the page. */
	virtual void StopLoad() = 0;

	/** Execute Javascript on the page. */
	virtual void ExecuteJavascript(const FString& Script) = 0;
	
	/** Close this window so that it can no longer be used. */
	virtual void CloseBrowser() = 0;

	/** 
	 * Expose a UObject instance to the browser runtime.
	 * Properties and Functions will be accessible from JavaScript side.
	 * As all communication with the rendering procesis asynchronous, return values (both for properties and function results) are wrapped into JS Future objects.
	 *
	 * @param Name The name of the object. The object will show up as window.ue4.{Name} on the javascript side. If there is an existing object of the same name, this object will replace it. If bIsPermanent is false and there is an existing permanent binding, the permanent binding will be restored when the temporary one is removed.
	 * @param Object The object instance.
	 * @param bIsPermanent If true, the object will be visible to all pages loaded through this browser widget, otherwise, it will be deleted when navigating away from the current page. Non-permanent bindings should be registered from inside an OnLoadStarted event handler in order to be available before JS code starts loading.
	 */
	virtual void BindUObject(const FString& Name, UObject* Object, bool bIsPermanent = true) = 0;

	/**
	 * Remove an existing script binding registered by BindUObject.
	 *
	 * @param Name The name of the object to remove.
	 * @param Object The object will only be removed if it is the same object as the one passed in.
	 * @param bIsPermanent Must match the bIsPermanent argument passed to BindUObject.
	 */
	virtual void UnbindUObject(const FString& Name, UObject* Object, bool bIsPermanent = true) = 0;

public:

	/** A delegate that is invoked when the loading state of a document changed. */
	DECLARE_EVENT_OneParam(IWebBrowserWindow, FOnDocumentStateChanged, EWebBrowserDocumentState /*NewState*/);
	virtual FOnDocumentStateChanged& OnDocumentStateChanged() = 0;

	/** A delegate to allow callbacks when a browser title changes. */
	DECLARE_EVENT_OneParam(IWebBrowserWindow, FOnTitleChanged, FString /*NewTitle*/);
	virtual FOnTitleChanged& OnTitleChanged() = 0;

	/** A delegate to allow callbacks when a frame url changes. */
	DECLARE_EVENT_OneParam(IWebBrowserWindow, FOnUrlChanged, FString /*NewUrl*/);
	virtual FOnUrlChanged& OnUrlChanged() = 0;

	/** A delegate to allow callbacks when a frame url changes. */
	DECLARE_EVENT_OneParam(IWebBrowserWindow, FOnToolTip, FString /*ToolTipText*/);
	virtual FOnToolTip& OnToolTip() = 0;

	/** A delegate that is invoked when the off-screen window has been repainted and requires an update. */
	DECLARE_EVENT(IWebBrowserWindow, FOnNeedsRedraw)
	virtual FOnNeedsRedraw& OnNeedsRedraw() = 0;
	
	/** A delegate that is invoked when JS code sends a query to the front end. The result delegate cand either be executed immediately or saved and executed later (multiple times if the boolean peristent argument is true). 
	 * The arguments are an integer error code (0 for success) and a reply string. If you need pass more complex data to the JS code, you will have to pack the data in some way (such as JSON encoding it). */
	DECLARE_DELEGATE_RetVal_FourParams(bool, FONJSQueryReceived, int64, FString, bool, FJSQueryResultDelegate)
	virtual FONJSQueryReceived& OnJSQueryReceived() = 0;

	/** A delegate that is invoked when an outstanding query is canceled. Implement this if you are saving delegates passed to OnQueryReceived. */
	DECLARE_DELEGATE_OneParam(FONJSQueryCanceled, int64)
	virtual FONJSQueryCanceled& OnJSQueryCanceled() = 0;
	
	/** A delegate that is invoked prior to browser navigation. */
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnBeforeBrowse, const FString& /*Url*/, bool /*bIsRedirect*/)
	virtual FOnBeforeBrowse& OnBeforeBrowse() = 0;
	
	/** A delegate that is invoked to allow user code to override the contents of a Url. */
	DECLARE_DELEGATE_RetVal_ThreeParams(bool, FOnLoadUrl, const FString& /*Method*/, const FString& /*Url*/, FString& /*OutBody*/)
	virtual FOnLoadUrl& OnLoadUrl() = 0;

	/** A delegate that is invoked when a popup window is attempting to open. */
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnBeforePopupDelegate, FString, FString);
	virtual FOnBeforePopupDelegate& OnBeforePopup() = 0;

	/** A delegate this is invoked when an existing browser requests creation of a new browser window. */
	DECLARE_DELEGATE_RetVal_OneParam(bool, FOnCreateWindow, const TWeakPtr<IWebBrowserWindow>& /*NewBrowserWindow*/)
	virtual FOnCreateWindow& OnCreateWindow() = 0;

protected:

	/** Virtual Destructor. */
	virtual ~IWebBrowserWindow() { };
};
