// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericApplication.h"

#include "AllowWindowsPlatformTypes.h"
	#include "Ole2.h"
	#include "OleIdl.h"
#include "HideWindowsPlatformTypes.h"
#include "IForceFeedbackSystem.h"
#include "WindowsTextInputMethodSystem.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWindowsDesktop, Log, All);

class FWindowsWindow;
class FGenericApplicationMessageHandler;

struct FDeferredWindowsMessage
{
	FDeferredWindowsMessage( const TSharedPtr<FWindowsWindow>& InNativeWindow, HWND InHWnd, uint32 InMessage, WPARAM InWParam, LPARAM InLParam, int32 InX=0, int32 InY=0, uint32 InRawInputFlags = 0 )
		: NativeWindow( InNativeWindow )
		, hWND( InHWnd )
		, Message( InMessage )
		, wParam( InWParam )
		, lParam( InLParam )
		, X( InX )
		, Y( InY )
		, RawInputFlags( InRawInputFlags )
	{}
	/** Native window that received the message */
	TWeakPtr<FWindowsWindow> NativeWindow;
	/** Window handle */
	HWND hWND;
	/** Message code */
	uint32 Message;
	/** Message data */
	WPARAM wParam;
	LPARAM lParam;
	/** Mouse coordinates */
	int32 X;
	int32 Y;
	uint32 RawInputFlags;
};

/**
 * Windows-specific application implementation.
 */
class FWindowsApplication : public GenericApplication, IForceFeedbackSystem
{

public:

	/**
	 * Static: Creates a new Win32 application
	 *
	 * @param	InstanceHandle  Win32 instance handle
	 * @param	IconHandle		Win32 application icon handle
	 *
	 * @return  New application object
	 */
	static FWindowsApplication* CreateWindowsApplication( const HINSTANCE InstanceHandle, const HICON IconHandle );


public:	

	virtual ~FWindowsApplication();
	
	virtual void DestroyApplication() OVERRIDE;


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

	virtual FModifierKeysState GetModifierKeys() const OVERRIDE;

	virtual FPlatformRect GetWorkArea( const FPlatformRect& CurrentWindow ) const OVERRIDE;

	virtual bool TryCalculatePopupWindowPosition( const FPlatformRect& InAnchor, const FVector2D& InSize, const EPopUpOrientation::Type Orientation, /*OUT*/ FVector2D* const CalculatedPopUpPosition ) const OVERRIDE;

	virtual void GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const OVERRIDE;

	virtual void GetInitialDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const OVERRIDE;

#if WITH_EDITOR
	virtual bool SupportsSourceAccess() const OVERRIDE;

	virtual void GotoLineInSource(const FString& FileAndLineNumber) OVERRIDE;
#endif

	virtual IForceFeedbackSystem *GetForceFeedbackSystem() OVERRIDE 
	{
		return this; 
	}

	virtual void SetChannelValue (int32 ControllerId, FForceFeedbackChannelType ChannelType, float Value) OVERRIDE;

	virtual void SetChannelValues (int32 ControllerId, const FForceFeedbackValues &Values) OVERRIDE;

	virtual ITextInputMethodSystem *GetTextInputMethodSystem() OVERRIDE
	{
		return TextInputMethodSystem.Get();
	}

protected:

	static LRESULT CALLBACK AppWndProc(HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam);

	int32 ProcessMessage( HWND hwnd, uint32 msg, WPARAM wParam, LPARAM lParam );

	int32 ProcessDeferredMessage( const FDeferredWindowsMessage& DeferredMessage );

public:

	/** Invoked by a window when an OLE Drag and Drop first enters it. */
	HRESULT OnOLEDragEnter( const HWND HWnd, IDataObject *DataObjectPointer, ::DWORD KeyState, POINTL CursorPosition, ::DWORD *CursorEffect);

	/** Invoked by a window when an OLE Drag and Drop moves over the window. */
	HRESULT OnOLEDragOver( const HWND HWnd, ::DWORD KeyState, POINTL CursorPosition, ::DWORD *CursorEffect);

	/** Invoked by a window when an OLE Drag and Drop exits the window. */
	HRESULT OnOLEDragOut( const HWND HWnd );

	/** Invoked by a window when an OLE Drag and Drop is dropped onto the window. */
	HRESULT OnOLEDrop( const HWND HWnd, IDataObject *DataObjectPointer, ::DWORD KeyState, POINTL CursorPosition, ::DWORD *CursorEffect);


private:

	FWindowsApplication( const HINSTANCE HInstance, const HICON IconHandle );

	/** Registers the Windows class for windows and assigns the application instance and icon */
	static bool RegisterClass( const HINSTANCE HInstance, const HICON HIcon );

	/**  @return  True if a windows message is related to user input (mouse, keyboard) */
	static bool IsInputMessage( uint32 msg );

	void DeferMessage( TSharedPtr<FWindowsWindow>& NativeWindow, HWND InHWnd, uint32 InMessage, WPARAM InWParam, LPARAM InLParam, int32 MouseX = 0, int32 MouseY = 0, uint32 RawInputFlags = 0 );

	void CheckForShiftUpEvents(const int32 KeyCode);

private:

	static const FIntPoint MinimizedWindowPosition;

	HINSTANCE InstanceHandle;

	bool bUsingHighPrecisionMouseInput;

	TArray< FDeferredWindowsMessage > DeferredMessages;

	TArray< TSharedRef< FWindowsWindow > > Windows;

	TSharedRef< class XInputInterface > XInput;

	/** List of input devices implemented in external modules. */
	TArray< TSharedPtr<class IInputDevice> > ExternalInputDevices;
	bool bHasLoadedInputPlugins;

	TArray< int32 > PressedModifierKeys;

	FAutoConsoleVariableRef CVarDeferMessageProcessing;

	int32 bAllowedToDeferMessageProcessing;
	
	/** True if we are in the middle of a windows modal size loop */
	bool bInModalSizeLoop;

	FDisplayMetrics InitialDisplayMetrics;

	TSharedPtr<FWindowsTextInputMethodSystem> TextInputMethodSystem;
};