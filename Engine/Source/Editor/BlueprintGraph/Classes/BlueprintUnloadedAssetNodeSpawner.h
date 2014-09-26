// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BlueprintNodeSpawner.h"
#include "BlueprintUnloadedAssetNodeSpawner.generated.h"

/**
 * Handles spawning nodes that use unloaded assets
 */
UCLASS(Transient)
class BLUEPRINTGRAPH_API UBlueprintUnloadedAssetNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_UCLASS_BODY()

public:
	/**
	 * @return A newly allocated instance of this class.
	 */
	static UBlueprintUnloadedAssetNodeSpawner* Create(TSubclassOf<UEdGraphNode> NodeClass, const FAssetData& AssetData, FText ActionMenuDescription, UObject* Outer = nullptr);

	// UBlueprintNodeSpawner interface
	virtual FText GetDefaultMenuName(FBindingSet const& Bindings) const override;
	// End UBlueprintNodeSpawner interface

public:
	/** The data from the Asset Registry detailing the unloaded asset */
	FAssetData AssetData;

	/** The asset's action menu description */
	FText ActionMenuDescription;
};
