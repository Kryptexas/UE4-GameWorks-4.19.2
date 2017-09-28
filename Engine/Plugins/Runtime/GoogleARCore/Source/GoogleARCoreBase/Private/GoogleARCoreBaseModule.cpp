// Copyright 2017 Google Inc.

#include "GoogleARCoreBaseModule.h"
#include "ISettingsModule.h"
#include "Features/IModularFeatures.h"
#include "Features/IModularFeature.h"

#include "GoogleARCoreMotionController.h"
#include "GoogleARCoreHMD.h"
#include "GoogleARCoreCameraManager.h"

#include "GoogleARCoreDevice.h"
#include "GoogleARCoreMotionManager.h"
#include "ARHitTestingSupport.h"


#define LOCTEXT_NAMESPACE "Tango"

class FGoogleARCoreBaseModule : public IGoogleARCoreBaseModule
{

	/** IHeadMountedDisplayModule implementation*/
	/** Returns the key into the HMDPluginPriority section of the config file for this module */
	virtual FString GetModuleKeyName() const override
	{
		return TEXT("GoogleARCoreHMD");
	}

	/**
	 * 
	 */
	virtual bool IsHMDConnected()
	{
		// @todo arcore do we have an API for querying this?
		return true;
	}

	/**
	 * Attempts to create a new head tracking device interface
	 *
	 * @return	Interface to the new head tracking device, if we were able to successfully create one
	 */
	virtual TSharedPtr< class IXRTrackingSystem, ESPMode::ThreadSafe > CreateTrackingSystem() override;

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Not really clear where this should go given that unlike other, input-derived controllers we never hand
	// off a shared pointer of the object to the engine.
	FGoogleARCoreMotionController ControllerInstance;
};

IMPLEMENT_MODULE(FGoogleARCoreBaseModule, GoogleARCoreBase)

TSharedPtr< class IXRTrackingSystem, ESPMode::ThreadSafe > FGoogleARCoreBaseModule::CreateTrackingSystem()
{
	TSharedPtr<FGoogleARCoreHMD, ESPMode::ThreadSafe> HMD(new FGoogleARCoreHMD());
	return HMD;
}

void FGoogleARCoreBaseModule::StartupModule()
{
	// Register editor settings:
    ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");
    if (SettingsModule != nullptr)
    {
        SettingsModule->RegisterSettings("Project", "Plugins", "GoogleARCore",
        LOCTEXT("GoogleARCoreSetting", "GoogleARCore"),
        LOCTEXT("GoogleARCoreSettingDescription", "Settings of the GoogleARCore plugin"),
        GetMutableDefault<UGoogleARCoreEditorSettings>());
    }

	// Complete Tango setup.
	FGoogleARCoreDevice::GetInstance()->OnModuleLoaded();

	// Register VR-like controller interface.
	ControllerInstance.RegisterController();

	// Register our ability to hit-test in AR with Unreal
	IModularFeatures::Get().RegisterModularFeature(IARHitTestingSupport::GetModularFeatureName(), this);

	// Register IHeadMountedDisplayModule
	IHeadMountedDisplayModule::StartupModule();
}

void FGoogleARCoreBaseModule::ShutdownModule()
{
	// Unregister IHeadMountedDisplayModule
	IHeadMountedDisplayModule::ShutdownModule();

	// Unregister VR-like controller interface.
	ControllerInstance.UnregisterController();

	// Unregister our ability to hit-test in AR with Unreal
	IModularFeatures::Get().UnregisterModularFeature(IARHitTestingSupport::GetModularFeatureName(), this);

	// Complete Tango teardown.
	FGoogleARCoreDevice::GetInstance()->OnModuleUnloaded();

	// Unregister editor settings.
	ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings");

	if (SettingsModule != nullptr)
	{
		SettingsModule->UnregisterSettings( "Project", "Plugins", "Tango");
	}
}