// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *	Lightmass Options Object
 *	Property window wrapper for various Lightmass settings
 *
 */

#pragma once
#include "LightmassOptionsObject.generated.h"

UCLASS(hidecategories=Object, dependson=UEngineTypes, editinlinenew)
class ULightmassOptionsObject : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Debug)
	struct FLightmassDebugOptions DebugSettings;

	UPROPERTY(EditAnywhere, Category=Swarm)
	struct FSwarmDebugOptions SwarmSettings;

};

