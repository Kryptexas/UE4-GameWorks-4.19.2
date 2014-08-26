// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNodeSpawner.h"
#include "BlueprintDelegateNodeSpawner.generated.h"

// Forward declarations
class UK2Node_BaseMCDelegate;

/**
 * Takes care of spawning various nodes associated with delegates. Serves as the 
 * "action" portion for certain FBlueprintActionMenuItems. Evolved from 
 * FEdGraphSchemaAction_K2Delegate, FEdGraphSchemaAction_K2AssignDelegate, etc.
 */
UCLASS(Transient)
class BLUEPRINTGRAPH_API UBlueprintDelegateNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Creates a new UBlueprintDelegateNodeSpawner for the specified property.
	 * Does not do any compatibility checking to ensure that the property is
	 * accessible from blueprints (do that before calling this).
	 *
	 * @param  NodeClass	The node type that you want the spawner to spawn.
	 * @param  Property		The property you want assigned to spawned nodes.
	 * @param  Outer		Optional outer for the new spawner (if left null, the transient package will be used).
	 * @return A newly allocated instance of this class.
	 */
	static UBlueprintDelegateNodeSpawner* Create(TSubclassOf<UK2Node_BaseMCDelegate> NodeClass, UMulticastDelegateProperty const* const Property, UObject* Outer = nullptr);

	// UBlueprintNodeSpawner interface
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph, FVector2D const Location) const override;
	virtual FText GetDefaultMenuName() const override;
	virtual FText GetDefaultMenuCategory() const override;
	virtual FName GetDefaultMenuIcon(FLinearColor& ColorOut) const override;
	// End UBlueprintNodeSpawner interface

	/**
	 * Accessor to the delegate property that this spawner wraps (the delegate
	 * that this will assign spawned nodes with).
	 *
	 * @return The delegate property that this was initialized with.
	 */
	UMulticastDelegateProperty const* GetProperty() const;
	
private:
	/** */
	UPROPERTY()
	TWeakObjectPtr<UMulticastDelegateProperty> Property;
};
