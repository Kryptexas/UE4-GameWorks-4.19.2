// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SkyLight.generated.h"

UCLASS(ClassGroup=Lights, hidecategories=(Input,Collision,Replication,Info), showcategories=("Input|MouseInput", "Input|TouchInput"), ConversionRoot, Blueprintable)
class ENGINE_API ASkyLight : public AInfo
{
	GENERATED_UCLASS_BODY()

	/** @todo document */
	UPROPERTY(Category=Light, VisibleAnywhere, BlueprintReadOnly, meta=(ExposeFunctionCategories="Light,Rendering,Rendering|Components|SkyLight"))
	TSubobjectPtr<class USkyLightComponent> LightComponent;

	/** replicated copy of LightComponent's bEnabled property */
	UPROPERTY(replicatedUsing=OnRep_bEnabled)
	uint32 bEnabled:1;

	/** Replication Notification Callbacks */
	UFUNCTION()
	virtual void OnRep_bEnabled();
};



