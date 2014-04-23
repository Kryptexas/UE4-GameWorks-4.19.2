// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GenericApplication.h"
#include "LinuxWindow.h"

#define STEAM_CONTROLLER_SUPPORT				(WITH_ENGINE && !UE_SERVER)

typedef SDL_GameController* SDL_HController;

class FLinuxWindow;
class FGenericApplicationMessageHandler;


class FLinuxApplication : public GenericApplication
{

public:
	static FLinuxApplication* CreateLinuxApplication();

public:	
	virtual ~FLinuxApplication();
	
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
	//	
	virtual FModifierKeysState GetModifierKeys() const OVERRIDE;
	//	
	virtual FPlatformRect GetWorkArea( const FPlatformRect& CurrentWindow ) const OVERRIDE;
	//	X
	virtual bool TryCalculatePopupWindowPosition( const FPlatformRect& InAnchor, const FVector2D& InSize, const EPopUpOrientation::Type Orientation, /*OUT*/ FVector2D* const CalculatedPopUpPosition ) const OVERRIDE;
	//	
	virtual void GetDisplayMetrics( FDisplayMetrics& OutDisplayMetrics ) const OVERRIDE;

	void AddPendingEvent( SDL_Event event );

	void OnMouseCursorLock( bool bLockEnabled );

private:
	FLinuxApplication();

	TCHAR ConvertChar( SDL_Keysym Keysym );

	TSharedPtr< FLinuxWindow > FindEventWindow( SDL_Event *pEvent );

	void UpdateMouseCaptureWindow( SDL_HWindow TargetWindow );

	void ProcessDeferredMessage( SDL_Event Event );

private:

	struct SDLControllerState
	{
		SDL_HController controller;

		// We need to remember if the "button" was previously pressed so we don't generate extra events
		bool analogOverThreshold[10];
	};

// 	static const FIntPoint MinimizedWindowPosition;

	bool bUsingHighPrecisionMouseInput;

	TArray< SDL_Event > PendingEvents;

	TArray< TSharedRef< FLinuxWindow > > Windows;

	int32 bAllowedToDeferMessageProcessing;

	bool bIsMouseCursorLocked;
	bool bIsMouseCaptureEnabled;

	TSharedPtr< FLinuxWindow > LastEventWindow;

	SDL_HWindow MouseCaptureWindow;

	SDLControllerState *ControllerStates;

#if STEAM_CONTROLLER_SUPPORT
	TSharedPtr< class SteamControllerInterface > SteamInput;
#endif // STEAM_CONTROLLER_SUPPORT
};


extern FLinuxApplication* LinuxApplication;
