// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNodeSpawner.h"
#include "BlueprintPropertyNodeSpawner.generated.h"

/**
 * Takes care of spawning various nodes associated with properties (like 
 * variable getter/setters, or one of the various delegate nodes). Acts as the 
 * "action" portion of certain FBlueprintActionMenuItems. Evolved from 
 * FEdGraphSchemaAction_K2Var, FEdGraphSchemaAction_K2Delegate, etc.
 */
UCLASS(Transient)
class BLUEPRINTGRAPH_API UBlueprintPropertyNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Creates a new UBlueprintPropertyNodeSpawner for the specified property.
	 * Does not do any compatibility checking to ensure that the property is
	 * accessible from blueprints (do that before calling this).
	 *
	 * @param  Property		The property you want assigned to spawned nodes.
	 * @param  Outer		Optional outer for the new spawner (if left null, the transient package will be used).
	 * @param  NodeClass	The type of node you want spawned (can't distinct a getter from a setter from the property alone).
	 * @return A newly allocated instance of this class.
	 */
	static UBlueprintPropertyNodeSpawner* Create(UProperty const* const Property, UObject* Outer = nullptr, TSubclassOf<UEdGraphNode> const NodeClass = nullptr);

	// UBlueprintNodeSpawner interface
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph) const override;
	virtual FText GetDefaultMenuName() const override;
	virtual FText GetDefaultMenuCategory() const override;
	// End UBlueprintNodeSpawner interface

	/**
	 * Retrieves the property that this assigns to spawned nodes.
	 *
	 * @return The property that this class was initialized with.
	 */
	UProperty const* GetProperty() const;

	/**
	 * Checks to see if the associated property is a delegate property.
	 *
	 * @return True if the property is a UMulticastDelegateProperty (otherwise false).
	 */
	bool IsDelegateProperty() const;
	
private:
	/** The property to configure new nodes with. */
	UPROPERTY()
	UProperty const* Property;
};