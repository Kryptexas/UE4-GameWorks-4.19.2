// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateApplicationBase.h: Declares the FSlateApplicationBase class.
=============================================================================*/

#pragma once


/**
 * Interface for window title bars.
 */
class IWindowTitleBar
{
public:

	virtual void Flash( ) = 0;
};


/**
 * Base class for Slate applications.
 *
 * This class currently serves a temporary workaround for solving SlateCore dependencies to FSlateApplication.
 * It should probably be removed once FSlateApplication has been refactored into SlateCore.
 */
class SLATECORE_API FSlateApplicationBase
{
public:

	/**
	 * Gets the renderer being used to draw this application.
	 *
	 * @return The Slate renderer.
	 */
	TSharedPtr<FSlateRenderer> GetRenderer( ) const
	{
		return Renderer;
	}

public:

	/**
	 * Associates a top level Slate Window with a native window and ensures that it is tracked properly by the application.
	 * Calling this method will cause the window to be displayed (unless specified otherwise), so be sure to associate
	 * content with the window object you're passing in first!
	 *
	 * @param InSlateWindow A SlateWindow to which to add a native window.
	 * @param bShowImmediately true to show the window, false if you're going to call ShowWindow() yourself later.
	 *
	 * @return a reference to the SWindow that was just added.
	 */
	virtual TSharedRef<SWindow> AddWindow( TSharedRef<SWindow> InSlateWindow, const bool bShowImmediately = true ) = 0;

	/**
	 * Reorders an array of windows so the specified window is "brought to the front"
	 */
	virtual void ArrangeWindowToFrontVirtual( TArray<TSharedRef<SWindow>>& Windows, const TSharedRef<SWindow>& WindowToBringToFront ) = 0;

	/**
	 * @todo slate: Remove this method or make it private.
	 * Searches for the specified widget and generates a full path to it.  Note that this is
	 * a relatively slow operation!
	 * 
	 * @param  InWidget       Widget to generate a path to
	 * @param  OutWidgetPath  The generated widget path
	 * @param  VisibilityFilter	Widgets must have this type of visibility to be included the path
	 *
	 * @return	True if the widget path was found
	 */
	virtual bool FindPathToWidgetVirtual( const TArray<TSharedRef<SWindow>> WindowsToSearch, TSharedRef<const SWidget> InWidget, FWidgetPath& OutWidgetPath, EVisibility VisibilityFilter = EVisibility::Visible ) = 0;

	/**
	 * Gets the active top-level window.
	 *
	 * @return The top level window, or nullptr if no Slate windows are currently active.
	 */
	virtual TSharedPtr<SWindow> GetActiveTopLevelWindow() const = 0;

	/**
	 * Gets the global application icon.
	 *
	 * @return The icon.
	 */
	virtual const FSlateBrush* GetAppIcon( ) const = 0;

	/**
	 * Gets the ratio SlateUnit / ScreenPixel.
	 *
	 * @return Application scale.
	 */
	virtual float GetApplicationScale( ) const = 0;

	/**
	 * Gets Slate's current cached real time.
	 *
	 * This time value is updated every frame right before we tick widgets and is the recommended time value to use
	 * for UI animations and transitions, as opposed to calling FPlatformTime::Seconds() (which is generally slower)
	 *
	 * @return  The current Slate real time in seconds
	 */
	virtual const double GetCurrentTime( ) const = 0;

	/**
	 * Gets the last known position of the cursor.
	 *
	 * @return Cursor position.
	 */
	virtual FVector2D GetCursorPos( ) const = 0;

	/**
	 * Gets the size of the cursor..
	 *
	 * @return Cursor size.
	 */
	virtual FVector2D GetCursorSize( ) const = 0;

	/**
	 * Gets the application's display metrics.
	 *
	 * @param OutDisplayMetrics Will contain the display metrics.
	 */
	virtual void GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const = 0;

	/**
	 * Gets the widget that currently has keyboard focus, if any.
	 *
	 * @return The focused widget, or nullptr if no widget has focus.
	 */
	virtual TSharedPtr< SWidget > GetKeyboardFocusedWidget( ) const = 0;

	/** Gets the Widget that currently captures the mouse.
	 *
	 * @return The captor widget, or nullptr if no widget captured the mouse.
	 */
	virtual TSharedPtr< SWidget > GetMouseCaptor( ) const = 0;

	/**
	 * Gets the rectangle of the current preferred work area.
	 *
	 * @return Area rectangle.
	 */
	virtual FSlateRect GetPreferredWorkArea( ) const = 0;

	/**
	 * Checks whether the specified widget has any descendants which are currently focused.
	 *
	 * @param Widget The widget to check.
	 *
	 * @return true if any descendants are focused, false otherwise.
	 */
	virtual bool HasFocusedDescendants( const TSharedRef< const SWidget >& Widget ) const = 0;

	/**
	 * Checks whether an UI for external services such as Steam is open.
	 *
	 * @return true if an external UI is open, false otherwise.
	 */
	virtual bool IsExternalUIOpened( ) = 0;

	/** 
	 * Given the screen-space coordinate of the mouse cursor, searches for a string of widgets that are under the mouse.
	 *
	 * The widgets will be returned with the associated geometry. The first item will always
	 * be the top-level window while the last item will be the leaf-most widget.
	 *
	 * @return The path to the widget.
	 */
	virtual FWidgetPath LocateWindowUnderMouse( FVector2D ScreenspaceMouseCoordinate, const TArray< TSharedRef<SWindow > >& Windows, bool bIgnoreEnabledStatus = false ) = 0;

	/**
	 * Creates an image widget.
	 *
	 * @return The new image widget.
	 */
	virtual TSharedRef<SWidget> MakeImage( const TAttribute<const FSlateBrush*>& Image, const TAttribute<FSlateColor>& Color, const TAttribute<EVisibility>& Visibility ) const = 0;

	/**
	 * Creates a tool tip with the specified string.
	 *
	 * @param ToolTipString The string attribute to assign to the tool tip.
	 *
	 * @return The tool tip.
	 */
	virtual TSharedRef<IToolTip> MakeToolTip( const TAttribute<FString>& ToolTipString ) = 0;

	/**
	 * Creates a tool tip with the specified text.
	 *
	 * @param ToolTipText The text attribute to assign to the tool tip.
	 *
	 * @return The tool tip.
	 */
	virtual TSharedRef<IToolTip> MakeToolTip(const TAttribute<FText>& ToolTipText) = 0;

	/**
	 * Creates a tool tip with the specified text.
	 *
	 * @param ToolTipText The text to assign to the tool tip.
	 *
	 * @return The tool tip.
	 */
	virtual TSharedRef<IToolTip> MakeToolTip( const FText& ToolTipText ) = 0;

	/**
	 * Creates a title bar for the specified window.
	 *
	 * @param Window The window to create the title bar for.
	 * @param CenterContent Optional content for the title bar's center (will override window title).
	 * @param OutTitleBar Will hold a pointer to the title bar's interface.
	 *
	 * @return The new title bar widget.
	 */
	virtual TSharedRef<SWidget> MakeWindowTitleBar( const TSharedRef<SWindow>& Window, const TSharedPtr<SWidget>& CenterContent, TSharedPtr<IWindowTitleBar>& OutTitleBar ) const = 0;

	/**
	 * Destroying windows has implications on some OSs (e.g. destroying Win32 HWNDs can cause events to be lost).
	 *
	 * Slate strictly controls when windows are destroyed.
	 *
	 * @param WindowToDestroy The window to queue for destruction.
	 */
	virtual void RequestDestroyWindow( TSharedRef<SWindow> WindowToDestroy ) = 0;

	/**
	 * Sets keyboard focus to the specified widget.  The widget must be allowed to receive keyboard focus.
	 *
	 * @param  InWidget WidgetPath to the Widget to being focused
	 * @param InCause The reason that keyboard focus is changing
	 *
	 * @return true if the widget is now focused, false otherwise.
	 */
	virtual bool SetKeyboardFocus( const FWidgetPath& InFocusPath, const EKeyboardFocusCause::Type InCause ) = 0;

public:

	/**
	 * Returns the current instance of the application. The application should have been initialized before
	 * this method is called
	 *
	 * @return  Reference to the application
	 */
	static FSlateApplicationBase& Get( )
	{
		check(IsInGameThread());
		return *CurrentBaseApplication;
	}

	/**
	 * Returns true if a Slate application instance is currently initialized and ready
	 *
	 * @return  True if Slate application is initialized
	 */
	static bool IsInitialized( )
	{
		return CurrentBaseApplication.IsValid();
	}

protected:

	// Holds the Slate renderer used to render this application.
	TSharedPtr<FSlateRenderer> Renderer;

protected:

	// Holds a pointer to the current application.
	static TSharedPtr<FSlateApplicationBase> CurrentBaseApplication;
};
