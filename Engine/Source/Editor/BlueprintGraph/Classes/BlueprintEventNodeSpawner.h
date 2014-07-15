// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNodeSpawner.h"
#include "BlueprintEventNodeSpawner.generated.h"

class UK2Node_Event;

/**
 * Takes care of spawning UK2Node_Event nodes. Acts as the "action" portion of
 * certain FBlueprintActionMenuItems. Will not spawn a new event node if one
 * associated with the specified function already exits (instead, Invoke() will
 * return the existing one). Evolved from FEdGraphSchemaAction_K2AddEvent and 
 * FEdGraphSchemaAction_K2ViewNode.
 */
UCLASS(Transient)
class UBlueprintEventNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Creates a new UBlueprintEventNodeSpawner for the specified function.
	 * Does not do any compatibility checking to ensure that the function is
	 * viable as a blueprint event (do that before calling this).
	 *
	 * @param  EventFunc	The function you want assigned to new nodes.
	 * @return A newly allocated instance of this class.
	 */
	BLUEPRINTGRAPH_API static UBlueprintEventNodeSpawner* Create(UFunction const* const EventFunc);

	/**
	 * Creates a new UBlueprintEventNodeSpawner for custom events. The 
	 * CustomEventName can be left blank if the node will pick one itself on
	 * instantiation.
	 *
	 * @param  NodeClass		The event node type that you want this to spawn.
	 * @param  CustomEventName	The name you want assigned to the event.
	 * @return A newly allocated instance of this class.
	 */
	BLUEPRINTGRAPH_API static UBlueprintEventNodeSpawner* Create(TSubclassOf<UK2Node_Event> NodeClass, FName CustomEventName);

	// UBlueprintNodeSpawner interface
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph) const override;
	virtual FText GetDefaultMenuName() const override;
	virtual FText GetDefaultSearchKeywords() const override;
	// End UBlueprintNodeSpawner interface
	
	/**
	 * Retrieves the function that this assigns to spawned nodes (defines the
	 * event's signature). Can be null if this spawner was for a custom event.
	 *
	 * @return The event function that this class was initialized with.
	 */
	UFunction const* GetEventFunction() const;

private:
	/** The function to configure new nodes with. */
	UPROPERTY()
    UFunction const* EventFunc;

	/** The custom name to configure new event nodes with. */
	UPROPERTY()
	FName CustomEventName;
};
