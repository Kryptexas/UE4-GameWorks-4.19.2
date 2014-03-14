// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** This class represents an APEX Destructible Actor. */

#include "DestructibleActor.generated.h"

class UDestructibleComponent;

UCLASS(MinimalAPI, hidecategories=(Input))
class ADestructibleActor : public AActor
{
	GENERATED_UCLASS_BODY()

	/**
	 * The component which holds the skinned mesh and physics data for this actor.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Destruction, meta=(ExposeFunctionCategories="Destruction,Components|Destructible"))
	TSubobjectPtr<UDestructibleComponent> DestructibleComponent;

	UPROPERTY(config, EditAnywhere, BlueprintReadWrite, Category=Navigation)
	uint32 bAffectNavigation : 1;

	// Begin AActor interface.
	virtual bool UpdateNavigationRelevancy() OVERRIDE { SetNavigationRelevancy(!!bAffectNavigation); return !!bAffectNavigation; }
#if WITH_EDITOR
	ENGINE_API virtual bool GetReferencedContentObjects( TArray<UObject*>& Objects ) const OVERRIDE;
#endif // WITH_EDITOR
	// End AActor interface.


};



