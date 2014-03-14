// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

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

	virtual void StartupModule() OVERRIDE
	{
		IModularFeatures::Get().RegisterModularFeature( GetModularFeatureName(), this );
	}

	/**
	 * Singleton-like access to IMovieSceneCore
	 *
	 * @return Returns IHeadMountedDisplayModule singleton instance, loading the module on demand if needed
	 */
	static inline IHeadMountedDisplayModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IHeadMountedDisplayModule >( "HeadMountedDisplay" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "HeadMountedDisplay" );
	}


	/**
	 * Attempts to create a new head tracking device interface
	 *
	 * @return	Interface to the new head tracking device, if we were able to successfully create one
	 */
	virtual TSharedPtr< class IHeadMountedDisplay > CreateHeadMountedDisplay() = 0;

};
