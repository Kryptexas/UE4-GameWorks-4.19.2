// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SessionLauncherPrivatePCH.h"


static const FName SessionLauncherTabName("SessionLauncher");


/**
 * Implements the SessionLauncher module.
 */
class FSessionLauncherModule
	: public ISessionLauncherModule
{
public:

	// ISessionLauncherModule interface

	virtual TSharedRef<class SWidget> CreateLauncherProgressPanel( const ILauncherWorkerRef& LauncherWorker ) override
	{
		TSharedRef<SSessionLauncherProgress> Panel = SNew(SSessionLauncherProgress);

		Panel->SetLauncherWorker(LauncherWorker);

		return Panel;
	}

public:

	// IModuleInterface interface
	
	virtual void StartupModule( ) override
	{
		FGlobalTabmanager::Get()->RegisterTabSpawner(SessionLauncherTabName, FOnSpawnTab::CreateRaw(this, &FSessionLauncherModule::SpawnSessionLauncherTab))
			.SetDisplayName(NSLOCTEXT("FSessionLauncherModule", "LauncherTabTitle", "Game Launcher"))
			.SetTooltipText(NSLOCTEXT("FSessionLauncherModule", "LauncherTooltipText", "Open the Game Launcher tab."))
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "SessionLauncher.TabIcon"));
	}

	virtual void ShutdownModule( ) override
	{
		FGlobalTabmanager::Get()->UnregisterTabSpawner(SessionLauncherTabName);
	}

private:

	/**
	 * Creates a new session launcher tab.
	 *
	 * @param SpawnTabArgs The arguments for the tab to spawn.
	 * @return The spawned tab.
	 */
	TSharedRef<SDockTab> SpawnSessionLauncherTab( const FSpawnTabArgs& SpawnTabArgs )
	{
		const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
			.Icon(FEditorStyle::GetBrush("SessionLauncher.TabIcon"))
			.TabRole(ETabRole::MajorTab);

		ILauncherServicesModule& LauncherServicesModule = FModuleManager::LoadModuleChecked<ILauncherServicesModule>("LauncherServices");
		ITargetDeviceServicesModule& TargetDeviceServicesModule = FModuleManager::LoadModuleChecked<ITargetDeviceServicesModule>("TargetDeviceServices");

		FSessionLauncherModelRef Model = MakeShareable(new FSessionLauncherModel(
			TargetDeviceServicesModule.GetDeviceProxyManager(),
			LauncherServicesModule.CreateLauncher(),
			LauncherServicesModule.GetProfileManager()
		));

		DockTab->SetContent(SNew(SSessionLauncher, DockTab, SpawnTabArgs.GetOwnerWindow(), Model));

		return DockTab;
	}
};


IMPLEMENT_MODULE(FSessionLauncherModule, SessionLauncher);
