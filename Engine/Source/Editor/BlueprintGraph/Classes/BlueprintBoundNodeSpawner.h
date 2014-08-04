// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNodeSpawner.h"
#include "BlueprintBoundNodeSpawner.generated.h"

// Forward declarations
class UActorComponent;

/**
 * Takes care of spawning arbitrary nodes that are bound to specific UObject 
 * instances (like: functions being called on a specific object, or events
 * triggering for specific level actors, etc.). UBlueprintNodeSpawner classes
 * act as the "action" portion of FBlueprintActionMenuItems. Evolved from 
 * UK2Node_CallFunctionOnMember and various other pieces of code from 
 * FK2ActionMenuBuilder.
 */
UCLASS(Transient)
class BLUEPRINTGRAPH_API UBlueprintBoundNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Creates a new UBlueprintBoundNodeSpawner that is charged with spawning 
	 * nodes (of the specified type) and bind those nodes to the supplied
	 * object.
	 *
	 * @param  NodeClass	The event node type that you want this to spawn.
	 * @param  BoundObject	The object you want spawned nodes bound to.
	 * @param  Outer		Optional outer for the new spawner (if left null, the transient package will be used).
	 * @return A newly allocated instance of this class.
	 */
	static UBlueprintBoundNodeSpawner* Create(TSubclassOf<UEdGraphNode> NodeClass, UObject* BoundObject, UObject* Outer = nullptr);

	/**
	 * Creates a new UBlueprintBoundNodeSpawner that is charged with using the 
	 * supplied SubSpawner to spawn nodes and bind those nodes to the 
	 * specified object. This allows us to leverage other UBlueprintNodeSpawner's
	 * knowledge on how to spawn certain node types, while this class is solely 
	 * in charge of binding.
	 *
	 * @param  SubSpawner	The spawner that will instantiate the node.
	 * @param  BoundObject	The object you want spawned nodes bound to.
	 * @param  Outer		Optional outer for the new spawner (if left null, the transient package will be used).
	 * @return A newly allocated instance of this class.
	 */
	static UBlueprintBoundNodeSpawner* Create(UBlueprintNodeSpawner* SubSpawner, UObject* BoundObject, UObject* Outer = nullptr);

	// UBlueprintNodeSpawner interface
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph, FVector2D const Location) const override;
	virtual FText GetDefaultMenuName() const override;
	virtual FText GetDefaultMenuCategory() const override;
	// End UBlueprintNodeSpawner interface
	
	/**
 	 * Retrieves the object that this binds to spawned nodes.
	 * @return The object that this class was initialized with.
	 */
	UObject const* GetBoundObject() const;
	
	/**
	 * Retrieves the sub-spawner that this wraps (which is actually in-charge of
	 * allocating the nodes that this produces). 
	 *
	 * @return The sub-spawner that this class was initialized with.
	 */
	UBlueprintNodeSpawner const* GetSubSpawner() const;

private:
	/** This object that this spawner attempts to bind new nodes with. */
	UPROPERTY()
	TWeakObjectPtr<UObject> BoundObjPtr;

	/** 
	 * Instead of allocating nodes itself, this class relies on this sub-spawner  
	 * to do the spawning, while this class takes care of binding the nodes. 
	 * This is to keep function/event/etc. node spawning logic contained and 
	 * un-duplicated.
	 */
	UPROPERTY()
	UBlueprintNodeSpawner* SubSpawner;
};
