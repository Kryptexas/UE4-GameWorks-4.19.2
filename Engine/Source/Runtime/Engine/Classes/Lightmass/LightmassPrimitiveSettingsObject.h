// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/** 
 *	Primitive settings for Lightmass
 */

#pragma once
#include "LightmassPrimitiveSettingsObject.generated.h"

UCLASS(hidecategories=Object, dependson=UEngineTypes, editinlinenew, MinimalAPI,collapsecategories)
class ULightmassPrimitiveSettingsObject : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Lightmass)
	struct FLightmassPrimitiveSettings LightmassSettings;

};

