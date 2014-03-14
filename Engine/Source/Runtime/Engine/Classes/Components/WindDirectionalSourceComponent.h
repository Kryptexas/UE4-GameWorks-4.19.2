// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "WindDirectionalSourceComponent.generated.h"

UCLASS(HeaderGroup=Component, collapsecategories, hidecategories=(Object, Mobility), editinlinenew)
class UWindDirectionalSourceComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(interp, Category=WindDirectionalSourceComponent)
	float Strength;

	UPROPERTY(interp, Category=WindDirectionalSourceComponent)
	float Speed;


public:
	FWindSourceSceneProxy* SceneProxy;

protected:
	// Begin UActorComponent interface.
	virtual void CreateRenderState_Concurrent() OVERRIDE;
	virtual void SendRenderTransform_Concurrent() OVERRIDE;
	virtual void DestroyRenderState_Concurrent() OVERRIDE;
	// End UActorComponent interface.

public:
	/**
	 * Creates a proxy to represent the primitive to the scene manager in the rendering thread.
	 * @return The proxy object.
	 */
	virtual class FWindSourceSceneProxy* CreateSceneProxy() const;
};



