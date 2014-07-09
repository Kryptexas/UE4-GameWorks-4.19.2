// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNodeSpawner.h"
#include "BlueprintFunctionNodeSpawner.generated.h"

/**
 * Takes care of spawning various UK2Node_CallFunction nodes. Acts as the 
 * "action" portion of certain FBlueprintActionMenuItems. 
 */
UCLASS(Transient)
class UBlueprintFunctionNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Creates a new UBlueprintFunctionNodeSpawner for the specified function. 
	 * Does not do any compatibility checking to ensure that the function is 
	 * viable as a blueprint function call (do that before calling this).
	 *
	 * @param  Function		The function you want assigned to new nodes.
	 * @return A newly allocated instance of this class.
	 */
	BLUEPRINTGRAPH_API static UBlueprintFunctionNodeSpawner* Create(UFunction const* const Function);

	// UBlueprintNodeSpawner interface
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph) const override;
	virtual FText GetDefaultMenuName() const override;
	virtual FText GetDefaultMenuCategory() const override;
	virtual FText GetDefaultMenuTooltip() const override;
	virtual FText GetDefaultSearchKeywords() const override;
	// End UBlueprintNodeSpawner interface
	
	/**
	 * Retrieves the function that this assigns to spawned nodes (defines the 
	 * node's signature).
	 *
	 * @return The function that this class was initialized with.
	 */
	UFunction const* GetFunction() const;

private:
	/** The function to configure new nodes with. */
	UPROPERTY()
	UFunction const* Function;
};
