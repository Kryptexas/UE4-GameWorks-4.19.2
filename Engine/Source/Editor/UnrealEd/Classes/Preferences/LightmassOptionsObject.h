// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
 *	Lightmass Options Object
 *	Property window wrapper for various Lightmass settings
 *
 */

#pragma once

#include "Engine/EngineTypes.h"

#include "LightmassOptionsObject.generated.h"

UCLASS(hidecategories=Object, editinlinenew)
class ULightmassOptionsObject : public UObject
{
	GENERATED_BODY()
public:
	ULightmassOptionsObject(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, Category=Debug)
	struct FLightmassDebugOptions DebugSettings;

	UPROPERTY(EditAnywhere, Category=Swarm)
	struct FSwarmDebugOptions SwarmSettings;

};

