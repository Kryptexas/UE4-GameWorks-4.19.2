// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ExponentialHeightFog.generated.h"

UCLASS(showcategories=(Movement, Rendering, "Utilities|Orientation"), ClassGroup=Fog, MinimalAPI)
class AExponentialHeightFog : public AInfo
{
	GENERATED_UCLASS_BODY()

	/** @todo document */
	UPROPERTY(Category=ExponentialHeightFog, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UExponentialHeightFogComponent> Component;

	/** replicated copy of ExponentialHeightFogComponent's bEnabled property */
	UPROPERTY(replicatedUsing=OnRep_bEnabled)
	uint32 bEnabled:1;

	/** Replication Notification Callbacks */
	UFUNCTION()
	virtual void OnRep_bEnabled();


	//Begin AActor Interface
	virtual void PostInitializeComponents() OVERRIDE;
	//End AActor Interface
};



