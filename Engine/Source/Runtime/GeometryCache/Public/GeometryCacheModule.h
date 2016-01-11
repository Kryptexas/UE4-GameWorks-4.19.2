// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

/**
 * The public interface to this module
 */
class FGeometryCacheModule : public IModuleInterface
{
public:
	virtual void StartupModule();
	virtual void ShutdownModule();

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FGeometryCacheModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FGeometryCacheModule >("GeometryCache");
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "GeometryCache" );
	}

	/**
	* OnObjectReimported, used to fix up rendering data for GeometryCacheComponent that's using the reimported GeometryCache
	*
	* @param InObject - Reimported object
	* @return void
	*/
	void OnObjectReimported(UObject* InObject);
	
private:
	FDelegateHandle OnAssetReimportDelegateHandle;
};

