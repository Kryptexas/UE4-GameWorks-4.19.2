// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core.h"
#include "ModuleInterface.h"

/**
 * Online subsystem module class
 * Wraps the loading of an online subsystem by name and allows new services to register themselves for use
 */
class FOnlineSubsystemModule : public IModuleInterface
{
private:

	/** Name of the default online service requested 
	 * Specified in DefaultEngine.ini 
	 *	[OnlineSubsystem] 
	 *	DefaultPlatformService 
	 */
	FName DefaultPlatformService;

	/** Mapping of all currently loaded platform service subsystems to their name */
	TMap<FName, class IOnlineSubsystem*> PlatformServices;

	/**
	 *	Shuts down all registered online subsystem platforms and unloads their modules
	 */
	virtual void ShutdownOnlineSubsystem();

public:

	FOnlineSubsystemModule() :
		DefaultPlatformService(NAME_None)
	{}
	virtual ~FOnlineSubsystemModule() {}

	/** 
	 * Main entry point for accessing an online subsystem by name
	 * Will load the appropriate module if the subsystem isn't currently loaded
	 * It's possible that the subsystem doesn't exist and therefore can return NULL
	 *
	 * @param SubsystemName - name of subsystem as referenced by consumers
	 * @return Requested online subsystem, or NULL if that subsystem was unable to load or doesn't exist
	 */
	virtual class IOnlineSubsystem* GetOnlineSubsystem(const FName InSubsystemName = NAME_None);

	/** 
	 * Determine if a subsystem is loaded by the OSS module
	 *
	 * @param SubsystemName - name of subsystem as referenced by consumers
	 * @return true if module for the subsystem is loaded
	 */
	virtual bool IsOnlineSubsystemLoaded(const FName InSubsystemName) const;

	/** 
	 * Register a new online subsystem interface with the base level factory provider
	 * @param FactoryName - name of subsystem as referenced by consumers
	 * @param Factory - instantiation of the online subsystem interface, this will take ownership
	 */
	virtual void RegisterPlatformService(const FName FactoryName, class IOnlineSubsystem* Factory);
	
	/** 
	 * Unregister an existing online subsystem interface from the base level factory provider
	 * @param FactoryName - name of subsystem as referenced by consumers
	 */
	virtual void UnregisterPlatformService(const FName FactoryName);


	// IModuleInterface

	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 * Overloaded to allow the default subsystem a chance to load
	 */
	virtual void StartupModule() OVERRIDE;

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 * Overloaded to shut down all loaded online subsystems
	 */
	virtual void ShutdownModule() OVERRIDE;

	/**
	 * Override this to set whether your module is allowed to be unloaded on the fly
	 *
	 * @return	Whether the module supports shutdown separate from the rest of the engine.
	 */
	virtual bool SupportsDynamicReloading() OVERRIDE
	{
		return false;
	}

	/**
	 * Override this to set whether your module would like cleanup on application shutdown
	 *
	 * @return	Whether the module supports shutdown on application exit
	 */
	virtual bool SupportsAutomaticShutdown() OVERRIDE
	{
		return false;
	}
};

/** Public references to the online subsystem module pointer should use this */
typedef TSharedPtr<FOnlineSubsystemModule, ESPMode::ThreadSafe> FOnlineSubsystemModulePtr;

