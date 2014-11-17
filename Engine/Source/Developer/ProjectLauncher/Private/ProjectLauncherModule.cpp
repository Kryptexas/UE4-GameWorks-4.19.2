// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "ProjectLauncherPrivatePCH.h"


static const FName ProjectLauncherTabName("ProjectLauncher");


/**
 * Implements the SessionSProjectLauncher module.
 */
class FProjectLauncherModule
	: public IProjectLauncherModule
{
public:

	// IProjectLauncherModule interface

	virtual TSharedRef<class SWidget> CreateSProjectLauncherProgressPanel( const ILauncherWorkerRef& LauncherWorker ) override
	{
		TSharedRef<SProjectLauncherProgress> Panel = SNew(SProjectLauncherProgress);

		Panel->SetLauncherWorker(LauncherWorker);

		return Panel;
	}

public:

	// IModuleInterface interface
	
	virtual void StartupModule( ) override
	{
		FGlobalTabmanager::Get()->RegisterTabSpawner(ProjectLauncherTabName, FOnSpawnTab::CreateRaw(this, &FProjectLauncherModule::SpawnProjectLauncherTab))
			.SetDisplayName(NSLOCTEXT("FProjectLauncherModule", "ProjectLauncherTabTitle", "Project Launcher"))
			.SetTooltipText(NSLOCTEXT("FProjectLauncherModule", "ProjectLauncherTooltipText", "Open the Project Launcher tab."))
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "Launcher.TabIcon"));
	}

	virtual void ShutdownModule( ) override
	{
		FGlobalTabmanager::Get()->UnregisterTabSpawner(ProjectLauncherTabName);
	}

private:

	/**
	 * Creates a new session launcher tab.
	 *
	 * @param SpawnTabArgs The arguments for the tab to spawn.
	 * @return The spawned tab.
	 */
	TSharedRef<SDockTab> SpawnProjectLauncherTab(const FSpawnTabArgs& SpawnTabArgs)
	{
		const TSharedRef<SDockTab> DockTab = SNew(SDockTab)
			.Icon(FEditorStyle::GetBrush("Launcher.TabIcon"))
			.TabRole(ETabRole::NomadTab);

		ILauncherServicesModule& ProjectLauncherServicesModule = FModuleManager::LoadModuleChecked<ILauncherServicesModule>("LauncherServices");
		ITargetDeviceServicesModule& TargetDeviceServicesModule = FModuleManager::LoadModuleChecked<ITargetDeviceServicesModule>("TargetDeviceServices");

		FProjectLauncherModelRef Model = MakeShareable(new FProjectLauncherModel(
			TargetDeviceServicesModule.GetDeviceProxyManager(),
			ProjectLauncherServicesModule.CreateLauncher(),
			ProjectLauncherServicesModule.GetProfileManager()
		));

		DockTab->SetContent(SNew(SProjectLauncher, DockTab, SpawnTabArgs.GetOwnerWindow(), Model));

		return DockTab;
	}
};


IMPLEMENT_MODULE(FProjectLauncherModule, SProjectLauncher);
