// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "AtmosphericFog.generated.h"

/** 
 *	A placeable fog actor that simulates atmospheric light scattering  
 *	@see https://docs.unrealengine.com/latest/INT/Engine/Actors/FogEffects/AtmosphericFog/index.html
 */
UCLASS(showcategories=(Movement, Rendering, "Utilities|Transformation", "Input|MouseInput", "Input|TouchInput"), ClassGroup=Fog, hidecategories=(Info,Object,Input), MinimalAPI)
class AAtmosphericFog : public AInfo
{
	GENERATED_UCLASS_BODY()

	/** Main fog component */
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category=Atmosphere)
	TSubobjectPtr<class UAtmosphericFogComponent> AtmosphericFogComponent;

#if WITH_EDITORONLY_DATA
	/** Arrow component to indicate default sun rotation */
	UPROPERTY()
	TSubobjectPtr<class UArrowComponent> ArrowComponent;
#endif

#if WITH_EDITOR
	virtual void PostActorCreated() override;
#endif
};





