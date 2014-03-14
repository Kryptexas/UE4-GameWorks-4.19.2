// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "InteractiveFoliageComponent.generated.h"

UCLASS(HeaderGroup=Component, hidecategories=Object)
class UInteractiveFoliageComponent : public UStaticMeshComponent
{
	GENERATED_UCLASS_BODY()


public:
	class FInteractiveFoliageSceneProxy* FoliageSceneProxy;

	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	// End UPrimitiveComponent Interface

	// Begin UActorComponent Interface
	virtual void DestroyRenderState_Concurrent() OVERRIDE;
	// End UActorComponent Interface

	friend class AInteractiveFoliageActor;
};

