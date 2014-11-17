// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SessionFrontendPrivatePCH.h"


static const FName SessionFrontendTabName("SessionFrontend");


/**
 * Implements the SessionFrontend module.
 */
class FSessionFrontendModule
	: public ISessionFrontendModule
{
public:

	// ISessionFrontendModule interface
	
	virtual TSharedRef<class SWidget> CreateSessionBrowser( const ISessionManagerRef& SessionManager ) override
	{
		return SNew(SSessionBrowser, SessionManager);
	}
	
	virtual TSharedRef<class SWidget> CreateSessionConsole( const ISessionManagerRef& SessionManager ) override
	{
		return SNew(SSessionConsole, SessionManager);
	}
	
public:

	// IModuleInterface interface
	
	virtual void StartupModule( ) override
	{
		FGlobalTabmanager::Get()->RegisterTabSpawner(SessionFrontendTabName, FOnSpawnTab::CreateRaw(this, &FSessionFrontendModule::SpawnSessionFrontendTab))
			.SetDisplayName(NSLOCTEXT("FSessionFrontendModule", "FrontendTabTitle", "Session Frontend"))
			.SetTooltipText(NSLOCTEXT("FSessionFrontendModule", "FrontendTooltipText", "Open the Session Frontend tab."))
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "SessionFrontEnd.TabIcon"));
	}

	virtual void ShutdownModule( ) override
	{
		FGlobalTabmanager::Get()->UnregisterTabSpawner(SessionFrontendTabName);
	}

private:

	/**
	 * Creates a new session front-end tab.
	 *
	 * @param SpawnTabArgs - The arguments for the tab to spawn.
	 * @return The spawned tab.
	 */
	TSharedRef<SDockTab> SpawnSessionFrontendTab( const FSpawnTabArgs& SpawnTabArgs )
	{
		const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
			.TabRole(ETabRole::MajorTab);

		DockTab->SetContent(SNew(SSessionFrontend, DockTab, SpawnTabArgs.GetOwnerWindow()));

		return DockTab;
	}
};


IMPLEMENT_MODULE(FSessionFrontendModule, SessionFrontend);
