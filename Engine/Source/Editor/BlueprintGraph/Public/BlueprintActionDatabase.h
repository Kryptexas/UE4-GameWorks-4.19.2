// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GCObject.h"
#include "TickableEditorObject.h"

// Forward declarations
class UBlueprint;
class FBlueprintActionFilter;
class UBlueprintNodeSpawner;
class FReferenceCollector;

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
	typedef TArray<UBlueprintNodeSpawner*> FActionList;
	typedef TMap<UClass*, FActionList>     FClassActionMap;

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
	 * Finds the database entry for the specified class and wipes it. The entry 
	 * won't be rebuilt, unless RefreshClassActions() is explicitly called after.
	 * 
	 * @param  Class	The class entry you want emptied.
	 */
	void ClearClassActions(UClass* const Class);
	
	/**
	 * Will populate the database first if it hasn't been created yet, and then 
	 * returns it in its entirety. 
	 * 
	 * Each node spawner is categorized by a class. A spawner that corresponds
	 * to a specific class field (like a function, property, enum, etc.) will
	 * be listed under that field's class outer. Remaining spawners that can't 
	 * be categorized this way will be filed by node type; for example: with 
	 * global enums (enums that don't belong to a specific class), the enum's 
	 * UK2Node_EnumLiteral spawner is found under the UK2Node_EnumLiteral class.
	 *
	 * @return The entire action database, which maps specific classes to arrays of associated node-spawners.
	 */
	FClassActionMap const& GetAllActions();

	/**
	 * @return An estimated memory footprint of this database (in bytes).
	 */
	int32 Size() const;

private:
	/** Private constructor for singleton purposes. */
	FBlueprintActionDatabase();

	/** 
	 * A map of associated node-spawners for each class. A spawner that 
	 * corresponds to a specific class field (like a function, property, enum, 
	 * etc.) will be mapped under that field's class outer. Other spawners (that
	 * can't be associated with a class outer), will be filed under the desired
	 * node's type.
	 */
	FClassActionMap ClassActions;

	/** 
	 * References newly allocated actions that need to be "primed". Priming is 
	 * something we do on Tick() aimed at speeding up performance (like pre-
	 * caching each spawner's template-node, etc.).
	 */
	TMap<TWeakObjectPtr<UClass>, int32> ActionPrimingQueue;
};
