// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PluginDescriptor.h"

/**
 * Enum for where a plugin is loaded from
 */
struct EPluginLoadedFrom
{
	enum Type
	{
		/** Plugin is built-in to the engine */
		Engine,

		/** Project-specific plugin, stored within a game project directory */
		GameProject
	};
};

/**
 * Instance of a plugin in memory
 */
class FPluginInstance
{
public:
	/** The name of the plugin */
	FString Name;

	/** The filename that the plugin was loaded from */
	FString FileName;

	/** The plugin's settings */
	FPluginDescriptor Descriptor;

	/** Where does this plugin live? */
	EPluginLoadedFrom::Type LoadedFrom;

	/** True if the plugin is marked as enabled */
	bool bEnabled;

	/**
	 * FPlugin constructor
	 */
	FPluginInstance(const FString &FileName, const FPluginDescriptor& InDescriptor, EPluginLoadedFrom::Type InLoadedFrom);

	/**
	 * Registers the content mount points for this delegate. This allows content to be created in the root folder for this plugin.
	 *
	 * @param	RegisterMountPointDelegate	Delegate for mounting content paths.  Bound by FPackageName code in CoreUObject, so that we can access content path mounting functionality from Core.
	 */
	void RegisterPluginMountPoints( const IPluginManager::FRegisterMountPointDelegate& RegisterMountPointDelegate );
};

/**
 * FPluginManager manages available code and content extensions (both loaded and not loaded.)
 */
class FPluginManager : public IPluginManager
{
public:
	/** Constructor */
	FPluginManager();

	/** Destructor */
	~FPluginManager();

	/** IPluginManager interface */
	virtual void LoadModulesForEnabledPlugins( const ELoadingPhase::Type LoadingPhase ) override;
	virtual void SetRegisterMountPointDelegate( const FRegisterMountPointDelegate& Delegate ) override;
	virtual bool IsPluginModule( const FName ModuleName ) const override;
	virtual bool AreEnabledPluginModulesUpToDate() override;
	virtual TArray< FPluginStatus > QueryStatusForAllPlugins() const override;
	virtual void SetPluginEnabled( const FString& PluginName, bool bEnabled ) override;
	virtual bool IsRestartRequired() const override;
	virtual bool HasThirdPartyPlugin() const override;

private:

	/** Searches for all plugins on disk and builds up the array of plugin objects.  Doesn't load any plugins. 
	    This is called when the plugin manager singleton is first accessed. */
	void DiscoverAllPlugins();

	/** Sets the bPluginEnabled flag on all plugins found from DiscoverAllPlugins that are enabled in config */
	void EnablePluginsThatAreConfiguredToBeEnabled();

	/** Registers content mount points for all enabled plugins */
	void RegisterEnabledPluginMountPoints();

	/** Returns all plugins currently configured to be enabled */
	void GetPluginsConfiguredToBeEnabled(TArray<FString>& OutPluginNames) const;

	/** Sets the list of currently configured plugins */
	void ConfigurePluginToBeEnabled(const FString& PluginName, bool bEnabled);

private:
	/** All of the plugins that we know about */
	TArray< TSharedRef< FPluginInstance > > AllPlugins;

	/** Map from module name to plugin containing that module */
	TMap< FName, TSharedRef< FPluginInstance > > ModulePluginMap;

	/** Delegate for mounting content paths.  Bound by FPackageName code in CoreUObject, so that we can access
	    content path mounting functionality from Core. */
	FRegisterMountPointDelegate RegisterMountPointDelegate;

	/** The earliest phase that modules are loaded requires some additional initialization */
	bool bEarliestPhaseProcessed;

	/** true if plugin changes have been made that will only take effect after a restart */
	bool bRestartRequired;
};


