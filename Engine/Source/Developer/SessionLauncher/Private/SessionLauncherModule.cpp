// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SessionLauncherModule.cpp: Implements the FSessionLauncherModule class.
=============================================================================*/

#include "SessionLauncherPrivatePCH.h"


static const FName SessionLauncherTabName("SessionLauncher");


/**
 * Implements the SessionLauncher module.
 */
class FSessionLauncherModule
	: public ISessionLauncherModule
{
public:

	// Begin ISessionLauncherModule interface

	virtual TSharedRef<class SWidget> CreateLauncherProgressPanel( const ILauncherWorkerRef& LauncherWorker ) OVERRIDE
	{
		TSharedRef<SSessionLauncherProgress> Panel = SNew(SSessionLauncherProgress);

		Panel->SetLauncherWorker(LauncherWorker);

		return Panel;
	}

	// End ISessionLauncherModule interface

public:

	// Begin IModuleInterface interface
	
	virtual void StartupModule( ) OVERRIDE
	{
		FGlobalTabmanager::Get()->RegisterTabSpawner(SessionLauncherTabName, FOnSpawnTab::CreateRaw(this, &FSessionLauncherModule::SpawnSessionLauncherTab))
			.SetDisplayName(NSLOCTEXT("FSessionLauncherModule", "LauncherTabTitle", "Game Launcher"))
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "SessionLauncher.TabIcon"));
	}

	virtual void ShutdownModule( ) OVERRIDE
	{
		FGlobalTabmanager::Get()->UnregisterTabSpawner(SessionLauncherTabName);
	}
	
	// End IModuleInterface interface

private:

	/**
	 * Creates a new session launcher tab.
	 *
	 * @param SpawnTabArgs - The arguments for the tab to spawn.
	 *
	 * @return The spawned tab.
	 */
	TSharedRef<SDockTab> SpawnSessionLauncherTab( const FSpawnTabArgs& SpawnTabArgs )
	{
		const TSharedRef<SDockTab> MajorTab = SNew(SDockTab)
			.Icon(FEditorStyle::GetBrush("SessionLauncher.TabIcon"))
			.TabRole(ETabRole::MajorTab);

		ILauncherServicesModule& LauncherServicesModule = FModuleManager::LoadModuleChecked<ILauncherServicesModule>("LauncherServices");
		ITargetDeviceServicesModule& TargetDeviceServicesModule = FModuleManager::LoadModuleChecked<ITargetDeviceServicesModule>("TargetDeviceServices");

		FSessionLauncherModelRef Model = MakeShareable(new FSessionLauncherModel(
			TargetDeviceServicesModule.GetDeviceProxyManager(),
			LauncherServicesModule.CreateLauncher(),
			LauncherServicesModule.GetProfileManager()
		));

		MajorTab->SetContent(SNew(SSessionLauncher, MajorTab, SpawnTabArgs.GetOwnerWindow(), Model));

		return MajorTab;
	}
};


IMPLEMENT_MODULE(FSessionLauncherModule, SessionLauncher);
