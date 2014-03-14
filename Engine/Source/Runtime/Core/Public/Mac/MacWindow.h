// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericWindow.h"
#include "SharedPointer.h"

#ifdef __OBJC__

/**
 * Custom window class used for input handling
 */
@interface FSlateCocoaWindow : NSWindow <NSWindowDelegate>
{
	NSWindow* Parent;
	EWindowMode::Type WindowMode;
	NSMutableArray* ChildWindows;
	bool bAcceptsInput;
	bool bRoundedCorners;
	bool bDisplayReconfiguring;
	bool bDeferOrderFront;
	CGFloat bDeferOpacity;
	bool bRenderInitialised;
@public
	bool bZoomed;
}

/** Get the frame filled by a child OpenGL view, which may cover the window or fill the content view depending upon the window style.
 @return The NSRect for a child OpenGL view. */
- (NSRect)openGLFrame;

- (void)performDeferredOrderFront;

/** Set whether the window should display with rounded corners. */
- (void)setRoundedCorners:(bool)bUseRoundedCorners;

/** Get whether the window should display with rounded corners. 
 @return True when window corners should be rounded, else false. */
- (bool)roundedCorners;

/** Lets window know if its owner (SWindow) accepts input */
- (void)setAcceptsInput:(bool)InAcceptsInput;

/** Redraws window's contents. */
- (void)redrawContents;

/** Set the initial window mode. */
- (void)setWindowMode:(EWindowMode::Type)WindowMode;

/**	@return The current mode for this Cocoa window. */
- (EWindowMode::Type)windowMode;

/** Mutator that specifies that the display arrangement is being reconfigured when bIsDisplayReconfiguring is true. */
- (void)setDisplayReconfiguring:(bool)bIsDisplayReconfiguring;

/** Record the parent window separately from Cocoa so that parented windows can still be minimised without hiding the app. */
- (void)setParent:(NSWindow*)InParent;

/** Cached current child windows & disconnect them so the window can be moved independently. */
- (void)disconnectChildWindows;

/** Order window to the front, on top of sibling windows if it is a child. */
- (void)orderFrontEvenIfChildAndMakeMain:(bool)bMain andKey:(bool)bKey;

/** Reattach cached child windows. */
- (void)reconnectChildWindows;

/** Returns true if it is permissable to add a child window, false if it will start being dragged around with the parent. */
- (bool)shouldAddChildWindows;

/** Get the cached parent window, regardless of whether it is attached. */
- (NSWindow*)getParent;

/** Remove InChild from any cached list of children. */
- (void)removeCachedChild:(NSWindow*)InChild;

@end

/**
 * Custom window class used for mouse capture
 */
@interface FMouseCaptureWindow : NSWindow <NSWindowDelegate>
{
	FSlateCocoaWindow*	TargetWindow;
}

- (id)initWithTargetWindow: (FSlateCocoaWindow*)Window;
- (FSlateCocoaWindow*)targetWindow;
- (void)setTargetWindow: (FSlateCocoaWindow*)Window;

@end

#else // __OBJC__

class FSlateCocoaWindow;
class FMouseCaptureWindow;
class NSWindow;
class NSEvent;
class NSScreen;

#endif // __OBJC__

/**
 * A platform specific implementation of FNativeWindow.
 * Native Windows provide platform-specific backing for and are always owned by an SWindow.
 */
class CORE_API FMacWindow : public FGenericWindow, public TSharedFromThis<FMacWindow>
{
public:
	~FMacWindow();

	/** Create a new FMacWindow.
	 *
	 * @param SlateWindows		List of all top level Slate windows.  This function will add the owner window to this list.
	 * @param OwnerWindow		The SlateWindow for which we are crating a backing Win32Window
	 * @param InHInstance		Win32 application instance handle
	 * @param InParent			Parent Win32 window; usually NULL.
	 * @param bShowImmediately	True to show this window as soon as its initialized
	 */
	static TSharedRef< FMacWindow > Make();

	FSlateCocoaWindow* GetWindowHandle() const;

	void Initialize( class FMacApplication* const Application, const TSharedRef< FGenericWindowDefinition >& InDefinition, const TSharedPtr< FMacWindow >& InParent, const bool bShowImmediately );
	
	void OnDisplayReconfiguration(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags);

public:

	virtual void ReshapeWindow( int32 X, int32 Y, int32 Width, int32 Height ) OVERRIDE;

	virtual bool GetFullScreenInfo( int32& X, int32& Y, int32& Width, int32& Height ) const OVERRIDE;

	virtual void MoveWindowTo ( int32 X, int32 Y ) OVERRIDE;

	virtual void BringToFront( bool bForce = false ) OVERRIDE;

	virtual void Destroy() OVERRIDE;

	virtual void Minimize() OVERRIDE;

	virtual void Maximize() OVERRIDE;

	virtual void Restore() OVERRIDE;

	virtual void Show() OVERRIDE;

	virtual void Hide() OVERRIDE;

	virtual void SetWindowMode( EWindowMode::Type NewWindowMode ) OVERRIDE;

	virtual EWindowMode::Type GetWindowMode() const OVERRIDE { return WindowMode; } 

	virtual bool IsMaximized() const OVERRIDE;

	virtual bool IsVisible() const OVERRIDE;

	virtual bool GetRestoredDimensions(int32& X, int32& Y, int32& Width, int32& Height) OVERRIDE;

	virtual void SetWindowFocus() OVERRIDE;

	virtual void SetOpacity( const float InOpacity ) OVERRIDE;

	virtual void Enable( bool bEnable ) OVERRIDE;

	virtual bool IsPointInWindow( int32 X, int32 Y ) const OVERRIDE;

	virtual int32 GetWindowBorderSize() const OVERRIDE;

	virtual void* GetOSWindowHandle() const  OVERRIDE { return WindowHandle; }

	virtual bool IsForegroundWindow() const OVERRIDE;

	virtual void SetText(const TCHAR* const Text) OVERRIDE;

	virtual void AdjustCachedSize( FVector2D& Size ) const OVERRIDE;

	/**
	 * Sets the window text - usually the title but can also be text content for things like controls
	 *
	 * @param Text	The window's title or content text
	 */
	bool IsRegularWindow() const;

	int32 PositionX;
	int32 PositionY;


private:

	/**
	 * Protect the constructor; only TSharedRefs of this class can be made.
	 */
	FMacWindow();


private:

	FMacApplication* OwningApplication;

	/** Mac window handle */
	FSlateCocoaWindow* WindowHandle;
	
	/** The mode that the window is in (windowed, fullscreen, windowedfullscreen ) */
	EWindowMode::Type WindowMode;

	RECT PreFullscreenWindowRect;

	bool bIsVisible : 1;
};
