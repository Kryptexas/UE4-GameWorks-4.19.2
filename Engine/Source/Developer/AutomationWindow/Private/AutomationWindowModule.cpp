// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AutomationWindowModule.cpp: Implements the FAutomationWindowModule class.
=============================================================================*/

#include "AutomationWindowPrivatePCH.h"


/**
 * Implements the AutomationWindow module.
 */
class FAutomationWindowModule
	: public IAutomationWindowModule
{
public:

	// Begin IAutomationWindowModule interface

	virtual TSharedRef<class SWidget> CreateAutomationWindow( const IAutomationControllerManagerRef& AutomationController, const ISessionManagerRef& SessionManager ) OVERRIDE
	{
		return SNew(SAutomationWindow, AutomationController, SessionManager);
	}

	virtual TWeakPtr<class SDockTab> GetAutomationWindowTab( ) OVERRIDE
	{
		return AutomationWindowTabPtr;
	}

	virtual FOnAutomationWindowModuleShutdown& OnShutdown( ) OVERRIDE
	{
		return ShutdownDelegate;
	}

	virtual void SetAutomationWindowTab(TWeakPtr<class SDockTab> AutomationWindowTab) OVERRIDE { AutomationWindowTabPtr = AutomationWindowTab; }

	// End IAutomationWindowModule interface

public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE
	{
	}

	virtual void ShutdownModule( ) OVERRIDE
	{
		ShutdownDelegate.ExecuteIfBound();	
	}

	// End IModuleInterface interface

private:

	// Holds the DockTab for the AutomationWindow
	TWeakPtr<class SDockTab> AutomationWindowTabPtr;

	// Holds FAutomationWindowModuleShutdownCallback
	FOnAutomationWindowModuleShutdown ShutdownDelegate;
};


IMPLEMENT_MODULE(FAutomationWindowModule, AutomationWindow);