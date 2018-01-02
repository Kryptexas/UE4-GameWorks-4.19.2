// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
//#include "UnrealNames.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"

/**
 * The public interface of the GLTFImporter module
 */
class IGLTFImporter : public IModuleInterface
{
	//static const FName ModuleName("GLTFImporter");

public:
	/**
	 * Singleton-like access to IGLTFImporter
	 *
	 * @return Returns IGLTFImporter singleton instance, loading the module on demand if needed
	 */
	static IGLTFImporter& Get()
	{
		return FModuleManager::LoadModuleChecked<IGLTFImporter>("GLTFImporter");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static bool IsAvailable()
	{
		// static const FName ModuleName("GLTFImporter");
		return FModuleManager::Get().IsModuleLoaded("GLTFImporter");
	}
};
