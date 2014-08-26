// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintEventNodeSpawner.h"
#include "BlueprintBoundEventNodeSpawner.generated.h"

class UK2Node_Event;

/**
 * Takes care of spawning UK2Node_Event nodes. Acts as the "action" portion of
 * certain FBlueprintActionMenuItems. Will not spawn a new event node if one
 * associated with the specified function already exits (instead, Invoke() will
 * return the existing one). Evolved from FEdGraphSchemaAction_K2AddEvent and 
 * FEdGraphSchemaAction_K2ViewNode.
 */
UCLASS(Transient)
class BLUEPRINTGRAPH_API UBlueprintBoundEventNodeSpawner : public UBlueprintEventNodeSpawner
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * Creates a new UBlueprintEventNodeSpawner for delegate bound events. Does
	 * not come bound, instead it is left up to the ui to bind 
	 *
	 * @param  NodeClass		The event node type that you want this to spawn.
	 * @param  EventDelegate	The delegate that you want to trigger the spawned event.
	 * @param  Outer			Optional outer for the new spawner (if left null, the transient package will be used).
	 * @return A newly allocated instance of this class.
	 */
	static UBlueprintBoundEventNodeSpawner* Create(TSubclassOf<UK2Node_Event> NodeClass, UMulticastDelegateProperty* EventDelegate, UObject* Outer = nullptr);

	// UBlueprintNodeSpawner interface
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph, FVector2D const Location) const override;
	virtual FText GetDefaultMenuName() const override;
	virtual FText GetDefaultMenuCategory() const override;
	// End UBlueprintNodeSpawner interface

	// UBlueprintEventNodeSpawner interface
	virtual UK2Node_Event const* FindPreExistingEvent(UBlueprint* Blueprint) const override;
	// End UBlueprintEventNodeSpawner interface

	// FBlueprintNodeBinder interface
	virtual bool CanBind(UObject const* BindingCandidate) const override;
	virtual bool BindToNode(UEdGraphNode* Node, UObject* Binding) const override;
	// End FBlueprintNodeBinder interface

	/**
	 * 
	 *
	 * @return 
	 */
	UMulticastDelegateProperty const* GetEventDelegate() const;

private:
	/** */
	UPROPERTY()
	TWeakObjectPtr<UMulticastDelegateProperty> EventDelegate;
};
