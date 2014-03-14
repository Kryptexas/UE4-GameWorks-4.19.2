// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ModuleManager.h"
#include "ActorPlacementInfo.h"
#include "IPlacementMode.h"

class IPlacementModeModule : public IModuleInterface
{
public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IPlacementModeModule& Get()
	{
		return FModuleManager::LoadModuleChecked< IPlacementModeModule >( "PlacementMode" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "PlacementMode" );
	}

	virtual void AddToRecentlyPlaced( const TArray< UObject* >& Assets, UActorFactory* FactoryUsed = NULL ) = 0;

	virtual void AddToRecentlyPlaced( UObject* Asset, UActorFactory* FactoryUsed = NULL ) = 0;

	virtual const TArray< FActorPlacementInfo >& GetRecentlyPlaced() const = 0;

	virtual bool IsPlacementModeAvailable() const = 0;

	virtual TSharedRef< class IPlacementMode > GetPlacementMode() const = 0;
};

