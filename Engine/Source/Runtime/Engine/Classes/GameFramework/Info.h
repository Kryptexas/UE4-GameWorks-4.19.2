// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Info, the root of all information holding classes.
 * Doesn't have any movement / collision related code.
  */

#pragma once
#include "Info.generated.h"

UCLASS(abstract, hidecategories=(Input, Movement, Collision, Rendering, "Utilities|Orientation"),MinimalAPI, NotBlueprintable)
class AInfo : public AActor
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	/** Billboard Component displayed in editor */
	UPROPERTY()
	TSubobjectPtr<class UBillboardComponent> SpriteComponent;
#endif

	virtual bool UpdateNavigationRelevancy() OVERRIDE { SetNavigationRelevancy(false); return false; }
	
	/** Indicates whether this actor should participate in level bounds calculations. */
	virtual bool IsLevelBoundsRelevant() const OVERRIDE { return false; }
};



