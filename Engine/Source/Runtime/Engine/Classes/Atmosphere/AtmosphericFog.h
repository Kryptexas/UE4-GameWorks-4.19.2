// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "AtmosphericFog.generated.h"

UCLASS(showcategories=(Movement, Rendering, "Utilities|Orientation"), ClassGroup=Fog, hidecategories=(Info,Object,Input), MinimalAPI)
class AAtmosphericFog : public AInfo
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category=Atmosphere)
	TSubobjectPtr<class UAtmosphericFogComponent> AtmosphericFogComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TSubobjectPtr<class UArrowComponent> ArrowComponent;
#endif

#if WITH_EDITOR
	virtual void PostActorCreated() OVERRIDE;
#endif
};





