// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNodeSpawner.h"
#include "BlueprintBoundNodeSpawner.generated.h"

// Forward declarations
class UActorComponent;

/**
 *
 */
UCLASS(Transient)
class BLUEPRINTGRAPH_API UBlueprintBoundNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * 
	 */
	static UBlueprintBoundNodeSpawner* Create(TSubclassOf<UEdGraphNode> NodeClass, UObject* BoundObject, UObject* Outer = nullptr);

	/**
	 * 
	 */
	static UBlueprintBoundNodeSpawner* Create(UBlueprintNodeSpawner* SubSpawner, UObject* BoundObject, UObject* Outer = nullptr);

	// UBlueprintNodeSpawner interface
	virtual UEdGraphNode* Invoke(UEdGraph* ParentGraph, FVector2D const Location) const override;
	virtual FText GetDefaultMenuName() const override;
	virtual FText GetDefaultMenuCategory() const override;
	// End UBlueprintNodeSpawner interface
	
	/**
	 * 
	 */
	UObject const* GetBoundObject() const;
	
	/**
	 *
	 */
	UBlueprintNodeSpawner const* GetSubSpawner() const;

private:
	/** */
	UPROPERTY()
	TWeakObjectPtr<UObject> BoundObjPtr;

	/** */
	UPROPERTY()
	UBlueprintNodeSpawner* SubSpawner;
};
