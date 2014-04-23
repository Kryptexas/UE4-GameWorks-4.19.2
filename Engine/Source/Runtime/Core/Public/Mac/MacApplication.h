// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Core.h"
#include "GenericApplication.h"
#include "MacWindow.h"
#include "MacTextInputMethodSystem.h"


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

	virtual void SetMessageHandler( const TSharedRef< class FGenericApplicationMessageHandler >& InMessageHandler ) OVERRIDE;

	virtual void PollGameDeviceState( const float TimeDelta ) OVERRIDE;

	virtual void PumpMessages( const float TimeDelta ) OVERRIDE;

	virtual void ProcessDeferredEvents( const float TimeDelta ) OVERRIDE;

	virtual TSharedRef< FGenericWindow > MakeWindow() OVERRIDE;

	virtual void InitializeWindow( const TSharedRef< FGenericWindow >& Window, const TSharedRef< FGenericWindowDefinition >& InDefinition, const TSharedPtr< FGenericWindow >& InParent, const bool bShowImmediately ) OVERRIDE;

	virtual void SetCapture( const TSharedPtr< FGenericWindow >& InWindow ) OVERRIDE;

	virtual void* GetCapture( void ) const OVERRIDE;

	virtual void SetHighPrecisionMouseMode( const bool Enable, const TSharedPtr< FGenericWindow >& InWindow ) OVERRIDE;

	virtual bool IsUsingHighPrecisionMouseMode() const OVERRIDE { return bUsingHighPrecisionMouseInput; }

	virtual bool IsUsingTrackpad() const OVERRIDE { return bUsingTrackpad; }

	virtual FModifierKeysState GetModifierKeys() const OVERRIDE;

	virtual FPlatformRect GetWorkArea( const FPlatformRect& CurrentWindow ) const OVERRIDE;

	virtual void GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const OVERRIDE;

	virtual ITextInputMethodSystem *GetTextInputMethodSystem() OVERRIDE
	{
		return TextInputMethodSystem.Get();
	}

	void AddPendingEvent( NSEvent* Event );

	void OnWindowDraggingFinished();

	bool IsWindowMovable(FSlateCocoaWindow* Win, bool* OutMovableByBackground);

	void ResetModifierKeys() { ModifierKeysFlags = 0; }

	TSharedPtr<FMacWindow> GetKeyWindow();

	uint32 GetModifierKeysFlags() { return ModifierKeysFlags; }

	void UseMouseCaptureWindow( bool bUseMouseCaptureWindow );

#if WITH_EDITOR
    virtual void SendAnalytics(IAnalyticsProvider* Provider) OVERRIDE;
#endif


public:

	void OnDragEnter( FSlateCocoaWindow* Window, void *InPasteboard );
	void OnDragOver( FSlateCocoaWindow* Window );
	void OnDragOut( FSlateCocoaWindow* Window );
	void OnDragDrop( FSlateCocoaWindow* Window );

	void OnWindowDidBecomeKey( FSlateCocoaWindow* Window );
	void OnWindowDidResignKey( FSlateCocoaWindow* Window );
	void OnWindowWillMove( FSlateCocoaWindow* Window );
	void OnWindowDidMove( FSlateCocoaWindow* Window );
	void OnWindowDidResize( FSlateCocoaWindow* Window );
	void OnWindowRedrawContents( FSlateCocoaWindow* Window );
	void OnWindowDidClose( FSlateCocoaWindow* Window );
	bool OnWindowDestroyed( FSlateCocoaWindow* Window );

	void OnMouseCursorLock( bool bLockEnabled );


private:
	static void OnDisplayReconfiguration(CGDirectDisplayID Display, CGDisplayChangeSummaryFlags Flags, void* UserInfo);

	FMacApplication();

	bool IsPrintableKey( uint32 Character );
	TCHAR ConvertChar( TCHAR Character );
	TCHAR TranslateCharCode( TCHAR CharCode, uint32 KeyCode );

	void ProcessEvent( NSEvent* Event );
	FSlateCocoaWindow* FindEventWindow( NSEvent* CocoaEvent );

	NSScreen* FindScreenByPoint( int32 X, int32 Y ) const;

	void UpdateMouseCaptureWindow( FSlateCocoaWindow* TargetWindow );

	void HandleExternallyChangedModifier(TSharedPtr< FMacWindow > CurrentEventWindow, NSUInteger NewModifierFlags, NSUInteger FlagsShift, NSUInteger UE4Shift, EMacModifierKeys TranslatedCode);

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

	TArray< TSharedRef< FMacWindow > > Windows;

	TArray< NSEvent* > PendingEvents;

	TSharedRef< class HIDInputInterface > HIDInput;

	FSlateCocoaWindow* DraggedWindow;

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

#if WITH_EDITOR
	/** Holds the last gesture used to try and capture unique uses for gestures. */
	EGestureEvent::Type LastGestureUsed;

	/** Stores the number of times a gesture has been used for analytics */
	int32 GestureUsage[EGestureEvent::Count];
#endif

	friend class FMacWindow;
};

extern FMacApplication* MacApplication;
