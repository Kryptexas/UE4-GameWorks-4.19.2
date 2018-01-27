// Copyright 2017 Google Inc.

#include "GoogleARCoreBaseModule.h"
#include "ISettingsModule.h"
#include "ModuleManager.h"
#include "Features/IModularFeatures.h"
#include "Features/IModularFeature.h"

#include "GoogleARCoreMotionController.h"
#include "GoogleARCoreXRTrackingSystem.h"
#include "GoogleARCoreDevice.h"

#define LOCTEXT_NAMESPACE "GoogleARCore"

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
	TSharedPtr<FGoogleARCoreXRTrackingSystem, ESPMode::ThreadSafe> ARCoreSystem = NewARSystem<FGoogleARCoreXRTrackingSystem>();
	FGoogleARCoreDevice::GetInstance()->SetARSystem(ARCoreSystem);
	return ARCoreSystem;
}

void FGoogleARCoreBaseModule::StartupModule()
{
	ensureMsgf(FModuleManager::Get().LoadModule("AugmentedReality"), TEXT("ARCore depends on the AugmentedReality module.") );

	// Complete ARCore setup.
	FGoogleARCoreDevice::GetInstance()->OnModuleLoaded();

	// Register VR-like controller interface.
	ControllerInstance.RegisterController();

	// Register IHeadMountedDisplayModule
	IHeadMountedDisplayModule::StartupModule();

	FModuleManager::LoadModulePtr<IModuleInterface>("AugmentedReality");
}

void FGoogleARCoreBaseModule::ShutdownModule()
{
	// Unregister IHeadMountedDisplayModule
	IHeadMountedDisplayModule::ShutdownModule();

	// Unregister VR-like controller interface.
	ControllerInstance.UnregisterController();

	// Complete ARCore teardown.
	FGoogleARCoreDevice::GetInstance()->OnModuleUnloaded();
}
