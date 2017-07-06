// Copyright 2017 Google Inc.

#include "TangoPluginPrivate.h"
#include "ISettingsModule.h"
#include "Features/IModularFeatures.h"

#include "TangoMotionController.h"
#include "TangoARHMD.h"
#include "TangoARCamera.h"

#include "TangoLifecycle.h"
#include "TangoMotion.h"

#define LOCTEXT_NAMESPACE "Tango"

class FTangoPlugin : public ITangoPlugin
{
	/** IHeadMountedDisplayModule implementation*/
	/** Returns the key into the HMDPluginPriority section of the config file for this module */
	virtual FString GetModuleKeyName() const override
	{
		return TEXT("TangoARHMD");
	}

	/**
	 * Always return true for GoogleVR, when enabled, to allow HMD Priority to sort it out
	 */
	virtual bool IsHMDConnected() {
		return true;
	}

	/**
	 * Attempts to create a new head tracking device interface
	 *
	 * @return	Interface to the new head tracking device, if we were able to successfully create one
	 */
	virtual TSharedPtr<class IHeadMountedDisplay, ESPMode::ThreadSafe> CreateHeadMountedDisplay() override;

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Not really clear where this should go given that unlike other, input-derived controllers we never hand
	// off a shared pointer of the object to the engine.
	FTangoMotionController ControllerInstance;
};

IMPLEMENT_MODULE( FTangoPlugin, Tango )

TSharedPtr<class IHeadMountedDisplay, ESPMode::ThreadSafe> FTangoPlugin::CreateHeadMountedDisplay()
{
	TSharedPtr<FTangoARHMD, ESPMode::ThreadSafe> HMD(new FTangoARHMD());
	return HMD;
}

void FTangoPlugin::StartupModule()
{
	// Register editor settings:
    ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
    if (SettingsModule != nullptr)
    {
        SettingsModule->RegisterSettings("Project", "Plugins", "Tango",
        LOCTEXT("TangoSettingsName", "Tango"),
        LOCTEXT("TangoSettingsDescription", "Tango Configuration Options and Runtime Settings"),
        GetMutableDefault<UTangoEditorSettings>());
    }

	// Complete Tango setup.
	FTangoDevice::GetInstance()->OnModuleLoaded();

	// Register VR-like controller interface.
	ControllerInstance.RegisterController();

	// Register IHeadMountedDisplayModule
	IHeadMountedDisplayModule::StartupModule();
}

void FTangoPlugin::ShutdownModule()
{
	// Unregister IHeadMountedDisplayModule
	IHeadMountedDisplayModule::ShutdownModule();

	// Unregister VR-like controller interface.
	ControllerInstance.UnregisterController();

	// Complete Tango teardown.
	FTangoDevice::GetInstance()->OnModuleUnloaded();

	// Unregister editor settings.
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings( "Project", "Plugins", "Tango");
	}
}