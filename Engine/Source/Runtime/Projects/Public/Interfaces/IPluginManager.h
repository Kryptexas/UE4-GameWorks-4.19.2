// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleDescriptor.h"
#include "PluginDescriptor.h"

/**
 * Enum for where a plugin is loaded from
 */
enum class EPluginLoadedFrom
{
	/** Plugin is built-in to the engine */
	Engine,

	/** Project-specific plugin, stored within a game project directory */
	GameProject
};


/**
 * Simple data structure that is filled when querying information about plug-ins.
 */
struct FPluginStatus
{
	/** The name of this plug-in. */
	FString Name;

	/** Path to plug-in directory on disk. */
	FString PluginDirectory;

	/** True if plug-in is currently enabled. */
	bool bIsEnabled;

	/** Where the plugin was loaded from */
	EPluginLoadedFrom LoadedFrom;

	/** The plugin descriptor */
	FPluginDescriptor Descriptor;
};


/**
 * Structure holding information about a plug-in content folder.
 */
struct FPluginContentFolder
{
	/** Name of the plug-in */
	FString Name;

	/** Virtual root path for asset paths */
	FString RootPath;

	/** Content path on disk */
	FString ContentPath;
};


/**
 * PluginManager manages available code and content extensions (both loaded and not loaded).
 */
class IPluginManager
{
public:

	/**
	 * Loads all plug-ins
	 *
	 * @param	LoadingPhase	Which loading phase we're loading plug-in modules from.  Only modules that are configured to be
	 *							loaded at the specified loading phase will be loaded during this call.
	 */
	virtual bool LoadModulesForEnabledPlugins( const ELoadingPhase::Type LoadingPhase ) = 0;

	/** Delegate type for mounting content paths.  Used internally by FPackageName code. */
	DECLARE_DELEGATE_TwoParams( FRegisterMountPointDelegate, const FString& /* Root content path */, const FString& /* Directory name */ );

	/**
	 * Sets the delegate to call to register a new content mount point.  This is used internally by the plug-in manager system
	 * and should not be called by you.  This is registered at application startup by FPackageName code in CoreUObject.
	 *
	 * @param	Delegate	The delegate to that will be called when plug-in manager needs to register a mount point
	 */
	virtual void SetRegisterMountPointDelegate( const FRegisterMountPointDelegate& Delegate ) = 0;

	/**
	 * Checks if all the required plug-ins are available. If not, will present an error dialog the first time a plug-in is loaded or this function is called.
	 *
	 * @returns true if all the required plug-ins are available.
	 */
	virtual bool AreRequiredPluginsAvailable() = 0;

	/** 
	 * Checks whether modules for the enabled plug-ins are up to date.
	 *
	 * @param OutIncompatibleNames	Array to receive a list of incompatible module names.
	 * @returns true if the enabled plug-in modules are up to date.
	 */
	virtual bool CheckModuleCompatibility( TArray<FString>& OutIncompatibleModules ) = 0;

	/**
	 * Gets status about all currently known plug-ins.
	 *
	 * @return	 Array of plug-in status objects.
	 */
	virtual TArray<FPluginStatus> QueryStatusForAllPlugins() const = 0;

	/**
	 * Gets a list of plug-in content folders.
	 *
	 * @return	 Array of plug-in content folders.
	 */
	virtual const TArray<FPluginContentFolder>& GetPluginContentFolders() const = 0;

public:

	/**
	 * Static: Access singleton instance.
	 *
	 * @return	Reference to the singleton object.
	 */
	static PROJECTS_API IPluginManager& Get();
};
