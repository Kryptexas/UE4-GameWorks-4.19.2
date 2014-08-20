// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"

/**
* HotReload module interface
*/
class COREUOBJECT_API IHotReloadInterface : public IModuleInterface
{
public:

	/**
	* Adds a function to re-map after hot-reload.
	*/
	virtual void AddHotReloadFunctionRemap(Native NewFunctionPointer, Native OldFunctionPointer) = 0;

	/**
	* Performs hot reload from the editor
	*/
	virtual void DoHotReloadFromEditor() = 0;

	/**
	* HotReload: Reloads the DLLs for given packages.
	* @param	Package				Packages to reload.
	* @param	DependentModules	Additional modules that don't contain UObjects, but rely on them
	* @param	bWaitForCompletion	True if RebindPackages should not return until the recompile and reload has completed
	* @param	Ar					Output device for logging compilation status
	*/
	virtual void RebindPackages(TArray<UPackage*> Packages, TArray<FName> DependentModules, const bool bWaitForCompletion, FOutputDevice &Ar) = 0;

	/** Called when a Hot Reload event has completed. 
	 * 
	 * @param	bWasTriggeredAutomatically	True if the hot reload was invoked automatically by the hot reload system after detecting a changed DLL
	 */
	DECLARE_MULTICAST_DELEGATE_OneParam(FHotReloadEvent, bool /* bWasTriggeredAutomatically */ );
	virtual FHotReloadEvent& OnHotReload() = 0;
};

