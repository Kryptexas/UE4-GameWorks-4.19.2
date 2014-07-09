// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNodeSpawner.h"
#include "BlueprintEventNodeSpawner.generated.h"

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
	static UBlueprintEventNodeSpawner* Create(UFunction const* const EventFunc);

	// UBlueprintNodeSpawner interface
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph) const override;
	virtual FText    GetDefaultMenuName() const override;
	virtual FText    GetDefaultMenuCategory() const override;
	// End UBlueprintNodeSpawner interface
	
	/**
	 * Retrieves the function that this assigns to spawned nodes (defines the
	 * event's signature).
	 *
	 * @return The event function that this class was initialized with.
	 */
	UFunction const* GetEventFunction() const;

private:
	/** The function to configure new nodes with. */
	UPROPERTY()
    UFunction const* EventFunc;
};
