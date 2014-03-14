// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Info about a single plugin
 */
class FPluginInfo : public FProjectOrPluginInfo
{
public:
	struct ELoadedFrom
	{
		enum Type
		{
			/** Plugin is built-in to the engine */
			Engine,

			/** Project-specific plugin, stored within a game project directory */
			GameProject
		};
	};
			

	/** Where does this plugin live? */
	ELoadedFrom::Type LoadedFrom;


	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Everything Below this line is serialized. If you add or remove properties, you must update
	// FPlugin::PerformAdditionalSerialization and FPlugin::PerformAdditionalDeserialization
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	/** Can this plugin contain content? */
	bool bCanContainContent;

	/** Marks the plugin as beta in the UI */
	bool bIsBetaVersion;

	/** Plugin info constructor */
	FPluginInfo()
		: FProjectOrPluginInfo()
		, LoadedFrom( ELoadedFrom::Engine )
		, bCanContainContent( false )
		, bIsBetaVersion( false )
	{
	}
};


/**
 * Instance of a plugin in memory
 */
class FPlugin : public FProjectOrPlugin
{

public:

	/**
	 * FPlugin constructor
	 */
	FPlugin();

	/**
	 * Registers the content mount points for this delegate. This allows content to be created in the root folder for this plugin.
	 *
	 * @param	RegisterMountPointDelegate	Delegate for mounting content paths.  Bound by FPackageName code in CoreUObject, so that we can access content path mounting functionality from Core.
	 */
	void RegisterPluginMountPoints( const IPluginManager::FRegisterMountPointDelegate& RegisterMountPointDelegate );

	/** @return Exposes access to the plugin's descriptor */
	const FPluginInfo& GetPluginInfo()
	{
		return PluginInfo;
	}

	/** Returns this plugin's name */
	const FString& GetPluginName() const;

	/** Sets the "enabled" state for this plugin */
	void SetEnabled(bool bNewPluginEnabled);

	/** Returns true if this plugin is currently marked as "enabled" */
	bool IsEnabled() const;

	/** Tells the descriptor where it was loaded from */
	void SetLoadedFrom(FPluginInfo::ELoadedFrom::Type NewLoadedFrom);

protected:
	virtual FProjectOrPluginInfo& GetProjectOrPluginInfo() OVERRIDE	{ return PluginInfo; }
	virtual const FProjectOrPluginInfo& GetProjectOrPluginInfo() const OVERRIDE { return PluginInfo; }

	virtual bool PerformAdditionalDeserialization(const TSharedRef< FJsonObject >& FileObject) OVERRIDE;
	virtual void PerformAdditionalSerialization(const TSharedRef< TJsonWriter<> >& Writer) const OVERRIDE;

private:

	/** The descriptor of this plugin that we loaded from disk */
	FPluginInfo PluginInfo;

	/** True if the plugin is marked as enabled */
	bool bPluginEnabled;
};



/**
 * ProjectAndPluginManager manages available code and content extensions (both loaded and not loaded.)
 */
class FPluginManager : public IPluginManager
{

public:

	/** Constructor */
	FPluginManager();

	/** Destructor */
	~FPluginManager();


public:

	/** IPluginManager interface */
	virtual void LoadModulesForEnabledPlugins( const ELoadingPhase::Type LoadingPhase ) OVERRIDE;
	virtual void SetRegisterMountPointDelegate( const FRegisterMountPointDelegate& Delegate ) OVERRIDE;
	virtual bool IsPluginModule( const FName ModuleName ) const OVERRIDE;
	virtual TArray< FPluginStatus > QueryStatusForAllPlugins() const OVERRIDE;
	virtual void SetPluginEnabled( const FString& PluginName, bool bEnabled ) OVERRIDE;
	virtual bool IsRestartRequired() const OVERRIDE;

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
	TArray< TSharedRef< FPlugin > > AllPlugins;

	/** Map from module name to plugin containing that module */
	TMap< FName, TSharedRef< FPlugin > > ModulePluginMap;

	/** Delegate for mounting content paths.  Bound by FPackageName code in CoreUObject, so that we can access
	    content path mounting functionality from Core. */
	FRegisterMountPointDelegate RegisterMountPointDelegate;

	/** The earliest phase that modules are loaded requires some additional initialization */
	bool bEarliestPhaseProcessed;

	/** true if plugin changes have been made that will only take effect after a restart */
	bool bRestartRequired;
};


