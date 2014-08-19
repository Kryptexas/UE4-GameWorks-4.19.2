// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

// Forward declarations
class UBlueprintNodeSpawner;


struct FBlueprintNodeSpawnerUtils
{
	/**
	 * All actions are associated with some class, whether it's the class of the
	 * node it will spawn, or the outer of the associated field (property,
	 * function, etc.).
	 * 
	 * @param  BlueprintAction	The node-spawner you want an associated class for.
	 * @return The action's associated class (null if it doesn't have one).
	 */
	static UClass* GetAssociatedClass(UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Certain node-spawners are associated with specific UFields (functions, 
	 * properties, etc.). This attempts to retrieve the field from the spawner.
	 * 
	 * @param  BlueprintAction	The node-spawner you want an associated field for.
	 * @return The action's associated field (null if it doesn't have one).
	 */
	static UField const* GetAssociatedField(UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * Certain node-spawners are associated with specific UFunction (call-
	 * function, and event spawners). This attempts to retrieve the function 
	 * from the spawner.
	 * 
	 * @param  BlueprintAction	The node-spawner you want an associated function for.
	 * @return The action's associated function (null if it doesn't have one).
	 */
	static UFunction const* GetAssociatedFunction(UBlueprintNodeSpawner const* BlueprintAction);

	/**
	 * 
	 * 
	 * @param  BlueprintAction	
	 * @return 
	 */
	static UProperty const* GetAssociatedProperty(UBlueprintNodeSpawner const* BlueprintAction);
};

