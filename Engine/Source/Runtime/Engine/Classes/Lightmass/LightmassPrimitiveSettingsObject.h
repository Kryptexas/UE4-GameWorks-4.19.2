// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/** 
 *	Primitive settings for Lightmass
 */

#pragma once

#include "Engine/EngineTypes.h"

#include "LightmassPrimitiveSettingsObject.generated.h"

UCLASS(hidecategories=Object, editinlinenew, MinimalAPI,collapsecategories)
class ULightmassPrimitiveSettingsObject : public UObject
{
	GENERATED_BODY()
public:
	ENGINE_API ULightmassPrimitiveSettingsObject(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, Category=Lightmass)
	struct FLightmassPrimitiveSettings LightmassSettings;

};

