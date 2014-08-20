// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Core.h"
#include "GenericApplication.h"
#include "MacWindow.h"
#include "MacTextInputMethodSystem.h"

class FMacEvent;

/**
 * Mac-specific application implementation.
 */
class CORE_API FMacApplication : public GenericApplication
{

public:

	static FMacApplication* CreateMacApplication();


public:	

	~FMacApplication();


public:

	virtual void SetMessageHandler( const TSharedRef< class FGenericApplicationMessageHandler >& InMessageHandler ) override;

	virtual void PollGameDeviceState( const float TimeDelta ) override;

	virtual void PumpMessages( const float TimeDelta ) override;

	virtual TSharedRef< FGenericWindow > MakeWindow() override;

	virtual void InitializeWindow( const TSharedRef< FGenericWindow >& Window, const TSharedRef< FGenericWindowDefinition >& InDefinition, const TSharedPtr< FGenericWindow >& InParent, const bool bShowImmediately ) override;

	virtual void SetCapture( const TSharedPtr< FGenericWindow >& InWindow ) override;

	virtual void* GetCapture( void ) const override;

	virtual void SetHighPrecisionMouseMode( const bool Enable, const TSharedPtr< FGenericWindow >& InWindow ) override;

	virtual bool IsUsingHighPrecisionMouseMode() const override { return bUsingHighPrecisionMouseInput; }

	virtual bool IsUsingTrackpad() const override { return bUsingTrackpad; }

	virtual FModifierKeysState GetModifierKeys() const override;

	virtual FPlatformRect GetWorkArea( const FPlatformRect& CurrentWindow ) const override;

	virtual EWindowTitleAlignment::Type GetWindowTitleAlignment() const override
	{
		return EWindowTitleAlignment::Center;
	}

	virtual ITextInputMethodSystem *GetTextInputMethodSystem() override
	{
		return TextInputMethodSystem.Get();
	}

	void ProcessNSEvent(NSEvent* const Event, TSharedPtr< FMacWindow > CurrentEventWindow, FVector2D const MousePosition);

	void ProcessEvent(NSEvent* Event);

	void OnWindowDraggingFinished();

	bool IsWindowMovable(FCocoaWindow* Win, bool* OutMovableByBackground);

	void ResetModifierKeys() { ModifierKeysFlags = 0; }

	TSharedPtr<FMacWindow> GetKeyWindow();

	uint32 GetModifierKeysFlags() { return ModifierKeysFlags; }

	void UseMouseCaptureWindow(bool bUseMouseCaptureWindow);

#if WITH_EDITOR
    virtual void SendAnalytics(IAnalyticsProvider* Provider) override;

	void SetIsUsingTrackpad(bool bInUsingTrackpad)
	{
		bUsingTrackpad = bInUsingTrackpad;
	}
#endif


public:

	void OnDragEnter( FCocoaWindow* Window, void *InPasteboard );
	void OnDragOver( FCocoaWindow* Window );
	void OnDragOut( FCocoaWindow* Window );
	void OnDragDrop( FCocoaWindow* Window );

	void OnWindowDidBecomeKey( FCocoaWindow* Window );
	void OnWindowDidResignKey( FCocoaWindow* Window );
	void OnWindowWillMove( FCocoaWindow* Window );
	void OnWindowDidMove( FCocoaWindow* Window );
	void OnWindowDidResize( FCocoaWindow* Window );
	void OnWindowRedrawContents( FCocoaWindow* Window );
	void OnWindowDidClose( FCocoaWindow* Window );
	bool OnWindowDestroyed( FCocoaWindow* Window );

	void OnMouseCursorLock( bool bLockEnabled );

	static FCocoaWindow* FindMacEventWindow( NSEvent* CocoaEvent );
	static TSharedPtr< FMacWindow > FindMacWindowByNSWindow( FCocoaWindow* const WindowHandle );
	static void ProcessEvent(FMacEvent const* const Event);

private:
	static void OnDisplayReconfiguration(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags, void* UserInfo);

	FMacApplication();

	bool IsPrintableKey( uint32 Character );
	TCHAR ConvertChar( TCHAR Character );
	TCHAR TranslateCharCode( TCHAR CharCode, uint32 KeyCode );

	FCocoaWindow* FindEventWindow( NSEvent* CocoaEvent );
	TSharedPtr<FGenericWindow> LocateWindowUnderCursor( const FVector2D& CursorPos );

	NSScreen* FindScreenByPoint( int32 X, int32 Y ) const;

	void UpdateMouseCaptureWindow( FCocoaWindow* TargetWindow );

	void HandleModifierChange(TSharedPtr< FMacWindow > CurrentEventWindow, NSUInteger NewModifierFlags, NSUInteger FlagsShift, NSUInteger UE4Shift, EMacModifierKeys TranslatedCode);

#if WITH_EDITOR
	void RecordUsage(EGestureEvent::Type Gesture);
#else
	void RecordUsage(EGestureEvent::Type Gesture) { }
#endif

private:

	bool bUsingHighPrecisionMouseInput;

	bool bUsingTrackpad;

	FVector2D HighPrecisionMousePos;

	EMouseButtons::Type LastPressedMouseButton;

	FCriticalSection WindowsMutex;
	TArray< TSharedRef< FMacWindow > > Windows;

	TSharedRef< class HIDInputInterface > HIDInput;

	FCocoaWindow* DraggedWindow;

	FMouseCaptureWindow* MouseCaptureWindow;
	bool bIsMouseCaptureEnabled;
	bool bIsMouseCursorLocked;

	TSharedPtr< FMacWindow > LastEventWindow;

	/** The current set of modifier keys that are pressed. This is used to detect differences between left and right modifier keys on key up events*/
	uint32 ModifierKeysFlags;

	/** The current set of Cocoa modifier flags, used to detect when Mission Control has been invoked & returned so that we can synthesis the modifier events it steals */
	NSUInteger CurrentModifierFlags;

	TArray< TSharedRef< FMacWindow > > KeyWindows;

	TSharedPtr<FMacTextInputMethodSystem> TextInputMethodSystem;

	/** Notification center observer for application activation events */
	id AppActivationObserver;

	/** Notification center observer for application deactivation events */
	id AppDeactivationObserver;

#if WITH_EDITOR
	/** Holds the last gesture used to try and capture unique uses for gestures. */
	EGestureEvent::Type LastGestureUsed;

	/** Stores the number of times a gesture has been used for analytics */
	int32 GestureUsage[EGestureEvent::Count];
#endif

	void* EventMonitor;

	friend class FMacWindow;
};

extern FMacApplication* MacApplication;
