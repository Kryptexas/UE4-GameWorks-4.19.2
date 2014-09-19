// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GCObject.h"
#include "TickableEditorObject.h"

// Forward declarations
class UBlueprint;
class FBlueprintActionFilter;
class UBlueprintNodeSpawner;
class FReferenceCollector;
class FBlueprintActionDatabaseRegistrar;

/**
 * Serves as a container for all available blueprint actions (no matter the 
 * type of blueprint/graph they belong in). The actions stored here are not tied
 * to a specific ui menu; each action is a UBlueprintNodeSpawner which is 
 * charged with spawning a specific node type. Should be set up in a way where
 * class specific actions are refreshed when the associated class is regenerated.
 * 
 * @TODO:  Hook up to handle class recompile events, along with enum/struct asset creation events.
 */
class BLUEPRINTGRAPH_API FBlueprintActionDatabase : public FGCObject, public FTickableEditorObject
{
public:
	typedef TMap<TWeakObjectPtr<UObject>, int32> FPrimingQueue;
	typedef TArray<UBlueprintNodeSpawner*>       FActionList;
	typedef TMap<UObject const*, FActionList>    FActionRegistry;
	
public:
	/**
	 * Getter to access the database singleton. Will populate the database first 
	 * if this is the first time accessing it.
	 *
	 * @return The singleton instance of FBlueprintActionDatabase.
	 */
	static FBlueprintActionDatabase& Get();

	// FTickableEditorObject interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override;
	// End FTickableEditorObject interface

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	// End FGCObject interface

	/**
	 * Will populate the database first if it hasn't been created yet, and then 
	 * returns it in its entirety. 
	 * 
	 * Each node spawner is categorized by a class orasset. A spawner that 
	 * corresponds to a specific class field (like a function, property, enum, 
	 * etc.) will be listed under that field's class owner. Remaining spawners 
	 * that can't be categorized this way will be registered by asset or node 
	 * type.
	 *
	 * @return The entire action database, which maps specific objects to arrays of associated node-spawners.
	 */
	FActionRegistry const& GetAllActions();

	/**
	 * Populates the action database from scratch. Loops over every known class
	 * and records a set of node-spawners associated with each.
	 */
	void RefreshAll();

	/**
	 * Finds the database entry for the specified class and wipes it, 
	 * repopulating it with a fresh set of associated node-spawners. 
	 *
	 * @param  Class	The class entry you want rebuilt.
	 */
	void RefreshClassActions(UClass* const Class);

	/**
	 * Finds the database entry for the specified asset and wipes it,
	 * repopulating it with a fresh set of associated node-spawners.
	 * 
	 * @param  AssetObject	The asset entry you want rebuilt.
	 */
	void RefreshAssetActions(UObject* const AssetObject);

	/**
	 * Finds the database entry for the specified class and wipes it. The entry 
	 * won't be rebuilt, unless RefreshAssetActions() is explicitly called after.
	 * 
	 * @param  AssetObject	
	 */
	void ClearAssetActions(UObject* const AssetObject);
	
	/**
	 * 
	 * @return An estimated memory footprint of this database (in bytes).
	 */
	int32 EstimatedSize() const;

private:
	/** Private constructor for singleton purposes. */
	FBlueprintActionDatabase();

	/**
	 * 
	 * 
	 * @param  Registrar	
	 */
	void RegisterAllNodeActions(FBlueprintActionDatabaseRegistrar& Registrar);

private:
	/** 
	 * A map of associated node-spawners for each class/asset. A spawner that 
	 * corresponds to a specific class field (like a function, property, enum, 
	 * etc.) will be mapped under that field's class outer. Other spawners (that
	 * can't be associated with a class outer), will be filed under the desired
	 * node's type, or an associated asset.
	 */
	FActionRegistry ActionRegistry;

	/** 
	 * References newly allocated actions that need to be "primed". Priming is 
	 * something we do on Tick() aimed at speeding up performance (like pre-
	 * caching each spawner's template-node, etc.).
	 */
	FPrimingQueue ActionPrimingQueue;
};
