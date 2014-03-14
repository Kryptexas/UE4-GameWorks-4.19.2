// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionFrontendModule.cpp: Implements the FSessionFrontendModule class.
=============================================================================*/

#include "SessionFrontendPrivatePCH.h"


static const FName SessionFrontendTabName("SessionFrontend");


/**
 * Implements the SessionFrontend module.
 */
class FSessionFrontendModule
	: public ISessionFrontendModule
{
public:

	// Begin ISessionFrontendModule interface
	
	virtual TSharedRef<class SWidget> CreateSessionBrowser( const ISessionManagerRef& SessionManager ) OVERRIDE
	{
		return SNew(SSessionBrowser, SessionManager);
	}
	
	virtual TSharedRef<class SWidget> CreateSessionConsole( const ISessionManagerRef& SessionManager ) OVERRIDE
	{
		return SNew(SSessionConsole, SessionManager);
	}
	
	// End ISessionFrontendModule interface
	
public:

	// Begin IModuleInterface interface
	
	virtual void StartupModule( ) OVERRIDE
	{
		FGlobalTabmanager::Get()->RegisterTabSpawner(SessionFrontendTabName, FOnSpawnTab::CreateRaw(this, &FSessionFrontendModule::SpawnSessionFrontendTab))
			.SetDisplayName(NSLOCTEXT("FSessionFrontendModule", "FrontendTabTitle", "Session Frontend"))
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "SessionFrontEnd.TabIcon"));
	}

	virtual void ShutdownModule( ) OVERRIDE
	{
		FGlobalTabmanager::Get()->UnregisterTabSpawner(SessionFrontendTabName);
	}
	
	// End IModuleInterface interface

private:

	/**
	 * Creates a new session front-end tab.
	 *
	 * @param SpawnTabArgs - The arguments for the tab to spawn.
	 *
	 * @return The spawned tab.
	 */
	TSharedRef<SDockTab> SpawnSessionFrontendTab( const FSpawnTabArgs& SpawnTabArgs )
	{
		const TSharedRef<SDockTab> MajorTab = SNew(SDockTab)
			.TabRole(ETabRole::MajorTab);

		MajorTab->SetContent(SNew(SSessionFrontend, MajorTab, SpawnTabArgs.GetOwnerWindow()));

		return MajorTab;
	}
};


IMPLEMENT_MODULE(FSessionFrontendModule, SessionFrontend);
