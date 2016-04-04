// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"
#include "ModuleManager.h"		// For inline LoadModuleChecked()
#include "Runtime/Core/Public/Features/IModularFeatures.h"

/**
 * The public interface of the MotionControlsModule
 */
class IHeadMountedDisplayModule : public IModuleInterface, public IModularFeature
{
public:
	static FName GetModularFeatureName()
	{
		static FName HMDFeatureName = FName(TEXT("HMD"));
		return HMDFeatureName;
	}

	/** Returns the key into the HMDPluginPriority section of the config file for this module */
	virtual FString GetModulePriorityKeyName() const = 0;

	/** Returns the priority of this module from INI file configuration */
	float GetHMDPriority() const
	{
		float ModulePriority = 0.f;
		FString KeyName = GetModulePriorityKeyName();
		GConfig->GetFloat(TEXT("HMDPluginPriority"), (!KeyName.IsEmpty() ? *KeyName : TEXT("Default")), ModulePriority, GEngineIni);
		return ModulePriority;
	}

	/** Sorting method for which plug-in should be given priority */
	struct FCompareModulePriority
	{
		bool operator()(IHeadMountedDisplayModule& A, IHeadMountedDisplayModule& B) const
		{
			FHeadMountedDisplayModuleExt* AExt = FHeadMountedDisplayModuleExt::GetExtendedInterface(&A);
			FHeadMountedDisplayModuleExt* BExt = FHeadMountedDisplayModuleExt::GetExtendedInterface(&B);

			bool IsHMDConnectedA = AExt ? AExt->IsHMDConnected() : false;
			bool IsHMDConnectedB = BExt ? BExt->IsHMDConnected() : false;

			if (IsHMDConnectedA && !IsHMDConnectedB)
				return true;
			if (!IsHMDConnectedA && IsHMDConnectedB)
				return false;

			return A.GetHMDPriority() > B.GetHMDPriority();
		}
	};
	
	/**
	 * Singleton-like access to IHeadMountedDisplayModule
	 *
	 * @return Returns reference to the highest priority IHeadMountedDisplayModule module
	 */
	static inline IHeadMountedDisplayModule& Get()
	{
		TArray<IHeadMountedDisplayModule*> HMDModules = IModularFeatures::Get().GetModularFeatureImplementations<IHeadMountedDisplayModule>(GetModularFeatureName());
		HMDModules.Sort(FCompareModulePriority());
		return *HMDModules[0];
	}

	/**
	 * Checks to see if there exists a module registered as an HMD.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if there exists a module registered as an HMD.
	 */
	static inline bool IsAvailable()
	{
		return IModularFeatures::Get().IsModularFeatureAvailable(GetModularFeatureName());
	}

	/**
	* Register module as an HMD on startup.
	*/
	virtual void StartupModule() override
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

	/**
	* Optionally pre-initialize the HMD module.  Return false on failure.
	*/
	virtual void PreInit() {};

	/**
	 * Attempts to create a new head tracking device interface
	 *
	 * @return	Interface to the new head tracking device, if we were able to successfully create one
	 */
	virtual TSharedPtr< class IHeadMountedDisplay, ESPMode::ThreadSafe > CreateHeadMountedDisplay() = 0;
};

/** DEADCODE:  Binary compat for 4.11.  Sorting method for which plugin should be given priority */
struct FHMDPluginSorter
{
	FHMDPluginSorter() {}

	// Sort predicate operator
	bool operator()(IHeadMountedDisplayModule& LHS, IHeadMountedDisplayModule& RHS) const
	{
		return LHS.GetHMDPriority() > RHS.GetHMDPriority();
	}
};