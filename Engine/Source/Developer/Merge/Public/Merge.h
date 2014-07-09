// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "IAssetTypeActions.h"

class UBlueprint;
class FBlueprintEditor;

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
	 * Generates a widget used to perform a three way merge operation.
	 *
	 * @return The merge view widget
	 */
	virtual TSharedPtr<class SWidget> GenerateMergeWidget( const UBlueprint& Object, TSharedRef<FBlueprintEditor> Editor ) = 0;

	/** 
	 * @return whether the blueprint is in a conflicted state, and can therefore be merged.
	 */
	virtual bool PendingMerge(const UBlueprint& BlueprintObj) const = 0;

};

