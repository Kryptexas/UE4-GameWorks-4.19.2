// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "GameFramework/Volume.h"
#include "HierarchicalLODVolume.generated.h"

/** An invisible volume used to manually define/create a HLOD cluster. */
UCLASS(MinimalAPI)
class AHierarchicalLODVolume : public AVolume
{
	GENERATED_UCLASS_BODY()

public:
	/** When set this volume will incorporate actors which bounds overlap with the volume, otherwise only actors which are completely inside of the volume are incorporated */
	UPROPERTY(EditAnywhere, Category = "HLOD Volume")
	bool bIncludeOverlappingActors;
};
