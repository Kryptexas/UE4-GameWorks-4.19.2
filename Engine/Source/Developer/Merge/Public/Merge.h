// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "IAssetTypeActions.h"

class UBlueprint;

struct FMergeDisplayArgs
{
	FRevisionInfo DepotHeadRevision;
	FRevisionInfo CommonBaseRevision;
};

/**
 * The public interface to this module
 */
class IMerge : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IMerge& Get()
	{
		return FModuleManager::LoadModuleChecked< IMerge >( "Merge" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "Merge" );
	}

	/**
	 * Performs a three way merge of AssetA and AssetB using CommonOrigin as a base. Takes a display args structure
	 * to customize the view.
	 *
	 * @return The resulting UBlueprint, NULL if merge was canceled or otherwise failed
	 */
	virtual void Merge( const UBlueprint& Object ) = 0;
};

