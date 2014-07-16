// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNodeSpawner.h"
#include "BlueprintComponentNodeSpawner.generated.h"

// Forward declarations
class UActorComponent;

/**
 * Takes care of spawning UK2Node_AddComponent nodes. Acts as the "action" 
 * portion of certain FBlueprintActionMenuItems. Evolved from 
 * FEdGraphSchemaAction_K2AddComponent.
 */
UCLASS(Transient)
class BLUEPRINTGRAPH_API UBlueprintComponentNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Creates a new UBlueprintComponentNodeSpawner for the specified class. 
	 * Does not do any compatibility checking to ensure that the class is 
	 * viable as a spawnable component (do that before calling this).
	 *
	 * @param  ComponentClass	The component type you want spawned nodes to spawn.
	 * @param  Outer			Optional outer for the new spawner (if left null, the transient package will be used).
	 * @return A newly allocated instance of this class.
	 */
	static UBlueprintComponentNodeSpawner* Create(TSubclassOf<UActorComponent> const ComponentClass, UObject* Outer = nullptr);

	// UBlueprintNodeSpawner interface
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph) const override;
	virtual FText    GetDefaultMenuName() const override;
	virtual FText    GetDefaultMenuCategory() const override;
	// End UBlueprintNodeSpawner interface
	
	/**
	 * Retrieves the component class that this configures spawned nodes with.
	 *
	 * @return The component type that this class was initialized with.
	 */
	TSubclassOf<UActorComponent> GetComponentClass() const;

private:
	/** The component class to configure new nodes with. */
	UPROPERTY()
	TSubclassOf<UActorComponent> ComponentClass;
};
