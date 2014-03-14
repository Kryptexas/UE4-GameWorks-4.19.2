// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	DeviceManagerModule.cpp: Implements the FDeviceManagerModule class.
=============================================================================*/

#include "DeviceManagerPrivatePCH.h"


static const FName DeviceManagerTabName("DeviceManager");


/**
 * Implements the DeviceManager module.
 */
class FDeviceManagerModule
	: public IDeviceManagerModule
{
public:

	// Begin IModuleInterface interface

	virtual void StartupModule( ) OVERRIDE
	{
		// @todo gmp: implement an IoC container
		ITargetDeviceServicesModule& TargetDeviceServicesModule = FModuleManager::LoadModuleChecked<ITargetDeviceServicesModule>(TEXT("TargetDeviceServices"));

		TargetDeviceServiceManager = TargetDeviceServicesModule.GetDeviceServiceManager();

		FGlobalTabmanager::Get()->RegisterTabSpawner(DeviceManagerTabName, FOnSpawnTab::CreateRaw(this, &FDeviceManagerModule::SpawnDeviceManagerTab))
			.SetDisplayName(NSLOCTEXT("FDeviceManagerModule", "DeviceManagerTabTitle", "Device Manager"))
			.SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "DeviceDetails.TabIcon"));
	}

	virtual void ShutdownModule( ) OVERRIDE
	{
		FGlobalTabmanager::Get()->UnregisterTabSpawner(DeviceManagerTabName);
	}

	// End IModuleInterface interface


public:

	// Begin IDeviceManagerModule interface

	virtual TSharedRef<SWidget> CreateDeviceManager( const ITargetDeviceServiceManagerRef& DeviceServiceManager, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow ) OVERRIDE
	{
		return SNew(SDeviceManager, DeviceServiceManager, ConstructUnderMajorTab, ConstructUnderWindow);
	}

	// End IDeviceManagerModule interface


private:

	/**
	 * Creates a new device manager tab.
	 *
	 * @param SpawnTabArgs - The arguments for the tab to spawn.
	 *
	 * @return The spawned tab.
	 */
	TSharedRef<SDockTab> SpawnDeviceManagerTab( const FSpawnTabArgs& SpawnTabArgs )
	{
		const TSharedRef<SDockTab> MajorTab = SNew(SDockTab)
			.TabRole(ETabRole::MajorTab);

		MajorTab->SetContent(CreateDeviceManager(TargetDeviceServiceManager.ToSharedRef(), MajorTab, SpawnTabArgs.GetOwnerWindow()));

		return MajorTab;
	}


private:

	// @todo gmp: implement an IoC container
	ITargetDeviceServiceManagerPtr TargetDeviceServiceManager;
};


IMPLEMENT_MODULE(FDeviceManagerModule, DeviceManager);
